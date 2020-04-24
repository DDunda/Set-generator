#include "config.h"

#include "KeyboardManager.h"

#include "TimeManager.h"

#include <iostream>
#include <string>
#include <math.h>
#include <map>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#include <GL/glew.h>
#include <SDL_opengl.h>

#include <SDL_image.h>

#include <thread>
#include <iostream>
#include <Windows.h>
#include <vector>
#include <atomic>

// A byte
typedef unsigned char byte_t;


// Self expanatory
const char* windowName = "Buddabrot";
bool fullscreen = true;

std::atomic<bool> running( true );

// Window and renderer of the main window
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture;
void* pixels;
int pitch;

const SDL_Color colorWhite = { 255, 255, 255 };
const SDL_Color colorBlack = { 0, 0, 0 };
TTF_Font* arial;

struct coord {
	long double x, y;
	coord() {
		x = 0;
		y = 0;
	}
	coord(long double x, long double y) {
		this->x = x;
		this->y = y;
	}
	static coord copy(coord src) {
		return coord(src.x, src.y);
	}
};

void DisplayMessage(std::string message, TTF_Font* font, int x, int y) {

	SDL_Surface* surface = TTF_RenderText_Blended(font, message.c_str(), colorWhite);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

	int w;
	int h;

	SDL_QueryTexture(texture, NULL, NULL, &w, &h);
	SDL_Rect locatingRect = { x, y, w, h };

	SDL_RenderCopy(renderer, texture, NULL, &locatingRect);

	SDL_DestroyTexture(texture);
	SDL_FreeSurface(surface);
}

int InitImageLibraries() {
	int error = 0;

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
		std::cout << "Failed to initialise SDL_image for PNG files. Error code: " << IMG_GetError() << std::endl;
		error -= 1;
	}

	if (!(IMG_Init(IMG_INIT_JPG) & IMG_INIT_JPG)) {
		std::cout << "Failed to initialise SDL_image for JPG files. Error code: " << IMG_GetError() << std::endl;
		error -= 2;
	}

	if (!(IMG_Init(IMG_INIT_TIF) & IMG_INIT_TIF)) {
		std::cout << "Failed to initialise SDL_image for TIF files. Error code: " << IMG_GetError() << std::endl;
		error -= 4;
	}

	if (!(IMG_Init(IMG_INIT_WEBP) & IMG_INIT_WEBP)) {
		std::cout << "Failed to initialise SDL_image for WEBP files. Error code: " << IMG_GetError() << std::endl;
		error -= 8;
	}

	if (TTF_Init() == 0) {
		arial = TTF_OpenFont("arial.ttf", 15);
	}
	else {
		std::cout << "Failed to initialise SDL_ttf. Error code: " << TTF_GetError() << std::endl;
		error -= 16;
	}

	return error;
}

int LoadImage(SDL_Surface*& surface, const char* path) {

	surface = IMG_Load(path);

	if (surface == NULL) {
		std::cout << "SDL could not load " << path << " (image). Error code: " << SDL_GetError() << std::endl;
		return -1;
	}

	return 0;
}

int BlitImage(SDL_Surface* surface, SDL_Surface* image, unsigned int x, unsigned int y) {
	SDL_Rect pos;
	pos.x = x;
	pos.y = y;
	SDL_BlitSurface(image, NULL, surface, &pos);

	return 0;
}

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
		case SDL_QUIT:
			running = false;
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

void DrawFilledCircle(int cx, int cy, int r, SDL_Color& line, SDL_Color& fill) {
	int x = r;
	int y = 0;
	while (x >= y) {
		SDL_SetRenderDrawColor(renderer, line.r, line.g, line.b, line.b);
		SDL_RenderDrawPoint(renderer, cx + x, cy + y);
		SDL_RenderDrawPoint(renderer, cx - x, cy + y);
		SDL_RenderDrawPoint(renderer, cx + x, cy - y);
		SDL_RenderDrawPoint(renderer, cx - x, cy - y);
		SDL_RenderDrawPoint(renderer, cx + y, cy + x);
		SDL_RenderDrawPoint(renderer, cx - y, cy + x);
		SDL_RenderDrawPoint(renderer, cx + y, cy - x);
		SDL_RenderDrawPoint(renderer, cx - y, cy - x);

		SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.b);
		SDL_RenderDrawLine(renderer, cx + x - 1, cy + y, cx - x + 1, cy + y);
		SDL_RenderDrawLine(renderer, cx + x - 1, cy - y, cx - x + 1, cy - y);
		SDL_RenderDrawLine(renderer, cx + y, cy + x - 1, cx + y, cy - x + 1);
		SDL_RenderDrawLine(renderer, cx - y, cy + x - 1, cx - y, cy - x + 1);

		y++;
		int tx = x - 1;
		if (abs(((tx * tx) + y * y) - r * r) < abs(((x * x) + y * y) - r * r)) {
			x = tx;
		}
	}
}

