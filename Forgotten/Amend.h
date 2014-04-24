#pragma once

#include "Join.h"

template<typename TRow, typename TLeft, typename TRight, bool CanMerge>
struct AmendIterator;
// Merge amend
template<typename TRow, typename TLeft, typename TRight>
struct AmendIterator<TRow, TLeft, TRight, true>
{
    static const bool Ordered = true;
    using KeyType = typename TLeft::IndexType::KeyType;
    static_assert(std::is_same<KeyType, typename TRight::IndexType::KeyType>::value, "must have same key to merge amend");
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    AmendIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.cbegin()), left_end(left_chan.cend()),
        right(right_chan.cbegin()), right_end(right_chan.cend())
    {
        findMatch();
    }
    void goToEnd()
    {
        left = left_end;
        right = right_end;
    }
    void findMatch()
    {
        while (right != right_end && KeyType::less(*right, *left)) {
            ++right_row;
        }
    }
    AmendIterator<TRow, TLeft, TRight, true>& operator++()
    {
        ++left;
        findMatch();
    }
    TRow operator*() const
    {
        auto amended_row = TRow(*left);
        if (right != right_end && !KeyType::less(*left, *right)) {
            amended_row.setAll(*right);
        }
        return amended_row;
    }
    bool operator==(const AmendIterator<TRow, TLeft, TRight, true>& other) const
    {
        return left == other.left && right == other.right && left_end == other.left_end && right_end == other.right_end;
    }
    bool operator!=(const AmendIterator<TRow, TLeft, TRight, true>& other) const
    {
        return !operator==(other);
    }
private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_end;
};
// Hash amend
template<typename TRow, typename TLeft, typename TRight>
struct AmendIterator<TRow, TLeft, TRight, false>
{
    static const bool Ordered = TLeft::IndexType::Ordered;
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    AmendIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.cbegin()), left_end(left_chan.cend()), right_chan(&right_chan)
    {
        findMatch();
    }
    void goToEnd()
    {
        left = left_end;
    }
    void findMatch()
    {
        auto range = right_chan->equalRange(*left);
        right = range.first;
        right_end = range.second;
    }
    AmendIterator<TRow, TLeft, TRight, true>& operator++()
    {
        ++left;
        findMatch();
    }
    TRow operator*() const
    {
        auto amended_row = TRow(*left);
        if (right != right_end) {
            amended_row.setAll(*right);
        }
        return amended_row;
    }
    bool operator==(const AmendIterator<TRow, TLeft, TRight, true>& other) const
    {
        return left == other.left && left_end == other.left_end && right_chan == other.right_chan;
    }
    bool operator!=(const AmendIterator<TRow, TLeft, TRight, true>& other) const
    {
        return !operator==(other);
    }
private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_end;
    const TRight* right_chan;
};

template<typename TLeft, typename TRight, typename TRow = typename TLeft::RowType::template Union<typename TRight::RowType>>
struct AmendStream
{
    using RowType = TRow;
    using IndexType = AmendIterator<TRow, TLeft, TRight, CanMerge<TLeft, TRight>::value>;
    using const_iterator = IndexType;

    AmendStream(const TLeft& left, const TRight& right) : left(left), right(right) {}

    virtual void forEachProducer(function<void(const Process&)> f) const override
    {
        left.forEachProducer(f);
        right.forEachProducer(f);
    }
    const_iterator begin() const
    {
        return const_iterator(left, right);
    }
    const_iterator end() const
    {
        const_iterator end_iterator(left, right);
        end_iterator.goToEnd();
        return end_iterator;
    }
private:
    const TLeft& left;
    const TRight& right;
};