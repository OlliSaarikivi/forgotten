#pragma once
#include "stdafx.h"
#include "Utils.h"

const string game_name = "Forgotten";

NORETURN inline void fatalError(string message = "A fatal error was encountered", string title = "Error");
NORETURN inline void fatalSDLError(string message = "SDL error");
inline void debugMsg(string message)
{
#ifndef NDEBUG
    std::cerr << message << "\n";
#endif
}

void swapGameWindow();