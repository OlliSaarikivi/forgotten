#pragma once

#include "NamedRows.h"
#include "Row.h"
#include "Sentinels.h"

template<class TLeftIndex, class TRightIndex, class TLeft, class TLeftSentinel, class TRight, class TRightSentinel> class MergeOuterJoinIterator
	: boost::equality_comparable<MergeOuterJoinIterator<TLeftIndex, TRightIndex, TLeft, TLeftSentinel, TRight, TRightSentinel>, End> {

	using LeftKey = typename TLeftIndex::Key;
	using LeftGetKey = TLeftIndex;
	using RightKey = typename TRightIndex::Key;
	using RightGetKey = TRightIndex;

	TLeft left;
	TLeftSentinel leftEnd;
	TRight right;
	TRightSentinel rightEnd;

public:
	using reference = typename NamedRowsConcat<typename TLeft::reference, typename AsMaybeRow<typename TRight::reference>::type>::type;

	MergeOuterJoinIterator(TLeft left, TLeftSentinel leftEnd, TRight right, TRightSentinel rightEnd) :
		left(left), leftEnd(leftEnd),
		right(right), rightEnd(rightEnd) {

		findMatch();
	}
	void findMatch() {
		if (left == leftEnd) return;
		while (right != rightEnd && RightGetKey()(*right) < LeftGetKey()(*left))
			++right;		
	}

	MergeOuterJoinIterator& operator++() {
		++left;
		findMatch();
		return *this;
	}
	MergeOuterJoinIterator operator++(int) {
		auto old = *this;
		this->operator++();
		return old;
	}

	reference operator*() const {
		if (right != rightEnd && RightGetKey()(*right) == LeftGetKey()(*left))
			return concatNamedRows(*left, someNamedRows(*right));
		else
			return concatNamedRows(*left, NoneNamedRows<typename TRight::reference>()());
	}
	FauxPointer<reference> operator->() const {
		return FauxPointer<reference>{ this->operator*() };
	}

	friend bool operator==(const MergeOuterJoinIterator& iter, const End& sentinel) {
		return iter.left == iter.leftEnd;
	}

	friend void swap(MergeOuterJoinIterator& left, MergeOuterJoinIterator& right) {
		using std::swap;
		swap(left.left, right.left);
		swap(left.leftEnd, right.leftEnd);
		swap(left.right, right.right);
		swap(left.rightEnd, right.rightEnd);
	}
};