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
        left(left_chan.begin()), left_end(left_chan.end()),
        right(right_chan.begin()), right_end(right_chan.end())
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
        if (left != left_end) {
            while (right != right_end && KeyType::less(*right, *left)) {
                ++right;
            }
        } else {
            right = right_end;
        }
    }
    AmendIterator<TRow, TLeft, TRight, true>& operator++()
    {
        ++left;
        findMatch();
        return *this;
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
    using KeyType = typename TLeft::IndexType::KeyType;
    using left_iterator = typename TLeft::const_iterator;
    using right_iterator = typename TRight::const_iterator;

    AmendIterator(const TLeft& left_chan, const TRight& right_chan) :
        left(left_chan.begin()), left_end(left_chan.end()), right_chan(&right_chan)
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
        if (left != left_end) {
            auto range = right_chan->equalRange(*left);
            right = range.first;
            right_end = range.second;
        }
    }
    AmendIterator<TRow, TLeft, TRight, false>& operator++()
    {
        ++left;
        findMatch();
        return *this;
    }
    TRow operator*() const
    {
        auto amended_row = TRow(*left);
        if (right != right_end) {
            amended_row.setAll(*right);
        }
        return amended_row;
    }
    bool operator==(const AmendIterator<TRow, TLeft, TRight, false>& other) const
    {
        return left == other.left && left_end == other.left_end && right_chan == other.right_chan;
    }
    bool operator!=(const AmendIterator<TRow, TLeft, TRight, false>& other) const
    {
        return !operator==(other);
    }

    template<typename TLeft, typename TRight>
    friend struct AmendStream;

private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_end;
    const TRight* right_chan;
};

template<typename TLeft, typename TRight>
struct AmendStream : Channel
{
    using RowType = typename TLeft::RowType;
    using IndexType = AmendIterator<RowType, TLeft, TRight, CanMerge<TLeft, TRight>::value>;
    using const_iterator = IndexType;

    AmendStream(TLeft& left, TRight& right) : left(left), right(right) {}

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
        // NOT NEEDED: static_assert(!Intersects<TRow2, typename TRight::IndexType::KeyType::AsRow>::value, "can not update key columns in place");
        // Do updates to right and then left (without the columns that right contained)
        if (position.right != position.right_end) {
            right.update(position.right, row);
        } else {
            right.put(TRight::RowType(*(position.left), row));
        }
    }
private:
    TLeft& left;
    TRight& right;
};