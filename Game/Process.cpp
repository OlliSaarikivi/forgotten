#include "stdafx.h"

#include "Process.h"

std::vector<unique_ptr<Process>>& processes() {
	static std::vector<unique_ptr<Process>> processes_;
	return processes_;
}