#pragma once

#include "Process.h"
#include "Channel.h"

struct ProcessHost
{
    void addProcess(unique_ptr<Process>);
    template<typename TProcess, typename... TArgs>
    TProcess& makeProcess(TArgs&&... args)
    {
        auto process = std::make_unique<TProcess>(std::forward<TArgs>(args)...);
        TProcess& ret = *process;
        addProcess(std::move(process));
        return ret;
    }

    void addChannel(unique_ptr<Channel>);
    template<typename TChannel, typename... TArgs>
    TChannel& makeChannel(TArgs&&... args)
    {
        auto channel = std::make_unique<TChannel>(std::forward<TArgs>(args)...);
        TChannel& ret = *channel;
        addChannel(std::move(channel));
        return ret;
    }

    void sortProcesses();
    void tick(float step);
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
	vector<const Process*> execution_order;
};

