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

extern BoardPoint openedStack[10000];
extern BoardPoint openedStackSpare[10000];
extern int openedStackIndex;

int CompPoint(BoardPoint point1, BoardPoint point2);
ConstraintPtr AddConstraintFromRevealed(BoardPoint revPoint);
ConstraintPtr GetSubConstraint(ConstraintPtr constPtrOut, ConstraintPtr constPtrIn);
int SolveTrivialConst(ConstraintPtr constPtr);

int Heuristic1(BoardPoint point);
int Heuristic2(BoardPoint point);
int Heuristic3(BoardPoint point);
int Heuristic(BoardPoint point);
BOOL Solve(BoardPoint entryPoint);
