#include "stdafx.h"
#include "Game.h"

Game::Game(Clock::duration max_sim_step, Clock::duration min_sim_step, int max_simulation_output_tick_ratio) :
max_sim_step(max_sim_step),
min_sim_step(min_sim_step),
max_simulation_substeps(max_simulation_substeps)
{
}

void Game::preRun()
{
    simulation.sortProcesses();
    output.sortProcesses();
}

/*
void Game::runFixSimVarOut()
{
preRun();

Clock::time_point simulation_time = Clock::now();
Clock::time_point now;
bool still_behind;

while (true) {
int simulation_tick_count = 0;
now = Clock::now();
while ((still_behind = (now - (simulation_time - simulation_step_size)) > simulation_step_size) &&
simulation_tick_count < max_simulation_output_tick_ratio) {
// Tick the simulation
simulation.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_step_size).count()) / 1e9);
// Do timing
simulation_time = simulation_time + simulation_step_size;
now = Clock::now();
++simulation_tick_count;
}

// If we are lagging behind advance the simulation time to the current time. This has the effect
// of slowing the game down.
if (still_behind) {
simulation_time = now;
}

// Tick the output (graphics etc.)
output.tick(Process::timeless_step);
}
}
*/

template<int WINDOW_SIZE>
struct FilteredMovingAveragePredictor
{
    static_assert(WINDOW_SIZE > 2, "the window size must be at least 3 so that the average is still defined after dropping min and max");

    FilteredMovingAveragePredictor() : window{}, next(window.begin())
    {
        for (auto& measurement : window) {
            measurement = Game::Clock::duration::zero();
        }
    }

    Game::Clock::duration predict()
    {
        Game::Clock::duration sum = Game::Clock::duration::zero(), min = window[0], max = window[0];
        for (const auto& measurement : window) {
            sum += measurement;
            if (measurement < min) {
                min = measurement;
            } else if (measurement > max) {
                max = measurement;
            }
        }
        return (sum - min - max) / (WINDOW_SIZE - 2);
    }

    void update(Game::Clock::duration duration)
    {
        *next = duration;
        ++next;
        next = next == window.end() ? window.begin() : next;
    }
private:
    typename std::array<Game::Clock::duration, WINDOW_SIZE> window;
    typename std::array<Game::Clock::duration, WINDOW_SIZE>::iterator next;
};

Game::Clock::duration absDifference(Game::Clock::time_point t1, Game::Clock::time_point t2)
{
    auto d1 = t1 - t2;
    auto d2 = t2 - t1;
    return d1 < d2 ? d1 : d2;
}

void Game::run()
{
    preRun();

    FilteredMovingAveragePredictor<5> simulation_predictor;
    FilteredMovingAveragePredictor<5> output_predictor;

    Clock::time_point now = Clock::now();
    Clock::time_point simulation_time = now;
    Clock::time_point output_time = now;

    while (true) {
        auto at_update_finish = now + output_predictor.predict();
        auto last_output_error = at_update_finish - output_time;
        auto new_output_error = absDifference(at_update_finish, simulation_time);

        /* If the simulation time is in the future we may sleep here until it is a better approximation. */
        if (new_output_error > last_output_error) {
            boost::this_thread::sleep_for((new_output_error - last_output_error) / 2);
            now = Clock::now();
        }

        auto begin_update = now;
        output.tick(timeless_step);
        now = Clock::now();
        output_time = now;
        output_predictor.update(now - begin_update);

        int simulation_substeps = 0;
        Clock::duration ideal_step, clamped_step;
        do {
            auto target_simulation_time = now + simulation_predictor.predict();
            ideal_step = target_simulation_time - simulation_time;
            clamped_step = std::max(min_sim_step, std::min(max_sim_step, ideal_step));

            auto begin_simulation = now;
            simulation.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(clamped_step).count()) / 1e9f);
            now = Clock::now();
            simulation_time += clamped_step;
            simulation_predictor.update(now - begin_simulation);

            ++simulation_substeps;

            /* We continue stepping while we are doing maximal steps and the maximum has not been reached.
             * If the maximum is ever reached the game will run slow. */
        } while (simulation_substeps < max_simulation_substeps && clamped_step < ideal_step);
    }
}