#ifndef BOARD_EVALUATE_H
#define BOARD_EVALUATE_H

#include "const.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The structure for a Boggle board is defined in this section.  This structure needs to be initialized only once.  The alphabetical order of a "Square"s neighbours is of no consequence.

// The "square" struct will represent one position in a Boggle board.
// A square also requires a flag to indicate use in the current word being formed, the number of valid neighbours, and the index of the letter showing on its face.
struct square {
	// The Used flag will indicate if a square is being used in constructing the current word, and hence to remove the used square from further inclusion in the same word.
	Bool Used;
	unsigned int NumberOfLivingNeighbours;
	struct square *LivingNeighbourSquarePointerArray[NEIGHBOURS];
	unsigned char LetterIndex;
};

// Define the "Square" type.
typedef struct square Square;
typedef Square* SquarePtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A board is defined as simply a static 2 dimensional "Block" of Squares.
struct board {
	Square Block[MAX_ROW][MAX_COL];
};

typedef struct board Board;
typedef Board* BoardPtr;

// An explicit stack will replace recursion and there are seven variables required to save a state while recursively discovering words on a Boggle board.
struct discoverystacknode {
	SquarePtr TheSquareNow;
	unsigned int LexiconPartOneIndex;
	unsigned int FirstChildIndex;
	unsigned int WordLengthNow;
	unsigned int NextMarkerNow;
	unsigned int TheChildToWorkOn;
	unsigned long int TheSecondPartNow;
};

typedef struct discoverystacknode DiscoveryStackNode;
typedef DiscoveryStackNode* DiscoveryStackNodePtr;

// Each worker thread requires its own stack, and the memory will be allocated inside of the main function before work starts.
DiscoveryStackNodePtr TheDiscoveryStacks[NUMBER_OF_WORKER_THREADS];

inline char SquareLetterIndex(SquarePtr ThisSquare){
	return ThisSquare->LetterIndex;
}

void SquareInit(SquarePtr ThisSquare, unsigned int RowPosition, unsigned int ColPosition);
void BoardInit(BoardPtr ThisBoard);
void BoardPopulate(BoardPtr ThisBoard, char *BoardString);
void BoardOutput(BoardPtr ThisBoard);
int SquareWordDiscoverStack(SquarePtr BeginSquare, unsigned int BeginIndex, unsigned int BeginMarker, unsigned int NowTime, unsigned int ThreadIdentity);
unsigned int BoardSquareWordDiscover(BoardPtr ThisBoard, unsigned int TheTimeNow, unsigned int CallingThread);

#endif // BOARD_EVALUTE_H
