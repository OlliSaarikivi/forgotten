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

static RegisterProcess<StaticProcess<configureSimulate, simulate>> X1 = {};
static RegisterProcess<StaticProcess<configureUpdate, update>> X2 = {};
static RegisterProcess<StaticProcess<configureTimeFrames, timeFrames>> X3 = {};

int _tmain(int argc, _TCHAR* argv[]) {
	FrameConfig cfg;
	std::vector<fibers::fiber> fibers;
	for (auto& process : processes())
		process->configure(cfg);
	Frames frames{ cfg, std::make_shared<Frame>(cfg) };
	for (auto& process : processes()) {
		auto fiber = process->start(frames);
		fibers.emplace_back(std::move(fiber));
	}
	for (auto& fiber : fibers)
		fiber.join();
}
