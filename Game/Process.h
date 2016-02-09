#pragma once

#include "Frame.h"
#include "State.h"

struct ProcessEntry {
	virtual void configure(FrameConfig& cfg) = 0;
	virtual fibers::fiber start(Frames frames, State* state) = 0;
};

std::vector<unique_ptr<ProcessEntry>>& processes();

template<void(*TRun)(Frames, State&), void(*TConfigure)(FrameConfig&) = nullptr> struct Process {
	static void CallRun(Frames frames, State* state) {
		TRun(frames, *state);
	}
	struct Entry : ProcessEntry {
		virtual void configure(FrameConfig& cfg) override {
			if (TConfigure)
				TConfigure(cfg);
		}
		virtual fibers::fiber start(Frames frames, State* state) override {
			return fibers::fiber{ CallRun, frames, state };
		}
	};
	Process() {
		processes().push_back(std::make_unique<Entry>());
	}
};
