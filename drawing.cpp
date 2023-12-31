#include <Windows.h>
#include <stdio.h>
#include "csweeper.h"
#include "drawing.h"
#include "game.h"
#include "config.h"

#define POINT_DIG1_NUM_X 16
#define POINT_DIG2_NUM_X 29
#define POINT_DIG3_NUM_X 42
#define POINT_DIG4_NUM_X 55

#define NUMBER_WIDTH 13
#define NUMBER_HEIGHT 23
#define NUMBER_Y 16

#define BLOCK_WIDTH 16
#define BLOCK_HEIGHT 16

#define NUM_MINUS 11

#define USE_FLAG_NUM 0
#define USE_TIME_NUM 1

PBITMAPINFO lpBitmapInfo;
HGLOBAL hBitmapResource;
HPEN hPen;

HDC blockDCs[2][18];
HBITMAP blockBitmaps[2][18];

__inline void InitializePen() {
    hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
}

__inline HGLOBAL TryLoadBitmapResource(USHORT resourceID) {
    HRSRC hRsrc = FindResourceW(hInst, (LPCWSTR)(resourceID), (LPWSTR)RT_BITMAP);

    if (hRsrc != NULL) {
        return LoadResource(hInst, hRsrc);
    }

    return NULL;
}

__inline BOOL LoadBitmapResources() {
    hBitmapResource = TryLoadBitmapResource(IDB_BITMAP);

    // Yea I know, it's wierd that the check is performed after all the resources are loaded..
    if (hBitmapResource == NULL) {
        return FALSE;
    }

    lpBitmapInfo = (PBITMAPINFO)LockResource(hBitmapResource);
    return TRUE;
}

__inline void ProcessBlockBitmaps() {
    HDC hWndDC = GetDC(hWnd);

    for (int i = 0; i < _countof(blockDCs[0]); ++i) {
        blockDCs[OFF_CURSOR][i] = CreateCompatibleDC(hWndDC);
        blockDCs[ON_CURSOR][i] = CreateCompatibleDC(hWndDC);

        if (blockDCs[OFF_CURSOR][i] == NULL || blockDCs[ON_CURSOR][i] == NULL) {
            // Maybe that means the original name of the function was "FLoad"
            // This is a bad name dude. 
            OutputDebugStringA("FLoad failed to create compatible dc\n");
            exit(1);
        }

        blockBitmaps[OFF_CURSOR][i] = CreateCompatibleBitmap(hWndDC, 16, 16);
        blockBitmaps[ON_CURSOR][i] = CreateCompatibleBitmap(hWndDC, 16, 16);

        if (blockBitmaps[OFF_CURSOR][i] == NULL || blockBitmaps[ON_CURSOR][i] == NULL) {
            OutputDebugStringA("Failed to create Bitmap\n");
            exit(1);
        }
        SelectObject(blockDCs[OFF_CURSOR][i], blockBitmaps[OFF_CURSOR][i]);
        SelectObject(blockDCs[ON_CURSOR][i], blockBitmaps[ON_CURSOR][i]);
        SetDIBitsToDevice(blockDCs[OFF_CURSOR][i],
            0, 0,
            16, 16,
            0 + 16 * i, 108,
            0, 108 + 16,
            lpBitmapInfo->bmiColors,
            lpBitmapInfo,
            DIB_RGB_COLORS
        );
        BitBlt(blockDCs[ON_CURSOR][i], 0, 0, 16, 16, blockDCs[OFF_CURSOR][i], 0, 0, SRCCOPY);
        BitBlt(blockDCs[ON_CURSOR][i], 3, 3, 11, 11, blockDCs[OFF_CURSOR][i], 3, 3, NOTSRCCOPY);

    }

    ReleaseDC(hWnd, hWndDC);
}

BOOL LoadBitmaps() {

    if (!LoadBitmapResources()) {
        return FALSE;
    }

    InitializePen();
    ProcessBlockBitmaps();

    return TRUE;
}

