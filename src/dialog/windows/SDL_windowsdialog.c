/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "SDL_internal.h"

#include <windows.h>
#include <shlobj.h>
#include "../../core/windows/SDL_windows.h"
#include "../../thread/SDL_systhread.h"

typedef struct
{
    int is_save;
    const SDL_DialogFileFilter *filters;
    const char* default_file;
    const char* default_folder;
    SDL_Window* parent;
    DWORD flags;
    SDL_DialogFileCallback callback;
    void* userdata;
} winArgs;

typedef struct
{
    SDL_Window* parent;
    SDL_DialogFileCallback callback;
    const char* default_folder;
    void* userdata;
} winFArgs;

/** Converts dialog.nFilterIndex to SDL-compatible value */
int getFilterIndex(int as_reported_by_windows, const SDL_DialogFileFilter *filters)
{
    int filter_index = as_reported_by_windows - 1;

    if (filter_index < 0) {
        filter_index = 0;
        for (const SDL_DialogFileFilter *filter = filters; filter && filter->name && filter->pattern; filter++) {
            filter_index++;
        }
    }

    return filter_index;
}

/* TODO: The new version of file dialogs */
void windows_ShowFileDialog(void *ptr)
{
    winArgs *args = (winArgs *) ptr;
    int is_save = args->is_save;
    const SDL_DialogFileFilter *filters = args->filters;
    const char* default_file = args->default_file;
    const char* default_folder = args->default_folder;
    SDL_Window* parent = args->parent;
    DWORD flags = args->flags;
    SDL_DialogFileCallback callback = args->callback;
    void* userdata = args->userdata;

    /* GetOpenFileName and GetSaveFileName have the same signature
       (yes, LPOPENFILENAMEW even for the save dialog) */
    typedef BOOL (WINAPI *pfnGetAnyFileNameW)(LPOPENFILENAMEW);
    typedef DWORD (WINAPI *pfnCommDlgExtendedError)(void);
    HMODULE lib = LoadLibraryW(L"Comdlg32.dll");
    pfnGetAnyFileNameW pGetAnyFileName = NULL;
    pfnCommDlgExtendedError pCommDlgExtendedError = NULL;

    if (lib) {
        pGetAnyFileName = (pfnGetAnyFileNameW) GetProcAddress(lib, is_save ? "GetSaveFileNameW" : "GetOpenFileNameW");
        pCommDlgExtendedError = (pfnCommDlgExtendedError) GetProcAddress(lib, "CommDlgExtendedError");
    } else {
        SDL_SetError("Couldn't load Comdlg32.dll");
        callback(userdata, NULL, -1);
        return;
    }

    if (!pGetAnyFileName) {
        SDL_SetError("Couldn't load GetOpenFileName/GetSaveFileName from library");
        callback(userdata, NULL, -1);
        return;
    }

    if (!pCommDlgExtendedError) {
        SDL_SetError("Couldn't load CommDlgExtendedError from library");
        callback(userdata, NULL, -1);
        return;
    }

    HWND window = NULL;

    if (parent) {
        window = (HWND) SDL_GetProperty(SDL_GetWindowProperties(parent), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    }

    wchar_t filebuffer[MAX_PATH] = L"";
    wchar_t initfolder[MAX_PATH] = L"";

    /* Necessary for the return code below */
    SDL_memset(filebuffer, 0, MAX_PATH * sizeof(wchar_t));

    if (default_file) {
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, default_file, -1, filebuffer, MAX_PATH);
    }

    if (default_folder) {
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, default_folder, -1, filebuffer, MAX_PATH);
    }

    size_t len = 0;
    for (const SDL_DialogFileFilter *filter = filters; filter && filter->name && filter->pattern; filter++) {
        const char *pattern_ptr = filter->pattern;
        len += SDL_strlen(filter->name) + SDL_strlen(filter->pattern) + 4;
        while (*pattern_ptr) {
            if (*pattern_ptr == ';') {
                len += 2;
            }
            pattern_ptr++;
        }
    }
    wchar_t *filterlist = SDL_malloc((len + 1) * sizeof(wchar_t));

    if (!filterlist) {
        SDL_OutOfMemory();
        callback(userdata, NULL, -1);
        return;
    }

    wchar_t *filter_ptr = filterlist;
    for (const SDL_DialogFileFilter *filter = filters; filter && filter->name && filter->pattern; filter++) {
        size_t l = SDL_strlen(filter->name);
        const char *pattern_ptr = filter->pattern;

        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, filter->name, -1, filter_ptr, MAX_PATH);
        filter_ptr += l + 1;

        *filter_ptr++ = L'*';
        *filter_ptr++ = L'.';
        while (*pattern_ptr) {
            if (*pattern_ptr == ';') {
                *filter_ptr++ = L';';
                *filter_ptr++ = L'*';
                *filter_ptr++ = L'.';
            } else if (*pattern_ptr == '*' && (pattern_ptr[1] == '\0' || pattern_ptr[1] == ';')) {
                *filter_ptr++ = L'*';
            } else if (!((*pattern_ptr >= 'a' && *pattern_ptr <= 'z') || (*pattern_ptr >= 'A' && *pattern_ptr <= 'Z') || (*pattern_ptr >= '0' && *pattern_ptr <= '9') || *pattern_ptr == '.' || *pattern_ptr == '_' || *pattern_ptr == '-')) {
                SDL_SetError("Illegal character in pattern name: %c (Only alphanumeric characters, periods, underscores and hyphens allowed)", *pattern_ptr);
                callback(userdata, NULL, -1);
            } else {
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pattern_ptr, 1, filter_ptr, 1);
                filter_ptr++;
            }
            pattern_ptr++;
        }
        *filter_ptr++ = '\0';
    }
    *filter_ptr = '\0';


    OPENFILENAMEW dialog;
    dialog.lStructSize = sizeof(OPENFILENAME);
    dialog.hwndOwner = window;
    dialog.hInstance = 0;
    dialog.lpstrFilter = filterlist;
    dialog.lpstrCustomFilter = NULL;
    dialog.nMaxCustFilter = 0;
    dialog.nFilterIndex = 0;
    dialog.lpstrFile = filebuffer;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrFileTitle = *filebuffer ? filebuffer : NULL;
    dialog.nMaxFileTitle = MAX_PATH;
    dialog.lpstrInitialDir = *initfolder ? initfolder : NULL;
    dialog.lpstrTitle = NULL;
    dialog.Flags = flags | OFN_EXPLORER | OFN_HIDEREADONLY;
    dialog.nFileOffset = 0;
    dialog.nFileExtension = 0;
    dialog.lpstrDefExt = NULL;
    dialog.lCustData = 0;
    dialog.lpfnHook = NULL;
    dialog.lpTemplateName = NULL;
    /* Skipped many mac-exclusive and reserved members */
    dialog.FlagsEx = 0;

    BOOL result = pGetAnyFileName(&dialog);

    SDL_free(filterlist);

    if (result) {
        if (!(flags & OFN_ALLOWMULTISELECT)) {
            /* File is a C string stored in dialog.lpstrFile */
            char *chosen_file = WIN_StringToUTF8W(dialog.lpstrFile);
            const char* opts[2] = { chosen_file, NULL };
            callback(userdata, opts, getFilterIndex(dialog.nFilterIndex, filters));
            SDL_free(chosen_file);
        } else {
            /* File is either a C string if the user chose a single file, else
               it's a series of strings formatted like:

                   "C:\\path\\to\\folder\0filename1.ext\0filename2.ext\0\0"

               The code below will only stop on a double NULL in all cases, so
               it is important that the rest of the buffer has been zeroed. */
            char chosen_folder[MAX_PATH];
            char chosen_file[MAX_PATH];
            wchar_t *file_ptr = dialog.lpstrFile;
            size_t nfiles = 0;
            size_t chosen_folder_size;
            char **chosen_files_list = (char **) SDL_malloc(sizeof(char *) * (nfiles + 1));

            if (!chosen_files_list) {
                SDL_OutOfMemory();
                callback(userdata, NULL, -1);
                return;
            }

            chosen_files_list[nfiles] = NULL;

            if (WideCharToMultiByte(CP_UTF8, 0, file_ptr, -1, chosen_folder, MAX_PATH, NULL, NULL) >= MAX_PATH) {
                SDL_SetError("Path too long or invalid character in path");
                SDL_free(chosen_files_list);
                callback(userdata, NULL, -1);
                return;
            }

            chosen_folder_size = SDL_strlen(chosen_folder);
            SDL_strlcpy(chosen_file, chosen_folder, MAX_PATH);
            chosen_file[chosen_folder_size] = '\\';

            file_ptr += SDL_strlen(chosen_folder) + 1;

            while (*file_ptr) {
                nfiles++;
                char **new_cfl = (char **) SDL_realloc(chosen_files_list, sizeof(char*) * (nfiles + 1));

                if (!new_cfl) {
                    SDL_OutOfMemory();

                    for (size_t i = 0; i < nfiles - 1; i++) {
                        SDL_free(chosen_files_list[i]);
                    }

                    SDL_free(chosen_files_list);
                    callback(userdata, NULL, -1);
                    return;
                }

                chosen_files_list = new_cfl;
                chosen_files_list[nfiles] = NULL;

                int diff = ((int) chosen_folder_size) + 1;

                if (WideCharToMultiByte(CP_UTF8, 0, file_ptr, -1, chosen_file + diff, MAX_PATH - diff, NULL, NULL) >= MAX_PATH - diff) {
                    SDL_SetError("Path too long or invalid character in path");

                    for (size_t i = 0; i < nfiles - 1; i++) {
                        SDL_free(chosen_files_list[i]);
                    }

                    SDL_free(chosen_files_list);
                    callback(userdata, NULL, -1);
                    return;
                }

                file_ptr += SDL_strlen(chosen_file) + 1 - diff;

                chosen_files_list[nfiles - 1] = SDL_strdup(chosen_file);

                if (!chosen_files_list[nfiles - 1]) {
                    SDL_OutOfMemory();

                    for (size_t i = 0; i < nfiles - 1; i++) {
                        SDL_free(chosen_files_list[i]);
                    }

                    SDL_free(chosen_files_list);
                    callback(userdata, NULL, -1);
                    return;
                }
            }

            callback(userdata, (const char * const*) chosen_files_list, getFilterIndex(dialog.nFilterIndex, filters));

            for (size_t i = 0; i < nfiles; i++) {
                SDL_free(chosen_files_list[i]);
            }

            SDL_free(chosen_files_list);
        }
    } else {
        DWORD error = pCommDlgExtendedError();
        /* Error code 0 means the user clicked the cancel button. */
        if (error == 0) {
            /* Unlike SDL's handling of errors, Windows does reset the error
               code to 0 after calling GetOpenFileName if another Windows
               function before set a different error code, so it's safe to
               check for success. */
            const char* opts[1] = { NULL };
            callback(userdata, opts, getFilterIndex(dialog.nFilterIndex, filters));
        } else {
            SDL_SetError("Windows error, CommDlgExtendedError: %ld", pCommDlgExtendedError());
            callback(userdata, NULL, -1);
        }
    }
}

