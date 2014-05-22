#pragma once

#include "Row.h"
#include "Process.h"

template<typename TStats, typename TStat, int MaximumX100, int RateX100>
struct LinearRegeneration : TimedGameProcess
{
    LinearRegeneration(Game& game, ProcessHost<Game>& host, TStats& stats) : TimedGameProcess(game, host),
    stats(stats)
    {
        registerInput(stats);
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
                ++current;
            }
        }
    }
private:
    TStats& stats;
};
