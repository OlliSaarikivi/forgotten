#pragma once

#include "Process.h"
#include "Buffer.h"
#include "Channel.h"
#include "SelectAnyTarget.h"
#include "MoveTowardsTarget.h"
#include "Tokens.h"

struct ProcessHost
{
    void addProcess(unique_ptr<Process>);
    void addChannel(unique_ptr<Channel>);
    void sortProcesses();
    void tick(float step);
private:
    flat_set<unique_ptr<Process>> processes;
    flat_set<unique_ptr<Channel>> channels;
	vector<const Process*> execution_order;
};

