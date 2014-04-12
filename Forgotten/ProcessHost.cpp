#include "stdafx.h"
#include "ProcessHost.h"

void ProcessHost::addProcess(unique_ptr<Process> process)
{
    processes.emplace(std::move(process));
    execution_order.clear();
}

void ProcessHost::addChannel(unique_ptr<Channel> channel)
{
    channels.emplace(std::move(channel));
}

void ProcessHost::sortProcesses()
{
    execution_order.clear();
    set<const Process*> visited;
    set<const Process*> in_this_host;
    for (const auto &process : processes) {
        in_this_host.emplace(process.get());
    }
    function<void(const Process &process)> visit = [&](const Process &process) {
        if (visited.find(&process) != visited.end() ||
            in_this_host.find(&process) == in_this_host.end())
        {
            return;
        }
        visited.emplace(&process);
        process.forEachInput([&](const Channel &input) {
            input.forEachImmediateDependency([&](const Process &producer) {
                visit(producer);
            });
        });
        execution_order.emplace_back(&process);
    };
    for (const auto &process : processes) {
        visit(*process);
    }
}

void ProcessHost::tick(float step)
{
    assert(execution_order.size() == processes.size());
    for (const auto &channel : channels) {
        channel->tick();
    }
    for (const auto &process : execution_order) {
        process->doTick(step);
    }
}
