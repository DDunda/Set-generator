#include "config.h"
#include "KeyboardManager.h"
#include "TimeManager.h"

#include <SDL.h>

#include <iostream>
#include <string>
#include <math.h>

#include <thread>
#include <atomic>
#include <mutex>

#include <map>
#include <vector>
#include <queue>

struct coord {
	float x, y;
	coord() {
		x = 0;
		y = 0;
	}
	coord(float x, float y) {
		this->x = x;
		this->y = y;
	}
	static coord copy(coord src) {
		return coord(src.x, src.y);
	}
};

struct colour {
	union {
		Uint32 buf;
		struct {
			char r, g, b, a;
		};
	};
};

// Self expanatory
const char* windowName = "Buddabrot";
bool fullscreen = true;
bool hasFocus = true;
unsigned long threshold = 0;

std::atomic<bool> running(true);

// Window and renderer of the main window
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture;
void* pixels;
int pitch;

// Stuff about the fractal
unsigned long* densityMap;
unsigned long maxVal = 0;

// Stuff for the rendering threads
int threads = 0;
bool rendering;

// Used fot limiting access to jobs and updating the density map
std::mutex joblock, updateblock;

// Jobs for render threads
std::queue<coord> jobs;

// Whether the render has finished (So the rendering threads know to stop)
bool renderFinished = false;

void ToggleFullscreen() {
	SDL_SetWindowFullscreen(window, fullscreen ? 0 : SDL_WINDOW_FULLSCREEN);
	SDL_ShowCursor(fullscreen);
	fullscreen = !fullscreen;
}

void HandleEvents() {
	SDL_Event event;

#ifdef KEYBOARD_MANAGED
	lastKeyboardState = keyboardState;
	KeyboardChange.clear();
#endif

	while (SDL_PollEvent(&event)) {
		switch (event.type)
		{
		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				hasFocus = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				hasFocus = false;
				break;
			default:
				break;
			}
			break;
		case SDL_QUIT:
			running.store(false);
			break;
		case SDL_MOUSEWHEEL:
			if (-event.wheel.y < 0) {
				if (event.wheel.y >= threshold) {
					threshold = 1;
				}
				else {
					threshold -= event.wheel.y;
				}
			}
			else if (-event.wheel.y > 0) {
				if (threshold - event.wheel.y <= threshold
					||
					threshold - event.wheel.y > maxVal
					) {
					threshold = maxVal;
				}
				else {
					threshold -= event.wheel.y;
				}
			}
			break;
#ifdef KEYBOARD_MANAGED
		case SDL_KEYDOWN:
			KeyDown(event.key);
			break;
		case SDL_KEYUP:
			KeyUp(event.key);
			break;
#endif
		}
	}
}

void SetColour(unsigned char lum, colour* location) {
#if COLOUR_TYPE == GREYSCALE
	//location->a = 255;
	location->r = lum;
	location->g = lum;
	location->b = lum;

#elif COLOUR_TYPE == GREEN
	// AARRGGBB
	if (val <= 0) *location = 0xFF000000;
	else if (val >= 1) *location = 0xFFFFFFFF;
	else {
		unsigned char red, green, blue;
		if (val < 1.0L / 3.0L) {
			red = floor(val * 765.0L);
			green = 0, blue = 0;
		}
		else if (val < 2.0L / 3.0L) {
			red = 255;
			green = floor((val - (1.0L / 3.0L)) * 765.0L);
			blue = 0;
		}
		else {
			red = 255;
			green = 255;
			blue = floor((val - (2.0L / 3.0L)) * 765.0L);
		}
		*location = 0xFF000000;
		*location |= red << 8;
		*location |= green;
		*location |= blue << 16;
	}
#elif COLOUR_TYPE == BANDED
	float tmp = 1.0L;
	unsigned char lum = (unsigned char)(modfl(val * 10.0L, &tmp) * 255);
	*location = 0xFF000000 | (lum << 16) | (lum << 8) | lum;
#elif COLOUR_TYPE == GOLD
	// AARRGGBB
	if (val <= 0) *location = 0xFF000000;
	else if (val >= 1) *location = 0xFFFFFFFF;
	else {
		unsigned char red, green, blue;
		if (val < 1.0L / 6.0L) red = floor(val * 1530.0L);
		else red = 255;

		if (val < 1.0L / 4.0L) green = floor(val * 1020.0L);
		else green = 255;

		if (val < 3.0 / 4.0L) blue = floor(val * 340.0L);
		else blue = 255;

		*location = 0xFF000000;
		*location |= red << 16;
		*location |= green << 8;
		*location |= blue;
	}

#endif
}

void FromScreenToGlobal(coord& current) {
#ifdef SAMPLE_RADIUS_X
	current.x *= SAMPLE_RADIUS_X;
#endif
#ifdef SAMPLE_RADIUS_Y
	current.y *= SAMPLE_RADIUS_Y;
#endif

#ifdef SAMPLE_OFFSET_X
	current.x += SAMPLE_OFFSET_X;
#endif
#ifdef SAMPLE_OFFSET_Y
	current.y += SAMPLE_OFFSET_Y;
#endif
}

