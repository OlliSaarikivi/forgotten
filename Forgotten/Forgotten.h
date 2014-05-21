#pragma once
#include "stdafx.h"

const string game_name = "Forgotten";

inline void fatalError(string message = "A fatal error was encountered", string title = "Error");
inline void fatalSDLError(string message = "SDL error");
inline void checkSDLError(int line = -1);
inline void debugMsg(string message)
{
#ifndef NDEBUG
    std::cerr << message << "\n";
#endif
}
#define CHECK_SDL_ERROR checkSDLError(__LINE__);

extern SDL_Window* main_window;