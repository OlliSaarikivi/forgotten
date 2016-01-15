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
	static const size_t alignment = mpl::max_element<
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
	tuple<TValues*...> columns;

	struct ReallocColumn {
		Columnar& columnar;
		template<class T> void operator()(const T& type) {
			using PointerType = AsPointer<T::type>;
			auto& column = get<PointerType>(columnar.columns);
			column = (PointerType)realloc(column, columnar.capacity_ * sizeof(T::type));
		}
	};
	void grow(size_t requiredSize) {
		capacity_ = (size_t)(requiredSize * 1.5);
		mpl::for_each<ValueTypes, TypeWrap<mpl::_1>>(ReallocColumn{ *this });
	}

public:
	Columnar() : size_(0), capacity_(0), columns((AsPointer<TValues>)nullptr...) {}
	~Columnar() {
		//free(data);
		//data = nullptr;
	}

	template<class T> auto colBegin() const {
		return get<T*>(columns);
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

	template<class TRow> void unsafePushBack(const TRow& row) {
		assert(size_ < capacity_);
		*(end()) |= row;
		++size_;
	}
	template<class TRow> void pushBack(const TRow& row) {
		if (size_ == capacity_) {
			grow(size_ + 1);
		}
		unsafePushBack(row);
	}

	template<class TRow> void unsafeInsert(iterator pos, const TRow& row) {
		auto to = end();
		auto from = to - 1;
		if (to != pos)
			for (;;) {
				*to |= *from;
				to = from;
				--from;
				if (to == pos)
					break;
			}
		*pos |= row;
		++size_;
	}
	template<class TRow> void unsafeInsert(size_t index, const TRow& row) {
		unsafeInsert(begin() + index, row);
	}
	template<class TRow> void insert(iterator pos, const TRow& row) {
		insert(pos - begin(), row);
	}
	template<class TRow> void insert(size_t index, const TRow& row) {
		if (size_ == capacity_) {
			grow(size_ + 1);
		}
		unsafeInsert(index, row);
	}

	template<class TIter> void unsafeInsert(iterator pos, TIter rangeBegin, TIter rangeEnd) {
		auto rangeSize = rangeEnd - rangeBegin;
		auto to = end() + (rangeSize - 1);
		auto from = to - rangeSize;
		if (to != pos)
			for (;;) {
				*to |= *from;
				--to;
				--from;
				if (to == pos)
					break;
			}

		TIter iter = rangeBegin;
		auto posIter = pos;
		while (iter != rangeEnd) {
			*posIter |= *iter;
			++iter;
			++posIter;
		}
		size_ += rangeSize;
	}
	template<class TIter> void unsafeInsert(size_t index, TIter rangeBegin, TIter rangeEnd) {
		unsafeMoveInsert(begin() + index, rangeBegin, rangeEnd);
	}
	template<class TIter> void insert(iterator pos, TIter rangeBegin, TIter rangeEnd) {
		moveInsert(pos - begin(), rangeBegin, rangeEnd);
	}
	template<class TIter> void insert(size_t index, TIter rangeBegin, TIter rangeEnd) {
		auto rangeSize = rangeEnd - rangeBegin;
		if (size_ + rangeSize >= capacity_) {
			grow(size_ + rangeSize);
		}
		unsafeMoveInsert(index, rangeBegin, rangeEnd);
	}

	void erase(iterator rangeBegin, iterator rangeEnd) {
		iterator from = rangeEnd;
		iterator fromEnd = end();
		iterator to = rangeBegin;
		while (from != fromEnd) {
			*to |= *from;
			++from;
			++to;
		}
		size_ -= (rangeEnd - rangeBegin);
	}

	void clear() {
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
		swap(left.columns, right.columns);
	}
};

template<class... Ts> inline auto begin(Columnar<Ts...>& c) { return c.begin(); }
template<class... Ts> inline auto end(Columnar<Ts...>& c) { return c.end(); }

template<int N, class... TValues> class ColumnarArray {
	using ValueTypes = typename mpl::vector<TValues...>::type;
	template<class TValue> using AsPointer = TValue*;

public:
	template<class... Ts> using Iterator = RowIterator<AsPointer, Ts...>;
	using iterator = Iterator<TValues...>;

private:
#ifndef ALLOW_COLS_OVER_HWPREFETCH_STREAMS
	static_assert(sizeof...(TValues) <= 16, "Columns exceeds number of prefetch streams supported by Intel Core. "
		"Iteration performance may be reduced. Define ALLOW_COLS_OVER_HWPREFETCH_STREAMS to disable this error.");
#endif

	tuple<array<TValues, N>...> columns;

public:
	template<class T> auto colBegin() {
		return get<array<T, N>>(columns).data();
	}
	template<class T, class... Ts> auto begin() {
		return Iterator<T, Ts...>{ colBegin<T>(), colBegin<Ts>()... };
	}
	auto begin() {
		return begin<TValues...>();
	}

	template<class T> auto colEnd() {
		return colBegin<T>() + N;
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
};

template<int N, class... Ts> inline auto begin(ColumnarArray<N, Ts...>& c) { return c.begin(); }
template<int N, class... Ts> inline auto end(ColumnarArray<N, Ts...>& c) { return c.end(); }