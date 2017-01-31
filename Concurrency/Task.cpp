#include <Core\stdafx.h>
#include "Task.h"

Task::Task(void(*func)(void*), void* param) :
	func{ func },
	param{ param },
	prev{ nullptr },
	next{ nullptr } {}