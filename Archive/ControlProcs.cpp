#include "stdafx.h"

#include "Game.h"

string applyEdit(string str, const SDL_Keysym& key)
{
    if ((key.sym & SDLK_SCANCODE_MASK) != 0) {
        return str;
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
    return str;
}
void Game::handleControls()
{
    static auto& action_controllables = from(controllables).subtract(current_sentences).select();
    static auto& controllable_sentences = from(controllables).join(current_sentences).select();

    // Continuous controls in action mode. These may theoretically see single frame
    // errors when the speech mode is entered and another key pressed in the same frame.
    vec2 move_sum(0, 0);
    for (auto key_down : keys_down) {
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
    // Movement
    if (glm::length(move_sum) > 0.0001f) {
        auto move_normalized = glm::normalize(move_sum);
        for (const auto& controllable : action_controllables) {
            move_actions.put({ (Eid)controllable, { move_normalized } });
        }
    }

    for (const Row<SDLKeysym>& key_press : key_presses) {
        vector<Eid> to_speech_mode;
        vector<Eid> to_action_mode;
        // Position based controls
        switch (key_press.sdl_keysym.scancode) {
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_RETURN2:
        case SDL_SCANCODE_KP_ENTER:
            for (const Row<Eid>& entity : action_controllables) {
                to_speech_mode.emplace_back(entity);
            }
            for (auto sentence : controllable_sentences) {
                std::cerr << sentence.eid << ": " << sentence.sentence_string << "\n";
                speak_actions.put(sentence);
                to_action_mode.emplace_back((Eid)sentence);
            }
            break;
        case SDL_SCANCODE_F1:
            debug_draw_commands.put({ { Box2DDebugDrawBit::ShapeBit } });
            break;
        case SDL_SCANCODE_F2:
            debug_draw_commands.put({ { Box2DDebugDrawBit::AABBBit } });
            break;
        case SDL_SCANCODE_F3:
            debug_draw_commands.put({ { Box2DDebugDrawBit::CenterOfMassBit } });
            break;
        case SDL_SCANCODE_F4:
            debug_draw_commands.put({ { Box2DDebugDrawBit::JointBit } });
            break;
        case SDL_SCANCODE_F5:
            debug_draw_commands.put({ { Box2DDebugDrawBit::PairBit } });
            break;
        }
        // True speech
		for (auto sentence : controllable_sentences) {
            string edited = applyEdit(sentence.sentence_string, key_press.sdl_keysym);
			sentence.sentence_string = edited;
        }
        // Apply the deferred mode changes
        for (const Eid& eid : to_speech_mode) {
			current_sentences.put({ eid, { "" } });
        }
        for (const Eid& eid : to_action_mode) {
			current_sentences.erase(Row<Eid>(eid));
        }
    }
}