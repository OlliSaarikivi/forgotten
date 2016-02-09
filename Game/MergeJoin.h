#pragma once

#include "NamedRows.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TRightIndex, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeJoinIterator
	: boost::equality_comparable<MergeJoinIterator<TLeftIndex, TRightIndex, TLeft, TLeftSentinel, TRight, TRightSentinel>, End> {

	using LeftGetKey = TLeftIndex;
	using RightGetKey = TRightIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TRight right;
	TRightSentinel rightEnd;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference, typename TRight::reference>::type;

	MergeJoinIterator(TLeft left, TLeftSentinel leftEnd, TRight right, TRightSentinel rightEnd) :
		left(left), leftEnd(leftEnd),
		right(right), rightEnd(rightEnd)
	{
		findMatch();
	}
	void findMatch()
	{
		if (left == leftEnd || right == rightEnd) return;
		for (;;) {
			while (LeftGetKey()(*left) < RightGetKey()(*right)) {
				++left;
				if (left == leftEnd)
					return;
			}
			if (RightGetKey()(*right) == LeftGetKey()(*left))
				return;
			while (RightGetKey()(*right) < LeftGetKey()(*left)) {
				++right;
				if (right == rightEnd)
					return;
			}
			if (LeftGetKey()(*left) == RightGetKey()(*right))
				return;
		}
	}

	MergeJoinIterator& operator++()
	{
		++left;
		++right;
		findMatch();
		return *this;
	}
	MergeJoinIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return concatNamedRows(*left, *right);
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	friend bool operator==(const MergeJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd || iter.right == iter.rightEnd;
	}

	friend void swap(MergeJoinIterator& left, MergeJoinIterator& right) {
		using std::swap;
		swap(left.left, right.left);
		swap(left.leftEnd, right.leftEnd);
		swap(left.right, right.right);
		swap(left.rightEnd, right.rightEnd);
	}
};