coord ToScreen(const coord& current) {
	coord out = coord::copy(current);

#ifdef CAMERA_OFFSET_X
	out.x -= CAMERA_OFFSET_X;
#endif
#ifdef CAMERA_OFFSET_Y
	out.y -= CAMERA_OFFSET_Y;
#endif

#ifdef Y_RATIO_CORRECTION
	out.y /= Y_RATIO_CORRECTION;
#endif
#ifdef CAMERA_RADIUS
	out.x /= CAMERA_RADIUS;
	out.y /= CAMERA_RADIUS;
#endif

	out.x += 1;
	out.y += 1;

	out.x *= 0.5L;
	out.y *= 0.5L;

	out.x *= ((int)WIDTH);
	out.y *= ((int)HEIGHT);

	//out.x = round(out.x);
	//out.y = round(out.y);

	return out;
}

void CloseLoop() {
	SDL_RenderPresent(renderer);
#ifdef TIME_MANAGED
	DoTick();
#endif
}

void RenderingThread() {
#if PLOT_TYPE == ESCAPING_BUDDHA
	coord* iters = new coord[MAX_PATH_LENGTH];
#endif
	threads++;

	std::queue<coord> private_jobs;

	while (running.load()) {
		if (private_jobs.size() == 0) {
			while (1) {
				if (jobs.size() > 0) {
					joblock.lock();
					if (jobs.size() > 0) break;
					else joblock.unlock();
				}
				if (renderFinished) {
#if PLOT_TYPE == ESCAPING_BUDDHA
					delete[] iters;
#endif
					threads--;
					return;
				}
			}
			for (; private_jobs.size() < 4 && jobs.size() > 0;) {
				private_jobs.push(jobs.front());
				jobs.pop();
			}
			joblock.unlock();
		}

		coord tmpSample = private_jobs.front();
		private_jobs.pop();

		coord tmp = tmpSample;
		FromScreenToGlobal(tmp);
		coord tmporigin = tmp;

		int escapes = 0;
		int i = 0;

#if PLOT_TYPE == JULIA
		coord location = ToScreen(tmporigin);
		if (location.x >= 0 && location.y >= 0 && location.x < WIDTH && location.y < HEIGHT) {
			for (; i < MAX_PATH_LENGTH; ++i) {
				float tx2 = tmp.x * tmp.x - tmp.y * tmp.y + JULIA_X,
					ty2 = 2 * tmp.x * tmp.y + JULIA_Y;
				tmp.x = tx2;
				tmp.y = ty2;
				if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) {
					escapes++;
					if (escapes == MAX_ESCAPES) break;
				}
			}
			unsigned long* base = densityMap
				+ (int)(location.x)
				+ WIDTH * (int)(location.y);
			updateblock.lock();
			if (i != MAX_PATH_LENGTH) {
				if (*base == maxVal) maxVal += i;
				*base += i;
			}
			updateblock.unlock();
		}
#elif PLOT_TYPE == ESCAPING_BUDDHA
		coord* itersHead = iters;
		float tx2, ty2;
		for (; i < MAX_PATH_LENGTH; ++i) {
			tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x;
			tmp.y = 2 * tmp.x * tmp.y + tmporigin.y;
			tmp.x = tx2;
			coord scrtmp = ToScreen(tmp);
			if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) {
				++escapes;
				if (escapes == MAX_ESCAPES) {
					updateblock.lock();
					for (coord* j = iters; j != itersHead; ++j) {
						unsigned long* base = densityMap + (int)j->x + (int)j->y * WIDTH;
						if (*base == maxVal) ++maxVal;
						++(*base);
					}
					updateblock.unlock();
					break;
				}
			}
			else if (scrtmp.x >= 0 && scrtmp.y >= 0 && scrtmp.x < WIDTH && scrtmp.y < HEIGHT) {
				*(itersHead++) = scrtmp;
			}
		}
#elif PLOT_TYPE == FULL_BUDDHA
		float tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x,
			ty2 = 2 * tmp.x * tmp.y + tmporigin.y;
		tmp.x = tx2;
		tmp.y = ty2;
		coord scrtmp = ToScreen(tmp);
		if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) escapes++;
		else if (scrtmp.x >= 0 && scrtmp.y >= 0 && scrtmp.x < WIDTH && scrtmp.y < HEIGHT) {
			Uint16* base = densityMap
				+ (int)(floor(scrtmp.x))
				+ WIDTH * (int)(floor(scrtmp.y));
			(*base)++;
			if (*base > maxVal) maxVal++;
			escapes = 0;
		}
	}
#elif PLOT_TYPE == MANDELBROT
		float tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x,
			ty2 = 2 * tmp.x * tmp.y + tmporigin.y;
		tmp.x = tx2;
		tmp.y = ty2;
		if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) escapes++;
}
#ifdef SAMPLE_MULT
	Uint16* base = densityMap
		+ (int)(floor(tmpSample.x / (float)SAMPLE_MULT))
		+ WIDTH * (int)(floor(tmpSample.y / (float)SAMPLE_MULT));
