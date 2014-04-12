#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TPositions>
struct Debug : Process
{
    Debug(const TPositions &positions) :
    positions(positions)
    {
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(positions);
    }
    void tick() const override
    {
        std::cerr << "Positions:\n";
        for (const auto& position : positions.read()) {
            std::cerr << "Eid: " << position.eid <<
                "\t x: " << position.position.x <<
                "\t y: " << position.position.y << "\n";
        }
        std::cerr << "\n";
    }
private:
    const TPositions &positions;
};

