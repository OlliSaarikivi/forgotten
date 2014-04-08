#pragma once

struct Channel;

struct Process
{
    virtual void tick(float step) const = 0;
	virtual void forEachInput(function<void(const Channel&)>) const = 0;
};

