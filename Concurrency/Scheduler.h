#pragma once

#include "Task.h"

using SchedulerThreadIndex = uint_fast8_t;
const SchedulerThreadIndex NotASchedulerThread = std::numeric_limits<SchedulerThreadIndex>::max();

SchedulerThreadIndex currentSchedulerThread();

void startScheduler(void(*mainTask)());

void executeContext(Task* task);
void enqueueContext(Task* task);
void enqueueTask(Task* task);

unsigned contextsCreated();

struct SchedulerContinuation {
	virtual void operator()(Task* currentTask) = 0;
};

template<class Function>
struct SchedulerContinuationImpl : public SchedulerContinuation {
	SchedulerContinuationImpl(Function func) : func{ func } {}

	virtual void operator()(Task* currentTask) {
		return func(currentTask);
	}
private:
	Function func;
};

template<class Function>
auto createSchedulerContinuation(Function&& func) {
	return SchedulerContinuationImpl<Function>{ func };
}

template<class T>
unique_ptr<Task> createTask(void(*func)(void*), const T& data) {
	static_assert(sizeof(T) <= sizeof(Task::data));
	auto task = make_unique<Task>(func);
	new (&task->data) T(data);
	return task;
}

template<class Function>
void callFunctor(void* funcVoidPtr) {
	(*static_cast<Function*>(funcVoidPtr))();
}

template<class Function>
auto createTask(const Function& f) {
	return createTask(&callFunctor<Function>, f);
}