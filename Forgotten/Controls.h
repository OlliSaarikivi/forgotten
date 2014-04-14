#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TKeysDown, typename TKeyPresses, typename TKeyReleases,
    typename TControllables, typename TMoveActions, typename THeadingActions>
struct Controls : Process
{
    Controls(const TKeysDown& keys_down, const TKeyPresses& key_presses, const TKeyReleases& key_releases,
    const TControllables& controllables, TMoveActions& move_actions, THeadingActions& heading_actions) :
    keys_down(keys_down),
    key_presses(key_presses),
    key_releases(key_releases),
    controllables(controllables),
    move_actions(move_actions),
    heading_actions(heading_actions)
    {
        move_actions.registerProducer(this);
        heading_actions.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(keys_down);
        f(key_presses);
        f(key_releases);
        f(controllables);
    }
    void tick() const override
    {
        vec2 move_sum(0, 0);
        bool move = false;
        for (const auto& key_down : keys_down.read()) {
            switch (key_down.sdl_scancode) {
            case SDL_SCANCODE_E:
                move_sum += vec2(0, -1);
                move = true;
                break;
            case SDL_SCANCODE_D:
                move_sum += vec2(0, 1);
                move = true;
                break;
            case SDL_SCANCODE_S:
                move_sum += vec2(-1, 0);
                move = true;
                break;
            case SDL_SCANCODE_F:
                move_sum += vec2(1, 0);
                move = true;
                break;
            }
        }
        if (move) {
            auto move_normalized = glm::normalize(move_sum);
            float heading = glm::atan(move_normalized.y, move_normalized.x);
            for (const auto& controllable : controllables.read()) {
                move_actions.write().put(TMoveActions::RowType((EidCol)controllable, { move_normalized }));
                heading_actions.write().put(THeadingActions::RowType((EidCol)controllable, { heading }));
            }
        }
    }
private:
    const TKeysDown& keys_down;
    const TKeyPresses& key_presses;
    const TKeyReleases& key_releases;
    const TControllables& controllables;
    TMoveActions& move_actions;
    THeadingActions& heading_actions;
};