int windows_file_dialog_thread(void* ptr)
{
    windows_ShowFileDialog(ptr);
    SDL_free(ptr);
    return 0;
}

int CALLBACK browse_callback_proc(
				HWND hwnd, 
				UINT uMsg, 
				LPARAM lParam, 
				LPARAM lpData)
{
  
	switch (uMsg)
	{
	case BFFM_INITIALIZED :
		if(lpData)
        {
		  SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
		break;
	case BFFM_SELCHANGED :
		break;
	case BFFM_VALIDATEFAILED :
		break;
	default:
		break;
	}
	return 0; 
}

void windows_ShowFolderDialog(void* ptr)
{
    winFArgs *args = (winFArgs *) ptr;
    SDL_Window *window = args->parent;
    SDL_DialogFileCallback callback = args->callback;
    void *userdata = args->userdata;

    HWND parent = NULL;

    if (window) {
        parent = (HWND) SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    }

    wchar_t buffer[MAX_PATH];

    BROWSEINFOW dialog;
    dialog.hwndOwner = parent;
    dialog.pidlRoot = NULL;
    /* Windows docs say this is `LPTSTR` - apparently it's actually `LPWSTR`*/
    dialog.pszDisplayName = buffer;
    dialog.lpszTitle = NULL;
    dialog.ulFlags = BIF_USENEWUI;
    dialog.lpfn = browse_callback_proc;
    dialog.lParam = (LPARAM)args->default_folder;
    dialog.iImage = 0;

    LPITEMIDLIST lpItem = SHBrowseForFolderW(&dialog);
    if (lpItem != NULL) {
        SHGetPathFromIDListW(lpItem, buffer);
        char *chosen_file = WIN_StringToUTF8W(buffer);
        const char *files[2] = { chosen_file, NULL };
        callback(userdata, (const char * const*) files, -1);
        SDL_free(chosen_file);
    } else {
        const char *files[1] = { NULL };
        callback(userdata, (const char * const*) files, -1);
    }
}

int windows_folder_dialog_thread(void* ptr)
{
    windows_ShowFolderDialog(ptr);
    SDL_free(ptr);
    return 0;
}

void SDL_ShowOpenFileDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter *filters, const char* default_location, SDL_bool allow_many)
{
    winArgs *args;
    SDL_Thread *thread;

    args = SDL_malloc(sizeof(winArgs));
    if (args == NULL) {
        SDL_OutOfMemory();
        callback(userdata, NULL, -1);
        return;
    }

    args->is_save = 0;
    args->filters = filters;
    args->default_file = default_location;
    args->default_folder = NULL;
    args->parent = window;
    args->flags = (allow_many == SDL_TRUE) ? OFN_ALLOWMULTISELECT : 0;
    args->callback = callback;
    args->userdata = userdata;

    thread = SDL_CreateThreadInternal(windows_file_dialog_thread, "SDL_ShowOpenFileDialog", 0, (void *) args);

    if (thread == NULL) {
        callback(userdata, NULL, -1);
        return;
    }

    SDL_DetachThread(thread);
}

