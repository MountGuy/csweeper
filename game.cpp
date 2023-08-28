#include <Windows.h>
#include <stdio.h>
#include "game.h"
#include "csweeper.h"
#include "drawing.h"
#include "config.h"
#include "util.h"

DWORD stateFlags = STATE_GAME_FINISHED | STATE_WINDOW_MINIMIZED;

BOOL isTimerOnAndShowed;
BOOL IsTimerOnTemp;

BYTE blockArray[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
BoardPoint clickedPoint = { -1, -1 };
const BoardPoint nullPoint = { -2, -2 };

int globalSmileId;
int leftFlags;
int timerSeconds;
int width;
int height;
int numberOfEmptyBlocks;
int numberOfRevealedBlocks;

int currentRowColumnListIndex;
int rowsList[10000];
int columnsList[10000];

// New game
void InitializeNewGame() {
    int minesCopy;
    DWORD borderFlags;

    if (gameConfig.width == width && gameConfig.height == height) {
        borderFlags = WINDOW_BORDER_REPAINT_WINDOW;
    }
    else {
        borderFlags = WINDOW_BORDER_MOVE_WINDOW | WINDOW_BORDER_REPAINT_WINDOW;
    }

    width = gameConfig.width;
    height = gameConfig.height;

    InitializeBlockArrayBorders();

    globalSmileId = 0;

    // Setup all the mines
    minesCopy = gameConfig.mines;

    do {
        BoardPoint randomPoint;

        // Find a location for the mine
        do {
            randomPoint.column = GetRandom(width) + 1;
            randomPoint.row = GetRandom(height) + 1;
        } while (BLOCK_IS_REVEALED(ACCESS_BLOCK(randomPoint)));

        // SET A MINE
        ACCESS_BLOCK(randomPoint) |= BOMB_FLAG;
        minesCopy--;
    } while (minesCopy);

    timerSeconds = 0;
    minesCopy = gameConfig.mines;
    leftFlags = minesCopy;
    numberOfRevealedBlocks = 0;
    numberOfEmptyBlocks = (height * width) - gameConfig.mines;
    stateFlags = STATE_GAME_IS_ON;
    AddAndDisplayLeftFlags(0); // Should have called DisplayLeftFlags()!
    InitializeWindowBorder(borderFlags);
}

// Load resource and initialize board
BOOL InitializeBitmapsAndBlockArray() {
    if (LoadBitmaps()) {
        InitializeBlockArrayBorders();
        return TRUE;
    }

    return FALSE;
}

// Fill in the board with borders
void InitializeBlockArrayBorders() {

    for (int row = 1; row <= height; row++) {
        for (int column = 1; column <= width; column++) {
            blockArray[row][column] = BLOCK_STATE_EMPTY_UNCLICKED;
        }
    }
    for (int column = 0; column <= width + 1; column++) {
        // Fill upper border
        blockArray[0][column] = BLOCK_STATE_BORDER_VALUE;

        // Fill lower border
        blockArray[height + 1][column] = BLOCK_STATE_BORDER_VALUE;
    }

    for (int row = 0; row <= height + 1; row++) {
        // Fill left border
        blockArray[row][0] = BLOCK_STATE_BORDER_VALUE;

        // Fill right border
        blockArray[row][width + 1] = BLOCK_STATE_BORDER_VALUE;
    }
}

// Called on button release
// Called on MouseMove 
void ReleaseMouseCapture() {
    hasMouseCapture = FALSE;
    ReleaseCapture();

    if (GAME_IS_ON()) {
        // Mouse move always gets here
        HandleBlockClick();
        ReleaseBlocksClick();
    }
    else {
        HandleBlockClick();
    }
}

__inline void ReleaseBlocksClick() {
    UpdateClickedPointsState(nullPoint);
}

// Change the block state and draw it
void ChangeBlockState(BoardPoint point, BYTE blockState) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | blockState;
    DrawBlock(point);
}

