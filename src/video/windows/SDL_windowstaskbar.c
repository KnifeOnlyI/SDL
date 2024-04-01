#include "SDL_windowstaskbar.h"

DEFINE_GUID(CLSID_TaskbarList, 0x56fdf344, 0xfd6d, 0x11d0, 0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0xa0, 0x90);
DEFINE_GUID(IID_ITaskbarList3, 0xea1afb91, 0x9e28, 0x4b86, 0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf);

typedef enum TBPFLAG
{
    TBPF_NOPROGRESS = 0x0,
    TBPF_INDETERMINATE = 0x1,
    TBPF_NORMAL = 0x2,
    TBPF_ERROR = 0x4,
    TBPF_PAUSED = 0x8
} TBPFLAG;

typedef ITaskbarList3 ITaskbarList3;

typedef struct ITaskbarList3Vtbl
{
    BEGIN_INTERFACE

    /*** ITaskbarList methods ***/

    HRESULT(STDMETHODCALLTYPE *HrInit)
    (ITaskbarList3 *This);

    /*** ITaskbarList3 methods ***/

    HRESULT(STDMETHODCALLTYPE *SetProgressValue)
    (ITaskbarList3 *This, HWND hwnd, ULONGLONG ullCompleted, ULONGLONG ullTotal);

    HRESULT(STDMETHODCALLTYPE *SetProgressState)
    (ITaskbarList3 *This, HWND hwnd, TBPFLAG tbpFlags);

    END_INTERFACE
} ITaskbarList3Vtbl;

struct ITaskbarList3
{
    CONST_VTBL ITaskbarList3Vtbl *lpVtbl;
};

SDL_Taskbar *SDL_WINDOWS_CreateTaskbar()
{
    HRESULT result = CoInitialize(NULL);

    if (result != S_OK && result != S_FALSE) {
        SDL_SetError("Cannot initialize windows COM library. (Windows ERROR : %ld)", result);

        return NULL;
    }

    ITaskbarList3 *pTaskbarList = NULL;

    result = CoCreateInstance(&CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, &IID_ITaskbarList3, (void **)&pTaskbarList);

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

SDL_bool SDL_WINDOWS_SetTaskbarProgressValue(const HWND hwnd, ITaskbarList3 *taskbar, const long currentValue, const long maximumValue)
{
    const HRESULT result = taskbar->lpVtbl->SetProgressValue(
        taskbar,
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