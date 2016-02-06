#include "stdafx.h"

#include <tchar.h>

#include "Phase.h"
#include "Eventual.h"
#include "Process.h"

static float dx = 0;
static float x = 0;

void configureSimulate(FrameConfig& cfg) {
}

void simulate(Frames frames) {
	for (;;) {
		auto dt = frames.current->dt.await();
		frames.current->phases[ReadWorld].start();
		frames.current->phases[ReadWorld].waitAllDone();
		x = x + dx * dt;
		std::cout << "x is " << x << "\n";
		frames.frameDone();
	}
}

void configureUpdate(FrameConfig& cfg) {
	cfg.phases[ReadWorld].required += 1;
}

void update(Frames frames) {
	for (;;) {
		frames.current->phases[ReadWorld].waitStart();
		dx = dx == 0 ? 1 : 0;
		frames.current->phases[ReadWorld].notifyDone();
		frames.frameDone();
	}
}

void configureTimeFrames(FrameConfig& cfg) {
}

void timeFrames(Frames frames) {
	for (;;) {
		frames.current->dt.set(0.016f);
		frames.frameDone();
		boost::this_fiber::sleep_for(16ms);
	}
}

int _tmain(int argc, _TCHAR* argv[]) {
	FrameConfig cfg;
	configureSimulate(cfg);
	configureUpdate(cfg);
	configureTimeFrames(cfg);

	Frames frames{ cfg, std::make_shared<Frame>(cfg) };

	fibers::fiber simulateFiber{ simulate, frames };
	fibers::fiber updateFiber{ update, frames };
	fibers::fiber timeFramesFiber{ timeFrames, frames };

	simulateFiber.join();
	updateFiber.join();
	timeFramesFiber.join();
}
