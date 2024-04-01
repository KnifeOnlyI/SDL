#include <SDL_stdinc.h>

#include "windows/SDL_windowstaskbar.h"
#include "SDL_taskbar_c.h"

int SDL_TestTaskbar(void)
{
#ifdef __WIN32__
    return SDL_WINDOWS_TestTaskbar();
#endif

    return -1;
}

SDL_Taskbar *SDL_CreateTaskbar()
{
    SDL_Taskbar *taskbar = NULL;

    taskbar = SDL_WINDOWS_CreateTaskbar();

    SDL_SetError("No available taskbar system for this platform.");

    return taskbar;
}

SDL_bool SDL_SetTaskbarProgressValue(
    SDL_Window *window,
    const SDL_Taskbar *taskbar,
    const float currentPercentage)
{
#if defined(SDL_PLATFORM_WINDOWS)
    const HWND hwnd = SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    return SDL_WINDOWS_SetTaskbarProgressValue(hwnd, taskbar, (long)(currentPercentage * 100.0f), 100);
#endif

    SDL_SetError("No available taskbar system for this platform.");
    return SDL_FALSE;
}

SDL_bool SDL_SetTaskbarState(SDL_Window *window, const SDL_Taskbar *taskbar, const SDL_TaskbarState state)
{
#if defined(SDL_PLATFORM_WINDOWS)
    const HWND hwnd = SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    return SDL_WINDOWS_SetTaskbarState(hwnd, taskbar, state);
#endif

    SDL_SetError("No available taskbar system for this platform.");
    return SDL_FALSE;
}

void SDL_DestroyTaskbar(SDL_Taskbar *taskbar)
{
    if (taskbar == NULL) {
        return;
    }

    SDL_free(taskbar);

    taskbar = NULL;
}
