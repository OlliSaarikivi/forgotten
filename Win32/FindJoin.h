#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeft, class TLeftSentinel, class TFinder, class TFinderFail> class FindJoinIterator
	: boost::equality_comparable<FindJoinIterator<TLeft, TLeftSentinel, TFinder, TFinderFail>, End> {

	TLeft left;
	TLeftSentinel leftEnd;
	TFinder finder;
	typename TFinder::Iterator result;
	TFinderFail finderFail;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference, typename TFinder::reference>::type;

	FindJoinIterator(TLeft left, TLeftSentinel leftEnd, TFinder finder, TFinderFail finderFail) :
		left(left), leftEnd(leftEnd), finder(finder), finderFail(finderFail) {

		findMatch();
	}
	void findMatch() {
		while (left != leftEnd) {
			result = finder.find(*left);
			if (result != finderFail)
				break;
			++left;
		}
	}

	FindJoinIterator& operator++()
	{
		++left;
		findMatch();
		return *this;
	}
	FindJoinIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return concatNamedRows(*left, *result);
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	friend bool operator==(const FindJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}
};

template<class TLeft, class TRight> auto findJoin(TLeft& left, TRight& right) {
	return FindJoinIterator<decltype(begin(left)), decltype(end(left)), decltype(right.finder()), decltype(right.finderFail())>
		(begin(left), end(left), right.finder(), right.finderFail());
}