#include "game.h"

#define CLOSED 0
#define OPENED 1
#define SOLVED 2
#define CONSTRAINED 3

#define EQ 0
#define GT 1
#define LT 2
#define INV_EQ(eq) ((eq) == EQ ? EQ : ((eq) == GT ? LT : GT))

typedef struct {
    int mineNum;
    int pointNum;
    BYTE constType;
    BoardPoint points[9];
} Constraint;

typedef Constraint* ConstraintPtr;

extern BoardPoint openedList[10000];
extern BoardPoint openedListCopy[10000];
extern int openedListIndex;
extern BYTE solverState[BOARD_MAX_HEIGHT][BOARD_MAX_WIDTH];

#define SOLVER_STATE(point) (solverState[point.row][point.column])

int CompPoint(BoardPoint point1, BoardPoint point2);
void GetEQConstraint(BoardPoint point, ConstraintPtr newConst);
int GetSubConstraint(ConstraintPtr constPtrOut, ConstraintPtr constPtrIn, ConstraintPtr newConst1, ConstraintPtr newConst2);
int SolveTrivialConst(ConstraintPtr constPtr);
int SearchConst(BoardPoint point);
BOOL Solve(BoardPoint entryPoint);
