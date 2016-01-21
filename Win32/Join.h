#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TKey, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeJoinIterator
	: boost::equality_comparable<MergeJoinIterator<TKey, TLeft, TLeftSentinel, TRight, TRightSentinel>
	, boost::equality_comparable<MergeJoinIterator<TKey, TLeft, TLeftSentinel, TRight, TRightSentinel>, End >> {

	using KeyType = typename TKey::KeyType;
	using KeyLess = typename TKey::Less;
	using GetKey = TKey;

	KeyType leftKey;
	TLeft left;
	TLeftSentinel leftEnd;
	KeyType rightKey;
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
			while (KeyLess()(leftKey, rightKey)) {
				++left;
				if (left != leftEnd)
					leftKey = GetKey()(*left);
				else
					return;
			}
			if (!KeyLess()(rightKey, leftKey))
				return;
			while (KeyLess()(rightKey, leftKey)) {
				++right;
				if (right != rightEnd)
					rightKey = GetKey()(*right);
				else
					return;
			}
			if (!KeyLess()(leftKey, rightKey))
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
		Iterator old = *this;
		this->operator++();
		return old;
	}

	RowType operator*() const {
		return 
	}
	FauxPointer<RowType> operator->() const {
		return FauxPointer<RowType>{ this->operator*() };
	}

	friend bool operator==(const MergeJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd || iter.right == iter.rightEnd;
	}
	friend bool operator==(const MergeJoinIterator& left, const MergeJoinIterator& right) {
		return left.left == right.left && left.right == right.right;
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