//-----------------------------------------------------------

void AddAndDisplayLeftFlags(DWORD leftFlagsToAdd) {
    leftFlags += leftFlagsToAdd;
    DisplayLeftFlags();
}

void DisplayNumber(HDC hDC, int xPosition, int numberToDisplay, int numberType) {
    int ySrc;
    if (numberType == USE_FLAG_NUM) {
        ySrc = 0;
    }
    else if(numberType == USE_TIME_NUM) {
        ySrc = 46;
    }
    else {
        ySrc = 23;
    }
    SetDIBitsToDevice(
        hDC,
        xPosition,
        NUMBER_Y,
        NUMBER_WIDTH,
        NUMBER_HEIGHT,
        NUMBER_WIDTH * numberToDisplay,
        ySrc,
        0,
        ySrc + 23,
        lpBitmapInfo->bmiColors,
        lpBitmapInfo,
        0
    );

}

void DisplayLeftFlags() {
    HDC hDC = GetDC(hWnd);
    DisplayLeftFlagsOnDC(hDC);
    ReleaseDC(hWnd, hDC);
}

void DisplayLeftFlagsOnDC(HDC hDC) {
    DWORD layout = GetLayout(hDC);

    if (layout & 1) {
        // Set layout to Left To Right
        SetLayout(hDC, 0);
    }

    int lowDigit;
    int highNum;

    if (leftFlags >= 0) {
        lowDigit = (leftFlags / 1000) % 10;
        highNum = leftFlags % 1000;
    }
    else {
        lowDigit = NUM_MINUS;
        highNum = leftFlags % 1000;
    }

    DisplayNumber(hDC, 16, lowDigit, USE_FLAG_NUM);
    DisplayNumber(hDC, 29, (highNum / 100) % 10, USE_FLAG_NUM);
    DisplayNumber(hDC, 42, (highNum / 10) % 10, USE_FLAG_NUM);
    DisplayNumber(hDC, 55, highNum % 10, USE_FLAG_NUM);

    if (layout & 1) {
        SetLayout(hDC, layout);
    }
}

void DisplaySmile(DWORD smileID) {
    HDC hDC = GetDC(hWnd);
    DisplaySmileOnDC(hDC, smileID);
    ReleaseDC(hWnd, hDC);
}

void DisplaySmileOnDC(HDC hDC, DWORD smileID) {
    SetDIBitsToDevice(
        hDC, // hdc
        (xRight - 24) / 2, // x
        16, // y
        23, // w
        23, // h
        150 + 23 * smileID, // xSrc
        69, // ySrc 
        0, // ScanStart
        23 + 69, // cLines
        lpBitmapInfo->bmiColors, // lpvBits
        lpBitmapInfo, // lpbmi
        TRUE // ColorUse
    );
}

void DisplayTimerSeconds() {
    HDC hDC = GetDC(hWnd);
    DisplayTimerSecondsOnDC(hDC);
    ReleaseDC(hWnd, hDC);
}

void DisplayTimerSecondsOnDC(HDC hDC) {

    DWORD layout = GetLayout(hDC);

    if (layout & 1) {
        SetLayout(hDC, 0);
    }

    DisplayNumber(hDC, xRight - 68, (timerSeconds / 1000) % 10, USE_TIME_NUM);
    DisplayNumber(hDC, xRight - 55, (timerSeconds / 100) % 10, USE_TIME_NUM);
    DisplayNumber(hDC, xRight - 42, (timerSeconds / 10) % 10, USE_TIME_NUM);
    DisplayNumber(hDC, xRight - 29, timerSeconds % 10, USE_TIME_NUM);

    if (layout & 1) {
        SetLayout(hDC, layout);
    }
}

void DisplayAllBlocks() {
    HDC hDC = GetDC(hWnd);
    DisplayAllBlocksInDC(hDC);
    ReleaseDC(hWnd, hDC);
}

