#pragma once

#ifndef KEYBOARD_MANAGED
#define KEYBOARD_MANAGED

#include <map>
#include <vector>
#include <SDL.h>

extern std::map<SDL_Scancode, bool> lastKeyboardState;
extern std::map<SDL_Scancode, bool> keyboardState;
extern std::vector<SDL_Scancode> KeyboardChange;

void KeyUp(SDL_KeyboardEvent event);
void KeyDown(SDL_KeyboardEvent event);

bool IsDown(SDL_Scancode code);
bool IsUp(SDL_Scancode code);
bool WasDown(SDL_Scancode code);
bool WasUp(SDL_Scancode code);
bool IsJustPressed(SDL_Scancode code);
bool IsJustReleased(SDL_Scancode code);
bool IsHeld(SDL_Scancode code);
bool IsNotHeld(SDL_Scancode code);
#endif // !KEYBOARD_MANAGED