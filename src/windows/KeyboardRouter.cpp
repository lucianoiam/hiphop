/*
 * dpf-webui
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "KeyboardRouter.hpp"

#include <processthreadsapi.h>

#include "macro.h"

USE_NAMESPACE_DISTRHO

KeyboardRouter::KeyboardRouter()
    : fRefCount(0)
    , fHostHWnd(0)
    , fKeyboardHook(0)
{
    // Some hosts need key events delivered directly to their main window
    EnumWindows(enumWindowsProc, (LPARAM)&fHostHWnd);
}

BOOL CALLBACK KeyboardRouter::enumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD winProcId = 0;
    GetWindowThreadProcessId(hWnd, &winProcId);

    if (winProcId == GetCurrentProcessId()) {
        char text[256];
        text[0] = '\0';
        GetWindowText(hWnd, (LPSTR)text, sizeof(text));

        if (strstr(text, "Ableton Live") != 0) {
            *((HWND *)lParam) = hWnd;
            return FALSE;
        }
    }

    return TRUE;
}

void KeyboardRouter::incRefCount()
{
    if (fRefCount++ == 0) {
        // Passing GetCurrentThreadId() to dwThreadId results in the hook never being called
        fKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardProc, GetModuleHandle(NULL), 0);
    }
}

void KeyboardRouter::decRefCount()
{
    if (--fRefCount == 0) {
        UnhookWindowsHookEx(fKeyboardHook);
    }
}

LRESULT CALLBACK KeyboardRouter::keyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{    
    // HC_ACTION means wParam & lParam contain info about keystroke message

    if (nCode == HC_ACTION) {
        HWND hFocusedPluginHelperWnd = 0;
        HWND hWnd = GetFocus();
        int level = 0;

        // Check if focused window belongs to the hierarchy of one of our plugin instances
        // Max 3 levels is reasonable for reaching plugin window from a Chrome child window

        while (level++ < 3) { 
            EnumChildWindows(hWnd, enumChildProc, (LPARAM)&hFocusedPluginHelperWnd);

            // Plugin DPF window should have the helper window as a child

            if (hFocusedPluginHelperWnd != 0) {
                break;
            }

            hWnd = GetParent(hWnd);
        }

        if (hFocusedPluginHelperWnd != 0) {
            bool grabKeyboardInput = (bool)GetClassLongPtr(hFocusedPluginHelperWnd, 0);

            if (grabKeyboardInput) {
                // The plugin decided to grab keyboard input, discard keystroke
            
            } else {
                // The root window is provided by the host and has DPF window as a child
                // Key events may be delivered to the plugin root window or host main window

                HWND hPluginRootWnd = GetParent(hFocusedPluginHelperWnd);

                KeyboardRouter::getInstance().handleLowLevelKeyEvent(hPluginRootWnd,
                    (UINT)wParam, (KBDLLHOOKSTRUCT *)lParam);
            }
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL CALLBACK KeyboardRouter::enumChildProc(HWND hWnd, LPARAM lParam)
{
    (void)lParam;

    char className[256];
    GetClassName(hWnd, (LPSTR)className, sizeof(className));

    if ((strstr(className, "EdgeWebWidget") != 0) && (strstr(className, XSTR(PROJECT_ID_HASH)) != 0)) {
        *((HWND *)lParam) = hWnd;
        return FALSE;
    }

    return TRUE;
}

void KeyboardRouter::handleLowLevelKeyEvent(HWND hPluginRootWnd, UINT message, KBDLLHOOKSTRUCT* lpData)
{
    // If targeting the plugin root window focus it first

    HWND hTargetWnd;

    if (fHostHWnd == 0) {
        hTargetWnd = hPluginRootWnd;
        SetFocus(hTargetWnd);
    } else {
        hTargetWnd = fHostHWnd;
    }

    // Translate low level keyboard events into a format suitable for SendMessage()

    WPARAM wParam = lpData->vkCode;
    LPARAM lParam = /* scan code */ lpData->scanCode << 16 | /* repeat count */ 0x1;

    switch (message) {
        case WM_KEYDOWN:
            // Basic logic that forwards a-z to allow playing with Live's virtual keyboard.
            SendMessage(hTargetWnd, WM_KEYDOWN, wParam, lParam);

            if ((lpData->vkCode >= 'A') && (lpData->vkCode <= 'Z')) {
                wParam |= 0x20; // to lowercase
                SendMessage(hTargetWnd, WM_CHAR, wParam, lParam);
            }

            break;
        case WM_KEYUP:
            // bit 30: The previous key state. The value is always 1 for a WM_KEYUP message.
            // bit 31: The transition state. The value is always 1 for a WM_KEYUP message.
            lParam |= 0xC0000000;
            SendMessage(hTargetWnd, WM_KEYUP, wParam, lParam);
            
            break;
    }
}