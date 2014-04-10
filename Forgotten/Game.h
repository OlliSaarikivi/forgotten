#pragma once

#include "ProcessHost.h"
#include "Channel.h"

struct Game
{
    typedef boost::chrono::high_resolution_clock Clock;

    Game(Clock::duration simulation_step_size, int max_simulation_output_tick_ratio);
    void run();
    ProcessHost simulation;
    ProcessHost output;
private:
    Clock::duration simulation_step_size;
    int max_simulation_output_tick_ratio;
};