// Handles every block click release
void HandleBlockClick() {
    if (IsInBoardRange(clickedPoint)) {
        // First Click! Initialize Timer 
        if (numberOfRevealedBlocks == 0 && timerSeconds == 0) {
            timerSeconds++;
            DisplayTimerSeconds();
            isTimerOnAndShowed = TRUE;

            if (!SetTimer(ghWnd, TIMER_ID, 1000, NULL)) {
                DisplayErrorMessage(ID_TIMER_ERROR);
            }
        }

        //In valid: skip this click
        if (!GAME_IS_ON()) {
            clickedPoint = nullPoint;
            return;
        }

        // Case of 3x3 click
        if (is3x3Click) {
            Handle3x3BlockClick(clickedPoint);
        }
        else {
            BYTE block = ACCESS_BLOCK(clickedPoint);

            if (!BLOCK_IS_REVEALED(block) && !BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG)) {
                HandleNormalBlockClick(clickedPoint);
            }
        }
    }

    DisplaySmile(globalSmileId);
}

void UpdateClickedPointsState(BoardPoint point) {
    if (point.column == clickedPoint.column && point.row == clickedPoint.row) {
        return;
    }

    // Save old click point
    const BoardPoint oldPoint = clickedPoint;

    // Update new click point
    clickedPoint = point;

    if (is3x3Click) {
        UpdateClickedPointsState3x3(point, oldPoint);
    }
    else {
        UpdateClickedPointsStateNormal(point, oldPoint);
    }
}

void HandleNormalBlockClick(BoardPoint point) {
    PBYTE pTargetBlock = &ACCESS_BLOCK(point);

    // Click an empty block
    if (!BLOCK_IS_BOMB(*pTargetBlock)) {
        ExpandEmptyBlock(point);

        if (numberOfRevealedBlocks == numberOfEmptyBlocks) {
            FinishGame(TRUE);
        }
    }
    // Clicked a bomb and it's the first block 
    else if (numberOfRevealedBlocks == 0) {
        ReplaceFirstNonBomb(point, pTargetBlock);
    }
    // Clicked A Bomb
    else {
        ChangeBlockState(point, REVEALED_FLAG | BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
        FinishGame(FALSE);
    }
}

__inline void UpdateClickedPointsStateNormal(BoardPoint newPoint, BoardPoint oldPoint) {
    if (IsInBoardRange(oldPoint) && !(BLOCK_IS_REVEALED(ACCESS_BLOCK(oldPoint)))) {
        UpdateBlockStateToUnClicked(oldPoint);
        DrawBlock(oldPoint);
    }

    if (IsInBoardRange(newPoint)) {
        const BYTE block = ACCESS_BLOCK(newPoint);

        if (!BLOCK_IS_REVEALED(block) && !BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG)) {
            UpdateBlockStateToClicked(clickedPoint);
            DrawBlock(clickedPoint);
        }
    }
}

// 3x3 click handler. Assume that the point is in board range
void Handle3x3BlockClick(BoardPoint point) {
    BYTE block = ACCESS_BLOCK(point);

    if (BLOCK_IS_REVEALED(block) && GetFlagBlocksCount(point) == BLOCK_STATE(block)) {
        BOOL lostGame = FALSE;

        for (int loopRow = (point.row - 1); loopRow <= (point.row + 1); loopRow++) {
            for (int loopColumn = (point.column - 1); loopColumn <= (point.column + 1); loopColumn++) {
                BoardPoint point = { loopRow, loopColumn };
                BYTE block = ACCESS_BLOCK(point);

                // The user clicked a non flaged bomb
                if (!BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG) && BLOCK_IS_BOMB(block)) {
                    lostGame = TRUE;
                    ChangeBlockState(point, REVEALED_FLAG | BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
                }
                // The user clicked an empty block
                else if (!BLOCK_IS_BOMB(block)) {
                    ExpandEmptyBlock(point);
                }
                // The rest case is flag
            }
        }

        if (lostGame) {
            FinishGame(FALSE);
        }
        else if (numberOfEmptyBlocks == numberOfRevealedBlocks) {
            FinishGame(TRUE);
        }
    }
    else {
        ReleaseBlocksClick();
        return;
    }
}

