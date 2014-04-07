#pragma once

struct Channel;

struct Process
{
	virtual void tick() const = 0;
	virtual void forEachInput(function<void(const Channel&)>) const = 0;
};

