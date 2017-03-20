#pragma once

#include <Concurrency\Promise.h>
#include <Concurrency\Event.h>

struct Frame {
	float dt;
	Event inputAvailable;
};
