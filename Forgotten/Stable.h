#pragma once

#include "Channel.h"

template<typename THandle, typename TIndex>
struct StableIndex
{
    using IndexType = TIndex;
    using RowType = typename TIndex::KeyType::template AsRowWithData<THandle>;
    using ContainerType = typename TIndex::template ContainerType<RowType>;
    using const_iterator = typename ContainerType::const_iterator;

    const_iterator begin() const
    {
        return container.cbegin();
    }
    const_iterator end() const
    {
        return container.cend();
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return EqualRangeHelper<RowType, ContainerType>
            ::tEqualRange(container, std::forward<TRow2>(row));
    }

    template<typename THandle, typename TData, typename... TIndices>
    friend struct Stable;
private:


    ContainerType container;
};

template<typename THandle, typename... TIndices>
struct IndicesHelper;
template<typename THandle, typename TIndex, typename... TIndices>
struct IndicesHelper<THandle, TIndex, TIndices...>
{
    using IndexType = StableIndex<THandle, TIndex>;

private:
    IndexType index;
    IndicesHelper<THandle, TIndices...> rest;
};

template<typename THandle>
struct HandleEntry
{
    THandle actual;
    THandle next_free;
};

template<typename TData, typename THandle, typename... TIndices>
struct Stable : Channel
{
    using HandleType = THandle;
    using RowType = typename ConcatRows<Row<HandleType>, TData>::type;
    using IndexType = None;
    using ContainerType = array<RowType, HandleType::MaxRows>;
    using const_iterator = typename ContainerType::const_iterator;
    using iterator = typename ContainerType::iterator;

    Stable() : rows(), valid_end(rows.begin())
    {
        free_handle.setField(0);
        for (int i = 0; i < HandleType::MaxRows; ++i) {
            handles[i].next_free.setField(i + 1);
        }
    }

    const_iterator begin() const
    {
        return rows.begin()
    }
    const_iterator end() const
    {
        return valid_end;
    }
    iterator begin()
    {
        return rows.begin()
    }
    iterator end()
    {
        return valid_end;
    }
    template<typename TRow2>
    RowType put(TRow2&& row)
    {
        assert(valid_end != rows.end());
        HandleType handle_handle = free_handle;
        HandleEntry<HandleType>& newEntry = handles[handle_handle.get()];
        free_handle = newEntry.next_free;
        newEntry.actual.setField(valid_end - rows.begin());
        valid_end->setAll(row);
        valid_end->set(handle_handle);
        return *(valid_end++);
    }
    template<typename TRow2>
    typename ContainerType::size_type erase(const TRow2& row)
    {
        const HandleType& handle = row;
        iterator to_row = rows.begin() + handle.get();
        --valid_end;
        *to_row = *valid_end;
        to_row->set(handle);
        handles[handle.get()].next_free = free_handle;
        free_handle = handle;
        return 1;
    }

private:
    array<HandleEntry<HandleType>, HandleType::MaxRows> handles;
    HandleType free_handle;
    ContainerType rows;
    iterator valid_end;
};