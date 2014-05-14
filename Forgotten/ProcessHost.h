#pragma once

#include "Process.h"
#include "Apply.h"
#include "Channel.h"
#include "Join.h"
#include "Amend.h"
#include "Subtract.h"
#include "Transform.h"

template<typename TGame>
struct ProcessHost;

template<typename TGame, typename TChannel>
struct JoinBuilder
{
    JoinBuilder(TChannel& chan, ProcessHost<TGame>& host) : chan(chan), host(host) {}
    template<typename TRight>
    JoinBuilder<TGame, JoinStream<TChannel, TRight>> join(TRight& right)
    {
        return JoinBuilder<TGame, JoinStream<TChannel, TRight>>(host.makeJoin(chan, right), host);
    }
    template<typename TRight>
    JoinBuilder<TGame, AmendStream<TChannel, TRight>> amend(TRight& right)
    {
        return JoinBuilder<TGame, AmendStream<TChannel, TRight>>(host.makeAmend(chan, right), host);
    }
    template<typename TRight>
    JoinBuilder<TGame, SubtractStream<TChannel, TRight>> subtract(TRight& right)
    {
        return JoinBuilder<TGame, SubtractStream<TChannel, TRight>>(host.makeSubtract(chan, right), host);
    }
    TChannel& select()
    {
        return chan;
    }
private:
    TChannel& chan;
    ProcessHost<TGame>& host;
};

template<typename TGame, typename TTable, typename... TArgs>
auto callMake(ProcessHost<TGame>* host, const TArgs&... args) -> decltype(host->make<TTable>(args...))
{
    return host->make<TTable>(args...);
}

template<typename TGame, typename TTable, typename... TArgs>
auto callMakeStream(ProcessHost<TGame>* host, const TArgs&... args) -> decltype(host->makeStream<TTable>(args...))
{
    return host->makeStream<TTable>(args...);
}

template<typename TGame, typename TTable, typename... TArgs>
auto callMakeBuffer(ProcessHost<TGame>* host, const TArgs&... args) -> decltype(host->makeBuffer<TTable>(args...))
{
    return host->makeBuffer<TTable>(args...);
}

template<typename TGame, typename... TArgs>
struct PlainBuilder
{
    PlainBuilder(ProcessHost<TGame>* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator TTable&()
    {
        return apply(callMake<TGame, std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost<TGame>*, TArgs...> args;
};

template<typename TGame, typename... TArgs>
struct StreamBuilder
{
    StreamBuilder(ProcessHost<TGame>* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator TTable&()
    {
        return apply(callMakeStream<TGame, std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost<TGame>*, TArgs...> args;
};

template<typename TGame, typename... TArgs>
struct BufferBuilder
{
    BufferBuilder(ProcessHost<TGame>* host, TArgs&&... args) : args(host, args...) {}

    template<typename TTable>
    operator std::pair<TTable&, TTable&>()
    {
        return apply(callMakeBuffer<TGame, std::remove_reference<TTable>::type, TArgs...>, args);
    }
private:
    std::tuple<ProcessHost<TGame>*, TArgs...> args;
};

template<typename TGame>
struct ProcessHost
{
    ProcessHost(TGame& game) : game(game) {}

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
    JoinBuilder<TGame, TChannel> from(TChannel& chan)
    {
        return JoinBuilder<TGame, TChannel>(chan, *this);
    }

    template<typename... TTransforms, typename TChannel>
    const TransformStream<TChannel, TTransforms...>& makeTransform(const TChannel& chan)
    {
        return make<TransformStream<TChannel, TTransforms...>>(chan);
    }

    template<typename... TArgs>
    PlainBuilder<TGame, TArgs...> plain(TArgs&&... args)
    {
        return PlainBuilder<TGame, TArgs...>(this, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    StreamBuilder<TGame, TArgs...> stream(TArgs&&... args)
    {
        return StreamBuilder<TGame, TArgs...>(this, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    BufferBuilder<TGame, TArgs...> buffer(TArgs&&... args)
    {
        return BufferBuilder<TGame, TArgs...>(this, std::forward<TArgs>(args)...);
    }



    void addProcess(unique_ptr<Process> process)
    {
        processes.emplace(std::move(process));
        execution_order.clear();
    }
    void addChannel(unique_ptr<Channel> channel)
    {
        channels.emplace(std::move(channel));
    }
    void addChannelTicker(unique_ptr<ChannelTicker> ticker)
    {
        channelTickers.emplace(std::move(ticker));
    }
    void sortProcesses()
    {
        execution_order.clear();
        std::set<Process*> visited;
        std::set<Process*> in_this_host;
        for (auto &process : processes) {
            in_this_host.emplace(process.get());
        }
        function<void(Process &process)> visit = [&](Process &process) {
            if (visited.find(&process) != visited.end() ||
                in_this_host.find(&process) == in_this_host.end()) {
                return;
            }
            visited.emplace(&process);
            process.forEachInput([&](const Channel &input) {
                input.forEachProducer([&](Process &producer) {
                    visit(producer);
                });
            });
            execution_order.emplace_back(&process);
        };
        for (auto &process : processes) {
            visit(*process);
        }
    }
    void tick(float step)
    {
        assert(execution_order.size() == processes.size());
        for (const auto &ticker : channelTickers) {
            ticker->tick();
        }
        for (auto &process : execution_order) {
            process->doTick(step);
        }
    }
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
    flat_set<unique_ptr<ChannelTicker>> channelTickers;
    vector<Process*> execution_order;
    TGame& game;
};