#include "stdafx.h"
#include "Game.h"

#include "Forgotten.h"

#include "Box2DStep.h"
#include "Box2DReader.h"
#include "Debug.h"
#include "SDLRender.h"
#include "SDLEvents.h"
#include "Controls.h"
#include "Actions.h"
#include "Behaviors.h"
#include "Regeneration.h"
#include "ContactEffects.h"
#include "TrueSentenceInterpreter.h"

Game::Game() :
Hosts(*this),
max_sim_step(boost::chrono::milliseconds(10)),
min_sim_step(boost::chrono::milliseconds(2)),
max_simulation_substeps(10),
world(b2Vec2(0, 0)) // Set gravity to zero
{
    simulation.makeProcess<Box2DReader>();
    simulation.makeProcess<Box2DStep>(8, 3);
    simulation.makeProcess<SDLEvents>();
    simulation.makeProcess<MoveActionApplier>();
    simulation.makeProcess<Controls>();
    simulation.makeProcess<TargetFollowing>();
    simulation.makeProcess<LinearRegeneration<std::remove_reference<decltype(knockback_energies)>::type, KnockbackEnergy, 100, 1000>>(knockback_energies);
    simulation.makeProcess<KnockbackEffect>();
    simulation.makeProcess<TrueSentenceInterpreter>();
    output.makeProcess<SDLRender>(window);
}

void Game::preRun()
{
    simulation.sortProcesses();
    output.sortProcesses();
}

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

            auto begin_simulation = now;
            simulation.tick(((float)boost::chrono::duration_cast<boost::chrono::nanoseconds>(clamped_step).count()) / 1e9f);
            now = Clock::now();
            simulation_time += clamped_step;
            auto simulation_actual = now - begin_simulation;
            simulation_predictor.update(simulation_actual);
            ++simulation_substeps;
        }

        if (simulation_time < now && now - simulation_time > max_sim_step) {
            simulation_time = now - max_sim_step;
        }
    }
}