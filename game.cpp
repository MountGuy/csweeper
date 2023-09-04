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

BOOL assistOn = FALSE;

BYTE blockArray[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
BYTE solveState[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];
int solvedNum = 0, openedNum = 0;
BoardPoint focusedPoints[2] = {{ -1, -1 }, { -1, -1 }};
BoardPoint cursorPoint = { -1, -1 };
const BoardPoint nullPoint = { -2, -2 };

BYTE blockStateFlags[2] = {BLOCK_STATE_1P_FLAG, BLOCK_STATE_2P_FLAG};
BYTE blockBombWithXs[2] = {BLOCK_STATE_1P_BOMB_WITH_X, BLOCK_STATE_2P_BOMB_WITH_X};
BYTE blockBombRedBackgrounds[2] = {BLOCK_STATE_1P_BOMB_RED_BACKGROUND, BLOCK_STATE_2P_BOMB_RED_BACKGROUND};
BYTE focusedFlags[2] = {FOCUSED_FLAG_1P, FOCUSED_FLAG_2P};

int globalSmileID;
int leftFlags;
int timerSeconds;
int width;
int height;
int numberOfEmptyBlocks;
int numberOfRevealedBlocks;

int currentRowColumnListIndex;
int rowsList[10000];
int columnsList[10000];

BoardPoint openedStack[10000];
BoardPoint openedStackSpare[10000];
int openedStackIndex = 0, openedStackSpareIndex = 0;

// New game
void InitializeNewGame() {
    int remainingMines = gameConfig.mines;
    DWORD borderFlags;

    if (gameConfig.width == width && gameConfig.height == height) {
        borderFlags = WINDOW_BORDER_REPAINT_WINDOW;
    }
    else {
        borderFlags = WINDOW_BORDER_MOVE_WINDOW | WINDOW_BORDER_REPAINT_WINDOW;
    }

    width = gameConfig.width;
    height = gameConfig.height;
    focusedPoints[ID_1P] = nullPoint;
    focusedPoints[ID_2P] = nullPoint;
    cursorPoint.row = height / 2 + 1;
    cursorPoint.column = width / 2 + 1;

    InitializeBlockArrayBorders();

    globalSmileID = 0;

    // Setup all the mines
    do {
        BoardPoint randomPoint;

        // Find a location for the mine
        do {
            randomPoint.column = GetRandom(width) + 1;
            randomPoint.row = GetRandom(height) + 1;
        } while (ACCESS_BLOCK(randomPoint) & BOMB_FLAG);

        // SET A MINE
        ACCESS_BLOCK(randomPoint) |= BOMB_FLAG;
        remainingMines--;
    } while (remainingMines);

    timerSeconds = 0;
    leftFlags = gameConfig.mines;
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

    for (int row = 0; row <= height + 1; row++) {
        for (int column = 0; column <= width + 1; column++) {
            blockArray[row][column] = BLOCK_STATE_EMPTY_UNCLICKED;
            solveState[row][column] = CLOSED;
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
    solvedNum = 0;
    openedNum = 0;
    openedStackIndex = 0;
    openedStackSpareIndex = 0;
}

// Called on button up and MouseMoveHandler with click
// The only function that makes hasInputCaptures = FALSE
void ReleaseInputCapture(int playerID) {
    hasInputCaptures[playerID] = FALSE;
    ReleaseCapture();

    // Mouse move always gets here
    if (GAME_IS_ON()) {
        HandleBlockInput(playerID);
        ReleaseBlocksInput(playerID);
    }
    else {
        HandleBlockInput(playerID);
    }
}

// Set clicked point to a nullPoint and call UpdateInputPointsState
// So the blocks will be redrawn
__inline void ReleaseBlocksInput(int playerID) {
    UpdateInputPointsState(nullPoint, playerID);
}

// Change the block state and draw it
void ChangeBlockState(BoardPoint point, BYTE blockState) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | blockState;
    DrawBlock(point);
}

// Handles every block click release
void HandleBlockInput(int playerID) {
    BoardPoint point = POINT_OF_PLAYER(playerID);
    if (IsInBoardRange(point)) {
        // First Click! Initialize Timer 
        if (numberOfRevealedBlocks == 0 && timerSeconds == 0) {
            timerSeconds++;
            DisplayTimerSeconds();
            isTimerOnAndShowed = TRUE;

            if (!SetTimer(hWnd, TIME_TIMER_ID, 1000, NULL)) {
                DisplayErrorMessage(ID_TIMER_ERROR);
            }
        }

        //Invalid: skip this input
        if (!GAME_IS_ON()) {
            POINT_OF_PLAYER(playerID) = nullPoint;
            return;
        }

        // Check if 3x3 click
        if (is3x3Clicks[playerID]) {
            Handle3x3BlockInput(playerID);
        }
        else {
            BYTE block = ACCESS_BLOCK(point);
            if (!BLOCK_IS_REVEALED(block) &&
                !BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG) &&
                !BLOCK_IS_STATE(block, BLOCK_STATE_2P_FLAG)) {
                HandleNormalBlockInput(playerID);
            }
        }
    }

    DisplaySmile(globalSmileID);
}

// Normal click handler. Assume that the point is in board range
// Called by click release
void HandleNormalBlockInput(int playerID) {
    // Click an empty block
    BoardPoint point = POINT_OF_PLAYER(playerID);
    if (!BLOCK_IS_BOMB(ACCESS_BLOCK(point))) {
        if (assistOn) Solve(point);
        else ExpandEmptyBlock(point);
        if (numberOfRevealedBlocks == numberOfEmptyBlocks) {
            FinishGame(TRUE, playerID);
        }
    }
    // Clicked a bomb and it's the first block 
    else if (numberOfRevealedBlocks == 0) {
        ReplaceFirstNonBomb(point);
    }
    // Clicked A Bomb
    else {
        BYTE bombRedBackground = BLOCK_STATE_BOMB_RED_BACKGROUND(playerID);
        ChangeBlockState(point, REVEALED_FLAG | bombRedBackground);
        FinishGame(FALSE, playerID);
    }
}

// 3x3 click handler. Assume that the point is in board range
// Called by click release
void Handle3x3BlockInput(int playerID) {
    BoardPoint point = POINT_OF_PLAYER(playerID);
    BYTE block = ACCESS_BLOCK(point);

    if (BLOCK_IS_REVEALED(block) && CountNearFlags(point) == BLOCK_STATE(block)) {
        BOOL lostGame = FALSE;

        for (int r = (point.row - 1); r <= (point.row + 1); r++) {
            for (int c = (point.column - 1); c <= (point.column + 1); c++) {
                BoardPoint point = { r, c };
                BYTE block = ACCESS_BLOCK(point);

                // The user clicked a non flaged bomb
                if (!BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG) &&
                    !BLOCK_IS_STATE(block, BLOCK_STATE_2P_FLAG) &&
                    BLOCK_IS_BOMB(block)) {
                    BYTE bombRedBackground = BLOCK_STATE_BOMB_RED_BACKGROUND(playerID);
                    lostGame = TRUE;
                    ChangeBlockState(point, REVEALED_FLAG | bombRedBackground);
                }
                // The user clicked an empty block
                else if (!BLOCK_IS_BOMB(block)) {
                    if (assistOn) Solve(point);
                    else ExpandEmptyBlock(point);
                }
                // The rest case is flag
            }
        }

        if (lostGame) {
            FinishGame(FALSE, playerID);
        }
        else if (numberOfEmptyBlocks == numberOfRevealedBlocks) {
            FinishGame(TRUE, playerID);
        }
    }
    else {
        ReleaseBlocksInput(playerID);
        return;
    }
}

