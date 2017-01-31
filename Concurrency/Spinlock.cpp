#include <Core\stdafx.h>
#include "Spinlock.h"

void Spinlock::lock() {
	while (flag.test_and_set(std::memory_order_acquire));
}

void Spinlock::unlock() {
	flag.clear(std::memory_order_release);
}