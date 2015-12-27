#pragma once

#include "Row.h"
#include "Utils.h"

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
	using reference = Row<TValues...>;
	using pointer = FauxPointer<reference>;
	using iterator_category = std::random_access_iterator_tag;

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

	reference operator*() const {
		return reference{ *std::get<TValues*>(iters)... };
	}
	pointer operator->() const {
		return pointer{ this->operator*() };
	}
	reference operator[](size_t n) const {
		return *(*this + n);
	}

	friend bool operator==(const RowIterator& left, const RowIterator& right) {
		bool result;
		ignore(equals<TIter<TValues>>(left, right, result)...);
		return result;
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

template<class... TValues> class Table {

	using ValueTypes = typename mpl::vector<TValues...>::type;

	template<class T> struct AlignmentOf {
		using type = mpl::int_<std::alignment_of<T>::value>;
	};

	template<class TValue> using iterator = TValue*;

	template<class T> struct SizeOfPreceding
	{
		using End = typename mpl::find<ValueTypes, T>::type;
		using Preceding = typename mpl::iterator_range<typename ValueTypes::begin, End>::type;
		using Sum = typename mpl::accumulate<Preceding, mpl::int_<0>, mpl::plus<mpl::_1, mpl::sizeof_<mpl::_2>>>::type;
		static const ::size_t value = Sum::value;
	};

	size_t size = 0;
	size_t capacity = 0;
	char* data = nullptr;

public:
	static const int alignment = mpl::max_element<
		typename mpl::transform<ValueTypes, AlignmentOf<mpl::_1>>::type>::type::type::value;

	template<class T> iterator<T> colBegin() const {
		return reinterpret_cast<iterator<T>>(data + (capacity * SizeOfPreceding<T>::value));
	}
	template<class... Ts> RowIterator<iterator, Ts...> begin() {
		return RowIterator<iterator, Ts...>{ colBegin<Ts>()... };
	}

	template<class T> iterator<T> colEnd() const {
		return colBegin<T>() + size;
	}
	template<class... Ts> RowIterator<iterator, Ts...> end() {
		return RowIterator<iterator, Ts...>{ colEnd<Ts>()... };
	}

	template<class... TValueRefs> bool pushBack(TValueRefs&&.... values) {
		if (size == capacity) {
			// TODO: realloc
		}
		
	}
};
