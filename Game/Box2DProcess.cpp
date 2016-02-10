#include "stdafx.h"
#include "Process.h"

void configure(FrameConfig& cfg) {
	cfg.phases[ReadForces].required++;
}

void run(Frames F, State& S) {
	b2World world{ b2Vec2(0,0) };

	for (;;) {
		auto& readWorld = F.current->phases[ReadWorld];
		auto& readForces = F.current->phases[ReadForces];

		readWorld.start();

		{
			InPhase guard{ readForces };

			forEach(S.entities.require<Body, ApplyForce>(), [&](auto row) {
				auto app = row|ApplyForce_;
				(row|Body_|BodyHandle_)->ApplyForce(app|Force_, app|Point_, true);
			});
			S.entities.component<ApplyForce>().clear();
		}

		auto dt = F.current->dt.await();
		world.Step(dt, 8, 3);
		readWorld.waitStop();

		auto& positions = S.entities.component<Position>();
		positions.clear();
		forEach(S.entities.component<Body>(), [&](auto row) {
			positions.append(makeRow(row|Eid_,
				Point((row|BodyHandle_)->GetPosition()),
				Angle((row|BodyHandle_)->GetAngle())));
		});

		F.done();
	}
}

static Process<run, configure> X{};
