#pragma once

#include "Row.h"

template<template<typename> class TIter, class... TValues> class RowIterator
	: public boost::totally_ordered<RowIterator<TIter, TValues...>> {

	using Values = typename mpl::vector<TValues...>::type;
	using Distinct = typename mpl::unique<Values, std::is_same<mpl::_1, mpl::_2>>::type;
	BOOST_MPL_ASSERT((mpl::equal<Values, Distinct>));

	using FirstIter = TIter<typename Values::begin::type>;

	template<class T> int increment() { ++get<T>(iters); return 0; }
	template<class T> int increment(size_t n) { get<T>(iters) += n; return 0; }
	template<class T> int decrement() { --get<T>(iters); return 0; }
	template<class T> int decrement(size_t n) { get<T>(iters) -= n; return 0; }
	template<class T> static int swap(RowIterator& left, RowIterator& right) {
		using std::swap;
		swap(get<T>(left.iters), get<T>(right.iters));
		return 0;
	}
	template<class T> static int equals(const RowIterator& left, const RowIterator& right, bool& result) {
		result |= (get<T>(left.iters) == get<T>(right.iters));
		return 0;
	}

	tuple<TIter<TValues>...> iters;

public:
	using difference_type = typename std::iterator_traits<FirstIter>::difference_type;
	//using value_type = ; // Not really there
	using reference = Row<TValues&...>;
	using pointer = FauxPointer<reference>;
	using iterator_category = std::random_access_iterator_tag;

	RowIterator() {}
	RowIterator(TIter<TValues>... iters) : iters(std::make_tuple(iters...)) {}

	RowIterator& operator++() {
		ignore(increment<TValues*>()...);
		return *this;
	}
	RowIterator operator++(int) {
		RowIterator old = *this;
		ignore(increment<TValues*>()...);
		return old;
	}
	RowIterator& operator--() {
		ignore(decrement<TValues*>()...);
		return *this;
	}
	RowIterator operator--(int) {
		RowIterator old = *this;
		ignore(decrement<TValues*>()...);
		return old;
	}
	RowIterator& operator+=(size_t n) {
		ignore(increment<TValues*>(n)...);
		return *this;
	}
	friend RowIterator operator+(const RowIterator& iter, size_t n) {
		RowIterator copy = iter;
		copy += n;
		return copy;
	}
	friend RowIterator operator+(size_t n, const RowIterator& iter) {
		return iter + n;
	}
	RowIterator& operator-=(size_t n) {
		ignore(decrement<TValues*>(n)...);
		return *this;
	}
	friend RowIterator operator-(const RowIterator& iter, size_t n) {
		RowIterator copy = iter;
		copy -= n;
		return copy;
	}
	friend RowIterator operator-(size_t n, const RowIterator& iter) {
		return iter - n;
	}

	friend difference_type operator-(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) - get<FirstIter>(right.iters);
	}

	reference operator*() const {
		return makeRow(*std::get<TValues*>(iters)...);
	}
	pointer operator->() const {
		return pointer{ this->operator*() };
	}
	reference operator[](size_t n) const {
		return *(*this + n);
	}

	friend bool operator==(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) == get<FirstIter>(right.iters);
	}
	friend bool operator<(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) < get<FirstIter>(right.iters);
	}

	friend void swap(RowIterator& left, RowIterator& right) {
		ignore(swap<TIter<TValues>>(left, right)...);
	}
};

namespace impl {
	template<class TTypes, class T> struct SizeOfPreceding
	{
		using End = typename mpl::find<TTypes, T>::type;
		using Preceding = typename mpl::iterator_range<typename TTypes::begin, End>::type;
		using Sum = typename mpl::accumulate<Preceding, mpl::int_<0>, mpl::plus<mpl::_1, mpl::sizeof_<mpl::_2>>>::type;
		static const ::size_t value = Sum::value;
	};

	template<class TTypes> struct SizeOfAll
	{
		using Sum = typename mpl::accumulate<TTypes, mpl::int_<0>, mpl::plus<mpl::_1, mpl::sizeof_<mpl::_2>>>::type;
		static const ::size_t value = Sum::value;
	};

	template<class T> struct AlignmentOf {
		using type = mpl::int_<std::alignment_of<T>::value>;
	};
}