void DisplayAllBlocksInDC(HDC hDC) {
    int y = 55;
    for (int r = 1; r <= height; ++r) {
        int x = 12;

        for (int c = 1; c <= width; c++) {
            // Get the current state of the block
            BYTE block = blockArray[r][c];
            int isOnCursor = (r == cursorPoint.row && c == cursorPoint.column);
            HDC blockState = blockDCs[isOnCursor? ON_CURSOR: OFF_CURSOR][BLOCK_STATE(block)];

            // Draw the block
            BitBlt(hDC, x, y, 16, 16, blockState, 0, 0, SRCCOPY);
            x += 16;
        }

        y += 16;
    }
}

//-----------------------------------------------------------

void DrawBlock(BoardPoint point) {
    HDC hDC = GetDC(hWnd);
    BYTE block = BLOCK_STATE(ACCESS_BLOCK(point));
    int cursorType = (point.row == cursorPoint.row && point.column == cursorPoint.column && blink)? ON_CURSOR: OFF_CURSOR;
    BitBlt(hDC, point.column * 16 - 4, point.row * 16 + 39, BLOCK_WIDTH, BLOCK_HEIGHT, blockDCs[cursorType][block], 0, 0, SRCCOPY);
    ReleaseDC(hWnd, hDC);
}

void RedrawUIOnDC(HDC hDC) {
    DrawBackground(hDC);
    DrawHUDRectangles(hDC);
    DisplayLeftFlagsOnDC(hDC);
    DisplaySmileOnDC(hDC, globalSmileID);
    DisplayTimerSecondsOnDC(hDC);
    DisplayAllBlocksInDC(hDC);
}

void DrawBackground(HDC hDC) {
    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = xRight;
    rect.bottom = yBottom;
    FillRect(hDC, &rect, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
}

void DrawHUDRectangle(HDC hDC, RECT rect, int lineWidth, char whiteOrCopyPen) {

    SetROPWrapper(hDC, whiteOrCopyPen);

    for (int i = 0; i < lineWidth; i++) {
        rect.bottom--;
        MoveToEx(hDC, rect.left, rect.bottom, NULL);

        // Draw left vertical line
        LineTo(hDC, rect.left, rect.top);
        rect.left++;

        // Draw top line
        LineTo(hDC, rect.right, rect.top);
        rect.right--;
        rect.top++;
    }

    if (whiteOrCopyPen < 2) {
        SetROPWrapper(hDC, whiteOrCopyPen ^ 1);
    }

    for (int i = 0; i < lineWidth; i++) {
        rect.bottom++;
        MoveToEx(hDC, rect.left - 1, rect.bottom, NULL);
        rect.left--;
        rect.right++;

        // Draw lower line
        LineTo(hDC, rect.right, rect.bottom);
        rect.top--;

        // Draw right line
        LineTo(hDC, rect.right, rect.top - 1);
    }
}

void DrawHUDRectangles(HDC hDC) {
    RECT rect;

    // Draw Frame Bar
    // Does not use the lower-right lines
    rect.left = 0;
    rect.top = 0;
    rect.right = xRight - 1;
    rect.bottom = yBottom - 1;
    DrawHUDRectangle(hDC, rect, 1, 0);

    //  Blocks Board Rectangle
    rect.left = 11;
    rect.top = 54;
    rect.right = xRight - 12;
    rect.bottom = yBottom - 12;
    DrawHUDRectangle(hDC, rect, 1, 0);

    // Upper Rectangle - Contains: LeftMines | Smile | SecondsLeft
    rect.left = 11;
    rect.top = 11;
    rect.right = xRight - 12;
    rect.bottom = 43;
    DrawHUDRectangle(hDC, rect, 1, 0);
}

void SetROPWrapper(HDC hDC, BYTE whiteOrCopyPen) {
    if (whiteOrCopyPen & 1) {
        SetROP2(hDC, R2_WHITE);
    }
    else {
        SetROP2(hDC, R2_COPYPEN);
        SelectObject(hDC, hPen);
    }
}
