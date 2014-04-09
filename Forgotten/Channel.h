#pragma once

struct Process;

struct Channel
{
    virtual void tick() {};
    virtual void registerProducer(const Process*) {};
    virtual void forEachImmediateDependency(function<void(const Process&)>) const {};
};

template<typename TRow, template<typename> class TContainer>
struct PersistentChannelBase : Channel
{
    typedef TRow RowType;
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
protected:
    TContainer<TRow> buffer;
    vector<const Process*> producers;
};

template<typename TRow, template<typename> class TContainer>
struct PersistentChannel : PersistentChannelBase<TRow, TContainer>
{
    template<typename TRow2>
    pair<typename TContainer::iterator, bool> emplace(TRow2&& row)
    {
        return buffer.emplace(std::forward<TRow2>(row));
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        auto result = buffer.emplace(row);
        if (!result.second) {
            *(result.first) = std::forward<TRow2>(row);
        }
    }
};

template<typename TRow>
struct PersistentChannel<TRow, Vector> : PersistentChannelBase<TRow, Vector>
{
    template<typename TRow2>
    pair<typename Vector<TRow>::iterator, bool> emplace(TRow2&& row)
    {
        buffer.emplace_back(std::forward<TRow2>(row));
        return std::make_pair(buffer.rbegin().base(), true);
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        buffer.emplace_back(row);
    }
};

template<typename TRow, template<typename> class TContainer = Vector>
struct TransientChannel : PersistentChannel<TRow, TContainer>
{
    virtual void tick() override
    {
        clear();
    }
};