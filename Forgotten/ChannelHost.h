#pragma once

#include "Channel.h"

struct ChannelHost
{
    void tick();
private:
    flat_set<unique_ptr<Channel>> channels;
};