template<class... TValues> class Columnar {
	using ValueTypes = typename mpl::vector<TValues...>::type;
	template<class TValue> using AsPointer = TValue*;

public:
	static const int alignment = mpl::max_element<
		typename mpl::transform<ValueTypes, impl::AlignmentOf<mpl::_1>>::type>::type::type::value;

	template<class... Ts> using Iterator = RowIterator<AsPointer, Ts...>;
	using iterator = Iterator<TValues...>;

private:

#ifndef ALLOW_COLS_OVER_HWPREFETCH_STREAMS
	static_assert(sizeof...(TValues) <= 16, "Columns exceeds number of prefetch streams supported by Intel Core. "
		"Iteration performance may be reduced. Define ALLOW_COLS_OVER_HWPREFETCH_STREAMS to disable this error.");
#endif

	size_t size_ = 0;
	size_t capacity_ = 0;
	char* data = nullptr;

	struct MoveConstructColumn {
		Columnar& from;
		Columnar& to;
		template<class T> void operator()(const T& type) {
			using Type = T::type;
			auto fromIter = from.colBegin<Type>();
			auto fromEnd = from.colEnd<Type>();
			auto toIter = to.colBegin<Type>();
			while (fromIter != fromEnd) {
				new (toIter) Type(std::move(*fromIter));
				++fromIter;
				++toIter;
			}
		}
	};
	void grow(size_t requiredSize) {
		auto newCapacity = (size_t)(requiredSize * 1.5);
		newCapacity += newCapacity % alignment;
		auto newMe = Columnar{ size_, newCapacity, (char*)malloc(newCapacity * impl::SizeOfAll<ValueTypes>::value) };

		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(MoveConstructColumn{ *this, newMe });

		using std::swap;
		swap(*this, newMe);
	}

	struct DestructColumn {
		Columnar& c;
		template<class T> void operator()(const T& type) {
			auto iter = c.colBegin<typename T::type>();
			for (size_t i = 0; i < c.size(); ++i) {
				using Type = T::type;
				iter[i].~Type();
			}
		}
	};

	struct DestructRow {
		Row<TValues&...>& row;
		template<class T> void operator()(const T& type) {
			using Type = T::type;
			(row.col<Type>()).~Type();
		}
	};

	template<class TRow> struct MoveConstructRow {
		Iterator<TValues...> pos;
		TRow&& row;

		MoveConstructRow(Iterator<TValues...> pos, TRow&& row) : pos(pos), row(std::forward<TRow>(row)) {}
		MoveConstructRow(const MoveConstructRow& other) : pos(other.pos), row(std::move(other.row)) {}

		template<class T> void operator()(const T& type) {
			using Type = T::type;
			new (&(pos->col<Type>())) Type(std::move(row.unsafeCol<Type>()));
		}
	};
	template<class TRow> struct CopyConstructRow {
		Iterator<TValues...> pos;
		const TRow& row;

		template<class T> void operator()(const T& type) {
			using Type = T::type;
			new (&(pos->col<Type>())) Type(row.col<Type>());
		}
	};
	template<class... Ts> auto getConstructRow(Iterator<TValues...> pos, Row<Ts...>&& row) {
		return MoveConstructRow<Row<Ts...>>{ pos, std::move(row) };
	}
	template<class... Ts> auto getConstructRow(Iterator<TValues...> pos, const Row<Ts...>& row) {
		return CopyConstructRow<Row<Ts...>>{ pos, row };
	}

	Columnar(size_t size, size_t capacity, char* data) : size_(size), capacity_(capacity), data(data) {}

public:
	Columnar() : size_(0), capacity_(0), data(nullptr) {}
	~Columnar() {
		clear();
		free(data);
		data = nullptr;
		capacity_ = 0;
	}

	template<class T> auto colBegin() const {
		return reinterpret_cast<T*>(data + (capacity_ * impl::SizeOfPreceding<ValueTypes, T>::value));
	}
	template<class T, class... Ts> auto begin() {
		return Iterator<T, Ts...>{ colBegin<T>(), colBegin<Ts>()... };
	}
	auto begin() {
		return begin<TValues...>();
	}

	template<class T> auto colEnd() const {
		return colBegin<T>() + size_;
	}
	template<class T, class... Ts> auto end() {
		return Iterator<T, Ts...>{ colEnd<T>(), colEnd<Ts>()... };
	}
	auto end() {
		return end<TValues...>();
	}

	Row<TValues&...> operator[](size_t pos) {
		return begin()[pos];
	}

	void reserve(size_t n) {
		if (n > capacity_) {
			grow(n);
		}
	}

	template<class TRow> void unsafePushBack(TRow&& row) {
		assert(size_ < capacity_);
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(getConstructRow(end(), std::forward<TRow>(row)));
		++size_;
	}
	template<class TRow> void pushBack(TRow&& row) {
		if (size_ == capacity_) {
			grow(size_ + 1);
		}
		unsafePushBack(std::forward<TRow>(row));
	}

	template<class TRow> void unsafeInsert(Iterator<TValues...> pos, TRow&& row) {
		auto to = end();
		auto from = to - 1;
		if (to != pos)
			while (true) {
				mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(getConstructRow(to, std::move(*from)));
				to = from;
				--from;
				if (to == pos) {
					mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(DestructRow{ *pos });
					break;
				}
			}
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(getConstructRow(pos, std::forward<TRow>(row)));
		++size_;
	}
	template<class TRow> void unsafeInsert(size_t index, TRow&& row) {
		unsafeInsert(begin() + index, std::forward<TRow>(row));
	}
	template<class TRow> void insert(Iterator<TValues...> pos, TRow&& row) {
		insert(pos - begin(), std::forward<TRow>(row));
	}
	template<class TRow> void insert(size_t index, TRow&& row) {
		if (size_ == capacity_) {
			grow(size_ + 1);
		}
		unsafeInsert(index, std::forward<TRow>(row));
	}

	void clear() {
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(DestructColumn{ *this });
		size_ = 0;
	}

	size_t size() {
		return size_;
	}
	size_t capacity() {
		return capacity_;
	}
	bool empty() {
		return size_ == 0;
	}

	friend void swap(Columnar& left, Columnar& right) {
		using std::swap;
		swap(left.size_, right.size_);
		swap(left.capacity_, right.capacity_);
		swap(left.data, right.data);
	}
};

template<class... Ts> inline auto begin(Columnar<Ts...>& c) { return c.begin(); }
template<class... Ts> inline auto end(Columnar<Ts...>& c) { return c.end(); }