#pragma once
#include "stdafx.h"

#define FORMAT(ITEMS) \
	((dynamic_cast<std::ostringstream &> ( \
		std::ostringstream().seekp(0, std::ios_base::cur) << ITEMS) \
	).str())

inline void fatal_error(string message = "A fatal error was encountered", string title = "Error");
