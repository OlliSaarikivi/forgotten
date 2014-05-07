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
    virtual void forEachInput(function<void(const Channel&)>) const = 0;
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