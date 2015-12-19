#include "stdafx.h"

#include "TrueLanguage.h"
#include "Game.h"

void Game::interpretTrueSentences()
{
	static auto& entity_positions = from(position_handles).join(positions).select();
	static auto& named_bodied = from(true_names).join(dynamic_bodies).join(position_handles).join(positions).select();

	for (const auto& sentence : speak_actions) {
		auto name_row = Row<TrueName>{ { sentence.sentence_string } };
		auto targets = named_bodied.equalRange(name_row);
		for (auto target = targets.first; target != targets.second; ++target) {
			auto speaker_position = entity_positions.equalRange(sentence);
			if (speaker_position.first != speaker_position.second) {
				vec2 to_target = (*target).position - (*speaker_position.first).position;
				if (glm::length(to_target) != 0) {
					center_impulses.put({ { target->body },{ glm::normalize(to_target) * 300.0f } });
				}
			}
		}
	}
}