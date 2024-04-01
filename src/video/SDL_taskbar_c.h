#ifndef SDL_taskbar_h_
#define SDL_taskbar_h_

#include "SDL_video_c.h"

SDL_Taskbar *SDL_CreateTaskbar();

SDL_bool SDL_SetTaskbarState(SDL_Window *window, const SDL_Taskbar *taskbar, const SDL_TaskbarState state);

SDL_bool SDL_SetTaskbarProgressValue(SDL_Window *window, const SDL_Taskbar *taskbar, const float currentPercentage);

void SDL_DestroyTaskbar(SDL_Taskbar *taskbar);

int SDL_TestTaskbar(void);

#endif /* SDL_taskbar_h_ */
