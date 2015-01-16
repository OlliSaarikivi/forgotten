#include "stdafx.h"
#include "MetricsService.h"

// Symbols from ProfilerConnection
extern "C" {
    bool metrics_start();
    void metrics_stop();
    void metrics_processTiming(const char* name, uint32_t duration_nanos);
    void metrics_beginTick(const char* phase, float step);
    void metrics_endTick(uint32_t total_duration_nanos);
}

void startMetricsService()
{
    metrics_start();
}

void stopMetricsService()
{
    metrics_stop();
}

void logProcess(const string& name, boost::chrono::milliseconds runtime)
{
    metrics_processTiming(name.c_str(), static_cast<boost::chrono::nanoseconds>(runtime).count());
}

void beginLogTick(const string& phase, float step)
{
    metrics_beginTick(phase.c_str(), step);
}

void endLogTick(boost::chrono::milliseconds runtime)
{
    metrics_endTick(static_cast<boost::chrono::nanoseconds>(runtime).count());
}
