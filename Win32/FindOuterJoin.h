#pragma once

#include "MergeOuterJoin.h"

template<class TLeftIndex, class TLeft, class TLeftSentinel, class TFinder, class TFinderFail> class FindOuterJoinIterator
	: boost::equality_comparable<FindOuterJoinIterator<TLeftIndex, TLeft, TLeftSentinel, TFinder, TFinderFail>, End> {

	using LeftGetKey = TLeftIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TFinder finder;
	TFinderFail finderFail;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference,
		typename AsMaybeRow<typename TFinder::iterator::reference>::type>::type;

	FindOuterJoinIterator(TLeft left, TLeftSentinel leftEnd, TFinder finder, TFinderFail finderFail) :
		left(left), leftEnd(leftEnd), finder(finder), finderFail(finderFail) {}

	FindOuterJoinIterator& operator++() {
		++left;
		return *this;
	}
	FindOuterJoinIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() {
		auto result = finder.find(LeftGetKey()(*left));
		if (result != finderFail)
			return concatNamedRows(*left, someNamedRows(*result));
		else
			return concatNamedRows(*left, NoneNamedRows<typename TFinder::iterator::reference>()());
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	friend bool operator==(const FindOuterJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}
};
