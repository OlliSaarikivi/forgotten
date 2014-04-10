#include "stdafx.h"
#include "Game.h"

Game::Game(Clock::duration simulation_step_size, int max_simulation_output_tick_ratio) :
simulation_step_size(simulation_step_size),
max_simulation_output_tick_ratio(max_simulation_output_tick_ratio)
{
}

void Game::run()
{
    simulation.sortProcesses();
    output.sortProcesses();

    Clock::time_point simulation_time = Clock::now();
    Clock::time_point previous_output = simulation_time;
    Clock::time_point now;
    bool still_behind;

    while (true) {
        int simulation_tick_count = 0;
        now = Clock::now();
        while ((still_behind = (now - (simulation_time - simulation_step_size)) > simulation_step_size) &&
            simulation_tick_count < max_simulation_output_tick_ratio)
        {
            // Tick the simulation
            simulation.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_step_size).count()) / 1e9);
            // Do timing
            simulation_time = simulation_time + simulation_step_size;
            now = Clock::now();
            ++simulation_tick_count;
        }

        /* If we are lagging behind advance the simulation time to the current time. This has the effect
         * of slowing the game down. */
        if (still_behind) {
            simulation_time = now;
        }

        // Tick the output (graphics etc.)
        Clock::duration graphics_step_size = now - previous_output;
        output.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(graphics_step_size).count()) / 1e9);
        previous_output = now;
    }
}