#pragma once

#include "Row.h"

template<template<typename> class TIter, class... TValues> class RowIterator {
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
	friend bool operator!=(const RowIterator& left, const RowIterator& right) {
		return !(left == right);
	}
	friend bool operator<(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) < get<FirstIter>(right.iters);
	}
	friend bool operator>(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) > get<FirstIter>(right.iters);
	}
	friend bool operator<=(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) <= get<FirstIter>(right.iters);
	}
	friend bool operator>=(const RowIterator& left, const RowIterator& right) {
		return get<FirstIter>(left.iters) >= get<FirstIter>(right.iters);
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

#ifndef ALLOW_COLS_OVER_HWPREFETCH_STREAMS
	static_assert(sizeof...(TValues) <= 16, "Columns exceeds number of prefetch streams supported by Intel Core. "
		"Iteration performance may be reduced. Define ALLOW_COLS_OVER_HWPREFETCH_STREAMS to disable this error.");
#endif

	size_t size_ = 0;
	size_t capacity = 0;
	unique_ptr<char[]> data = nullptr;

	void grow(size_t requiredSize) {
		auto newCapacity = (size_t)(requiredSize * 1.5);
		newCapacity += newCapacity % alignment;
		auto newMe = Columnar{ size_, newCapacity, std::make_unique<char[]>(newCapacity) };

		auto iter = begin();
		auto endIter = end();
		auto toIter = newMe.begin();
		while (iter != endIter) {
			*toIter = std::move(*iter);
			++iter;
			++toIter;
		}

		capacity = newMe.capacity;
		using std::swap;
		swap(data, newMe.data);
	}

	Columnar(size_t size, size_t capacity, unique_ptr<char[]> data) : size_(size), capacity(capacity), data(std::move(data)) {}

public:
	static const int alignment = mpl::max_element<
		typename mpl::transform<ValueTypes, impl::AlignmentOf<mpl::_1>>::type>::type::type::value;

	template<class... Ts> using Iterator = RowIterator<AsPointer, Ts...>;

	Columnar() : size_(0), capacity(0), data(nullptr) {}

	template<class T> auto colBegin() const {
		return reinterpret_cast<T*>(data.get() + (capacity * impl::SizeOfPreceding<ValueTypes, T>::value));
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
		if (size_ + n > capacity) {
			grow(size_ + n);
		}
	}

	template<class... Ts> void unsafePushBack(const Row<Ts...>& row) {
		assert(size_ < capacity);
		*end() = row;
		++size_;
	}
	template<class... Ts> void pushBack(const Row<Ts...>& row) {
		if (size_ == capacity) {
			grow(size_ + 1);
		}
		unsafePushBack(row);
	}

	template<class... Ts> void unsafeInsert(Iterator<TValues...> pos, const Row<Ts...>& row) {
		auto to = end();
		auto from = to - 1;
		while (from != pos) {
			*to = std::move(*from);
			to = from;
			--from;
		}
		*pos = row;
	}
	template<class... Ts> void insert(Iterator<TValues...> pos, const Row<Ts...>& row) {
		if (size_ == capacity) {
			grow(size_ + 1);
		}
		unsafeInsert(pos, row);
	}

	size_t size() {
		return size_;
	}
	bool empty() {
		return size_ == 0;
	}
};