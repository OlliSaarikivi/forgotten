#include "stdafx.h"
#include "Process.h"

void configure(FrameConfig& cfg) {

}

void run(Frames F, State& S) {
	for (;;) {
		auto& readWorld = F.current->phases[ReadWorld];
		auto& readForces = F.current->phases[ReadForces];

		readWorld.start();
		// TODO: export world

		{
			InPhase guard{ readForces };

		}

		auto dt = F.current->dt.await();
		S.world.Step(dt, 8, 3);
		readWorld.waitStop();
		F.done();
	}
}

static Process<run, configure> X{};