#pragma once

#include "GameProcess.h"

struct TrueSentenceInterpreter : GameProcess
{
    TrueSentenceInterpreter(Game& game, ProcessHost<Game>& host) : GameProcess(game, host) {}
    void tick() override
    {
        static auto& entity_positions = host.from(position_handles).join(positions).select();
        static auto& named_bodied = host.from(true_names).join(bodies).join(position_handles).join(positions).select();

        for (const auto& sentence : speak_actions) {
            auto name_row = Row<TrueName>({ sentence.sentence_string });
            auto targets = named_bodied.equalRange(name_row);
            for (auto target = targets.first; target != targets.second; ++target) {
                auto speaker_position = entity_positions.equalRange(sentence);
                if (speaker_position.first != speaker_position.second) {
                    vec2 speaker_vec = (*speaker_position.first).position;
                    vec2 target_vec = (*target).position;
                    impulses.put({ { target->body }, { glm::normalize(target_vec - speaker_vec) * 300.0f } });
                }
            }
        }
    }
private:
    SOURCE(position_handles, position_handles);
    SOURCE(positions, positions);
    SOURCE(speak_actions, speak_actions);
    SOURCE(true_names, true_names);
    SOURCE(bodies, bodies);
    SINK(impulses, center_impulses);
};