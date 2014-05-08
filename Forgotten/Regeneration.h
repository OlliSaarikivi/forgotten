#pragma once

#include "Row.h"
#include "Process.h"

template<typename TStats, typename TStat, int MaximumX100, int RateX100>
struct LinearRegeneration : TimedProcess
{
    LinearRegeneration(TStats& stats) :
    stats(stats)
    {
        stats.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(stats);
    }
    void tick(float step) override
    {
        const float Maximum = MaximumX100 / 100;
        const float Rate = RateX100 / 100;

        auto current = stats.begin();
        auto end = stats.end();
        while (current != end) {
            auto row = *current;
            TStat& stat = row;
            stat.setField(stat.get() + (Rate) * step);
            if (stat.get() > Maximum) {
                current = stats.erase(current);
            } else {
                stats.update(current, Key<TStat>::project(row));
            }
        }
    }
private:
    TStats& stats;
};