// For the mouse move with 3x3 click, redraw old blocks and draw new blocks
__inline void UpdateClickedPointsState3x3(BoardPoint newPoint, BoardPoint oldPoint) {
    // Change old to unclicked
    if (IsInBoardRange(oldPoint)) {
        for (int r = oldPoint.row - 1; r <= oldPoint.row + 1; r++) {
            for (int c = oldPoint.column - 1; c <= oldPoint.column + 1; c++) {
                BoardPoint point = { r, c };
                BYTE block = ACCESS_BLOCK(point);
                if (IsInBoardRange(point) && !BLOCK_IS_REVEALED(block) && BLOCK_STATE(block) == BLOCK_STATE_READ_EMPTY) {
                    UpdateBlockStateToUnClicked(point);
                    DrawBlock(point);
                }
            }
        }
    }
    if (IsInBoardRange(newPoint)) {
        for (int r = newPoint.row - 1; r <= newPoint.row + 1; r++) {
            for (int c = newPoint.column - 1; c <= newPoint.column + 1; c++) {
                BoardPoint point = { r, c };
                BYTE block = ACCESS_BLOCK(point);
                if (IsInBoardRange(point) && !BLOCK_IS_REVEALED(block) && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                    UpdateBlockStateToClicked(point);
                    DrawBlock(point);
                }
            }
        }
    }
}

// Set the block state to clicked
__inline void UpdateBlockStateToClicked(BoardPoint point) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | BLOCK_STATE_READ_EMPTY;
}

// Reset the block state to unclicked
__inline void UpdateBlockStateToUnClicked(BoardPoint point) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | BLOCK_STATE_EMPTY_UNCLICKED;
}

//-----------------------------------------------------------

// Check if the block is in the board range
__inline BOOL IsInBoardRange(BoardPoint point) {
    return point.column > 0 && point.row > 0 && point.column <= width && point.row <= height;
}

// Handler when the first click is bomb
__inline void ReplaceFirstNonBomb(BoardPoint point, PBYTE pFunctionBlock) {
    // The first block! Change a normal block to a bomb, 
    // Replace the current block into an empty block
    // Reveal the current block
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            PBYTE pLoopBlock = &blockArray[r][c];

            // Find the first non-bomb
            if (!BLOCK_IS_BOMB(*pLoopBlock)) {
                // Replace bomb place
                *pFunctionBlock = BLOCK_STATE_EMPTY_UNCLICKED;
                *pLoopBlock |= BOMB_FLAG;

                ExpandEmptyBlock(point);
                return;
            }
        }
    }
}

// Count the number of flags near the block
int GetFlagBlocksCount(BoardPoint point) {
    int flagsCount = 0;

    // Search in the surrounding blocks
    for (int r = (point.row - 1); r <= (point.row + 1); ++r) {
        for (int c = (point.column - 1); c <= (point.column + 1); ++c) {
            if (BLOCK_STATE(blockArray[r][c]) == BLOCK_STATE_1P_FLAG) {
                flagsCount++;
            }
        }
    }

    return flagsCount;
}

// Count the number of bombs near the block
int CountNearBombs(BoardPoint point) {
    int bombCount = 0;

    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            if (BLOCK_IS_BOMB(blockArray[r][c])) {
                bombCount++;
            }
        }
    }

    return bombCount;
}

// BFS for empty blocks and show them
void ExpandEmptyBlock(BoardPoint point) {
    currentRowColumnListIndex = 1;
    ShowBlockValue(point);

    int i = 1;

    while (i != currentRowColumnListIndex) {
        int row = rowsList[i];
        int column = columnsList[i];

        for (int c = column - 1; c <= column + 1; c++) {
            for (int r = row - 1; r <= row + 1; r++) {
                if (r == row && c == column) {
                    continue;
                }
                BoardPoint point = { r, c };
                ShowBlockValue(point);
            }
        }
        i++;

        if (i == 10000) {
            i = 0;
        }
    }
}

// Open the block and show it
void ShowBlockValue(BoardPoint point) {
    BYTE blockValue = ACCESS_BLOCK(point);

    if (BLOCK_IS_REVEALED(blockValue)) {
        return;
    }

    BYTE state = blockValue & BLOCK_STATE_MASK;

    if (state == BLOCK_STATE_BORDER_VALUE || state == BLOCK_STATE_1P_FLAG) {
        return;
    }

    numberOfRevealedBlocks++;

    int nearBombsCount = CountNearBombs(point);
    ACCESS_BLOCK(point) = nearBombsCount | REVEALED_FLAG;
    DrawBlock(point);

    if (nearBombsCount == 0) {
        rowsList[currentRowColumnListIndex] = point.row;
        columnsList[currentRowColumnListIndex] = point.column;

        currentRowColumnListIndex++;

        if (currentRowColumnListIndex == 10000) {
            currentRowColumnListIndex = 0;
        }
    }
}

