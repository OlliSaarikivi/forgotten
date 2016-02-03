#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TLeft, class TLeftSentinel, class TFinder, class TFinderFail> class FindJoinIterator
	: boost::equality_comparable<FindJoinIterator<TLeftIndex, TLeft, TLeftSentinel, TFinder, TFinderFail>, End> {

	using LeftGetKey = TLeftIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TFinder finder;
	using ResultIterator = typename TFinder::iterator;
	ResultIterator result;
	TFinderFail finderFail;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference, typename TFinder::iterator::reference>::type;

	FindJoinIterator(TLeft left, TLeftSentinel leftEnd, TFinder finder, TFinderFail finderFail) :
		left(left), leftEnd(leftEnd), finder(finder), finderFail(finderFail) {

		findMatch();
	}

	void findMatch() {
		while (left != leftEnd) {
			result = finder.find(LeftGetKey()(*left));
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
