#pragma once

#include "Process.h"
#include "BufferedChannel.h"
#include "Channel.h"
#include "SelectAnyTarget.h"
#include "MoveTowardsTarget.h"
#include "Tokens.h"

struct ProcessHost
{
    void addProcess(unique_ptr<Process>);
    void sortProcesses();
    void tick(float step);
private:
    flat_set<unique_ptr<Process>> processes;
	vector<const Process*> execution_order;
};

