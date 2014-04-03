#pragma once
#include "stdafx.h"

#define FORMAT(ITEMS) \
	((dynamic_cast<std::ostringstream &> ( \
		std::ostringstream().seekp(0, std::ios_base::cur) << ITEMS) \
	).str())

void fatal_error(std::string message = "A fatal error was encou	ntered", std::string title = "Error");
