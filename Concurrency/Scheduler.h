#pragma once

#include "Task.h"

using SchedulerThreadIndex = int_fast8_t;
const SchedulerThreadIndex NotASchedulerThread = std::numeric_limits<SchedulerThreadIndex>::max();

SchedulerThreadIndex currentSchedulerThread();

void startScheduler();

void executeContext(Task* task);
void enqueueContext(Task* task);
void enqueueTask(Task* task);

struct SchedulerContinuation {
	virtual void operator()(Task* currentTask) = 0;
};

template<class Function>
struct SchedulerContinuationImpl : public SchedulerContinuation {
	SchedulerContinuationImpl(Function func) : func{ func } {}

	virtual void operator()(Task* currentTask) {
		func(currentTask);
	}
private:
	Function func;
};

template<class Function>
auto createSchedulerContinuation(Function&& func) {
	return SchedulerContinuationImpl<Function>{ func };
}

template<class Function>
void callFunction(void* funcVoidPtr) {
	auto funcPtr = static_cast<Function*>(funcVoidPtr);
	Function func = *funcPtr;
	delete funcPtr;
	func();
}

template<class Function, class... Args>
void run(Function&& f, Args&&... args) {
	auto binder = std::bind(f, args...);
	auto binderPtr = new decltype(binder)(binder);
	enqueueTask(make_unique<Task>(&callFunction<decltype(binder)>, static_cast<void*>(binderPtr)));
}