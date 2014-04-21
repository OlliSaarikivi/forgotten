#pragma once

#include "Channel.h"

// This is a workaround for weird error complaining about ambiguous destructors of Row base classes when objects are destructed
// in one of Boost's allocators.
namespace boost
{
    namespace detail
    {
        namespace allocator
        {
            template<typename... TColumns>
            void destroy(const Row<TColumns...>* p)
            {
                p->~Row();
            }
        }
    }
}

template<typename TRow, typename TIndex, typename TContainer>
struct Index : Channel
{
    using RowType = TRow;
    using IndexType = TIndex;
    using ContainerType = TContainer;
    using const_iterator = typename ContainerType::const_iterator;

    Index(ContainerType& boost_index) : boost_index(boost_index) {}

    const_iterator begin() const
    {
        return boost_index.cbegin();
    }
    const_iterator end() const
    {
        return boost_index.cend();
    }
    virtual void clear() override
    {
        boost_index.clear();
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        IndexPutHelper<TRow, TIndex>
            ::tPut(boost_index, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    typename ContainerType::size_type erase(TRow2&& row)
    {
        return IndexEraseHelper<TRow, TIndex>
            ::tErase(boost_index, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row)
    {
        return IndexEqualRangeHelper<TRow, TIndex>
            ::tEqualRange(boost_index, std::forward<TRow2>(row));
    }
private:
    ContainerType& boost_index;
};

template<typename TKey, typename TIndex>
struct IndexPutHelper
{
    template<typename TRow, typename TBoostIndex>
    static void tPut(TBoostIndex& index, TRow&& row)
    {
        auto result = index.emplace(std::forward<TRow>(row));
        while (!result.second) {
            index.erase(result.first);
            result = index.emplace(std::forward<TRow>(row));
        }
    }
};
template<typename TKey>
struct IndexPutHelper<TKey, None>
{
    template<typename TRow, typename TBoostIndex>
    static void tPut(TBoostIndex& index, TRow&& row)
    {
        auto result = index.emplace_back(std::forward<TRow>(row));
        while (!result.second) {
            index.erase(result.first);
            result = index.emplace_back(std::forward<TRow>(row));
        }
    }
};

template<typename TKey, typename TIndex>
struct IndexEraseHelper
{
    template<typename TRow, typename TBoostIndex>
    static typename TBoostIndex::size_type tErase(TBoostIndex& index, TRow&& row)
    {
        return index.erase(std::forward<TRow>(row));
    }
};

template<typename TRow, typename TIndex>
struct IndexEqualRangeHelper
{
    template<typename TRow2, typename TBoostIndex>
    static pair<typename TBoostIndex::const_iterator, typename TBoostIndex::const_iterator> tEqualRange(TBoostIndex& index, TRow2&& row)
    {
        return index.equal_range(std::forward<TRow2>(row));
    }
};

template<typename TRow, int N, template<int> class TIndexList, typename... TIndices>
struct IndicesHelper;
template<typename TRow, int N, template<int> class TIndexList, typename TIndex>
struct IndicesHelper<TRow, N, TIndexList, TIndex>
{
    template<typename TIndex2>
    using IndexNum = typename std::enable_if<std::is_same<TIndex, TIndex2>::value, std::integral_constant<int, N>>::type;
    template<int M>
    using NthIndex = typename std::enable_if<M == N, TIndex>::type;
    template<typename TContainer>
    IndicesHelper(TContainer& container) : index(container.get<N>()) {}
    template<int M, typename TIndex2>
    Index<TRow, TIndex2, TIndexList<M>>& getIndex()
    {
        assert(M == N);
        return index;
    }
    Index<TRow, TIndex, typename TIndexList<N>::type> index;
};
template<typename TRow, int N, template<int> class TIndexList, typename TIndex, typename... TIndices>
struct IndicesHelper<TRow, N, TIndexList, TIndex, TIndices...>
{
    using RestType = IndicesHelper<TRow, N + 1, TIndexList, TIndices...>;
    template<typename TIndex2>
    using IndexNum = typename std::conditional<std::is_same<TIndex, TIndex2>::value, std::integral_constant<int, N>, typename RestType::template IndexNum<TIndex2>>::type;
    template<int M>
    using NthIndex = typename std::conditional<M == N, TIndex, typename RestType::template NthIndex<M>>::type;
    template<typename TContainer>
    IndicesHelper(TContainer& container) : index(container.get<N>()), rest(container) {}
    template<int M, typename TIndex2>
    Index<TRow, TIndex2, TIndexList<M>>& getIndex()
    {
        return M == N ? index : rest.getIndex<M>();
    }
    Index<TRow, TIndex, typename TIndexList<N>::type> index;
    RestType rest;
};

template<typename TRow, typename TIndex, typename... TIndices>
struct Table<TRow, TIndex, TIndices...> : Channel
{
    using RowType = TRow;
    using IndexType = TIndex;
    using ContainerType = bmi::multi_index_container<TRow, bmi::indexed_by<TIndex, TIndices...>>;
    using const_iterator = typename ContainerType::const_iterator;
    using HelperType = IndicesHelper<TRow, 0, ContainerType::nth_index, TIndex, TIndices...>;

    Table() : container(), indices(container) {}

    const_iterator begin() const
    {
        return container.cbegin();
    }
    const_iterator end() const
    {
        return container.cend();
    }
    virtual void clear() override
    {
        container.clear();
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        IndexPutHelper<TRow, TIndex>
            ::tPut(container, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    typename ContainerType::size_type erase(TRow2&& row)
    {
        return IndexEraseHelper<TRow, TIndex>
            ::tErase(container, std::forward<TRow2>(row));
    }
    template<typename TIndex>
    Index<TRow, TIndex, typename ContainerType::template nth_index<HelperType::template IndexNum<TIndex>::value>::type>& getIndex()
    {
        return indices.getIndex<HelperType::template IndexNum<TIndex>::value, TIndex>();
    }
    template<int N>
    Index<TRow, typename HelperType::template NthIndex<N>, typename ContainerType::template nth_index<N>::type>& getIndex()
    {
        return indices.getIndex<N, typename HelperType::template NthIndex<N>>();
    }
private:
    HelperType indices;
    ContainerType container;
};
