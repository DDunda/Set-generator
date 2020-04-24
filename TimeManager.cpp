#include "TimeManager.h"
#include <SDL.h>

// Time in milliseconds
typedef unsigned long int millitime_t;

millitime_t lastStart;
millitime_t thisStart;

// Gets the time since the beginning of the frame
millitime_t GetDeltaTime() {
	millitime_t now = SDL_GetTicks();

	// Time overflow ( Once every 49 days and 17 hours )
	if (now < thisStart) {
		//                      v Max ticks
		return now + (0xFFFFFFFF - thisStart);
	}
	else {
		return now - thisStart;
	}
}

// Gets the length of the last frame
millitime_t GetLastDeltaTime() {

	// Time overflow ( Once every 49 days and 17 hours )
	if (thisStart < lastStart) {
		//                      v Max ticks
		return thisStart + (0xFFFFFFFF - lastStart);
	}
	else {
		return thisStart - lastStart;
	}
}

// Updates time variables at the end of the frame
void DoTick() {
	lastStart = thisStart;
	thisStart = SDL_GetTicks();

#ifdef MIN_DELTA
	millitime_t delta = GetLastDeltaTime();
	if (delta < MIN_DELTA) {
		millitime_t delay = MIN_DELTA - delta;
		SDL_Delay(delay);
	}
#endif // MIN_DELTA
}