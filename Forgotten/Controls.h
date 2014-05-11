#pragma once

#include "Row.h"
#include "Process.h"

enum class ControlMode
{
    Act, Speak
};

template<typename TKeysDown, typename TKeyPresses, typename TKeyReleases,
    typename TControllables, typename TMoveActions>
struct Controls : Process
{
    Controls(const TKeysDown& keys_down, const TKeyPresses& key_presses, const TKeyReleases& key_releases,
    const TControllables& controllables, TMoveActions& move_actions) :
    keys_down(keys_down),
    key_presses(key_presses),
    key_releases(key_releases),
    controllables(controllables),
    move_actions(move_actions)
    {
        move_actions.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(keys_down);
        f(key_presses);
        f(key_releases);
        f(controllables);
    }
    void tickAct()
    {
        vec2 move_sum(0, 0);
        for (const auto& key_down : keys_down) {
            switch (key_down.sdl_scancode) {
            case SDL_SCANCODE_E:
                move_sum += vec2(0, -1);
                break;
            case SDL_SCANCODE_D:
                move_sum += vec2(0, 1);
                break;
            case SDL_SCANCODE_S:
                move_sum += vec2(-1, 0);
                break;
            case SDL_SCANCODE_F:
                move_sum += vec2(1, 0);
                break;
            }
        }
        if (glm::length(move_sum) > 0.0001f) {
            auto move_normalized = glm::normalize(move_sum);
            for (const auto& controllable : controllables) {
                move_actions.put(TMoveActions::RowType((Eid)controllable, { move_normalized }));
            }
        }
    }
    void tickSpeak()
    {

    }
    string applyEdits(string str)
    {
        for (const auto& key_press : key_presses) {
            SDL_Keysym key = key_presses.sdl_keysym;
            if (key.sym & SDLK_SCANCODE_MASK != 0) {
                continue;
            }
            // We now know that the sym is printable (after a fashion, it does include e.g. '\b' backspaces)
            switch (key.sym) {
            case SDLK_BACKSPACE:
                str.pop_back();
                break;
            default:
                if ((SDLK_a <= key.sym && key.sym <= SDLK_z) ||
                    key.sym == SDLK_SPACE) {

                    str.push_back(key.sym);
                }
                break;
            }
        }
        return str;
    }
    void tick() override
    {
        switch (mode) {
        case ControlMode::Act:
            tickAct();
            break;
        case ControlMode::Speak:
            tickSpeak();
            break;
        }
    }
private:
    const TKeysDown& keys_down;
    const TKeyPresses& key_presses;
    const TKeyReleases& key_releases;
    const TControllables& controllables;
    TMoveActions& move_actions;

    ControlMode mode = ControlMode::Act;
};