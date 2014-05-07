#pragma once

#include "Channel.h"

template<typename TRow, typename TIndex>
struct Table : Channel
{
    using IndexType = TIndex;
    using RowType = typename RowWithKey<TRow, typename IndexType::KeyType>::type;
    using ContainerType = typename TIndex::template ContainerType<RowType>;
    using iterator = typename ContainerType::iterator;
    using const_iterator = typename ContainerType::const_iterator;

    virtual void registerProducer(const Process *process) override
    {
        producers.emplace_back(process);
    }
    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        for (const auto& producer : producers) {
            f(*producer);
        }
    }
    iterator begin()
    {
        return container.begin();
    }
    iterator end()
    {
        return container.end();
    }
    void clear()
    {
        container.clear();
    }
    void put(const RowType& row)
    {
        PutHelper<RowType, ContainerType>
            ::tPut(container, row);
    }
    template<typename TRow2>
    typename ContainerType::size_type erase(TRow2&& row)
    {
        return EraseHelper<RowType, ContainerType>
            ::tErase(container, std::forward<TRow2>(row));
    }
    iterator erase(const_iterator first, const_iterator last)
    {
        return PositionEraseHelper<ContainerType, iterator>
            ::tErase(container, first, last);
    }
    iterator erase(const_iterator position)
    {
        return erase(position, std::advance(position, 1));
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return EqualRangeHelper<RowType, ContainerType>
            ::tEqualRange(container, std::forward<TRow2>(row));
    }
private:
    ContainerType container;
    vector<const Process*> producers;
};

template<typename TRow, typename TContainer>
struct PutHelper
{
    template<typename TRow2>
    static void tPut(TContainer& buffer, TRow2&& row)
    {
        auto result = buffer.emplace(row);
        if (!result.second) {
            /* This is safe because result.first is guaranteed to be equal (in the ordering)
            * to the row to insert. */
            const_cast<TRow&>(*(result.first)) = std::forward<TRow2>(row);
        }
    }
};
template<typename TRow>
struct PutHelper<TRow, vector<TRow>>
{
    template<typename TRow2>
    static void tPut(vector<TRow>& buffer, TRow2&& row)
    {
        buffer.emplace_back(std::forward<TRow2>(row));
    }
};

template<typename TRow, typename TContainer>
struct EraseHelper
{
    template<typename TRow2>
    static typename TContainer::size_type tErase(TContainer& buffer, TRow2&& row)
    {
        return buffer.erase(std::forward<TRow2>(row));
    }
};
template<typename TRow>
struct EraseHelper<TRow, vector<TRow>>
{
    template<typename TRow2>
    static typename vector<TRow>::size_type tErase(vector<TRow>& buffer, TRow2&& row)
    {
        auto original_size = buffer.size();
        buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [&](const TRow& other) {
            return row == other;
        }), buffer.end());
        return original_size - buffer.size();
    }
};

template<typename TContainer, typename TIterator>
struct PositionEraseHelper
{
    static TIterator tErase(TContainer& container, TIterator first, TIterator last)
    {
        return container.erase(first, last);
    }
};

template<typename TRow, typename TContainer>
struct EqualRangeHelper
{
    template<typename TRow2>
    static pair<typename TContainer::const_iterator, typename TContainer::const_iterator> tEqualRange(const TContainer& container, TRow2&& row)
    {
        return container.equal_range(TRow(std::forward<TRow2>(row)));
    }
};