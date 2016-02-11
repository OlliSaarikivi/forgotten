#include "stdafx.h"

#include <tchar.h>

#include "Phase.h"
#include "Eventual.h"
#include "Process.h"
#include "Timer.h"

//static float dx = 0;
//static float x = 0;
//
//void configureSimulate(FrameConfig& cfg) {
//}
//
//void simulate(Frames frames) {
//	for (;;) {
//		auto dt = frames.current->dt.await();
//		frames.current->phases[ReadWorld].start();
//		frames.current->phases[ReadWorld].waitAllDone();
//		x = x + dx * dt;
//		std::cout << "x is " << x << "\n";
//		frames.frameDone();
//	}
//}
//
//void configureUpdate(FrameConfig& cfg) {
//	cfg.phases[ReadWorld].required += 1;
//}
//
//void update(Frames frames) {
//	for (;;) {
//		frames.current->phases[ReadWorld].waitStart();
//		dx = dx < 0.5 ? 1.0f : 0.0f;
//		frames.current->phases[ReadWorld].notifyDone();
//		frames.frameDone();
//	}
//}
//
//void configureTimeFrames(FrameConfig& cfg) {
//}
//
//void timeFrames(Frames frames) {
//	for (;;) {
//		frames.current->dt.set(0.016f);
//		frames.frameDone();
//		boost::this_fiber::sleep_for(16ms);
//	}
//}
//
//static Process<simulate,configureSimulate> X1 = {};
//static Process<update, configureUpdate> X2 = {};
//static Process<timeFrames, configureTimeFrames> X3 = {};

COL(float, X)
COL(float, Y)
COL(float, Z)

NUMERIC_COL(int, A)

int _tmain(int argc, _TCHAR* argv[]) {
	MBPlusTree<ColIndex<A>, A> tree;

	for (int i = 0; i < 20; ++i) {
		Time("append", [&]() {
			for (int i = 0; i < 1000000; ++i)
				tree.append(makeRow(A(i)));
		});
		tree.clear();
	}
	string line;
	std::cin >> line;

	//State state;

	//FrameConfig cfg;
	//std::vector<fibers::fiber> fibers;
	//for (auto& process : processes())
	//	process->configure(cfg);
	//Frames frames{ cfg, std::make_shared<Frame>(cfg) };
	//for (auto& process : processes()) {
	//	auto fiber = process->start(frames, &state);
	//	fibers.emplace_back(std::move(fiber));
	//}
	//for (auto& fiber : fibers)
	//	fiber.join();
}
