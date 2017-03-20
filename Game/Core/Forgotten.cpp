#include <Core/stdafx.h>
#include <Concurrency/Scheduler.h>
#include <Concurrency/Promise.h>
#include <Concurrency/Mutex.h>
#include <Concurrency/Event.h>
#include <Concurrency/Latch.h>
#include <Concurrency/Task.h>
#include "Game.h"

void assertLockFree() {
	ConcurrentTaskStack().assertLockFree();
	Event().assertLockFree();
	Mutex().assertLockFree();
	Latch(1).assertLockFree();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR, int) {
	assertLockFree();
	startScheduler(gameLoop);
}
