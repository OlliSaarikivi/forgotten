#pragma once

#include "Process.h"
#include "Channel.h"
#include "Join.h"
#include "Amend.h"
#include "Transform.h"

struct ProcessHost;

template<typename TChannel>
struct JoinBuilder
{
    JoinBuilder(const TChannel& chan, ProcessHost& host) : chan(chan), host(host) {}
    template<typename TRight>
    JoinBuilder<JoinStream<TChannel, TRight>> join(const TRight& right)
    {
        return JoinBuilder<JoinStream<TChannel, TRight>>(host.makeJoin(chan, right), host);
    }
    template<typename TRight>
    JoinBuilder<AmendStream<TChannel, TRight>> amend(const TRight& right)
    {
        return JoinBuilder<AmendStream<TChannel, TRight>>(host.makeAmend(chan, right), host);
    }
    const TChannel& select()
    {
        return chan;
    }
private:
    const TChannel& chan;
    ProcessHost& host;
};

struct ProcessHost
{
    void sortProcesses();
    void tick(float step);

    void addProcess(unique_ptr<Process>);
    template<typename TProcess, typename... TArgs>
    TProcess& makeProcess(TArgs&&... args)
    {
        static_assert(std::is_base_of<Process, TProcess>::value, "the type must be a subclass of Process");
        auto process = std::make_unique<TProcess>(std::forward<TArgs>(args)...);
        TProcess& ret = *process;
        addProcess(std::move(process));
        return ret;
    }

    template<template<typename, typename, typename, typename, typename, typename> class TProcess,
        typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
        typename... TRest>
        TProcess<T1, T2, T3, T4, T5, T6>& makeProcess(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, TRest&&... rest)
    {
            return makeProcess<TProcess<T1, T2, T3, T4, T5, T6>>(t1, t2, t3, t4, t5, t6, std::forward<TRest>(rest)...);
        }
    template<template<typename, typename, typename, typename, typename> class TProcess,
        typename T1, typename T2, typename T3, typename T4, typename T5,
        typename... TRest>
        TProcess<T1, T2, T3, T4, T5>& makeProcess(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, TRest&&... rest)
    {
            return makeProcess<TProcess<T1, T2, T3, T4, T5>>(t1, t2, t3, t4, t5, std::forward<TRest>(rest)...);
        }
    template<template<typename, typename, typename, typename> class TProcess,
        typename T1, typename T2, typename T3, typename T4,
        typename... TRest>
        TProcess<T1, T2, T3, T4>& makeProcess(T1& t1, T2& t2, T3& t3, T4& t4, TRest&&... rest)
    {
            return makeProcess<TProcess<T1, T2, T3, T4>>(t1, t2, t3, t4, std::forward<TRest>(rest)...);
        }
    template<template<typename, typename, typename> class TProcess, typename T1, typename T2, typename T3, typename... TRest>
    TProcess<T1, T2, T3>& makeProcess(T1& t1, T2& t2, T3& t3, TRest&&... rest)
    {
        return makeProcess<TProcess<T1, T2, T3>>(t1, t2, t3, std::forward<TRest>(rest)...);
    }
    template<template<typename, typename> class TProcess, typename T1, typename T2, typename... TRest>
    TProcess<T1, T2>& makeProcess(T1& t1, T2& t2, TRest&&... rest)
    {
        return makeProcess<TProcess<T1, T2>>(t1, t2, std::forward<TRest>(rest)...);
    }
    template<template<typename> class TProcess, typename T1, typename... TRest>
    TProcess<T1>& makeProcess(T1& t1, TRest&&... rest)
    {
        return makeProcess<TProcess<T1>>(t1, std::forward<TRest>(rest)...);
    }

    void addChannel(unique_ptr<Channel>);
    void addChannelTicker(unique_ptr<ChannelTicker>);

    template<typename TChannel, typename... TArgs>
    TChannel& makeChannel(TArgs&&... args)
    {
        auto channel = std::make_unique<TChannel>(std::forward<TArgs>(args)...);
        auto& ret = *channel;
        addChannel(std::move(channel));
        return ret;
    }

    template<typename TRow, typename TIndex = None, typename... TArgs>
    Table<TRow, TIndex>& makeTable(TArgs&&... args)
    {
        return makeChannel<Table<TRow, TIndex>>(std::forward<TArgs>(args)...);
    }

    template<typename TRow, typename THandle, typename... TArgs>
    Stable<TRow, THandle>& makeStable(TArgs&&... args)
    {
        return makeChannel<Stable<TRow, THandle>>(std::forward<TArgs>(args)...);
    }

    template<typename TRow, typename TIndex = None, typename... TArgs>
    Table<TRow, TIndex>& makeStream(TArgs&&... args)
    {
        auto& ret = makeTable<TRow, TIndex>(std::forward<TArgs>(args)...);
        auto ticker = std::make_unique<ClearChannelTicker<Table<TRow, TIndex>>>(ret);
        addChannelTicker(std::move(ticker));
        return ret;
    }

    template<typename TLeft, typename TRight>
    const JoinStream<TLeft, TRight>& makeJoin(const TLeft& left, const TRight& right)
    {
        return makeChannel<JoinStream<TLeft, TRight>>(left, right);
    }

    template<typename TLeft, typename TRight>
    const AmendStream<TLeft, TRight>& makeAmend(const TLeft& left, const TRight& right)
    {
        return makeChannel<AmendStream<TLeft, TRight>>(left, right);
    }

    template<typename TChannel>
    JoinBuilder<TChannel> from(const TChannel& chan)
    {
        return JoinBuilder<TChannel>(chan, *this);
    }

    template<typename... TTransforms, typename TChannel>
    const TransformStream<TChannel, TTransforms...>& makeTransform(const TChannel& chan)
    {
        return makeChannel<TransformStream<TChannel, TTransforms...>>(chan);
    }
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
    flat_set<unique_ptr<ChannelTicker>> channelTickers;
    vector<const Process*> execution_order;
};