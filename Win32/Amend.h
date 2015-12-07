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

    AmendIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end),
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
        } else
            goToEnd();
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
        if (right != right_end && !KeyType::less(*left, *right))
            amended_row.setAll(*right);
        return amended_row;
    }
    FauxRowPointer<TRow> operator->() const
    {
        return FauxRowPointer<TRow>(this->operator*());
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

    AmendIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end), right_chan(&right_chan)
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
        } else {
            goToEnd();
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
    FauxRowPointer<TRow> operator->() const
    {
        return FauxRowPointer<TRow>(this->operator*());
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
        return const_iterator(left.begin(), left.end(), right);
    }
    const_iterator end() const
    {
        const_iterator end_iterator(left.end(), left.end(), right);
        end_iterator.goToEnd();
        return end_iterator;
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        if (position.right != position.right_end) {
            right.update(position.right, row);
        } else {
            right.put(TRight::RowType(*(position.left), row));
        }
        ConditionalUpdateHelper<TLeft, typename TLeft::const_iterator, typename SubtractColumns<TRow2, typename TRight::RowType>::type>::
            tUpdate(left, position.left, row);
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        auto left_range = left.equalRange(row);
        const_iterator range_begin(left_range.first, left_range.second, right);
        const_iterator range_end = range_begin;
        range_end.goToEnd();
        return std::make_pair(range_begin, range_end);
    }
private:
    TLeft& left;
    TRight& right;
};