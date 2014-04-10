#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TPositionsChannel>
struct Debug : Process
{
    Debug(const TPositionsChannel &positions) :
    positions(positions)
    {
    }
    void tick(float step) const override
    {
        std::cerr << "Positions:\n";
        for (const auto& position : positions.read()) {
            std::cerr << "Eid: " << position.eid <<
                "\t x: " << position.position.x <<
                "\t y: " << position.position.y << "\n";
        }
        std::cerr << "\n";
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(positions);
    }
private:
    const TPositionsChannel &positions;
};

