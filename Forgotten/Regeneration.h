#pragma once

#include "Row.h"
#include "Process.h"

template<typename TStats, typename TStat, int MaximumX100, int RateX100>
struct LinearRegeneration : TimedProcess
{
    const float Maximum = MaximumX100 / 100;
    const float Rate = RateX100 / 100;

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
        auto current = stats.begin();
        while (current != stats.end()) {
            TStat& stat = *current;
            stat.setField(stat.get() + (Rate) * step);
            if (stat.get() > Maximum) {

            }
        }
    }
private:
    TStats& stats;
};
