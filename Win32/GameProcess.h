#pragma once

#include "Game.h"
#include "ProcessHost.h"
#include "Process.h"

struct GameProcess : Process
{
    GameProcess(Game& game, ProcessHost<Game>& host) : game(game), host(host) {}

    Game& game;
    ProcessHost<Game>& host;
};

struct TimedGameProcess : TimedProcess
{
    TimedGameProcess(Game& game, ProcessHost<Game>& host) : game(game), host(host) {}

    Game& game;
    ProcessHost<Game>& host;
};