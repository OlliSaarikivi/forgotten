#pragma once

#include "Process.h"
#include "Channel.h"

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
    template<typename TChannel, typename... TArgs>
    TChannel& makeChannel(TArgs&&... args)
    {
        static_assert(std::is_base_of<Channel, TChannel>::value, "the type must be a subclass of Channel");
        auto channel = std::make_unique<TChannel>(std::forward<TArgs>(args)...);
        TChannel& ret = *channel;
        addChannel(std::move(channel));
        return ret;
    }

    template<typename TJoinRow, template<typename> class TJoinContainer = Vector, typename TLeft, typename TRight>
    TransientChannel<TJoinRow, TJoinContainer>& makeTransientMergeJoin(TLeft&& left, TRight&& right)
    {
        auto& joined = makeChannel<TransientChannel<TJoinRow, TJoinContainer>>();
        makeProcess<MergeJoin>(left, right, joined);
        return joined;
    }
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
    vector<const Process*> execution_order;
};

