#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TIndex, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeJoinIterator
	: boost::equality_comparable<MergeJoinIterator<TIndex, TLeft, TLeftSentinel, TRight, TRightSentinel>, End> {
	using Index = TIndex;

private:
	using Key = typename Index::Key;
	using GetKey = typename Index::GetKey;

	Key leftKey;
	TLeft left;
	TLeftSentinel leftEnd;
	Key rightKey;
	TRight right;
	TRightSentinel rightEnd;

public:
	using RowType = typename RowUnion<typename TLeft::RowType, typename TRight::RowType>::type;

	MergeJoinIterator(TLeft left, TLeftSentinel leftEnd, TRight right, TRightSentinel rightEnd) :
		left(left), leftEnd(leftEnd),
		right(right), rightEnd(rightEnd)
	{
		if (left != leftEnd && right != rightEnd) {
			leftKey = GetKey()(*left);
			rightKey = GetKey()(*right);
			findMatch();
		}
	}
	void findMatch()
	{
		for (;;) {
			while (leftKey < rightKey) {
				++left;
				if (left != leftEnd)
					leftKey = GetKey()(*left);
				else
					return;
			}
			if (rightKey == leftKey)
				return;
			while (rightKey < leftKey) {
				++right;
				if (right != rightEnd)
					rightKey = GetKey()(*right);
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

	RowType operator*() const {
		return JoinRows<RowType>()(*left, *right);
	}
	FauxPointer<RowType> operator->() const {
		return FauxPointer<RowType>{ this->operator*() };
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

template<class TLeft, class TRight> auto mergeJoin(TLeft& left, TRight& right) {
	return MergeJoinIterator<typename TRight::Index, decltype(begin(left)), decltype(end(left)), decltype(begin(right)), decltype(end(right))>
		(begin(left), end(left), begin(right), end(right));
}
