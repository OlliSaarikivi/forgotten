#pragma once

#include "Channel.h"
#include <boost/pool/poolfwd.hpp>

template<typename HandleType>
BUILD_COLUMN(Handle, HandleType, handle)

template<typename HandleType>
struct HandleEntry
{
    HandleType actual;
    HandleType next_free;
};

template<typename... TColumns, int MaxRows>
struct Table<Row<TColumns...>, Handles<MaxRows>> : Channel
{
    using HandleType = typename boost::uint_value_t<MaxRows - 1>::least;
    using RowType = Row<Handle<HandleType>, TColumns...>;
    using IndexType = Handles<MaxRows>;
    using ContainerType = array<RowType, MaxRows>;
    using const_iterator = typename ContainerType::const_iterator;
    using iterator = typename ContainerType::iterator;

    Table() : free_handle(0), rows(), valid_end(rows.begin())
    {
        for (int i = 0; i < MaxRows; ++i) {
            handles[i].next_free = i + 1;
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
    HandleType put(TRow2&& row)
    {
        assert(valid_end != rows.end());
        HandleType handle_handle = free_handle;
        HandleEntry<HandleType>& newEntry = handles[handle_handle];
        free_handle = newEntry.next_free;
        newEntry.actual = valid_end - rows.begin();
        valid_end->setAll(row);
        valid_end->handle = handle_handle;
        ++valid_end;
        return handle_handle;
    }
    typename ContainerType::size_type erase(HandleType handle)
    {
        iterator to_row = rows.begin() + handle;
        --valid_end;
        *to_row = *valid_end;
        to_row->handle = handle;
        handles[handle].next_free = free_handle;
        free_handle = handle;
        return 1;
    }

private:
    array<HandleEntry<HandleType>, MaxRows> handles;
    HandleType free_handle;
    ContainerType rows;
    iterator valid_end;
};