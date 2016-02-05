#include "stdafx.h"

#include <tchar.h>

#include "Phase.h"

namespace phase {
	enum Phases {
		ReadWorld,
		Everything,
		NumPhases
		// NumPhases will have the correct value
	};
}

static int dx = 0;
static int x = 0;

struct FrameConfig {
	array<PhaseConfig, phase::NumPhases> phases;
};

struct Frame {
	future<float> dt;
	array<Phase, phase::NumPhases> phases;
};

Frame& getFrame(uint64_t frameNum) {

}

void simulate() {
	uint64_t frameNum = 0;

	for (;;) {
		auto& frame = getFrame(frameNum);
		frame.dt.wait();
		auto dt = frame.dt.get();

		x = x + dx * dt;
	}
}

void configureAct(FrameConfig& config) {
	config.phases[phase::ReadWorld].required += 1;
}

void act() {
	uint64_t frameNum = 0;

	for (;;) {
		auto& frame = getFrame(frameNum);

		frame.phases[phase::ReadWorld].waitStart();
		dx = (dx + 1) % 10; // nonsense
		frame.phases[phase::ReadWorld].notifyDone();
	}
}

int _tmain(int argc, _TCHAR* argv[]) {

}
