#pragma once

#include "Channel.h"
#include "RowProxy.h"

template<typename THandle>
struct Reference
{
    typename THandle::Type actual;
    THandle next_free;
};

template<typename THandle>
struct StableIndex
{
	static const bool Ordered = false;
	using KeyType = Key<THandle>;
};

template<typename TData, typename THandle>
struct Stable : Channel
{
    using HandleType = THandle;
    using RowType = typename ConcatRows<Row<HandleType>, TData>::type;
    using IndexType = StableIndex<HandleType>;
    using ContainerType = array<RowType, HandleType::MaxRows>;
	using ProxyType = RowProxy<RowType, Row<HandleType>>;
	using iterator = ProxyIterator<typename ContainerType::iterator, ProxyType>;

    Stable() : rows(), valid_end(rows.begin())
    {
        free.setField(0);
        for (int i = 0; i < HandleType::MaxRows; ++i) {
            references[i].next_free.setField(i + 1);
            references[i].actual = HandleType::NullHandle();
        }
    }
	iterator begin() const
    {
		return iterator{ rows.begin() };
    }
	iterator end() const
    {
		return iterator{ valid_end };
    }
	iterator put(TData&& row)
    {
        assert(valid_end != rows.end());
        HandleType handle = free;
        auto& reference = references[handle.get()];
        free = reference.next_free;
        reference.actual = valid_end - rows.begin();
        valid_end->moveAll(std::forward<TData>(row));
        valid_end->set(handle);
		return iterator{ *(valid_end++) };
    }
    typename ContainerType::size_type erase(const HandleType& handle)
    {
        auto& reference = references[handle.get()];
        if (reference.actual != HandleType::NullHandle()) {
            iterator to_row = rows.begin() + reference.actual;
            --valid_end;
            *to_row = std::move(*valid_end);
            references[static_cast<HandleType&>(*to_row).get()] = to_row - rows.begin();
            references[handle.get()].next_free = free;
            references[handle.get()].actual = HandleType::NullHandle();
            free = handle;
            return 1;
        } else
            return 0;
    }
    pair<iterator, iterator> equalRange(const HandleType& handle) const
    {
        auto& reference = references[handle.get()];
        if (reference.actual != HandleType::NullHandle()) {
			auto range_begin = iterator{ rows.begin() + reference.actual };
			auto range_end = range_begin;
			++range_end;
            return std::make_pair(range_begin, range_end);
        } else {
			return std::make_pair(end(), end());
        }
    }
private:
    array<Reference<HandleType>, HandleType::MaxRows> references;
    HandleType free;
    ContainerType rows;
	typename ContainerType::iterator valid_end;
};
