#pragma once

#include "ProcessHost.h"
#include "Channel.h"

struct Game
{
    typedef boost::chrono::high_resolution_clock Clock;

    Game(Clock::duration max_sim_step, Clock::duration min_sim_step, int max_simulation_substeps);
    void run();
    ProcessHost simulation;
    ProcessHost output;
private:
    void preRun();
    Clock::duration max_sim_step;
    Clock::duration min_sim_step;
    int max_simulation_substeps;
};

