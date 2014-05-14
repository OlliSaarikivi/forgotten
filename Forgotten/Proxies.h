#pragma once

#include "Row.h"
#include "Process.h"
#include "Channel.h"

template<typename TChannel>
struct Source : Channel
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = typename TChannel::const_iterator;

    Source(Process* process, const TChannel& channel) : channel(channel)
    {
        process->registerInput(channel);
    }
    Source(const Source<TChannel>&) = delete;
    Source<TChannel>& operator=(const Source<TChannel>&) = delete;

    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        channel.forEachProducer(f);
    }
    const_iterator begin() const
    {
        return channel.begin();
    }
    const_iterator end() const
    {
        return channel.end();
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return channel.equalRange(std::forward<TRow2>(row));
    }
    void update(const_iterator position, const Row<>& row)
    {
    }
private:
    const TChannel& channel;
};

template<typename TChannel>
struct Sink : Channel
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = typename TChannel::const_iterator;

    Sink(Process* process, TChannel& channel) : channel(channel)
    {
        channel.registerProducer(process);
    }
    Sink(const Sink<TChannel>&) = delete;
    Sink<TChannel>& operator=(const Sink<TChannel>&) = delete;

    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        channel.forEachProducer(f);
    }
    void clear()
    {
        channel.clear();
    }
    void put(const RowType& row)
    {
        channel.put(row);
    }
    template<typename TRow2>
    typename TChannel::ContainerType::size_type erase(const TRow2& row)
    {
        return channel.erase(row);
    }
    const_iterator erase(const_iterator first, const_iterator last)
    {
        return channel.erase(first, last);
    }
    const_iterator erase(const_iterator position)
    {
        return channel.erase(position);
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        channel.update(position, row);
    }
private:
    TChannel& channel;
};

template<typename TChannel>
struct Mutable : Channel
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = typename TChannel::const_iterator;

    Mutable(Process* process, TChannel& channel) : channel(channel)
    {
        process->registerInput(channel);
        channel.registerProducer(process);
    }
    Mutable(const Mutable<TChannel>&) = delete;
    Mutable<TChannel>& operator=(const Mutable<TChannel>&) = delete;

    virtual void forEachProducer(function<void(Process&)> f) const override
    {
        channel.forEachProducer(f);
    }
    const_iterator begin() const
    {
        return channel.begin();
    }
    const_iterator end() const
    {
        return channel.end();
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return channel.equalRange(std::forward<TRow2>(row));
    }
    void clear()
    {
        channel.clear();
    }
    void put(const RowType& row)
    {
        channel.put(row);
    }
    template<typename TRow2>
    typename TChannel::ContainerType::size_type erase(const TRow2& row)
    {
        return channel.erase(row);
    }
    const_iterator erase(const_iterator first, const_iterator last)
    {
        return channel.erase(first, last);
    }
    const_iterator erase(const_iterator position)
    {
        return channel.erase(position);
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        channel.update(position, row);
    }
private:
    TChannel& channel;
};

#define SOURCE(VAR,GAME_VAR) Source<std::remove_reference<decltype(Game::GAME_VAR)>::type> VAR \
    = Source<std::remove_reference<decltype(Game::GAME_VAR)>::type>(this, game.##GAME_VAR);

#define BUFFER_SOURCE(VAR,GAME_VAR) Source<std::remove_reference<decltype(Game::GAME_VAR)::second_type>::type> VAR \
    = Source<std::remove_reference<decltype(Game::GAME_VAR)::second_type>::type>(this, game.##GAME_VAR##.second);

#define SINK(VAR,GAME_VAR) Sink<std::remove_reference<decltype(Game::GAME_VAR)>::type> VAR \
    = Sink<std::remove_reference<decltype(Game::GAME_VAR)>::type>(this, game.##GAME_VAR);

#define BUFFER_SINK(VAR,GAME_VAR) Sink<std::remove_reference<decltype(Game::GAME_VAR)::first_type>::type> VAR \
    = Sink<std::remove_reference<decltype(Game::GAME_VAR)::first_type>::type>(this, game.##GAME_VAR##.first);

#define MUTABLE(VAR,GAME_VAR) Mutable<std::remove_reference<decltype(Game::GAME_VAR)>::type> VAR \
    = Mutable<std::remove_reference<decltype(Game::GAME_VAR)>::type>(this, game.##GAME_VAR);