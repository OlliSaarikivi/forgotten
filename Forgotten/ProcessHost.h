#pragma once

#include "Process.h"
#include "BufferedChannel.h"
#include "TransientChannel.h"
#include "SelectAnyTarget.h"
#include "MoveTowardsTarget.h"
#include "Tokens.h"

struct ProcessHost
{
    void addProcess(shared_ptr<Process>);
    void sortProcesses();
    void tick();
private:
	set<shared_ptr<Process>> processes;
	vector<const Process*> execution_order;
};

