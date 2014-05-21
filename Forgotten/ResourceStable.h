#pragma once

#include "Channel.h"
#include "DefaultValue.h"

template<typename TResource, typename THandle>
struct ResourceStable : Channel
{
    using ResourceType = TResource;
    using HandleType = THandle;
    using RowType = Row<HandleType, ResourceType>;
    using IndexType = None;
    using ValueType = typename std::remove_pointer<typename ResourceType::Type>::type;
    using ContainerType = array<pair<HandleType, unique_ptr<ValueType>>, HandleType::MaxRows>;
    using const_iterator = SingleValueIterator<RowType>;

    ResourceStable() : rows(), valid_end(rows.begin())
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
    RowType putResource(unique_ptr<ValueType> resource)
    {
        assert(valid_end != rows.end());
        HandleType handle = free;
        auto& reference = references[handle.get()];
        free = reference.next_free;
        reference.actual = valid_end - rows.begin();
        valid_end->first = handle;
        valid_end->second = std::move(resource);
        auto result = RowType({ handle }, { resource.get() });
        valid_end++;
        return result;
    }
    typename ContainerType::size_type erase(const HandleType& handle)
    {
        auto& reference = references[handle.get()];
        if (reference.actual != HandleType::NullHandle()) {
            auto to_row = rows.begin() + reference.actual;
            --valid_end;

            to_row->first = valid_end->first;
            to_row->second.swap(valid_end->second);
            valid_end->second.release();

            references[to_row->first] = to_row - rows.begin();
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
            auto iter = rows.begin() + reference.actual;
            return std::make_pair(const_iterator(RowType({ handle }, { iter->second.get() })), const_iterator());
        } else {
            return std::make_pair(const_iterator(), const_iterator());
        }
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        static_assert(!Intersects<TRow2, typename Row<HandleType>>::value, "can not update handle column");
        auto& reference = references[static_cast<HandleType&>(*position).get()];
        if (reference.actual != HandleType::NullHandle()) {
            auto iter = rows.begin() + reference.actual;
            iter->second.reset(static_cast<const ResourceType&>(row).get());
        }
    }
private:
    array<Reference<HandleType>, HandleType::MaxRows> references;
    HandleType free;
    ContainerType rows;
    typename ContainerType::iterator valid_end;
};
