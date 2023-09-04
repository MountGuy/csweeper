#include <stdio.h>
#include "solver.h"
#include "game.h"
#include "drawing.h"

BoardPoint openedStack[10000];
BoardPoint openedStackSpare[10000];
int openedStackIndex;

ConstraintPtr constPtrs[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH][100];
int constPtrNum[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];

void PrintConst(ConstraintPtr constPtr) {
    char str[1000];
    const char *typeString[5] = { "equal to\0", "greater than\0", "less than\0" };
    switch (constPtr->constType) {
    case EQ:
        sprintf_s(str, sizeof(str), "Constraint: equal to %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    case GT:
        sprintf_s(str, sizeof(str), "Constraint: greater than %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    case LT:
        sprintf_s(str, sizeof(str), "Constraint: less than %d mines %d points: ", constPtr->mineNum, constPtr->pointNum);
        break;
    }
    OutputDebugStringA(str);
    for (int i = 0; i < constPtr->pointNum; i++) {
        sprintf_s(str, sizeof(str), "(%d, %d)", constPtr->points[i].row, constPtr->points[i].column);
        OutputDebugStringA(str);
    }
    sprintf_s(str, sizeof(str), "\n");
    OutputDebugStringA(str);
}

int CompPoint(BoardPoint point1, BoardPoint point2) {
    if (point1.row != point2.row) {
        return point1.row - point2.row;
    }
    else {
        if (point1.column != point2.column) {
            return point1.column - point2.column;
        }
        else {
            return 0;
        }
    }
}

ConstraintPtr AddConstraintFromRevealed(BoardPoint revPoint) {
    ConstraintPtr newConst = (ConstraintPtr) malloc(sizeof(Constraint));
    BoardPoint point;
    int idxNew = 0, ptrN = 0, numFlags = 0;

    for (int r = (revPoint.row - 1); r <= (revPoint.row + 1); r++) {
        for (int c = (revPoint.column - 1); c <= (revPoint.column + 1); c++) {
            BoardPoint candPoint = { r, c };
            BYTE block = ACCESS_BLOCK(candPoint);            

            if (CompPoint(candPoint, revPoint) && IsInBoardRange(candPoint)) {
                if (BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED && solveState[r][c] != SOLVED) newConst->points[idxNew++] = candPoint;
                else if (BLOCK_STATE(block) == blockStateFlags[ID_1P]) numFlags++;
            }
        }
    }
    point = newConst->points[0];
    ptrN = constPtrNum[point.row][point.column]++;
    constPtrs[point.row][point.column][ptrN] = newConst;

    newConst->constType = EQ;
    newConst->mineNum = CountNearBombs(revPoint) - numFlags;
    newConst->pointNum = idxNew;

    if (newConst->mineNum == 0) {
        free(newConst);
        constPtrNum[point.row][point.column]--;

        return NULL;
    }

    return newConst;
}

ConstraintPtr GetSubConstraint(ConstraintPtr constPtrOut, ConstraintPtr constPtrIn) {
    ConstraintPtr newConst = (ConstraintPtr) malloc(sizeof(Constraint));
    BoardPoint pointOut, pointIn, point;
    int idxOut = 0, idxIn = 0, idxNew = 0, comp = 0, ptrN = 0, remainNum = 0;
    int typeOut = constPtrOut->constType, typeIn = constPtrIn->constType;
    BOOL deleteOut = (typeOut == EQ && typeIn == EQ);

    while (idxOut < constPtrOut->pointNum && idxIn < constPtrIn->pointNum) {
        pointOut = constPtrOut->points[idxOut];
        pointIn = constPtrIn->points[idxIn];
        comp = CompPoint(pointOut, pointIn);

        if (comp == 0) {
            idxOut++;
            idxIn++;
        }
        else if (comp < 0) {
            newConst->points[idxNew] = pointOut;
            idxOut++;
            idxNew++;
        }
        else {
            idxIn++;
            remainNum++;
        }
    }
    
    for (; idxOut < constPtrOut->pointNum; idxOut++) {
        newConst->points[idxNew] = constPtrOut->points[idxOut];
        idxNew++;
    }
    for (; idxIn < constPtrIn->pointNum; idxIn++) {
        remainNum++;
    }

    if (typeOut == EQ && typeIn == EQ) {
        if (remainNum) newConst->constType = GT;
        else newConst->constType = EQ;
    }
    else if (typeOut == EQ) newConst->constType = INV_EQ(typeIn);
    else if (typeIn == EQ) newConst->constType = typeOut;
    else if (typeOut != typeIn) {
        // GT & LT or LT & GT
        newConst->constType = typeOut;
    }
    else {
        // GT & GT or LT & LT
        free(newConst);
        return NULL;
    }

    switch (newConst->constType)
    {
    case EQ:
        newConst->mineNum = constPtrOut->mineNum - constPtrIn->mineNum;
        break;
    case GT:
        newConst->mineNum = max(0, constPtrOut->mineNum - max(0, min(constPtrIn->mineNum, constPtrIn->pointNum - remainNum)));
        break;
    case LT:
        newConst->mineNum = constPtrOut->mineNum - max(constPtrIn->mineNum, constPtrIn->pointNum - remainNum);
        break;
    }
    newConst->pointNum = idxNew;

    return newConst;
}

void ResetConsts() {
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            for (int n = 0; n < constPtrNum[r][c]; n++) {
                free(constPtrs[r][c][n]);
            }
            constPtrNum[r][c] = 0;
            solveState[r][c] = (solveState[r][c] == CONSTRAINED ? OPENED : solveState[r][c]);
        }
    }
}

