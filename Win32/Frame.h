#pragma once

enum Phases {
	ReadWorld,
	Everything,
	NumPhases
	// NumPhases will have the correct value
};

struct FrameConfig {
	array<PhaseConfig, NumPhases> phases;

	FrameConfig() {
		for (auto& cfg : phases)
			cfg.required = 0;
	}
};

struct FramePlan {
	float dt;
	bool render;
};

class Frame {
	union {
		int noInit;
		Phase phases_[NumPhases];
	};
	shared_ptr<Frame> nextFrame_;
	fibers::mutex mtx; // atomic<shared_ptr>?
public:
	Eventual<float> dt;
	Phase* phases;

	Frame(const FrameConfig& cfg) {
		for (unsigned i = 0; i < NumPhases; ++i)
			new (&phases_[i]) Phase(cfg.phases[i]);
		phases = phases_;
	}
	~Frame() {
		for (unsigned i = 0; i < NumPhases; ++i)
			phases[i].~Phase();
	}

	shared_ptr<Frame> nextFrame(const FrameConfig& cfg) {
		std::unique_lock<fibers::mutex> lk(mtx);
		if (!nextFrame_)
			nextFrame_ = std::make_shared<Frame>(cfg);
		return nextFrame_;
	}
};

struct Frames {
	const FrameConfig& cfg;
	shared_ptr<Frame> previous;
	shared_ptr<Frame> current;

	Frames(const FrameConfig& cfg, shared_ptr<Frame> initialFrame) : cfg(cfg), current(initialFrame) {}

	void frameDone() {
		previous = std::move(current);
		current = previous->nextFrame(cfg);
	}
};
