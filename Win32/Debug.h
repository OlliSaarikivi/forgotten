#pragma once

#include "GameProcess.h"

struct Debug : GameProcess
{
    Debug(Game& game, ProcessHost<Game>& host) : GameProcess(game, host) {}

    void tick() override
    {
        static auto& entity_positions = host.from(position_handles).join(positions).select();

        std::cerr << "Positions:\n";
        for (const auto& position : entity_positions) {
            std::cerr << "Eid: " << position.eid <<
                "\t x: " << position.position.x <<
                "\t y: " << position.position.y << "\n";
        }
        std::cerr << "\n";
    }
private:
    SOURCE(position_handles, position_handles);
    SOURCE(positions, positions);
};

