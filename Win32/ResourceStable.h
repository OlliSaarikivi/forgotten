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
	/* Are these actually needed?
    const_iterator begin() const
    {
        auto begin = rows.begin();
        return const_iterator(RowType({ begin->first }, { begin->second.get() }));
    }
    const_iterator end() const
    {
        return const_iterator(RowType({ valid_end->first }, { valid_end->second.get() }));
    } */
    RowType putResource(unique_ptr<ValueType> resource)
    {
        assert(valid_end != rows.end());
        HandleType handle = free;
        auto result = RowType({ handle }, { resource.get() });
        auto& reference = references[handle.get()];
        free = reference.next_free;
        reference.actual = valid_end - rows.begin();
        valid_end->first = handle;
        valid_end->second = std::move(resource);
        valid_end++;
        return result;
    }
    void updateResource(const_iterator position, unique_ptr<ValueType> resource)
    {
        auto& reference = references[static_cast<const HandleType&>(*position).get()];
        if (reference.actual != HandleType::NullHandle()) {
            auto iter = rows.begin() + reference.actual;
            iter->second = std::move(resource);
        }
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
        } else
            return 0;
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
private:
    array<Reference<HandleType>, HandleType::MaxRows> references;
    HandleType free;
    ContainerType rows;
    typename ContainerType::iterator valid_end;
};
