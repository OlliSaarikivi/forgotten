#include <Core/stdafx.h>
#include <Concurrency\Scheduler.h>
#include <Concurrency\Promise.h>
#include <Concurrency\Mutex.h>
#include <Concurrency\Event.h>
#include <Concurrency\Latch.h>
#include <Concurrency\Task.h>

void assertLockFree() {
	ConcurrentTaskDeque().assertLockFree();
	ConcurrentTaskStack().assertLockFree();
	Event().assertLockFree();
	Mutex().assertLockFree();
	Latch(1).assertLockFree();
}

void mainTask() {
	std::vector<unique_ptr<Task>> tasks1;
	Latch latch1{ 63000 };
	for (unsigned i = 0; i < 63000; ++i) {
		tasks1.emplace_back(createTask([&latch1]() {
			latch1.countDown();
		}));
	}
	benchmark([&tasks1, &latch1]() {
		for each (auto& task in tasks1) {
			enqueueTask(task.get());
		}
		latch1.wait();
	});
	std::cout << contextsCreated() << std::endl;
	std::vector<unique_ptr<Task>> tasks2;
	Latch latch2{ 63000 };
	for (unsigned i = 0; i < 63000; ++i) {
		tasks2.emplace_back(createTask([&latch2]() {
			latch2.countDown();
		}));
	}
	benchmark([&tasks2, &latch2]() {
		for each (auto& task in tasks2) {
			enqueueTask(task.get());
		}
		latch2.wait();
	});
	std::cout << contextsCreated() << std::endl;
	std::vector<unique_ptr<Task>> tasks3;
	Latch latch3{ 6300 };
	for (unsigned i = 0; i < 6300; ++i) {
		tasks3.emplace_back(createTask([&latch3, i]() {
			auto s = i + 1;
			for (int j = 1; j < 10000000; ++j)
				s *= j;
			if (s != 0)
				latch3.countDown();
		}));
	}
	benchmark([&tasks3, &latch3]() {
		for each (auto& task in tasks3) {
			enqueueTask(task.get());
		}
		latch3.wait();
	});
	std::cout << contextsCreated() << std::endl;
	std::string line;
	std::cin >> line;
}

int _tmain(int argc, _TCHAR* argv[]) {
	assertLockFree();

	startScheduler(mainTask);
}
