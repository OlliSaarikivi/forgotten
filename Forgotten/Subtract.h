#pragma once

#include "Join.h"

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
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    SubtractIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.begin()), left_end(left_chan.end()),
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
    SubtractIterator<TLeft, TRight, true>& operator++()
    {
        ++left;
        findNoMatch();
        return *this;
    }
    RowType operator*() const
    {
        return *left;
    }
    bool operator==(const SubtractIterator<TLeft, TRight, true>& other) const
    {
        return left == other.left && right == other.right && left_end == other.left_end && right_end == other.right_end;
    }
    bool operator!=(const SubtractIterator<TLeft, TRight, true>& other) const
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
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    SubtractIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.begin()), left_end(left_chan.end()), right_chan(&right_chan)
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
    SubtractIterator<TLeft, TRight, false>& operator++()
    {
        ++left;
        findNoMatch();
        return *this;
    }
    RowType operator*() const
    {
        return *left;
    }
    bool operator==(const SubtractIterator<TLeft, TRight, false>& other) const
    {
        return left == other.left && left_end == other.left_end && right_chan == other.right_chan;
    }
    bool operator!=(const SubtractIterator<TLeft, TRight, false>& other) const
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
    using const_iterator = IndexType;

    SubtractStream(TLeft& left, TRight& right) : left(left), right(right) {}

    virtual void forEachProducer(function<void(Process&)> f) const override
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
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        static_assert(!Intersects<TRow2, typename TRight::IndexType::KeyType::AsRow>::value, "can not update a subtract's right key columns");
        left.update(position.left, row);
    }
private:
    TLeft& left;
    TRight& right;
};