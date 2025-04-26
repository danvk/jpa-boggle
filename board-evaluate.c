#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "board-evaluate.h"
#include "adtdawg.h"
#include "const.h"

// This function has been converted into a macro, because it is used in the often called "BoardPopulate" routine, and it needs to be fast.
#define SQUARE_CHANGE_LETTER_INDEX(thissquare, newindex) ((thissquare)->LetterIndex = (newindex))

// This Function initializes ThisSquare when passed its row and column position on the board.
// Important note:  The function is going to use the low level C concept of pointer arithmatic to fill the LivingNeighbourSquarePointerArray, which will be filled from the top-left, clockwise.
void SquareInit(SquarePtr ThisSquare, unsigned int RowPosition, unsigned int ColPosition){
	ThisSquare->LetterIndex = SIZE_OF_CHARACTER_SET;
	ThisSquare->Used = FALSE;
	if ( RowPosition == 0 ) {
		// ThisSquare is in the top-left position.
		if ( ColPosition == 0 ) {
			ThisSquare->NumberOfLivingNeighbours = 3;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare + MAX_COL + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare + MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
		// ThisSquare is in the top-right position.
		else if ( ColPosition == (MAX_COL - 1) ) {
			ThisSquare->NumberOfLivingNeighbours = 3;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare + MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare + MAX_COL - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
		// ThisSquare is in a top-middle position.
		else {
			ThisSquare->NumberOfLivingNeighbours = 5;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare + MAX_COL + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare + MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = ThisSquare + MAX_COL - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = ThisSquare - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
	}
	else if ( RowPosition == (MAX_ROW - 1) ) {
		// ThisSquare is in the bottom-left position.
		if ( ColPosition == 0 ) {
			ThisSquare->NumberOfLivingNeighbours = 3;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
		// ThisSquare is in the bottom-right position.
		else if ( ColPosition == (MAX_COL - 1) ) {
			ThisSquare->NumberOfLivingNeighbours = 3;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL -1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
		// ThisSquare is in a bottom-middle position.
		else {
			ThisSquare->NumberOfLivingNeighbours = 5;
			(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL -1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare - MAX_COL + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[3] = ThisSquare + 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[4] = ThisSquare - 1;
			(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
			(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
		}
	}
	// ThisSquare is in a middle-left position.
	else if ( ColPosition == 0 ) {
		ThisSquare->NumberOfLivingNeighbours = 5;
		(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[3] = ThisSquare + MAX_COL + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[4] = ThisSquare + MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
	}
	// ThisSquare is in a middle-right position.
	else if ( ColPosition == (MAX_COL - 1) ) {
		ThisSquare->NumberOfLivingNeighbours = 5;
		(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL -1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare + MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[3] = ThisSquare + MAX_COL - 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[4] = ThisSquare - 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[5] = NULL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[6] = NULL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[7] = NULL;
	}
	// ThisSquare is in a middle-middle position.
	else {
		ThisSquare->NumberOfLivingNeighbours = NEIGHBOURS;
		(ThisSquare->LivingNeighbourSquarePointerArray)[0] = ThisSquare - MAX_COL -1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[1] = ThisSquare - MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[2] = ThisSquare - MAX_COL + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[3] = ThisSquare + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[4] = ThisSquare + MAX_COL + 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[5] = ThisSquare + MAX_COL;
		(ThisSquare->LivingNeighbourSquarePointerArray)[6] = ThisSquare + MAX_COL - 1;
		(ThisSquare->LivingNeighbourSquarePointerArray)[7] = ThisSquare - 1;
	}
}


// This initialization function sets the neighbour array for all of the Squares in ThisBoard's Block (this only needs to be done once).
// The Letter index for each square will be a blank space ' ', and they will all be not Used.
void BoardInit(BoardPtr ThisBoard){
	unsigned int Row, Col;
	for ( Row = 0; Row < MAX_ROW; Row++ ) {
		for ( Col = 0; Col < MAX_COL; Col++ ) {
			SquareInit(&(ThisBoard->Block[Row][Col]), Row, Col);
		}
	}
}

// This function simply transcribes the BoardString data into ThisBoard using the correct format.
// A major optimization has taken place at this level because the ADTDAWG's direct property enforces the "Order Does Not Matter," paradigm, and thus, no sorting of neighbours is required.
void BoardPopulate(BoardPtr ThisBoard, char *BoardString){
	unsigned int Row, Col;
	for ( Row = 0; Row < MAX_ROW; Row++ ) {
		for ( Col = 0; Col < MAX_COL; Col++ ) {
			SQUARE_CHANGE_LETTER_INDEX(&((ThisBoard->Block)[Row][Col]), CHARACTER_LOCATIONS[BoardString[Row * MAX_COL + Col] - 'A']);
		}
	}
}

// This function displays the important boards, like the "MASTER_SEED_BOARD".
void BoardOutput(BoardPtr ThisBoard){
	unsigned int Row, Col;
	char *PrintLine = (char*)malloc( (2*(MAX_COL)*sizeof( char ) + 2) );
	printf( "-----------\n" );
	for ( Row = 0; Row < MAX_ROW; Row++ ) {
		for ( Col = 0; Col < MAX_COL; Col++ ) {
			PrintLine[2*Col] = '|';
			PrintLine[2*Col + 1] = CHARACTER_SET[SquareLetterIndex(&((ThisBoard->Block)[Row][Col]))];
		}
		PrintLine[2*MAX_COL] = '|';
		PrintLine[2*MAX_COL + 1] = '\0';
		printf( "%s\n", PrintLine );
		printf( "-----------\n" );
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The sequential board scoring functions are found here for the new ADTDAWG.
// The ADTDAWG board analysis scheme is extremely powerful when applied to parallel batch processing.


// This is where the recursion-replacement stack is implemented.
// Using macros allows the programmer to change the value of an argument directly.
// The stack needs room for the NULL position 0, (MAX_STRING_LENGTH - 1) square positions, and a buffer for the top pointer.
#define DISCOVERY_STACK_PUSH(top, square, indie, fcindie, len, next, workon, second) (((top->TheSquareNow = (square)), (top->LexiconPartOneIndex = (indie)), (top->FirstChildIndex = (fcindie)), (top->WordLengthNow = (len)), (top->NextMarkerNow = (next)), (top->TheChildToWorkOn = (workon)), (top->TheSecondPartNow = (second)), ++top))
#define	DISCOVERY_STACK_POP(square, indie, fcindie, len, next, workon, second, top) ((--top, (square = top->TheSquareNow), (indie = top->LexiconPartOneIndex), (fcindie = top->FirstChildIndex), (len = top->WordLengthNow), (next = top->NextMarkerNow), (workon = top->TheChildToWorkOn), (second = top->TheSecondPartNow)))
#define	DISCOVERY_STACK_NOT_EMPTY(ThreadID, TipTop) (TheDiscoveryStacks[ThreadID] < TipTop)

// This is the central piece of code in the "BIGS.c" Boggle board analysis scheme.
// An explicit stack is used to traverse the neighbours of "BeginSquare," regardless of alphabetical order.  It updates the global "LexiconTimeStamps" to eliminate the duplicate word problem.
// The algorithm is inherently recursive, and this drawback has been excised.
// Every letter on the board must be contained in the lexicon character set, and this is easy to enforce because the user is not involved.

int SquareWordDiscoverStack(SquarePtr BeginSquare, unsigned int BeginIndex, unsigned int BeginMarker, unsigned int NowTime, unsigned int ThreadIdentity){
	Bool FirstTime = TRUE;
	Bool DoWeNeedToPop;
	unsigned int X;
	unsigned int WorkOnThisChild;
	SquarePtr WorkingSquare = BeginSquare;
	unsigned int WorkingMarker = BeginMarker;
	unsigned int WorkingIndex = BeginIndex;
	unsigned int WorkingNumberOfChars = 1;
	unsigned long int WorkingSecondPart;
	unsigned int TheChosenLetterIndex;
	unsigned int WorkingChildIndex;
	unsigned int Result = 0;
	SquarePtr *WorkingNeighbourList;
	unsigned long int WorkingOffset;
	int WorkingNextMarker;
	DiscoveryStackNodePtr TheTop = TheDiscoveryStacks[ThreadIdentity] + 1;
	//printf("In\n");
	while ( DISCOVERY_STACK_NOT_EMPTY(ThreadIdentity, TheTop) ) {
		//printf("1\n");
		// The first time that we land on a square, set it to used, and check if it represents a word, and then a new word.
		if ( FirstTime == TRUE ) {
			WorkingChildIndex = (PartOneArray[WorkingIndex] & CHILD_MASK);
			WorkingNextMarker = 0;
			// Tag "WorkingSquare" as being used.
			WorkingSquare->Used = TRUE;
			// Check to see if we have arrived at a new word, and if so, add the correct score to the result.
			if ( PartOneArray[WorkingIndex] & END_OF_WORD_FLAG ) {
				//printf("Bingo Word At |%u|\n", WorkingMarker);
				if ( (LexiconTimeStamps[ThreadIdentity])[WorkingMarker] < NowTime ) {
					Result += THE_SCORE_CARD[WorkingNumberOfChars];
					(LexiconTimeStamps[ThreadIdentity])[WorkingMarker] = NowTime;
				}
				// No matter what, we need to reduce the "WorkingNextMarker"
				WorkingNextMarker -= 1;
			}
		}
		//printf("2\n");
		// If "WorkingSquare" has children, visit the next one up to "NumberOfLivingNeighbours" - 1.  There will be no scrolling through a list of children in the lexicon data structure when using the ADTDAWG.
		DoWeNeedToPop = TRUE;
		if ( WorkingChildIndex ) {
			//printf("3\n");
			WorkingNeighbourList = WorkingSquare->LivingNeighbourSquarePointerArray;
			if ( FirstTime == TRUE ) {
				WorkingNextMarker += (WorkingMarker - PartThreeArray[WorkingChildIndex]);
				WorkingSecondPart = PartTwoArray[(PartOneArray[WorkingIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT];
				WorkOnThisChild = WorkingSquare->NumberOfLivingNeighbours;
			}
			for ( X = WorkOnThisChild; X-- > 0; ) {
				//printf("4|%u|\n", X);
				if ( (WorkingNeighbourList[X])->Used == FALSE ) {
					TheChosenLetterIndex = (WorkingNeighbourList[X])->LetterIndex;
					if ( WorkingOffset = ( WorkingSecondPart & CHILD_LETTER_BIT_MASKS[TheChosenLetterIndex]) ) {
						//printf("5|%c| - |%u|\n", CHARACTER_SET[TheChosenLetterIndex], WorkingMarker);
						WorkingOffset >>= CHILD_LETTER_BIT_SHIFTS[TheChosenLetterIndex];
						WorkingOffset -= 1;
						//printf("Pre-WorkingNextMarker |%u| at |%u|\n", WorkingNextMarker, WorkingNumberOfChars);
						// Now that we are ready to move down to the next level, push the current state onto the stack if we are not on the final neighbour, and update the working variables.
						DISCOVERY_STACK_PUSH(TheTop, WorkingSquare, WorkingIndex, WorkingChildIndex, WorkingNumberOfChars, WorkingNextMarker, X, WorkingSecondPart);
						WorkingSquare = WorkingNeighbourList[X];
						WorkingIndex = WorkingChildIndex + (unsigned int)WorkingOffset;
						WorkingMarker = WorkingNextMarker + PartThreeArray[WorkingChildIndex + (unsigned int)WorkingOffset];
						WorkingNumberOfChars += 1;
						FirstTime = TRUE;
						DoWeNeedToPop = FALSE;
						break;
					}
				}
			}
		}
		if ( DoWeNeedToPop ) {
			// We have now finished using "WorkingSquare", so set its Used element to FALSE.
			WorkingSquare->Used = FALSE;
			// Pop the top of the stack into the function and pick up where we left off at that particular square.
			DISCOVERY_STACK_POP(WorkingSquare, WorkingIndex, WorkingChildIndex, WorkingNumberOfChars, WorkingNextMarker, WorkOnThisChild, WorkingSecondPart, TheTop);
			//printf("WorkingNextMarker |%u|\n", WorkingNextMarker);
			FirstTime = FALSE;
		}
	}
	return Result;
}

// This function returns the Boggle score for "ThisBoard".  It will stamp the "LexiconTimeStamps[CallingThread]" with "TheTimeNow".
unsigned int BoardSquareWordDiscover(BoardPtr ThisBoard, unsigned int TheTimeNow, unsigned int CallingThread){
	unsigned int TheScoreTotal = 0;
	unsigned int CurrentRow;
	unsigned int CurrentCol;
	unsigned int CurrentPartOneIndex;
	for ( CurrentRow = 0; CurrentRow < MAX_ROW; CurrentRow++ ) {
		for ( CurrentCol = 0; CurrentCol < MAX_COL; CurrentCol++ ) {
			CurrentPartOneIndex = (((ThisBoard->Block)[CurrentRow][CurrentCol]).LetterIndex + 1);
			TheScoreTotal += SquareWordDiscoverStack(&((ThisBoard->Block)[CurrentRow][CurrentCol]), CurrentPartOneIndex, PartThreeArray[CurrentPartOneIndex], TheTimeNow, CallingThread);
		}
	}
	return TheScoreTotal;
}
