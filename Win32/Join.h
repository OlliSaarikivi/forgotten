#pragma once

#include "Utils.h"
#include "Row.h"
#include "Sentinels.h"

template<class TKey, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeJoinIterator {
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
	using reference = RowUnion<TLeft::reference, TRight::reference>;
	using pointer = FauxPointer<reference>;
	using iterator_category = std::forward_iterator_tag;

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

	JoinIterator& operator++()
	{
		++left;
		++right;
		findMatch();
		return *this;
	}
	Iterator operator++(int) {
		Iterator old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		return current->rows[pos];
	}
	pointer operator->() const {
		return pos.operator->();
	}

	friend bool operator==(const Iterator& left, const Iterator& right) {
		return left.current == right.current && left.pos == right.pos;
	}
	friend bool operator!=(const Iterator& left, const Iterator& right) {
		return !(left == right);
	}

	friend void swap(Iterator& left, Iterator& right) {
		using std::swap;
		swap(left.current, right.current);
		swap(left.pos, right.pos);
	}
};