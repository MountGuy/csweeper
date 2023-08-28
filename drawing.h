#pragma once
#include "game.h"

#define SMILE_NORMAL  0
#define SMILE_WOW     1
#define SMILE_LOST    2
#define SMILE_WINNER  3
#define SMILE_CLICKED 0

#define SMILE_BITMAP_WIDTH 24
#define SMILE_BITMAP_HEIGHT 24

__inline void InitializePen();
__inline HGLOBAL TryLoadBitmapResource(USHORT resourceID);
__inline BOOL LoadBitmapResources();
__inline void ProcessBlockBitmaps();

BOOL LoadBitmaps();

void AddAndDisplayLeftFlags(DWORD leftFlagsToAdd);
void DisplayNumber(HDC hDC, int xPosition, int numberToDisplay, int numberType);
void DisplayLeftFlags();
void DisplayLeftFlagsOnDC(HDC hDC);
void DisplaySmile(DWORD smileID);
void DisplaySmileOnDC(HDC hDC, DWORD smileID);
void DisplayTimerSeconds();
void DisplayTimerSecondsOnDC(HDC hDC);
void DisplayAllBlocks();
void DisplayAllBlocksInDC(HDC hDC);

void DrawBackground(HDC hDC);
void DrawBlock(BoardPoint point);
void RedrawUIOnDC(HDC hDC);
void DrawHUDRectangle(HDC hDC, RECT rect, int linesWidth, char whiteOrCopyPen);
void DrawHUDRectangles(HDC hDC);
void SetROPWrapper(HDC hDC, BYTE whiteOrCopyPen);
