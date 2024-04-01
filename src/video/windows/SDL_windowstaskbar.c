#include "SDL_windowstaskbar.h"

/*
SDL_Taskbar *SDL_WINDOWS_CreateTaskbar()
{
    HRESULT result = CoInitialize(NULL);

    if (result != S_OK && result != S_FALSE) {
        SDL_SetError("Cannot initialize windows COM library. (Windows ERROR : %ld)", result);

        return NULL;
    }

    ITaskbarList3 *pTaskbarList = NULL;

    result = CoCreateInstance(
        &CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
        &IID_ITaskbarList3,
        (void **)&pTaskbarList);

    if (result != S_OK) {
        SDL_SetError("Cannot create windows TaskbarList instance. (Windows ERROR : %ld)", result);

        return NULL;
    }

    result = pTaskbarList->lpVtbl->HrInit(pTaskbarList);

    if (result != S_OK) {
        SDL_SetError("Cannot initialize windows TaskbarList instance. (Windows ERROR : %ld)", result);

        return NULL;
    }

    SDL_Taskbar *taskbar = SDL_malloc(sizeof(SDL_Taskbar));

    if (taskbar == NULL) {
        SDL_SetError("Cannot allocate memory for windows Taskbar.");

        return NULL;
    }

    taskbar->handle = pTaskbarList;

    return taskbar;
}

SDL_bool SDL_WINDOWS_SetTaskbarProgressValue(const HWND hwnd, const SDL_Taskbar *taskbar, const long currentValue, const long maximumValue)
{
    const HRESULT result = taskbar->handle->lpVtbl->SetProgressValue(
        taskbar->handle,
        hwnd,
        currentValue,
        maximumValue);

    if (result != S_OK) {
        SDL_SetError("Cannot set windows Taskbar progress value. (Windows ERROR : %ld)", result);
        return SDL_FALSE;
    }

    return SDL_TRUE;
}

SDL_bool SDL_WINDOWS_SetTaskbarState(const HWND hwnd, const SDL_Taskbar *taskbar, const SDL_TaskbarState state)
{
    TBPFLAG flag;

    switch (state) {
    case SDL_TASKBAR_STATE_IDLE:
        flag = TBPF_NOPROGRESS;
        break;
    case SDL_TASKBAR_STATE_IN_PROGRESS:
        flag = TBPF_NORMAL;
        break;
    case SDL_TASKBAR_STATE_PAUSED:
        flag = TBPF_PAUSED;
        break;
    case SDL_TASKBAR_STATE_ERROR:
        flag = TBPF_ERROR;
        break;
    default:
        flag = TBPF_NOPROGRESS;
    }

    const HRESULT result = taskbar->handle->lpVtbl->SetProgressState(taskbar->handle, hwnd, flag);

    if (result != S_OK) {
        SDL_SetError("Cannot set windows Taskbar progress state to normal. (Windows ERROR : %ld)", result);

        return SDL_FALSE;
    }

    return SDL_TRUE;
}
*/

int SDL_WINDOWS_TestTaskbar(void)
{
    return 123;
}