#pragma once

#ifndef TIME_MANAGED
#define TIME_MANAGED

typedef unsigned long int millitime_t;

extern millitime_t lastStart;
extern millitime_t thisStart;

millitime_t GetDeltaTime();
millitime_t GetLastDeltaTime();

void DoTick();

#endif // !TIME_MANAGED