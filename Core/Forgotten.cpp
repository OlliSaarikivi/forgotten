#include <Core/stdafx.h>
#include <Concurrency\Scheduler.h>

void lolFunction1(int a) {

}

int _tmain(int argc, _TCHAR* argv[]) {
	//run(&lolFunction1, 3);
	startScheduler();

	string line;
	std::cin >> line;
}