void SDL_ShowSaveFileDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const SDL_DialogFileFilter *filters, const char* default_location)
{
    winArgs *args;
    SDL_Thread *thread;

    args = SDL_malloc(sizeof(winArgs));
    if (args == NULL) {
        SDL_OutOfMemory();
        callback(userdata, NULL, -1);
        return;
    }

    args->is_save = 1;
    args->filters = filters;
    args->default_file = default_location;
    args->default_folder = NULL;
    args->parent = window;
    args->flags = 0;
    args->callback = callback;
    args->userdata = userdata;

    thread = SDL_CreateThreadInternal(windows_file_dialog_thread, "SDL_ShowSaveFileDialog", 0, (void *) args);

    if (thread == NULL) {
        callback(userdata, NULL, -1);
        return;
    }

    SDL_DetachThread(thread);
}

void SDL_ShowOpenFolderDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window, const char* default_location, SDL_bool allow_many)
{
    winFArgs *args;
    SDL_Thread *thread;

    args = SDL_malloc(sizeof(winFArgs));
    if (args == NULL) {
        SDL_OutOfMemory();
        callback(userdata, NULL, -1);
        return;
    }

    args->parent = window;
    args->callback = callback;
    args->default_folder = default_location;
    args->userdata = userdata;

    thread = SDL_CreateThreadInternal(windows_folder_dialog_thread, "SDL_ShowOpenFolderDialog", 0, (void *) args);

    if (thread == NULL) {
        callback(userdata, NULL, -1);
        return;
    }

    SDL_DetachThread(thread);
}
