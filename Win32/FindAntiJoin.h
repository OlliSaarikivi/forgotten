#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TLeft, class TLeftSentinel, class TFinder, class TFinderFail> class FindAntiJoinIterator
	: boost::equality_comparable<FindAntiJoinIterator<TLeftIndex, TLeft, TLeftSentinel, TFinder, TFinderFail>, End> {

	using LeftGetKey = TLeftIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TFinder finder;
	TFinderFail finderFail;

public:
	using reference = typename TLeft::reference;

	FindAntiJoinIterator(TLeft left, TLeftSentinel leftEnd, TFinder finder, TFinderFail finderFail) :
		left(left), leftEnd(leftEnd), finder(finder), finderFail(finderFail) {

		findMatch();
	}

	void findMatch() {
		while (left != leftEnd) {
			auto result = finder.find(LeftGetKey()(*left));
			if (result == finderFail)
				break;
			++left;
		}
	}

	FindAntiJoinIterator& operator++()
	{
		++left;
		findMatch();
		return *this;
	}
	FindAntiJoinIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return *left;
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	friend bool operator==(const FindAntiJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}
};