int ComposeConst() {
    int result = 0;
    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            for (int n = 0; n < constPtrNum[r][c]; n++) {
                ConstraintPtr constPtrOut = constPtrs[r][c][n];
                BoardPoint pointOut = constPtrOut->points[0];
                for (int rr = pointOut.row - 2; rr <= pointOut.row + 2; rr++) {
                    for (int cc = pointOut.column - 2; cc <= pointOut.column + 2; cc++) {
                        BoardPoint pointIn = { rr, cc };
                        if (!IsInBoardRange(pointIn)) continue;
                        for (int nn = 0; nn < constPtrNum[rr][cc]; nn++) {
                            ConstraintPtr constPtrIn = constPtrs[rr][cc][nn];
                            ConstraintPtr constPtrNew = GetSubConstraint(constPtrOut, constPtrIn);
                            result += SolveTrivialConst(constPtrNew);
                            free(constPtrNew);
                        }
                    }
                }
            }
        }
    }
    return result;
}

int SolveTrivialConst(ConstraintPtr constPtr) {
    int idxNew = 0, ptrN = 0, result = 0;


    if (constPtr->mineNum == 0 && constPtr->constType != GT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (solveState[point.row][point.column] != SOLVED) {
                ExpandEmptyBlock(point);
                result++;
            }
        }
    }
    else if (constPtr->mineNum == constPtr->pointNum && constPtr->constType != LT) {
        for (int idx = 0; idx < constPtr->pointNum; idx++) {
            BoardPoint point = constPtr->points[idx];
            if (solveState[point.row][point.column] != SOLVED) {
                ChangeBlockState(point, blockStateFlags[ID_1P]);
                AddAndDisplayLeftFlags(-1);
                result++;
            }
        }
    }
    return result;
}

int Heuristic1(BoardPoint point) {
    int result = 0;

    SET_SOLVE_STATE(point, SOLVED);
    for (int r = (point.row - 1); r <= (point.row + 1); r++) {
        for (int c = (point.column - 1); c <= (point.column + 1); c++) {
            BoardPoint tPoint = { r, c };
            BYTE block = ACCESS_BLOCK(tPoint);
            if (IsInBoardRange(tPoint) && solveState[r][c] != SOLVED && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                if (!BLOCK_IS_BOMB(block)) {
                    ChangeBlockState(tPoint, BLOCK_STATE_1P_BOMB_RED_BACKGROUND);
                    DrawBlock(tPoint);
                    int a = 1;
                }
                ChangeBlockState(tPoint, blockStateFlags[ID_1P]);
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
            BoardPoint tPoint = { r, c };
            BYTE block = ACCESS_BLOCK(tPoint);
            if (IsInBoardRange(tPoint) && solveState[r][c] != SOLVED && BLOCK_STATE(block) == BLOCK_STATE_EMPTY_UNCLICKED) {
                ExpandEmptyBlock(tPoint);
                result++;
            }
        }
    }

    return result;
}

int Heuristic3(BoardPoint point) {
    solveState[point.row][point.column] = CONSTRAINED;
    AddConstraintFromRevealed(point);
    openedStack[openedStackIndex++] = point;

    return 0;
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
        openedStackSpare[openedStackIndex++] = point;
        return 0;
    }
    if (numUnRevealed == numBombs) result += Heuristic1(point);
    else if (numBombs == numFlags) result += Heuristic2(point);
    else if (solveState[point.row][point.column] != CONSTRAINED) result += Heuristic3(point);

    return result;
}

// Solve the board
BOOL Solve(BoardPoint entryPoint) {
    char str[100];
    int stackSizeCopy = 0, newOpened;

    for (int r = 1; r <= height; r++) {
        for (int c = 1; c <= width; c++) {
            constPtrNum[r][c] = 0;
        }
    }

    openedStackIndex = 0;
    ExpandEmptyBlock(entryPoint);

    while (1) {
        newOpened = 0;
        ResetConsts();

        for (int idx = 0; idx < openedStackIndex; idx++) {
            openedStackSpare[idx] = openedStack[idx];
        }
        stackSizeCopy = openedStackIndex;
        openedStackIndex = 0;

        for (int idx = 0; idx < stackSizeCopy; idx++) {
            newOpened += Heuristic(openedStackSpare[idx]);
        }

        if (newOpened == 0) {
            newOpened += ComposeConst();
        }
        if (newOpened == 0) {
            break;
        }
    }

    
    return FALSE;
}