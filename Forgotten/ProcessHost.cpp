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
    function<void(const Process &process)> visit = [&](const Process &process) {
        if (visited.find(&process) != visited.end()) {
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
