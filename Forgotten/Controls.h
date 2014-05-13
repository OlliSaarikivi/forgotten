#pragma once

#include "Row.h"
#include "Process.h"

template<typename TKeysDown, typename TKeyPresses, typename TKeyReleases,
    typename TControllables, typename TMoveActions, typename TSpeakActions,
    typename TSentences>
struct Controls : Process
{
    Controls(const TKeysDown& keys_down, const TKeyPresses& key_presses, const TKeyReleases& key_releases,
    TControllables& controllables, TMoveActions& move_actions, TSpeakActions& speak_actions, TSentences& sentences) :
    keys_down(keys_down),
    key_presses(key_presses),
    key_releases(key_releases),
    controllables(controllables),
    move_actions(move_actions),
    speak_actions(speak_actions),
    sentences(sentences),
    action_controllables(controllables, sentences),
    controllable_sentences(controllables, sentences)
    {
        registerInput(keys_down);
        registerInput(key_presses);
        registerInput(key_releases);
        registerInput(controllables);
        move_actions.registerProducer(this);
        speak_actions.registerProducer(this);
        sentences.registerProducer(this);
    }
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
    void tick() override
    {
        // Continuous controls in action mode. These may theoretically see single frame
        // errors when the speech mode is entered and another key pressed in the same frame.
        vec2 move_sum(0, 0);
        for (const Row<SDLScancode>& key_down : keys_down) {
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
                move_actions.put(TMoveActions::RowType((Eid)controllable, { move_normalized }));
            }
        }

        for (const Row<SDLKeysym>& key_press : key_presses) {
            // Controls in action mode
            switch (key_press.sdl_keysym.scancode) {
            case SDL_SCANCODE_SPACE:
                for (const Row<Eid>& entity : action_controllables) {
                    sentences.put(TSentences::RowType(entity, { "" }));
                }
                break;
            }
            // Controls
            switch (key_press.sdl_keysym.sym) {
            case SDLK_RETURN:
            case SDLK_RETURN2:
            case SDLK_KP_ENTER:
                for (const Row<Eid, SentenceString>& sentence : controllable_sentences) {
                    std::cerr << sentence.eid << ": " << sentence.sentence_string << "\n";
                    speak_actions.put(sentence);
                }
                for (const Row<Eid>& entity : controllables) {
                    sentences.erase(entity);
                }
                break;
            }
            // True speech
            auto s_current = controllable_sentences.begin();
            auto s_end = controllable_sentences.end();
            while (s_current != s_end) {
                auto sentence = *s_current;
                string edited = applyEdit(sentence.sentence_string, key_press.sdl_keysym);
                controllable_sentences.update(s_current, Row<SentenceString>({ edited }));
                ++s_current;
            }
        }
    }
private:
    const TKeysDown& keys_down;
    const TKeyPresses& key_presses;
    const TKeyReleases& key_releases;
    const TControllables& controllables;
    TMoveActions& move_actions;
    TSpeakActions& speak_actions;
    TSentences& sentences;

    SubtractStream<TControllables, TSentences> action_controllables;
    JoinStream<TControllables, TSentences> controllable_sentences;
};