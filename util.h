#pragma once
#include <Windows.h>

// Misc
int GetRandom(DWORD maxValue);
void DisplayErrorMessage(UINT uID);
void LoadResourceString(UINT uID, LPWSTR lpBuffer, DWORD cchBufferMax);
