#pragma once

#include "Row.h"
#include "RowProxy.h"

template<typename TLeft, typename TRight>
struct CanMerge
{
    static const bool value =
    TLeft::IndexType::Ordered &&
    TRight::IndexType::Ordered &&
    std::is_same<typename TLeft::IndexType::KeyType, typename TRight::IndexType::KeyType>::value;
};

template<typename TRow>
struct FauxRowPointer
{
    FauxRowPointer(const TRow& row) : row(row) {}
    const TRow& operator*() const
    {
        return row;
    }
    const TRow* operator->() const
    {
        return &row;
    }
private:
    TRow row;
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
	using ProxyType = RowProxy<TRow, typename KeyType::AsRow>;
    using left_iterator = typename TLeft::iterator;
    using right_iterator = typename TRight::iterator;

    JoinIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end),
        right(right_chan.begin()), right_end(right_chan.end())
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
    JoinIterator& operator++()
    {
        ++right_subscan;
        if (right_subscan == right_end || KeyType::less(*left, *right_subscan)) {
            ++left;
            findMatch();
        }
        return *this;
    }
	ProxyType operator*() const
    {
		static_assert(std::is_reference<decltype(*left)>::value, "Can only proxy iterators that return references");
		static_assert(std::is_reference<decltype(*right_subscan)>::value, "Can only proxy iterators that return references");
        return ProxyType::ConstructFromTwo(*left, *right_subscan);
    }
	FauxPointer<ProxyType> operator->() const
    {
		return FauxPointer<ProxyType>{this->operator*()};
    }
    bool operator==(const JoinIterator& other) const
    {
        return left == other.left && right == other.right && right_subscan == other.right_subscan &&
            left_end == other.left_end && right_end == other.right_end;
    }
    bool operator!=(const JoinIterator& other) const
    {
        return !operator==(other);
    }
private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_subscan;
    right_iterator right_end;

    right_iterator getRightIterator()
    {
        return right_subscan;
    }

    template<typename TLeft, typename TRight>
    friend struct JoinStream;
};
// Hash join
template<typename TRow, typename TLeft, typename TRight>
struct JoinIterator<TRow, TLeft, TRight, false>
{
    static const bool Ordered = TLeft::IndexType::Ordered;
    using KeyType = typename TLeft::IndexType::KeyType;
	using ProxyType = typename RowProxy<TRow, typename KeyType::AsRow>;
	using left_iterator = typename TLeft::iterator;
	using right_iterator = typename TRight::iterator;

    JoinIterator(left_iterator left, left_iterator left_end, const TRight& right_chan) :
        left(left), left_end(left_end), right_end(right_chan.end()), right(right_chan.end()), right_chan(&right_chan)
    {
        findMatch();
    }
    void goToEnd()
    {
        left = left_end;
        right_end = right_chan->end();
        right = right_end;
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
        goToEnd();
    }
    JoinIterator& operator++()
    {
        ++right;
        if (right == right_end) {
            ++left;
            findMatch();
        }
        return *this;
    }
	ProxyType operator*() const
    {
		static_assert(std::is_reference<decltype(*left)>::value, "Can only proxy iterators that return references");
		static_assert(std::is_reference<decltype(*right)>::value, "Can only proxy iterators that return references");
		return ProxyType::ConstructFromTwo(*left, *right);
    }
    FauxPointer<ProxyType> operator->() const
    {
		return FauxPointer<ProxyType>{this->operator*()};
    }
    bool operator==(const JoinIterator& other) const
    {
        return left == other.left && left_end == other.left_end &&
            right == other.right && right_end == other.right_end && right_chan == other.right_chan;
    }
    bool operator!=(const JoinIterator& other) const
    {
        return !operator==(other);
    }

private:
    left_iterator left;
    left_iterator left_end;
    right_iterator right;
    right_iterator right_end;
    const TRight* right_chan;

    right_iterator getRightIterator()
    {
        return right;
    }

    template<typename TLeft, typename TRight>
    friend struct JoinStream;
};

template<typename TLeft, typename TRight>
struct JoinStream : Channel
{
    using RowType = typename ConcatRows<typename TLeft::RowType, typename TRight::RowType>::type;
    using IndexType = JoinIterator<RowType, TLeft, TRight, CanMerge<TLeft, TRight>::value>;
    using iterator = IndexType;

    JoinStream(TLeft& left, TRight& right) : left(left), right(right) {}

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