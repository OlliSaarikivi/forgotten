#pragma once

void startMetricsService();
void stopMetricsService();
void logProcess(string name, boost::chrono::milliseconds runtime);
void beginLogTick(string phase, float step);
void endLogTick(boost::chrono::milliseconds runtime);