void DrawCircle(int cx, int cy, int r, SDL_Color& line) {
	int x = r;
	int y = 0;
	SDL_SetRenderDrawColor(renderer, line.r, line.g, line.b, line.b);
	while (x >= y) {
		SDL_RenderDrawPoint(renderer, cx + x, cy + y);
		SDL_RenderDrawPoint(renderer, cx - x, cy + y);
		SDL_RenderDrawPoint(renderer, cx + x, cy - y);
		SDL_RenderDrawPoint(renderer, cx - x, cy - y);
		SDL_RenderDrawPoint(renderer, cx + y, cy + x);
		SDL_RenderDrawPoint(renderer, cx - y, cy + x);
		SDL_RenderDrawPoint(renderer, cx + y, cy - x);
		SDL_RenderDrawPoint(renderer, cx - y, cy - x);

		y++;
		int tx = x - 1;
		if (abs(((tx * tx) + y * y) - r * r) < abs(((x * x) + y * y) - r * r)) {
			x = tx;
		}
	}
}

void Reverse_endianness(byte_t* bytes, size_t byteLength) {
	size_t cap = byteLength >> 1;
	for (byte_t i = 0; i < cap; i++) {
		bytes[i] ^= bytes[byteLength - 1 - i];
		bytes[byteLength - 1 - i] ^= bytes[i];
		bytes[i] ^= bytes[byteLength - 1 - i];
	}
}

void SetColour(long double val, Uint32* location) {
#if COLOUR_TYPE == GREYSCALE
	// AARRGGBB
	unsigned char lum = (unsigned char)(val * 255);
	*location = 0xFF000000;
	*location |= lum << 8;
	*location |= lum;
	*location |= lum << 16;
	
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
	long double tmp = 1.0L;
	unsigned char lum = (unsigned char)(modfl(val * 10.0L,&tmp) * 255);
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


coord FromScreenToGlobal(const coord& current) {
	coord out = coord::copy(current);

	out.x /= ((long double)WIDTH);
	out.y /= ((long double)HEIGHT);

	out.x *= 2.0;
	out.y *= 2.0;

	out.x -= 1;
	out.y -= 1;

	out.x *= CAMERA_RADIUS;
	out.y *= CAMERA_RADIUS;
	out.y *= Y_RATIO_CORRECTION;

#ifdef SAMPLING
	out.x *= SAMPLE_RADIUS_X;
	out.y *= SAMPLE_RADIUS_Y;

	out.x += SAMPLE_OFFSET_X;
	out.y += SAMPLE_OFFSET_Y;

#endif // SAMPLING

	out.x += CAMERA_OFFSET_X;
	out.y += CAMERA_OFFSET_Y;

	return out;
}

coord ToScreen(const coord& current) {
	coord out = coord::copy(current);

	out.x -= CAMERA_OFFSET_X;
	out.y -= CAMERA_OFFSET_Y;

	out.x /= CAMERA_RADIUS;
	out.y /= CAMERA_RADIUS;
	out.y /= Y_RATIO_CORRECTION;

	out.x += 1;
	out.y += 1;

	out.x *= 0.5L;
	out.y *= 0.5L;

	out.x *= ((long double)WIDTH);
	out.y *= ((long double)HEIGHT);

	out.x = round(out.x);
	out.y = round(out.y);

	return out;
}

double* densityMap;
double maxVal = 0;

int threads = 0;
bool rendering;

int a = 0;
int b = 0;
int g;

void CloseLoop() {
	SDL_RenderPresent(renderer);
#ifdef TIME_MANAGED
	DoTick();
#endif
}

void RenderSet(int yMin, int yMax) {
	threads++;
	std::vector<coord> iters(MAX_PATH_LENGTH);

#if SAMPLE_MULT == 1
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			coord tmpSample(x, y);
#else
	for (int y = yMin; y < yMax; y++) {
		for (int x = 0; x < WIDTH * SAMPLE_MULT; x++) {
			coord tmpSample(x / (long double)SAMPLE_MULT, y / (long double)SAMPLE_MULT);
#endif
			coord tmp = FromScreenToGlobal(tmpSample);
			coord tmporigin = coord::copy(tmp);

			int escapes = 0;

			iters.clear();
			int i = 0;

#if MAX_ESCAPES == 1
			for (; escapes == 0 && i < MAX_PATH_LENGTH; i++) {
#else
			for (; escapes < MAX_ESCAPES && i < MAX_PATH_LENGTH; i++) {
#endif

#if PLOT_TYPE == JULIA
				long double tx2 = tmp.x * tmp.x - tmp.y * tmp.y + JULIA_X,
					ty2 = 2 * tmp.x * tmp.y + JULIA_Y;
				tmp.x = tx2;
				tmp.y = ty2;
				if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) escapes++;
			}
			double* base = densityMap
				+ (int)(tmpSample.x)
				+ WIDTH * (int)(tmpSample.y);
			if (i != MAX_PATH_LENGTH) {
				(*base) = i;
				if (*base > maxVal) maxVal++;
			}
#elif PLOT_TYPE == ESCAPING_BUDDHA
				long double tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x,
					ty2 = 2 * tmp.x * tmp.y + tmporigin.y;
				tmp.x = tx2;
				tmp.y = ty2;
				coord scrtmp = ToScreen(tmp);
				if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) escapes++;
				else if (scrtmp.x >= 0 && scrtmp.y >= 0 && scrtmp.x < WIDTH && scrtmp.y < HEIGHT) iters.push_back(scrtmp);
			}

			if (escapes >= MAX_ESCAPES) {
				for (int j = 0; j < iters.size(); j++) {
					int x = (int)iters[j].x;
					int y = (int)iters[j].y;
					double fx = iters[j].x - x;
					double fy = iters[j].y - y;

					double* base = densityMap
						+ x
						+ WIDTH * y;

					if (y >= 0 && y < HEIGHT) {
						if (x >= 0 && x < WIDTH) {
							*(base) += (1 - fx) * (1 - fy);
							if (*(base) > maxVal) maxVal = *(base);
						}
						if (x >= -1 && x < WIDTH - 1) {
							*(base + 1) += (fx) * (1 - fy);
							if (*(base + 1) > maxVal) maxVal = *(base + 1);
						}
					}
					if (y >= -1 && y < HEIGHT - 1) {
						if (x >= 0 && x < WIDTH) {
							*(base + WIDTH) += (1 - fx) * (fy);
							if (*(base + WIDTH) > maxVal) maxVal = *(base + WIDTH);
						}
						if (x >= -1 && x < WIDTH - 1) {
							*(base + WIDTH + 1) += (fx) * (fy);
							if (*(base + WIDTH + 1) > maxVal) maxVal = *(base + WIDTH + 1);
						}
					}
				}
			}
#elif PLOT_TYPE == FULL_BUDDHA
				long double tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x,
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
				long double tx2 = tmp.x * tmp.x - tmp.y * tmp.y + tmporigin.x,
					ty2 = 2 * tmp.x * tmp.y + tmporigin.y;
				tmp.x = tx2;
				tmp.y = ty2;
				if (tmp.x * tmp.x + tmp.y * tmp.y > SQR_ESCAPE_RADIUS) escapes++;
	        }
			Uint16* base = densityMap
				+ (int)(floor(tmpSample.x / (long double)SAMPLE_MULT))
				+ WIDTH * (int)(floor(tmpSample.y / (long double)SAMPLE_MULT));
			(*base) += i;
			if (*base > maxVal) maxVal++;

#else
		    }
