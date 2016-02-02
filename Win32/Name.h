#pragma once

#define NAME(N) template<class T> struct N : T { \
	N(const T& t) : T(t) {} \
};

template<template<typename> class TName, class TIter> class NamingIterator {
	TIter iter;

public:
	using reference = TName<typename TIter::reference>;

	NamingIterator(TIter iter) : iter(iter) {}

	NamingIterator& operator++()
	{
		++iter;
		return *this;
	}
	NamingIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return reference{ *iter };
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	template<class TSentinel> friend bool operator==(const NamingIterator& iter, const TSentinel& sentinel) {
		return iter.iter == sentinel;
	}
	template<class TSentinel> friend bool operator!=(const NamingIterator& iter, const TSentinel& sentinel) {
		return iter.iter != sentinel;
	}

	friend void swap(NamingIterator& left, NamingIterator& right) {
		using std::swap;
		swap(left.iter, right.iter);
	}
};

template<class T> struct NameOf;