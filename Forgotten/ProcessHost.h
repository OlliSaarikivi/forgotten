#pragma once

#include "Process.h"
#include "Channel.h"
#include "Join.h"

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
    TProcess<T1, T2, T3>& makeProcess(T1& t1, T2& t2, T3& t3, TRest&... rest)
    {
        return makeProcess<TProcess<T1, T2, T3>>(t1, t2, t3, std::forward<TRest>(rest)...);
    }
    template<template<typename, typename> class TProcess, typename T1, typename T2, typename... TRest>
    TProcess<T1, T2>& makeProcess(T1& t1, T2& t2, TRest&... rest)
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

    template<typename TRow, typename TIndex = None, typename... TIndices, typename... TArgs>
    Table<TRow, TIndex, TIndices...>& makeTable(TArgs&&... args)
    {
        auto table = std::make_unique<Table<TRow, TIndex, TIndices...>>(std::forward<TArgs>(args)...);
        auto& ret = *table;
        addChannel(std::move(table));
        return ret;
    }

    template<typename TRow, typename TIndex = None, typename... TIndices, typename... TArgs>
    Table<TRow, TIndex, TIndices...>& makeStream(TArgs&&... args)
    {
        auto& ret = makeTable<TRow, TIndex, TIndices...>(std::forward<TArgs>(args)...);
        auto ticker = std::make_unique<ClearChannelTicker>(ret);
        addChannelTicker(std::move(ticker));
        return ret;
    }

    template<typename TRow, typename... TChans>
    JoinStream<TRow, TChans...>& makeJoin(TChans&... chans)
    {

        auto join = std::make_unique<JoinStream<TRow, TChans...>>(chans...);
        auto& ret = *join;
        addChannel(std::move(join));
        return ret;
    }
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
    flat_set<unique_ptr<ChannelTicker>> channelTickers;
    vector<const Process*> execution_order;
};

