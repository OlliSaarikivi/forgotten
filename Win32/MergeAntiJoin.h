#pragma once

#include "NamedRows.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TRightIndex, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeAntiJoinIterator
	: boost::equality_comparable<MergeAntiJoinIterator<TLeftIndex, TRightIndex, TLeft, TLeftSentinel, TRight, TRightSentinel>, End> {

	using LeftKey = typename TLeftIndex::Key;
	using LeftGetKey = TLeftIndex;
	using RightKey = typename TRightIndex::Key;
	using RightGetKey = TRightIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TRight right;
	TRightSentinel rightEnd;

public:
	using reference = typename TLeft::reference;

	MergeAntiJoinIterator(TLeft left, TLeftSentinel leftEnd, TRight right, TRightSentinel rightEnd) :
		left(left), leftEnd(leftEnd),
		right(right), rightEnd(rightEnd)
	{
		findMatch();
	}
	void findMatch()
	{
		if (left == leftEnd || right == rightEnd) return;
		for (;;) {
			if (RightGetKey()(*right) < LeftGetKey()(*left)) {
				++right;
				if (right == rightEnd) return;
			}
			else if (RightGetKey()(*right) == LeftGetKey()(*left)) {
				++left;
				++right;
				if (left == leftEnd || right == rightEnd) return;
			}
			else
				return;
		}
	}

	MergeAntiJoinIterator& operator++()
	{
		++left;
		findMatch();
		return *this;
	}
	MergeAntiJoinIterator operator++(int) {
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

	friend bool operator==(const MergeAntiJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}

	friend void swap(MergeAntiJoinIterator& left, MergeAntiJoinIterator& right) {
		using std::swap;
		swap(left.left, right.left);
		swap(left.leftEnd, right.leftEnd);
		swap(left.right, right.right);
		swap(left.rightEnd, right.rightEnd);
	}
};