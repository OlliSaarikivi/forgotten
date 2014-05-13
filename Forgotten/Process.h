#pragma once

#include "Proxies.h"

struct Channel;

static const float timeless_step = -1;

struct Game;

struct Process
{
    Process(Game& game, ProcessHost& host) : game(game), host(host) {}

    virtual void doTick(float step)
    {
        tick();
    }
    virtual void tick() = 0;

    void forEachInput(function<void(const Channel&)> f) const
    {
        for (auto input : inputs) {
            f(*input);
        }
    }
    void registerInput(const Channel& channel)
    {
        inputs.emplace_back(&channel);
    }

    Game& game;
    ProcessHost& host;
private:
    vector<const Channel*> inputs;
};

template<typename TGame>
struct TimedProcess : Process
{
    TimedProcess(Game& game, ProcessHost& host) : Process(game, host) {}

    virtual void doTick(float step) override
    {
        tick(step);
    }
    virtual void tick(float step) = 0;
    virtual void tick() override
    {
        assert(false);
    };
};

#define SOURCE(VAR,GAME_VAR) Source<std::remove_reference<decltype(GAME_VAR)>::type> VAR \
    = Source<std::remove_reference<decltype(GAME_VAR)>::type>(this, GAME_VAR);

#define SINK(VAR,GAME_VAR) Sink<std::remove_reference<decltype(GAME_VAR)>::type> VAR \
    = Sink<std::remove_reference<decltype(GAME_VAR)>::type>(this, GAME_VAR);

#define MUTABLE(VAR,GAME_VAR) Mutable<std::remove_reference<decltype(GAME_VAR)>::type> VAR \
    = Mutable<std::remove_reference<decltype(GAME_VAR)>::type>(this, GAME_VAR);