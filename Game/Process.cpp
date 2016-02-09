#include "stdafx.h"

#include "Process.h"

std::vector<unique_ptr<ProcessEntry>>& processes() {
	static std::vector<unique_ptr<ProcessEntry>> processes_;
	return processes_;
}