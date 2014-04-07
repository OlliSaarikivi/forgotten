#pragma once

struct Process;

struct Channel
{
    virtual void tick() = 0;
    virtual void registerProducer(weak_ptr<const Process>) {};
    virtual void forEachImmediateDependency(function<void(const Process&)>) {};
};

