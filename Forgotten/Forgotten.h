#pragma once
#include "stdafx.h"

const string game_name = "Forgotten";

#define FORMAT(ITEMS) \
	((dynamic_cast<std::ostringstream &> ( \
		std::ostringstream().seekp(0, std::ios_base::cur) << ITEMS) \
	).str())

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