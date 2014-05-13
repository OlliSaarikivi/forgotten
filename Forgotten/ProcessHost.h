#pragma once

#include "Apply.h"
#include "Process.h"
#include "Channel.h"
#include "Join.h"
#include "Amend.h"
#include "Subtract.h"
#include "Transform.h"

struct ProcessHost;

template<typename TChannel>
struct JoinBuilder
{
    JoinBuilder(TChannel& chan, ProcessHost& host) : chan(chan), host(host) {}
    template<typename TRight>
    JoinBuilder<JoinStream<TChannel, TRight>> join(TRight& right)
    {
        return JoinBuilder<JoinStream<TChannel, TRight>>(host.makeJoin(chan, right), host);
    }
    template<typename TRight>
    JoinBuilder<AmendStream<TChannel, TRight>> amend(TRight& right)
    {
        return JoinBuilder<AmendStream<TChannel, TRight>>(host.makeAmend(chan, right), host);
    }
    template<typename TRight>
    JoinBuilder<SubtractStream<TChannel, TRight>> subtract(TRight& right)
    {
        return JoinBuilder<SubtractStream<TChannel, TRight>>(host.makeSubtract(chan, right), host);
    }
    TChannel& select()
    {
        return chan;
    }
private:
    TChannel& chan;
    ProcessHost& host;
};

template<typename TTable, typename... TArgs>
auto callMake(ProcessHost* host, const TArgs&... args) -> decltype(host->make<TTable>(args...))
{
    return host->make<TTable>(args...);
}

template<typename TTable, typename... TArgs>
auto callMakeStream(ProcessHost* host, const TArgs&... args) -> decltype(host->makeStream<TTable>(args...))
{
    return host->makeStream<TTable>(args...);
}

template<typename TTable, typename... TArgs>
auto callMakeBuffer(ProcessHost* host, const TArgs&... args) -> decltype(host->makeBuffer<TTable>(args...))
{
    return host->makeBuffer<TTable>(args...);
}

template<typename... TArgs>
struct PlainBuilder
{
    PlainBuilder(ProcessHost* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator TTable&()
    {
        return apply(callMake<std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost*, TArgs...> args;
};

template<typename... TArgs>
struct StreamBuilder
{
    StreamBuilder(ProcessHost* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator TTable&()
    {
        return apply(callMakeStream<std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost*, TArgs...> args;
};

template<typename... TArgs>
struct BufferBuilder
{
    BufferBuilder(ProcessHost* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator std::pair<TTable&, TTable&>()
    {
        return apply(callMakeBuffer<std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost*, TArgs...> args;
};

struct ProcessHost
{
    ProcessHost(ForgottenGame& game) : game(game) {}

    void sortProcesses();
    void tick(float step);

    void addProcess(unique_ptr<Process>);
    template<typename TProcess, typename... TArgs>
    TProcess& makeProcess(TArgs&&... args)
    {
        static_assert(std::is_base_of<Process, TProcess>::value, "the type must be a subclass of Process");
        auto process = std::make_unique<TProcess>(game, *this, std::forward<TArgs>(args)...);
        TProcess& ret = *process;
        addProcess(std::move(process));
        return ret;
    }

    template<template<typename, typename, typename, typename, typename, typename, typename> class TProcess,
        typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7,
        typename... TRest>
        TProcess<T1, T2, T3, T4, T5, T6, T7>& makeProcess(T1& t1, T2& t2, T3& t3, T4& t4, T5& t5, T6& t6, T7& t7, TRest&&... rest)
    {
            return makeProcess<TProcess<T1, T2, T3, T4, T5, T6, T7>>(t1, t2, t3, t4, t5, t6, t7, std::forward<TRest>(rest)...);
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
    TChannel& make(TArgs&&... args)
    {
        auto channel = std::make_unique<TChannel>(std::forward<TArgs>(args)...);
        auto& ret = *channel;
        addChannel(std::move(channel));
        return ret;
    }

    template<typename TTable, typename... TArgs>
    pair<TTable&, TTable&> makeBuffer(TArgs&&... args)
    {
        auto& new_table = make<TTable>(args...);
        auto& old_table = make<TTable>(std::forward<TArgs>(args)...);
        auto ticker = std::make_unique<BufferTicker<TTable>>(new_table, old_table);
        addChannelTicker(std::move(ticker));
        return std::pair<TTable&, TTable&>(new_table, old_table);
    }

    template<typename TTable, typename... TArgs>
    TTable& makeStream(TArgs&&... args)
    {
        auto& ret = make<TTable>(std::forward<TArgs>(args)...);
        auto ticker = std::make_unique<ClearChannelTicker<TTable>>(ret);
        addChannelTicker(std::move(ticker));
        return ret;
    }

    template<typename TLeft, typename TRight>
    JoinStream<TLeft, TRight>& makeJoin(TLeft& left, TRight& right)
    {
        return make<JoinStream<TLeft, TRight>>(left, right);
    }

    template<typename TLeft, typename TRight>
    AmendStream<TLeft, TRight>& makeAmend(TLeft& left, TRight& right)
    {
        return make<AmendStream<TLeft, TRight>>(left, right);
    }

    template<typename TLeft, typename TRight>
    SubtractStream<TLeft, TRight>& makeSubtract(TLeft& left, TRight& right)
    {
        return make<SubtractStream<TLeft, TRight>>(left, right);
    }

    template<typename TChannel>
    JoinBuilder<TChannel> from(TChannel& chan)
    {
        return JoinBuilder<TChannel>(chan, *this);
    }

    template<typename... TTransforms, typename TChannel>
    const TransformStream<TChannel, TTransforms...>& makeTransform(const TChannel& chan)
    {
        return make<TransformStream<TChannel, TTransforms...>>(chan);
    }

    template<typename... TArgs>
    PlainBuilder<TArgs...> plain(TArgs&&... args)
    {
        return PlainBuilder<TArgs...>(this, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    StreamBuilder<TArgs...> stream(TArgs&&... args)
    {
        return StreamBuilder<TArgs...>(this, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    BufferBuilder<TArgs...> buffer(TArgs&&... args)
    {
        return BufferBuilder<TArgs...>(this, std::forward<TArgs>(args)...);
    }
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
    flat_set<unique_ptr<ChannelTicker>> channelTickers;
    vector<Process*> execution_order;
    ForgottenGame& game;
};