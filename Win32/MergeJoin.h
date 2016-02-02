#pragma once

#include "NamedRows.h"
#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TRightIndex, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeJoinIterator
	: boost::equality_comparable<MergeJoinIterator<TLeftIndex, TRightIndex, TLeft, TLeftSentinel, TRight, TRightSentinel>, End> {

	using LeftKey = typename TLeftIndex::Key;
	using LeftGetKey = TLeftIndex;
	using RightKey = typename TRightIndex::Key;
	using RightGetKey = TRightIndex;

	LeftKey leftKey;
	TLeft left;
	TLeftSentinel leftEnd;
	RightKey rightKey;
	TRight right;
	TRightSentinel rightEnd;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference, typename TRight::reference>::type;

	MergeJoinIterator(TLeft left, TLeftSentinel leftEnd, TRight right, TRightSentinel rightEnd) :
		left(left), leftEnd(leftEnd),
		right(right), rightEnd(rightEnd)
	{
		if (left != leftEnd && right != rightEnd) {
			leftKey = LeftGetKey()(*left);
			rightKey = RightGetKey()(*right);
			findMatch();
		}
	}
	void findMatch()
	{
		for (;;) {
			while (leftKey < rightKey) {
				++left;
				if (left != leftEnd)
					leftKey = LeftGetKey()(*left);
				else
					return;
			}
			if (rightKey == leftKey)
				return;
			while (rightKey < leftKey) {
				++right;
				if (right != rightEnd)
					rightKey = RightGetKey()(*right);
				else
					return;
			}
			if (leftKey == rightKey)
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
		swap(left.leftKey, right.leftKey);
		swap(left.left, right.left);
		swap(left.leftEnd, right.leftEnd);
		swap(left.rightKey, right.rightKey);
		swap(left.right, right.right);
		swap(left.rightEnd, right.rightEnd);
	}
};
