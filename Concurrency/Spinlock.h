#pragma once

struct Spinlock {
	using Guard = std::lock_guard<Spinlock>;

	void lock();
	void unlock();
private:
	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};
