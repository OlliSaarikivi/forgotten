#include "stdafx.h"
#include "Game.h"

Game::Game(Clock::duration max_sim_step, Clock::duration min_sim_step, int max_simulation_substeps) :
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
        //return (sum - min - max) / (WINDOW_SIZE - 2);
        return sum / WINDOW_SIZE;
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

Game::Clock::duration abs(Game::Clock::duration d)
{
    return d < Game::Clock::duration::zero() ? -d : d;
}

void Game::run()
{
    static_assert(std::numeric_limits<Clock::rep>::digits > 60, "clock representation is not large enough");

    preRun();

    FilteredMovingAveragePredictor<7> simulation_predictor;
    FilteredMovingAveragePredictor<7> output_predictor;

    Clock::time_point now = Clock::now();
    Clock::time_point simulation_time = now;
    Clock::time_point output_time = now;

    while (true) {
        auto at_update_finish = now + output_predictor.predict();
        auto last_output_error = at_update_finish - output_time;
        auto new_output_error = abs(at_update_finish - simulation_time);

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
        while (simulation_substeps < max_simulation_substeps && now - simulation_time > min_sim_step) {

            auto simulation_prediction = simulation_predictor.predict();
            auto delta_to_now = (now - simulation_time);

            if (simulation_prediction < max_sim_step) {
                auto substeps_needed = delta_to_now / (max_sim_step - simulation_prediction) + 1;
                ideal_step = substeps_needed > max_simulation_substeps ? max_sim_step :
                    (delta_to_now + simulation_prediction * substeps_needed) / substeps_needed;
            } else {
                ideal_step = max_sim_step;
            }

            clamped_step = std::max(min_sim_step, std::min(max_sim_step, ideal_step));

            //std::cerr << std::fixed << std::setprecision(5);
            //std::cerr << "Ideal: " << std::setw(9) << (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(ideal_step).count()) / 1e6f);
            //std::cerr << " Clamped: " << std::setw(9) << (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(clamped_step).count()) / 1e6f);
            //std::cerr << " Prediction: " << std::setw(9) << (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_prediction).count()) / 1e6f);

            auto begin_simulation = now;
            simulation.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(clamped_step).count()) / 1e9f);
            now = Clock::now();
            simulation_time += clamped_step;
            auto simulation_actual = now - begin_simulation;
            simulation_predictor.update(simulation_actual);
            ++simulation_substeps;

            //std::cerr << " Error: " << std::setw(9) << ((((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_actual - simulation_prediction).count()) / 1e6f) - (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_prediction).count()) / 1e6f));
            //std::cerr << "\n";

        }

        //std::cerr << std::fixed << std::setprecision(5);
        //std::cerr << " Clamped: " << std::setw(9) << (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(clamped_step).count()) / 1e6f);
        //std::cerr << " Drift: " << std::setw(9) << (((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(simulation_time - now).count()) / 1e6f);
        //std::cerr << "\n";

        if (simulation_time < now && now - simulation_time > max_sim_step) {
            simulation_time = now - max_sim_step;
        }
    }
}