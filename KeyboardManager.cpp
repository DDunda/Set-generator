#pragma once

#include "KeyboardManager.h"

// What was pressed last frame
std::map<SDL_Scancode, bool> lastKeyboardState;
// What is pressed this frame
std::map<SDL_Scancode, bool> keyboardState;
// Indicates a change in key state (up OR down)
std::vector<SDL_Scancode> KeyboardChange;

// Keyboard functions

// Registers a keyup event
void KeyUp(SDL_KeyboardEvent event) {
	keyboardState[event.keysym.scancode] = false;
	KeyboardChange.push_back(event.keysym.scancode);
}

// Registers a keydown event
void KeyDown(SDL_KeyboardEvent event) {
	keyboardState[event.keysym.scancode] = true;
	KeyboardChange.push_back(event.keysym.scancode);
}

// Queries if a key is currently down
bool IsDown(SDL_Scancode code) {
	return  keyboardState[code];
}
// Queries if a key is currently up
bool IsUp(SDL_Scancode code) {
	return !keyboardState[code];
}
// Queries if a key was down last frame
bool WasDown(SDL_Scancode code) {
	return lastKeyboardState[code];
}
// Queries if a key was up last frame
bool WasUp(SDL_Scancode code) {
	return !lastKeyboardState[code];
}
// Queries if a key has just been pressed (up, then down)
bool IsJustPressed(SDL_Scancode code) {
	return IsDown(code) && WasUp(code);
}
// Queries if a key has just been released (down, then up)
bool IsJustReleased(SDL_Scancode code) {
	return IsUp(code) && WasDown(code);
}
// Queries if a key has been held (down since last frame)
bool IsHeld(SDL_Scancode code) {
	return IsDown(code) && WasDown(code);
}
// Queries if a key has been idle (up since last frame)
bool IsNotHeld(SDL_Scancode code) {
	return IsUp(code) && WasUp(code);
}