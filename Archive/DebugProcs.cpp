#include "stdafx.h"

#include "Game.h"

void Game::debug()
{
#ifndef NDEBUG
	static auto& entity_positions = from(position_handles).join(positions).select();

	std::cerr << "Positions:\n";
	for (const auto& position : entity_positions) {
		std::cerr << "Eid: " << position.eid <<
			"\t x: " << position.position.x <<
			"\t y: " << position.position.y << "\n";
	}
	std::cerr << "\n";
#endif
}