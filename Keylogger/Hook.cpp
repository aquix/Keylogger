﻿#include "stdafx.h"
#include "Hook.h"

std::function<void(const wchar_t[])> Hook::callback;
HHOOK Hook::hHook;

void Hook::setHook(std::function<void(const wchar_t[])> keyboardCallback) {
	callback = keyboardCallback;
	std::cout << "set hook" << std::endl;
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, this->hookProc, nullptr, 0);
}

void Hook::unsetHook() {
	std::cout << "unset" << std::endl;
	UnhookWindowsHookEx(hHook);
}

void Hook::blockKey(std::string keyName) {}

void Hook::unblockKey(std::string keyName) {}

LRESULT CALLBACK Hook::hookProc(const int nCode, const WPARAM wParam, const LPARAM lParam) {
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
		result = L"<ALT_SHIFT>";
		isKeyCombination = true;
	}

	if (!isKeyCombination && wParam != WM_KEYDOWN) {
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
		return CallNextHookEx(hHook, nCode, wParam, lParam);
	}

	result = getKeyNameByVkCode(p, keyState, true);
	callback(result.c_str());

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

std::wstring Hook::getKeyNameByVkCode(PKBDLLHOOKSTRUCT p, BYTE* keyState, bool escaped) {
	std::wstring result = KeyboardHelper::getInstance()->getNameOrUnicode(p->vkCode, p->scanCode, keyState);

	if (p->vkCode == VK_RETURN && escaped) {
		result = L"\n";
	} else if (p->vkCode == VK_TAB && escaped) {
		result = L"\t";
	}
	return result;
}

bool Hook::isBlocked(int keyCode) {
	return false;
}