// Handle for redraw when the mouse is moved
void UpdateInputPointsState(BoardPoint point, int playerID) {
    const BoardPoint oldPoint = POINT_OF_PLAYER(playerID);
    //if (point.column == oldPoint.column && point.row == oldPoint.row) {
    //    return;
    //}

    // Update new click point
    POINT_OF_PLAYER(playerID) = point;

    if (is3x3Clicks[playerID]) {
        UpdateInputPointsState3x3(point, oldPoint, playerID);
    }
    else {
        UpdateInputPointsStateNormal(point, oldPoint, playerID);
    }
}

// For the mouse move with 3x3 click, redraw old blocks and draw new blocks
// Called by UpdateInputPointsState
void UpdateInputPointsStateNormal(BoardPoint newPoint, BoardPoint oldPoint, int playerID) {
    BoardPoint focusedPoint = POINT_OF_PLAYER(playerID);
    
    // Change old to unclicked
    if (IsInBoardRange(oldPoint) && !(BLOCK_IS_REVEALED(ACCESS_BLOCK(oldPoint)))) {
        UpdateBlockStateToUnFocused(oldPoint, playerID);
        DrawBlock(oldPoint);
    }

    // Change new to clicked
    if (IsInBoardRange(newPoint)) {
        const BYTE block = ACCESS_BLOCK(newPoint);

        if (!BLOCK_IS_REVEALED(block) && !BLOCK_IS_STATE(block, BLOCK_STATE_1P_FLAG)) {
            UpdateBlockStateToFocused(focusedPoint, playerID);
            DrawBlock(focusedPoint);
        }
    }
}

