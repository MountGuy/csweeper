#include <Windows.h>
#include "csweeper.h"
#include "resource.h"

int GetRandom(DWORD maxValue) {
    return rand() % maxValue;
}

// .text:01003950
void DisplayErrorMessage(UINT uID) {
    WCHAR Buffer[128];
    WCHAR Caption[128];

    if (uID >= 999) {
        LoadStringW(hInst, ID_ERR_MSG_D, Caption, sizeof(Caption));
        wsprintfW(Buffer, Caption, uID);
    }
    else {
        LoadStringW(hInst, uID, Buffer, sizeof(Buffer));
    }

    // Show Title: Minesweeper Error, Content: Buffer from above
    LoadStringW(hInst, ID_ERROR, Caption, sizeof(Caption));
    MessageBoxW(NULL, Buffer, Caption, MB_ICONERROR);
}

void LoadResourceString(UINT uID, LPWSTR lpBuffer, DWORD cchBufferMax) {
    if (!LoadStringW(hInst, uID, lpBuffer, cchBufferMax)) {
        DisplayErrorMessage(1001);
    }
}
