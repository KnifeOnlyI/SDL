#ifndef SDL3_taskbar_h_
#define SDL3_taskbar_h_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ITaskbarList3 ITaskbarList3;

typedef struct SDL_Taskbar SDL_Taskbar;
typedef enum SDL_TaskbarState SDL_TaskbarState;

struct SDL_Taskbar
{
#if defined(SDL_PLATFORM_WINDOWS)
    ITaskbarList3 *handle;
#endif
};

enum SDL_TaskbarState
{
    SDL_TASKBAR_STATE_IDLE,
    SDL_TASKBAR_STATE_IN_PROGRESS,
    SDL_TASKBAR_STATE_PAUSED,
    SDL_TASKBAR_STATE_ERROR
};

extern DECLSPEC SDL_Taskbar *SDLCALL SDL_CreateTaskbar();
extern DECLSPEC SDL_bool SDLCALL SDL_SetTaskbarState(SDL_Window *window, const SDL_Taskbar *taskbar, const SDL_TaskbarState state);
extern DECLSPEC SDL_bool SDLCALL SDL_SetTaskbarProgressValue(SDL_Window *window, const SDL_Taskbar *taskbar, const float currentPercentage);
extern DECLSPEC void SDLCALL SDL_DestroyTaskbar(SDL_Taskbar *taskbar);

#ifdef __cplusplus
}
#endif

#endif
