#pragma once

struct PhaseConfig {
	int required;
};

class Phase {
	fibers::mutex mtx;
	fibers::condition_variable started;
	fibers::condition_variable ended;
	PhaseConfig cfg;
	int numDone;

	void startImpl() {
		assert(numDone == cfg.required);
		numDone = 0;
		started.notify_all();
	}

	void waitStopImpl(std::unique_lock<fibers::mutex>& lk) {
		while (numDone != cfg.required)
			ended.wait(lk);
	}

public:
	Phase(PhaseConfig cfg) : cfg(cfg), numDone(cfg.required) {}

	void waitEnter() {
		std::unique_lock<fibers::mutex> lk(mtx);
		while (numDone == cfg.required)
			started.wait(lk);
	}

	void exit() {
		std::unique_lock<fibers::mutex> lk(mtx);
		++numDone;
		assert(numDone <= cfg.required);
		if (numDone == cfg.required)
			ended.notify_all();
	}

	void start() {
		std::unique_lock<fibers::mutex> lk(mtx);
		startImpl();
	}

	void waitStop() {
		std::unique_lock<fibers::mutex> lk(mtx);
		waitStopImpl(lk);
	}

	void run() {
		std::unique_lock<fibers::mutex> lk(mtx);
		startImpl();
		waitStopImpl(lk);
	}
};

struct InPhase {
	Phase& phase;
	InPhase(Phase& phase) : phase(phase) {
		phase.waitEnter();
	}
	~InPhase() {
		phase.exit();
	}
};

struct RunPhase {
	Phase& phase;
	RunPhase(Phase& phase) : phase(phase) {
		phase.start();
	}
	~RunPhase() {
		phase.waitStop();
	}
};
