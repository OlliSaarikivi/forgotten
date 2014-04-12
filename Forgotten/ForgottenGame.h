#pragma once

#include "Game.h"

static const Game::Clock::duration forgotten_max_step_size = boost::chrono::milliseconds(10);
static const Game::Clock::duration forgotten_min_step_size = boost::chrono::milliseconds(2);
static const int forgotten_max_simulation_substeps = 10;

struct ForgottenGame : Game
{

    ForgottenGame() :
    Game(forgotten_max_step_size, forgotten_min_step_size, forgotten_max_simulation_substeps),
    world(b2Vec2(0,0)) // Set gravity to zero
    {
    }

    b2World world;
};