// Display all blocks when the game is over
void RevealAllBombs(BYTE revealedBombsState) {

    for (int r = 1; r <= height; ++r) {
        for (int c = 1; c <= width; ++c) {
            BYTE block = blockArray[r][c];
            PBYTE pBlock = &blockArray[r][c];
            BYTE blockState = BLOCK_STATE(block);
            BYTE blockInfo = BLOCK_INFO(block);

            if (BLOCK_IS_REVEALED(block)) {
                continue;
            }

            if (BLOCK_IS_BOMB(block)) {
                if (!BLOCK_IS_STATE(blockState, BLOCK_STATE_1P_FLAG)) {
                    blockArray[r][c] = blockInfo | revealedBombsState;
                }
            }
            else if (blockState == BLOCK_STATE_1P_FLAG) {
                // This is not a bomb, but flagged by the user
                blockArray[r][c] = blockInfo | BLOCK_STATE_1P_BOMB_WITH_X;
            }
        }
    }
    DisplayAllBlocks();
}

// Game finisher
void FinishGame(BOOL isWon) {
    isTimerOnAndShowed = FALSE;
    globalSmileId = (isWon) ? SMILE_WINNER : SMILE_LOST;
    DisplaySmile(globalSmileId);

    // If the player wins, bombs are changed into borderFlags
    // If the player loses, bombs change into black bombs
    RevealAllBombs((isWon) ? BLOCK_STATE_1P_FLAG : BLOCK_STATE_BLACK_BOMB);

    if (isWon && leftFlags != 0) {
        AddAndDisplayLeftFlags(-leftFlags);
    }

    stateFlags = STATE_GAME_FINISHED;

    // Check if it is the best time
    if (isWon && gameConfig.difficulty != DIFFICULTY_CUSTOM) {
        if (timerSeconds < gameConfig.times[gameConfig.difficulty]) {
            gameConfig.times[gameConfig.difficulty] = timerSeconds;
        }
    }
}

//-----------------------------------------------------------

// Handle for smile button left click
BOOL HandleLeftClick(DWORD dwLocation) {
    MSG msg;
    RECT rect;

    // Copy point into struct
    msg.pt.y = HIWORD(dwLocation);
    msg.pt.x = LOWORD(dwLocation);

    // The rect represents the game board
    rect.left = (xRight - 24) / 2;
    rect.right = rect.left + 24;
    rect.top = 16;
    rect.bottom = 40;

    // Check if the click is in the range of the board
    if (!PtInRect(&rect, msg.pt)) {
        return FALSE;
    }
    
    globalSmileId = SMILE_NORMAL;
    DisplaySmile(SMILE_NORMAL);
    InitializeNewGame();
    return TRUE;
}

void HandleRightClick(BoardPoint point) {
    if (IsInBoardRange(point)) {
        BYTE block = ACCESS_BLOCK(point);

        if (!BLOCK_IS_REVEALED(block)) {
            BYTE blockState = block & BLOCK_STATE_MASK;

            switch (blockState) {
            case BLOCK_STATE_1P_FLAG:
                blockState = BLOCK_STATE_EMPTY_UNCLICKED;
                AddAndDisplayLeftFlags(1);
                break;
            default: // Assume BLOCK_STATE_EMPTY_UNCLICKED
                blockState = BLOCK_STATE_1P_FLAG;
                AddAndDisplayLeftFlags(-1);
            }

            ChangeBlockState(point, blockState);

            if (BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG) &&
                numberOfRevealedBlocks == numberOfEmptyBlocks) {
                FinishGame(TRUE);
            }
        }
    }
}

void TickSeconds() {
    if (isTimerOnAndShowed && timerSeconds < 999) {
        timerSeconds++;
        DisplayTimerSeconds();
    }
}