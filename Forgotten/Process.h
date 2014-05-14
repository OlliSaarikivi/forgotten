#pragma once

struct Channel;

static const float timeless_step = -1;

struct Process
{
    virtual void doTick(float step)
    {
        tick();
    }
    virtual void tick() = 0;

    void forEachInput(function<void(const Channel&)> f) const
    {
        for (auto input : inputs) {
            f(*input);
        }
    }
    void registerInput(const Channel& channel)
    {
        inputs.emplace_back(&channel);
    }
private:
    vector<const Channel*> inputs;
};

struct TimedProcess : Process
{
    virtual void doTick(float step) override
    {
        tick(step);
    }
    virtual void tick(float step) = 0;
    virtual void tick() override
    {
        assert(false);
    };
};

#include "Proxies.h"
#include "Row.h"