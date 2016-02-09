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

public:
	Phase(PhaseConfig cfg) : cfg(cfg), numDone(cfg.required) {}

	void waitStart() {
		std::unique_lock<fibers::mutex> lk(mtx);
		while (numDone == cfg.required)
			started.wait(lk);
	}

	void notifyDone() {
		std::unique_lock<fibers::mutex> lk(mtx);
		++numDone;
		assert(numDone <= cfg.required);
		if (numDone == cfg.required)
			ended.notify_all();
	}

	void start() {
		std::unique_lock<fibers::mutex> lk(mtx);
		assert(numDone == cfg.required);
		numDone = 0;
		started.notify_all();
	}

	void waitAllDone() {
		std::unique_lock<fibers::mutex> lk(mtx);
		while (numDone != cfg.required)
			ended.wait(lk);
	}
};
