#pragma once

#include "Row.h"

template<typename TLeft, typename TRight>
struct CanMerge
{
    static const bool value =
    TLeft::IndexType::Ordered &&
    TRight::IndexType::Ordered &&
    std::is_same<typename TLeft::IndexType::KeyType, typename TRight::IndexType::KeyType>::value;
};

template<typename TRow, typename TLeft, typename TRight, bool CanMerge>
struct JoinIterator;
// Merge join
template<typename TRow, typename TLeft, typename TRight>
struct JoinIterator<TRow, TLeft, TRight, true>
{
    static const bool Ordered = true;
    using KeyType = typename TLeft::IndexType::KeyType;
    static_assert(std::is_same<KeyType, typename TRight::IndexType::KeyType>::value, "must have same key to merge join");
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    JoinIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.cbegin()), left_end(left_chan.cend()),
        right(right_chan.cbegin()), right_end(right_chan.cend())
    {
        findMatch();
    }
    void goToEnd()
    {
        left = left_end;
        right = right_end;
        right_subscan = right_end;
    }
    void findMatch()
    {
        while (left != left_end && right != right_end) {
            if (KeyType::less(*left, *right)) {
                ++left;
            } else if (KeyType::less(*right, *left)) {
                ++right;
            } else {
                right_subscan = right;
                return;
            }
        }
        goToEnd();
    }
    JoinIterator<TRow, TLeft, TRight, true>& operator++()
    {
        ++right_subscan;
        if (right_subscan == right_end || KeyType::less(*left, *right_subscan)) {
            ++left;
            findMatch();
        }
    }
    TRow operator*() const
    {
        auto joined_row = TRow(*left);
        joined_row.setAll(*right_subscan);
        return joinded_row;
    }
    bool operator==(const JoinIterator<TRow, TLeft, TRight, true>& other) const
    {
        return left == other.left && right == other.right && right_subscan == other.right_subscan &&
            left_end == other.left_end && right_end == other.right_end;
    }
    bool operator!=(const JoinIterator<TRow, TLeft, TRight, true>& other) const
    {
        return !operator==(other);
    }
private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_subscan;
    right_iterator right_end;
};
// Hash join
template<typename TRow, typename TLeft, typename TRight>
struct JoinIterator<TRow, TLeft, TRight, false>
{
    static const bool Ordered = TLeft::IndexType::Ordered;
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    JoinIterator(const TLeft& left_chan, const TRight& right_chan) :
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
        while (left != left_end) {
            auto range = right_chan->equalRange(*left);
            right = range.first;
            right_end = range.second;
            if (right != right_end) {
                return;
            }
            ++left;
        }
    }
    JoinIterator<TRow, TLeft, TRight, true>& operator++()
    {
        ++right;
        if (right == right_end) {
            ++left;
            findMatch();
        }
    }
    TRow operator*() const
    {
        auto joined_row = TRow(*left);
        joined_row.setAll(*right);
        return joined_row;
    }
    bool operator==(const JoinIterator<TRow, TLeft, TRight, true>& other) const
    {
        return left == other.left && left_end == other.left_end && right_chan == other.right_chan;
    }
    bool operator!=(const JoinIterator<TRow, TLeft, TRight, true>& other) const
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

template<typename TLeft, typename TRight>
using RowUnion = typename TLeft::RowType::template Union<typename TRight::RowType>;

template<typename TLeft, typename TRight, typename TRow = RowUnion<TLeft, TRight>>
struct JoinStream
{
    using RowType = TRow;
    using IndexType = JoinIterator<TRow, TLeft, TRight, CanMerge<TLeft, TRight>::value>;
    using const_iterator = IndexType;

    JoinStream(const TLeft& left, const TRight& right) : left(left), right(right) {}

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