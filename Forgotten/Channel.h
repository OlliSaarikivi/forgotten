#pragma once

#include "Traits.h"

struct Process;

struct Channel
{
    Channel() = default;

    // No copies
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    virtual void tick() {};
    virtual void registerProducer(const Process*) {};
    virtual void forEachImmediateDependency(function<void(const Process&)>) const {};
};

template<typename TRow, typename TKey, template<typename, typename> class TContainer>
struct Table : Channel
{
    typedef TRow RowType;
    typedef typename TContainer<TRow, TKey>::const_iterator const_iterator;
    typedef TContainer<TRow, TKey> ContainerType;

    virtual void registerProducer(const Process *process) override
    {
        producers.emplace_back(process);
    }
    virtual void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        for (const auto& producer : producers) {
            f(*producer);
        }
    }
    typename ContainerType::const_iterator begin() const
    {
        return buffer.cbegin();
    }
    typename ContainerType::const_iterator end() const
    {
        return buffer.cend();
    }
    typename ContainerType::iterator begin()
    {
        return buffer.begin();
    }
    typename ContainerType::iterator end()
    {
        return buffer.end();
    }
    void clear()
    {
        buffer.clear();
    }
    template<typename TRow2>
    pair<typename ContainerType::iterator, bool> emplace(TRow2&& row)
    {
        return EmplaceHelper<TRow, TKey, ContainerType, IsMultiset<TRow, TKey, ContainerType>::value>
            ::tEmplace(buffer, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        PutHelper<TRow, TKey, ContainerType, IsMultiset<TRow, TKey, ContainerType>::value>
            ::tPut(buffer, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    typename ContainerType::size_type erase(TRow2&& row)
    {
        return EraseHelper<TRow, TKey, ContainerType>
            ::tErase(buffer, std::forward<TRow2>(row));
    }
private:
    ContainerType buffer;
    vector<const Process*> producers;
};

template<typename TRow, typename TKey, typename TContainer, bool IsMultiset>
struct EmplaceHelper
{
    template<typename TRow2>
    static pair<typename TContainer::iterator, bool> tEmplace(TContainer& buffer, TRow2&& row)
    {
        return buffer.emplace(std::forward<TRow2>(row));
    }
};
template<typename TRow, typename TKey, typename TContainer>
struct EmplaceHelper<TRow, TKey, TContainer, true>
{
    template<typename TRow2>
    static pair<typename TContainer::iterator, bool> tEmplace(TContainer& buffer, TRow2&& row)
    {
        return std::make_pair(buffer.emplace(std::forward<TRow2>(row)), true);
    }
};
template<typename TRow, typename TKey, bool IsMultiset>
struct EmplaceHelper<TRow, TKey, Vector<TRow, TKey>, IsMultiset>
{
    template<typename TRow2>
    static pair<typename Vector<TRow, TKey>::iterator, bool> tEmplace(Vector<TRow, TKey>& buffer, TRow2&& row)
    {
        buffer.emplace_back(std::forward<TRow2>(row));
        return std::make_pair(buffer.rbegin().base(), true);
    }
};

template<typename TRow, typename TKey, typename TContainer, bool IsMultiset>
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
template<typename TRow, typename TKey, typename TContainer>
struct PutHelper<TRow, TKey, TContainer, true>
{
    template<typename TRow2>
    static void tPut(TContainer& buffer, TRow2&& row)
    {
        return std::make_pair(buffer.emplace(std::forward<TRow2>(row)), true);
    }
};
template<typename TRow, typename TKey, bool IsMultiset>
struct PutHelper<TRow, TKey, Vector<TRow, TKey>, IsMultiset>
{
    template<typename TRow2>
    static void tPut(Vector<TRow, TKey>& buffer, TRow2&& row)
    {
        buffer.emplace_back(std::forward<TRow2>(row));
    }
};

template<typename TRow, typename TKey, typename TContainer>
struct EraseHelper
{
    template<typename TRow2>
    static typename TContainer::size_type tErase(TContainer& buffer, TRow2&& row)
    {
        return buffer.erase(std::forward<TRow2>(row));
    }
};
template<typename TRow, typename TKey>
struct EraseHelper<TRow, TKey, Vector<TRow, TKey>>
{
    template<typename TRow2>
    static typename Vector<TRow, TKey>::size_type tErase(Vector<TRow, TKey>& buffer, TRow2&& row)
    {
        auto original_size = buffer.size();
        buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [&](const TRow& other) {
            return row == other;
        }), buffer.end());
        return original_size - buffer.size();
    }
};

template<typename TRow, typename TKey, template<typename,typename> class TContainer>
struct Stream : Table<TRow, TKey, TContainer>
{
    typedef typename TContainer<TRow, TKey>::const_iterator const_iterator;
    virtual void tick() override
    {
        clear();
    }
};