#else
	Uint16* base = densityMap
		+ (int)floor(tmpSample.x)
		+ WIDTH * (int)floor(tmpSample.y);
#endif
	(*base) += i;
	if (*base > maxVal) maxVal++;
#elif PLOT_TYPE == DEBUG
		coord scrtmp = ToScreen(tmporigin);
		int x = round(scrtmp.x);
		int y = round(scrtmp.y);
		//double fx = iters[j].x - x;
		//double fy = iters[j].y - y;

		unsigned long* base = densityMap + x + y * WIDTH;
		updateblock.lock();
		if (y >= 0 && y < HEIGHT) {
			if (x >= 0 && x < WIDTH) {
				if (*base == maxVal) ++maxVal;
				(*base)++;
			}
		}
		updateblock.unlock();
#endif
	}
#if PLOT_TYPE == ESCAPING_BUDDHA
delete[] iters;
#endif
threads--;
}

void RenderSet() {
	threads++;

	renderFinished = false;

	int z = 8;
	std::vector<std::thread> workers(z);
	for (int i = 0; i < z; ++i) workers[i] = std::thread(RenderingThread);

	joblock.lock();
	int generate_jobs = z * 5;

	for (int y = 0; y < SAMPLE_COUNT_Y; ++y) {
		for (int x = 0; x < SAMPLE_COUNT_X; ++x) {
			coord tmpSample(x, y);
			tmpSample.x /= (float)SAMPLE_COUNT_X*0.5;
			tmpSample.y /= (float)SAMPLE_COUNT_Y*0.5;

			tmpSample.x -= 1;
			tmpSample.y -= 1;

			jobs.push(tmpSample);
			--generate_jobs;
			if (generate_jobs == 0) {
				joblock.unlock();
				while (jobs.size() > z) {
					if (!running.load()) {
						renderFinished = true;
						for (int i = 0; i < z; ++i) workers[i].join();
						threads--;
						return;
					}
				}
				joblock.lock();
				generate_jobs = z * 5 - jobs.size();
			}
		}
	}
	joblock.unlock();
	while (jobs.size() > 0) {
		if (!running.load()) {
			renderFinished = true;
			break;
		}
	}

	renderFinished = true;

	for (int i = 0; i < z; ++i) workers[i].join();

	threads--;
	return;
}

void OpenLoop() {
	HandleEvents();

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
}

int GameLoop() {
	std::thread renderThread = std::thread(RenderSet);

	while (running.load()) {
		OpenLoop();

		SDL_LockTexture(texture, NULL, &pixels, &pitch);

#if SCALING_TYPE == LOGARITHMIC
		float invMaxVal = 9.0L / (float)(maxVal);
#else
		if (threshold == 0) threshold = maxVal;
		float invMaxVal = 255.0L / (float)threshold;
		//float invMaxVal = 255.0L / (float)(maxVal);
#endif
		colour* base = (colour*)pixels;
		unsigned long* src = densityMap;

		if (hasFocus) {
			for (int y = 0; y < HEIGHT; y++) {
				for (int x = 0; x < WIDTH; x++) {
#if SCALING_TYPE == LOGARITHMIC
					float lum = 255.0 * log10((float)(*src) * invMaxVal + 1);
#elif SCALING_TYPE == SQR
					float lum = ((float)(*src) * invMaxVal);
					lum *= lum;
#elif SCALING_TYPE == SQRT
					float lum = sqrtl((float)(*src) * invMaxVal);
#elif SCALING_TYPE == NORMAL
					float lum = (*src * invMaxVal);
#endif
					lum = lum > 255 ? 255 : lum;
					SetColour(lum, base);
					++base;
					++src;
				}
			}
		}
		SDL_UnlockTexture(texture);

		SDL_RenderCopy(renderer, texture, NULL, NULL);

#ifdef KEYBOARD_MANAGED
		if (IsJustReleased(SDL_SCANCODE_F11)) ToggleFullscreen();
		if (IsJustPressed(SDL_SCANCODE_ESCAPE)) running.store(false);
#endif

		CloseLoop();
	}

	renderThread.join();

	return 0;
}

int main(int argc, char* argv[]) {

	if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
		int winFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
		if (fullscreen) winFlags |= SDL_WINDOW_FULLSCREEN;

		if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, winFlags, &window, &renderer) == 0) {
			SDL_SetWindowTitle(window, windowName);

			texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

			running.store(true);

			densityMap = new unsigned long[WIDTH * HEIGHT]();
			GameLoop();
			delete[] densityMap;
			SDL_DestroyTexture(texture);

			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
		}
		else {
			std::cout << "Could not create window! Error code: " << SDL_GetError() << std::endl;
			return EXIT_FAILURE;
		}

		SDL_Quit();
	}
	else {
		std::cout << "SDL could not be initialised! Error code: " << SDL_GetError() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}