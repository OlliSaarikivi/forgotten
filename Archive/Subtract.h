#pragma once

#include "Join.h"
#include "RowProxy.h"

template<typename TLeft, typename TRight, bool CanMerge>
struct SubtractIterator;
// Merge Subtract
template<typename TLeft, typename TRight>
struct SubtractIterator<TLeft, TRight, true>
{
    static const bool Ordered = true;
    using RowType = typename TLeft::RowType;
    using KeyType = typename TLeft::IndexType::KeyType;
    static_assert(std::is_same<KeyType, typename TRight::IndexType::KeyType>::value, "must have same key to merge Subtract");
	using ProxyType = typename TLeft::ProxyType;
    using left_iterator = typename TLeft::iterator;
    using right_iterator = typename TRight::iterator;

    SubtractIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end),
        right(right_chan.begin()), right_end(right_chan.end())
    {
        findNoMatch();
    }
    void goToEnd()
    {
        left = left_end;
        right = right_end;
    }
    void findNoMatch()
    {
        if (left == left_end) {
            goToEnd();
            return;
        }
        while (right != right_end && KeyType::less(*right, *left)) {
            ++right;
        }
        if (right != right_end && !KeyType::less(*left, *right)) {
            while (left != left_end && !KeyType::less(*right, *left)) {
                ++left;
            }
        }
        if (left == left_end) {
            goToEnd();
        }
    }
    SubtractIterator& operator++()
    {
        ++left;
        findNoMatch();
        return *this;
    }
	ProxyType operator*() const
    {
        return *left;
    }
    FauxPointer<ProxyType> operator->() const
    {
        return FauxPointer<ProxyType>(this->operator*());
    }
    bool operator==(const SubtractIterator& other) const
    {
        return left == other.left && right == other.right && left_end == other.left_end && right_end == other.right_end;
    }
    bool operator!=(const SubtractIterator& other) const
    {
        return !operator==(other);
    }
private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_end;
};
// Hash Subtract
template<typename TLeft, typename TRight>
struct SubtractIterator<TLeft, TRight, false>
{
    static const bool Ordered = TLeft::IndexType::Ordered;
    using RowType = typename TLeft::RowType;
    using KeyType = typename TLeft::IndexType::KeyType;
	using ProxyType = typename TLeft::ProxyType;
    using left_iterator = typename TLeft::iterator;
    using right_iterator = typename TRight::iterator;

    SubtractIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end), right_chan(&right_chan)
    {
        findNoMatch();
    }
    void goToEnd()
    {
        left = left_end;
    }
    void findNoMatch()
    {
        while (left != left_end) {
            auto range = right_chan->equalRange(*left);
            if (range.first != range.second) {
                ++left;
            } else {
                return;
            }
        }
    }
    SubtractIterator& operator++()
    {
        ++left;
        findNoMatch();
        return *this;
    }
	ProxyType operator*() const
    {
        return *left;
    }
    FauxPointer<ProxyType> operator->() const
    {
        return FauxPointer<ProxyType>(this->operator*());
    }
    bool operator==(const SubtractIterator& other) const
    {
        return left == other.left && left_end == other.left_end && right_chan == other.right_chan;
    }
    bool operator!=(const SubtractIterator& other) const
    {
        return !operator==(other);
    }

    template<typename TLeft, typename TRight>
    friend struct SubtractStream;

private:
    left_iterator left;
    left_iterator left_end;
    const TRight* right_chan;
};

template<typename TLeft, typename TRight>
struct SubtractStream : Channel
{
    using RowType = typename TLeft::RowType;
    using IndexType = SubtractIterator<TLeft, TRight, CanMerge<TLeft, TRight>::value>;
    using iterator = IndexType;

    SubtractStream(TLeft& left, TRight& right) : left(left), right(right) {}

	iterator begin() const
    {
        return iterator(left.begin(), left.end(), right);
    }
	iterator end() const
    {
		iterator end_iterator(left.end(), left.end(), right);
        end_iterator.goToEnd();
        return end_iterator;
    }
    template<typename TRow2>
    pair<iterator, iterator> equalRange(TRow2&& row) const
    {
        auto left_range = left.equalRange(row);
		iterator range_begin(left_range.first, left_range.second, right);
		iterator range_end = range_begin;
        range_end.goToEnd();
        return std::make_pair(range_begin, range_end);
    }
private:
    TLeft& left;
    TRight& right;
};