#endif
			if (running.load() == false) {
				threads--;
				return;
			}
		}
	}
	threads--;
	return;
}

void OpenLoop() {
	HandleEvents();

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
}

int GameLoop() {
	running.store(true);
	std::vector<std::thread> threads;
	int totalHeight = HEIGHT * SAMPLE_MULT;
	int z = 16;
	for (int i = 0; i < z; i++) {
		threads.push_back(std::thread(RenderSet, i * totalHeight / z, (i + 1) * totalHeight / z));
	}

	while (running.load()) {
		OpenLoop();

		SDL_LockTexture(texture, NULL, &pixels, &pitch);

#if SCALING_TYPE == LOGARITHMIC
		long double invMaxVal = 9.0L / (long double)(maxVal);
#else
		long double invMaxVal = 1.0L / (long double)(maxVal);
#endif

		Uint32* base = (Uint32*)pixels;
		double* src = densityMap;

		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
#if SCALING_TYPE == LOGARITHMIC
				long double lum = log10((long double)(*src) * invMaxVal + 1);
#elif SCALING_TYPE == SQR
				long double lum = ((long double)(*src) * invMaxVal);
				lum *= lum;
#elif SCALING_TYPE == SQRT
				long double lum = sqrtl((long double)(*src) * invMaxVal);
#elif SCALING_TYPE == NORMAL
				long double lum = ((long double)(*src) * invMaxVal);
#endif
				SetColour(lum, base);
				base++;
				src++;
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

	for (int i = 0; i < z; i++) {
		threads[i].join();
	}

	return 0;
}

int main(int argc, char* argv[]) {

	if (SDL_Init(SDL_INIT_EVERYTHING) == 0) {
		int winFlags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
		//if (fullscreen) winFlags |= SDL_WINDOW_FULLSCREEN;

		if (InitImageLibraries() == 0) {
			if (SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, winFlags, &window, &renderer) == 0) {
				SDL_SetWindowTitle(window, windowName);

				texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

				densityMap = new double[WIDTH * HEIGHT]();
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
			TTF_Quit();
		}
		SDL_Quit();
	}
	else {
		std::cout << "SDL could not be initialised! Error code: " << SDL_GetError() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}