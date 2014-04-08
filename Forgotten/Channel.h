#pragma once

struct Process;

struct Channel
{
    virtual void tick() {};
    virtual void registerProducer(const Process*) {};
    virtual void forEachImmediateDependency(function<void(const Process&)>) const {};
};

