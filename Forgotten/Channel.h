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

template<typename TRow, template<typename> class TContainer>
struct PersistentChannel : Channel
{
    typedef TRow RowType;
    typedef typename TContainer<TRow>::const_iterator const_iterator;
    typedef TContainer<TRow> ContainerType;

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
    typename TContainer<TRow>::const_iterator begin() const
    {
        return buffer.cbegin();
    }
    typename TContainer<TRow>::const_iterator end() const
    {
        return buffer.cend();
    }
    typename TContainer<TRow>::iterator begin()
    {
        return buffer.begin();
    }
    typename TContainer<TRow>::iterator end()
    {
        return buffer.end();
    }
    void clear()
    {
        buffer.clear();
    }
    template<typename TRow2>
    pair<typename TContainer<TRow>::iterator, bool> emplace(TRow2&& row)
    {
        return EmplaceHelper<TRow, TContainer<TRow>, IsMultiset<TRow,TContainer<TRow>>::value>
            ::tEmplace(buffer, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        PutHelper<TRow, TContainer<TRow>, IsMultiset<TRow, TContainer<TRow>>::value>
            ::tPut(buffer, std::forward<TRow2>(row));
    }
    template<typename TRow2>
    typename TContainer<TRow>::size_type erase(TRow2&& row)
    {
        return EraseHelper<TRow, TContainer<TRow>>
            ::tErase(buffer, std::forward<TRow2>(row));
    }
private:
    TContainer<TRow> buffer;
    vector<const Process*> producers;
};

template<typename TRow, typename TContainer, bool IsMultiset>
struct EmplaceHelper
{
    template<typename TRow2>
    static pair<typename TContainer::iterator, bool> tEmplace(TContainer& buffer, TRow2&& row)
    {
        return buffer.emplace(std::forward<TRow2>(row));
    }
};
template<typename TRow, typename TContainer>
struct EmplaceHelper<typename TRow, typename TContainer, true>
{
    template<typename TRow2>
    static pair<typename TContainer::iterator, bool> tEmplace(TContainer& buffer, TRow2&& row)
    {
        return std::make_pair(buffer.emplace(std::forward<TRow2>(row)), true);
    }
};
template<typename TRow, bool IsMultiset>
struct EmplaceHelper<TRow, Vector<TRow>, IsMultiset>
{
    template<typename TRow2>
    static pair<typename Vector<TRow>::iterator, bool> tEmplace(Vector<TRow>& buffer, TRow2&& row)
    {
        buffer.emplace_back(std::forward<TRow2>(row));
        return std::make_pair(buffer.rbegin().base(), true);
    }
};

template<typename TRow, typename TContainer, bool IsMultiset>
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
template<typename TRow, typename TContainer>
struct PutHelper<typename TRow, typename TContainer, true>
{
    template<typename TRow2>
    static void tPut(TContainer& buffer, TRow2&& row)
    {
        return std::make_pair(buffer.emplace(std::forward<TRow2>(row)), true);
    }
};
template<typename TRow, bool IsMultiset>
struct PutHelper<TRow, Vector<TRow>, IsMultiset>
{
    template<typename TRow2>
    static void tPut(Vector<TRow>& buffer, TRow2&& row)
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
struct EraseHelper<TRow, Vector<TRow>>
{
    template<typename TRow2>
    static typename Vector<TRow>::size_type tErase(Vector<TRow>& buffer, TRow2&& row)
    {
        auto original_size = buffer.size();
        buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [&](const TRow& other) {
            return row == other;
        }), buffer.end());
        return original_size - buffer.size();
    }
};

template<typename TRow, template<typename> class TContainer>
struct TransientChannel : PersistentChannel<TRow, TContainer>
{
    typedef typename TContainer<TRow>::const_iterator const_iterator;
    virtual void tick() override
    {
        clear();
    }
};