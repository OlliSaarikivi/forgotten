#pragma once

#include "Task.h"

void startScheduler();

void runTask(unique_ptr<Task> task);

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
	runTask(make_unique<Task>(&callFunction<decltype(binder)>, static_cast<void*>(binderPtr)));
}