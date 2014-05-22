#pragma once

#include "Channel.h"

template<typename THandle>
struct Reference
{
    typename THandle::Type actual;
    THandle next_free;
};

template<typename TData, typename THandle>
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
        free.setField(0);
        for (int i = 0; i < HandleType::MaxRows; ++i) {
            references[i].next_free.setField(i + 1);
            references[i].actual = HandleType::NullHandle();
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
    RowType put(const TData& row)
    {
        assert(valid_end != rows.end());
        HandleType handle = free;
        auto& reference = references[handle.get()];
        free = reference.next_free;
        reference.actual = valid_end - rows.begin();
        valid_end->setAll(row);
        valid_end->set(handle);
        return *(valid_end++);
    }
    typename ContainerType::size_type erase(const HandleType& handle)
    {
        auto& reference = references[handle.get()];
        if (reference.actual != HandleType::NullHandle()) {
            iterator to_row = rows.begin() + reference.actual;
            --valid_end;
            *to_row = *valid_end;
            references[static_cast<HandleType&>(*to_row).get()] = to_row - rows.begin();
            references[handle.get()].next_free = free;
            references[handle.get()].actual = HandleType::NullHandle();
            free = handle;
            return 1;
        } else {
            return 0;
        }
    }
    pair<const_iterator, const_iterator> equalRange(const HandleType& handle) const
    {
        auto& reference = references[handle.get()];
        if (reference.actual != HandleType::NullHandle()) {
            const_iterator iter = rows.begin() + reference.actual;
            return std::make_pair(iter, iter + 1);
        } else {
            return std::make_pair(rows.end(), rows.end());
        }
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        static_assert(!Intersects<TRow2, typename Row<HandleType>>::value, "can not update handle column");
        const_cast<RowType&>(*position).setAll(row);
    }
private:
    array<Reference<HandleType>, HandleType::MaxRows> references;
    HandleType free;
    ContainerType rows;
    iterator valid_end;
};
