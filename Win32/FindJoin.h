#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeft, class TLeftSentinel, class TFinder, class TFinderFail> class FindJoinIterator
	: boost::equality_comparable<FindJoinIterator<TLeft, TLeftSentinel, TFinder>, End> {

	TLeft left;
	TLeftSentinel leftEnd;
	TFinder finder;
	TFinder::Iterator result;
	TFinderFail finderFail;

public:
	using RowType = typename RowUnion<typename TLeft::RowType, typename TRight::RowType>::type;

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
		Iterator old = *this;
		this->operator++();
		return old;
	}

	RowType operator*() const {
		return JoinRows<RowType>()(*left, *result);
	}
	FauxPointer<RowType> operator->() const {
		return FauxPointer<RowType>{ this->operator*() };
	}

	friend bool operator==(const FindJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}

	friend void swap(FindJoinIterator& left, FindJoinIterator& right) {
		using std::swap;
		swap(left.left, right.left);
		swap(left.leftEnd, right.leftEnd);
		swap(left.finder, right.finder);
		swap(left.result, right.result);
		swap(left.finderFail, right.finderFail);
	}
};

template<class TLeft, class TRight> auto findJoin(TLeft& left, TRight& right) {
	return FindJoinIterator<typename TRight::Key, decltype(begin(left)), decltype(end(left)), decltype(begin(right)), decltype(end(right))>
		(begin(left), end(left), begin(right), end(right));
}
