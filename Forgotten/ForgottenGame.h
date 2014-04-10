#pragma once

#include "Game.h"

static const Game::Clock::duration forgotten_simulation_step_size = boost::chrono::milliseconds(10);
static const int forgotten_max_simulation_output_tick_ratio = 10;

struct ForgottenGame : Game
{

    ForgottenGame() :
    Game(forgotten_simulation_step_size, forgotten_max_simulation_output_tick_ratio),
    world(b2Vec2(1,0)) // Set gravity to zero
    {
    }

    b2World world;
};

