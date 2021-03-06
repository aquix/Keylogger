﻿#include "stdafx.h"
#include "Hook.h"

#include "helpers/KeyboardHelper.h"
#include "services/KeyBlockService.h"

std::function<void(const wchar_t[])> Hook::callback;
HHOOK Hook::hHook;
bool Hook::isEnabled;

void Hook::setHook(std::function<void(const wchar_t[])> keyboardCallback) {
	isEnabled = true;
    callback = keyboardCallback;
    std::wcout << L"set hook" << std::endl;
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, Hook::hookProc, nullptr, 0);
}

void Hook::unsetHook() {
	isEnabled = false;
    std::wcout << L"unset" << std::endl;
    UnhookWindowsHookEx(hHook);
}

void Hook::enable() {
	Hook::isEnabled = true;
}

void Hook::disable() {
	Hook::isEnabled = false;
}

bool Hook::checkEnabled() {
	return Hook::isEnabled;
}

LRESULT CALLBACK Hook::hookProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
	if (!Hook::isEnabled) {
		return CallNextHookEx(hHook, nCode, wParam, lParam);
	}

    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
    BYTE keyState[256] = { 0 };
    std::wstring result;
    bool isKeyCombination = false;

    GetKeyState(0);
    GetKeyboardState(keyState);

    PMSG pmsg = (PMSG)lParam;

    // Check alt combinations
    if (p->flags == 32 && p->vkCode == VK_TAB) {
        result = L"<ALT-TAB>";
        isKeyCombination = true;
    } else if (p->flags == 32 && p->vkCode == 160) {
        result = L"<ALT-SHIFT>";
        isKeyCombination = true;
    }

    if (!isKeyCombination && wParam != WM_KEYDOWN) {
        if (KeyBlockService::getInstance()->isBlocked(p->vkCode)) {
            return 1;
        }
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }

    // Check control combinations
    int ctrlState = GetKeyState(VK_LCONTROL);
    if (ctrlState) {
        if (p->flags && LLKHF_ALTDOWN && p->vkCode == VK_DELETE) {
            result = L"<CTRL-ALT-DEL>";
            isKeyCombination = true;
        }
    }

    if (isKeyCombination) {
        callback(result.c_str());
        if (KeyBlockService::getInstance()->isBlocked(p->vkCode)) {
            return 1;
        }
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    }

    result = getKeyNameByVkCode(p, keyState, true);
    callback(result.c_str());

    if (KeyBlockService::getInstance()->isBlocked(p->vkCode)) {
        return 1;
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

std::wstring Hook::getKeyNameByVkCode(PKBDLLHOOKSTRUCT p, BYTE* keyState, bool asText) {
    std::wstring result = KeyboardHelper::getInstance()->getKeyByCode(p->vkCode, p->scanCode, keyState);

    if (p->vkCode == VK_RETURN && asText) {
        result = L"\n";
    } else if (p->vkCode == VK_TAB && asText) {
        result = L"\t";
    } else if (p->vkCode == VK_SPACE && asText) {
        result = L" ";
    }
    return result;
}