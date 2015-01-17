#pragma once

#ifdef METRICS

void startMetricsService();
void stopMetricsService();
void logProcess(const string& name, boost::chrono::nanoseconds runtime);
void beginLogTick(const string& phase, float step);
void endLogTick(boost::chrono::nanoseconds runtime);

#endif