#ifndef SDL_windowstaskbar_h_
#define SDL_windowstaskbar_h_

#include "SDL_msctf.h"

SDL_Taskbar *SDL_WINDOWS_CreateTaskbar();

SDL_bool SDL_WINDOWS_SetTaskbarProgressValue(const HWND hwnd, ITaskbarList3 *taskbar, long currentValue, long maximumValue);

SDL_bool SDL_WINDOWS_SetTaskbarState(const HWND hwnd, const SDL_Taskbar *taskbar, const SDL_TaskbarState state);

#endif /* SDL_windowstaskbar_h_ */