// For the mouse move with 3x3 click, redraw old blocks and draw new blocks
// Called by UpdateInputPointsState
void UpdateInputPointsState3x3(BoardPoint newPoint, BoardPoint oldPoint, int playerID) {
    // Change old to unclicked
    if (IsInBoardRange(oldPoint)) {
        for (int r = oldPoint.row - 1; r <= oldPoint.row + 1; r++) {
            for (int c = oldPoint.column - 1; c <= oldPoint.column + 1; c++) {
                BoardPoint point = { r, c };
                BYTE block = ACCESS_BLOCK(point);
                if (IsInBoardRange(point) && !BLOCK_IS_REVEALED(block) && BLOCK_STATE(block) == BLOCK_STATE_READ_EMPTY) {
                    UpdateBlockStateToUnFocused(point, playerID);
                    DrawBlock(point);
                }
            }
        }
    }
    // Change new to clicked
    if (IsInBoardRange(newPoint)) {
        for (int r = newPoint.row - 1; r <= newPoint.row + 1; r++) {
            for (int c = newPoint.column - 1; c <= newPoint.column + 1; c++) {
                BoardPoint point = { r, c };
                BYTE block = ACCESS_BLOCK(point);
                if (IsInBoardRange(point) && !BLOCK_IS_REVEALED(block) && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                    UpdateBlockStateToFocused(point, playerID);
                    DrawBlock(point);
                }
            }
        }
    }
}

// Set the block state to clicked
__inline void UpdateBlockStateToFocused(BoardPoint point, int playerID) {
    ACCESS_BLOCK(point) = BLOCK_INFO(ACCESS_BLOCK(point)) | focusedFlags[playerID] | BLOCK_STATE_READ_EMPTY;
}

// Reset the block state to unclicked
__inline void UpdateBlockStateToUnFocused(BoardPoint point, int playerID) {
    BYTE newBlockInfo = BLOCK_INFO(ACCESS_BLOCK(point)) & ~focusedFlags[playerID];
    BYTE newBlockState = BLOCK_IS_FOCUSED(newBlockInfo)? BLOCK_STATE_READ_EMPTY : BLOCK_STATE_EMPTY_UNCLICKED;
    ACCESS_BLOCK(point) = newBlockInfo | newBlockState;
}

//-----------------------------------------------------------

// Check if the block is in the board range
__inline BOOL IsInBoardRange(BoardPoint point) {
    return point.column > 0 && point.row > 0 && point.column <= width && point.row <= height;
}

