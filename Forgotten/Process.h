#pragma once

struct Channel;

static const float timeless_step = -1;

struct Process
{

    virtual void doTick(float step) const
    {
        tick();
    }
    virtual void tick() const = 0;
    virtual void forEachInput(function<void(const Channel&)>) const = 0;
};

struct TimedProcess : Process
{
    virtual void doTick(float step) const override
    {
        tick(step);
    }
    virtual void tick(float step) const = 0;
    virtual void tick() const override
    {
        assert(false);
    };
};