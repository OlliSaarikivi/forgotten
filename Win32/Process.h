#pragma once

#include "Frame.h"

struct Process {
	virtual void configure(FrameConfig& cfg) = 0;
	virtual fibers::fiber start(Frames frames) = 0;
};

template<void (*TConfigure)(FrameConfig&), void (*TRun)(Frames)> struct StaticProcess : Process {
	virtual void configure(FrameConfig& cfg) override {
		TConfigure(cfg);
	}
	virtual fibers::fiber start(Frames frames) override {
		return fibers::fiber{ TRun, frames };
	}
};

std::vector<unique_ptr<Process>>& processes();

template<class TProcess> struct RegisterProcess {
	template<class... TArgs> RegisterProcess(TArgs&&... args) {
		processes().push_back(unique_ptr<Process>(new TProcess{ std::forward<TArgs>(args)... }));
	}
};