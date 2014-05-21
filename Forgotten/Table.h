#pragma once

#include "Channel.h"

template<typename TRow, typename TIndex = None>
struct Table : Channel
{
    using IndexType = TIndex;
    using RowType = typename ConcatRows<typename IndexType::KeyType::AsRow, TRow>::type;
    using ContainerType = typename TIndex::template ContainerType<RowType>;
    using const_iterator = typename ContainerType::const_iterator;

    virtual void registerProducer(Process *process) override
    {
        producers.emplace_back(process);
    }
    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        for (auto& producer : producers) {
            f(*producer);
        }
    }
    const_iterator begin() const
    {
        return container.begin();
    }
    const_iterator end() const
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
    typename ContainerType::size_type erase(const TRow2& row)
    {
        return EraseHelper<RowType, ContainerType>
            ::tErase(container, MakeKeyRow<typename IndexType::KeyType, RowType>(row));
    }
    const_iterator erase(const_iterator first, const_iterator last)
    {
        return container.erase(first, last);
    }
    const_iterator erase(const_iterator position)
    {
        return container.erase(position);
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        static_assert(!Intersects<TRow2, typename IndexType::KeyType::AsRow>::value, "can not update key columns in place");
        const_cast<RowType&>(*position).setAll(row);
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return container.equal_range(MakeKeyRow<typename IndexType::KeyType, RowType>(std::forward<TRow2>(row)));
    }
    void copyRowsFrom(const Table<TRow, TIndex>& other)
    {
        container = other.container;
    }
private:
    ContainerType container;
    vector<Process*> producers;
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
    static typename TContainer::size_type tErase(TContainer& buffer, const TRow& row)
    {
        return buffer.erase(row);
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