// Handler when the first click is bomb
__inline void ReplaceFirstNonBomb(BoardPoint point) {
    // The first block! Change a normal block to a bomb, 
    // Replace the current block into an empty block
    // Reveal the current block
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            // Find the first non-bomb
            if (!BLOCK_IS_BOMB(blockArray[r][c])) {
                // Replace bomb place
                ACCESS_BLOCK(point) = BLOCK_STATE_EMPTY_UNCLICKED;
                blockArray[r][c] |= BOMB_FLAG;

                if (assistOn) Solve(point);
                else ExpandEmptyBlock(point);
                return;
            }
        }
    }
}

// Count the number of flags near the block
int CountNearFlags(BoardPoint point) {
    int flagsCount = 0;

    // Search in the surrounding blocks
    for (int r = (point.row - 1); r <= (point.row + 1); ++r) {
        for (int c = (point.column - 1); c <= (point.column + 1); ++c) {
            BYTE blockState = BLOCK_STATE(blockArray[r][c]);
            if (blockState == BLOCK_STATE_1P_FLAG || blockState == BLOCK_STATE_2P_FLAG) {
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

// Count the number of revealed blocks near the block
int CountNearUnRevealed(BoardPoint point) {
    int revealedCount = 0;

    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint point = { r, c };
            if (IsInBoardRange(point) && !BLOCK_IS_REVEALED(blockArray[r][c])) {
                revealedCount++;
            }
        }
    }

    return revealedCount;
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
    int nearBombsCount = CountNearBombs(point);
    if (nearBombsCount == 0) {
        solvedNum++;
        SET_SOLVE_STATE(point, SOLVED);
    } else if (nearBombsCount > 0) {
        openedNum++;
        openedStackSpare[openedStackSpareIndex++] = point;
        SET_SOLVE_STATE(point, OPENED);
    }

    if (BLOCK_IS_REVEALED(blockValue)) {
        return;
    }

    BYTE state = blockValue & BLOCK_STATE_MASK;

    if (state == BLOCK_STATE_BORDER_VALUE || state == BLOCK_STATE_1P_FLAG || state == BLOCK_STATE_2P_FLAG) {
        return;
    }

    numberOfRevealedBlocks++;

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
void RevealAllBombs(BYTE revealedBombsState, int playerID) {

    for (int r = 1; r <= height; ++r) {
        for (int c = 1; c <= width; ++c) {
            BYTE block = blockArray[r][c];
            BYTE blockState = BLOCK_STATE(block);
            BYTE blockInfo = BLOCK_INFO(block);

            if (BLOCK_IS_REVEALED(block)) {
                continue;
            }

            if (BLOCK_IS_BOMB(block)) {
                if (!BLOCK_IS_STATE(blockState, BLOCK_STATE_1P_FLAG) &&
                    !BLOCK_IS_STATE(blockState, BLOCK_STATE_2P_FLAG)) {
                    blockArray[r][c] = blockInfo | revealedBombsState;
                }
            }
            else if (blockState == BLOCK_STATE_1P_FLAG) {
                // This is not a bomb, but flagged by the user
                blockArray[r][c] = blockInfo | BLOCK_STATE_1P_BOMB_WITH_X;
            }
            else if (blockState == BLOCK_STATE_2P_FLAG) {
                // This is not a bomb, but flagged by the user
                blockArray[r][c] = blockInfo | BLOCK_STATE_2P_BOMB_WITH_X;
            }
        }
    }
    DisplayAllBlocks();
}

// Game finisher
void FinishGame(BOOL isWon, int playerID) {
    isTimerOnAndShowed = FALSE;
    globalSmileID = (isWon) ? SMILE_WINNER : SMILE_LOST;
    DisplaySmile(globalSmileID);

    // If the player wins, bombs are changed into borderFlags
    // If the player loses, bombs change into black bombs
    RevealAllBombs((isWon) ? blockStateFlags[playerID] : BLOCK_STATE_BLACK_BOMB, playerID);

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
// Return TRUE if the click is in the range of the smile button
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
    
    globalSmileID = SMILE_NORMAL;
    DisplaySmile(SMILE_NORMAL);
    InitializeNewGame();
    return TRUE;
}

// Handle for right click. The game reacts to the right click immediately
void HandleRightInput(BoardPoint point, int playerID) {
    if (IsInBoardRange(point)) {
        BYTE block = ACCESS_BLOCK(point);

        if (!BLOCK_IS_REVEALED(block)) {
            BYTE blockState = BLOCK_STATE(block);

            switch (blockState) {
            case BLOCK_STATE_1P_FLAG:
            case BLOCK_STATE_2P_FLAG:
                blockState = BLOCK_STATE_EMPTY_UNCLICKED;
                AddAndDisplayLeftFlags(1);
                break;
            default: // Assume BLOCK_STATE_EMPTY_UNCLICKED
                blockState = blockStateFlags[playerID];
                AddAndDisplayLeftFlags(-1);
            }

            ChangeBlockState(point, blockState);
            
            if (BLOCK_IS_STATE(block, blockStateFlags[playerID]) &&
                numberOfRevealedBlocks == numberOfEmptyBlocks) {
                FinishGame(TRUE, playerID);
            }
        }
    }
}

void TickSeconds() {
    if (isTimerOnAndShowed) {
        timerSeconds++;
        DisplayTimerSeconds();
    }
}

//-----------------------------------------------------------

int Heuristic1(BoardPoint point) {
    int result = 0;

    SET_SOLVE_STATE(point, SOLVED);
    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint point = { r, c };
            BYTE block = ACCESS_BLOCK(point);
            if (IsInBoardRange(point) && solveState[r][c] != SOLVED && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                ChangeBlockState(point, blockStateFlags[ID_1P]);
                AddAndDisplayLeftFlags(-1);
                result++;
            }
        }
    }

    return result;
}

int Heuristic2(BoardPoint point) {
    int result = 0;

    SET_SOLVE_STATE(point, SOLVED);
    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint point = { r, c };
            BYTE block = ACCESS_BLOCK(point);
            if (IsInBoardRange(point) && solveState[r][c] != SOLVED && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                ExpandEmptyBlock(point);
                result++;
            }
        }
    }

    return result;
}

int Heuristic3(BoardPoint point) {
    int result = 0;

    return result;
}

int Heuristic(BoardPoint point) {
    BYTE block = ACCESS_BLOCK(point);
    int numBombs = CountNearBombs(point);
    int numFlags = CountNearFlags(point);
    int numUnRevealed = CountNearUnRevealed(point);
    int result = 0;

    if (solveState[point.row][point.column] == SOLVED) {
        return 0;
    }
    if (solveState[point.row][point.column] == CLOSED) {
        openedStackSpare[openedStackSpareIndex++] = point;
        return 0;
    }

    if (numUnRevealed == numBombs) result += Heuristic1(point);
    else if (numBombs == numFlags) result += Heuristic2(point);
    else {
        openedStackSpare[openedStackSpareIndex++] = point;
    }

    return result;
}

// Solve the board
BOOL Solve(BoardPoint entryPoint) {
    char str[100];
    static int calls = 0;

    openedStackIndex = 0;
    openedStackSpareIndex = 0;
    ExpandEmptyBlock(entryPoint);

    while (1) {
        int newOpened = 0;
        for (int idx = 0; idx < openedStackSpareIndex; idx++) {
            openedStack[idx] = openedStackSpare[idx];
        }
        openedStackIndex = openedStackSpareIndex;
        openedStackSpareIndex = 0;
        for (int idx = 0; idx < openedStackIndex; idx++) {
            newOpened += Heuristic(openedStack[idx]);
            //Sleep(1000);
        }
        if (openedStackSpareIndex == 0 || newOpened == 0) {
            break;
        }
    }


    return FALSE;
}