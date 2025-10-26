#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h> // Ensure this is included for clock()

// The ADTDAWG for Lexicon_14, a subset of TWL06, is located in the 4 data files listed below.
#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"

// General "Boggle" Constants.
#define MAX_ROW 5
#define MAX_COL 5
#define SQUARE_COUNT 25
#define BOARD_STRING_SIZE 31
#define NEIGHBOURS 8
#define NUMBER_OF_ENGLISH_LETTERS 26
#define SIZE_OF_CHARACTER_SET 14
#define MAX_STRING_LENGTH 15
#define BOGUS -99
#define NUMBER_OF_DOUBLE_DEVIATIONS (SQUARE_COUNT*(SQUARE_COUNT - 1)*(SIZE_OF_CHARACTER_SET - 1)*(SIZE_OF_CHARACTER_SET - 1)/2)


// Constants that are lexicon specific.
#define TOTAL_WORDS_IN_LEXICON 44220
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15

// Constants that define the high level algorithm.
#define NUMBER_OF_WORKER_THREADS	1

unsigned int THE_SCORE_CARD[MAX_STRING_LENGTH + 1] = { 0, 0, 0, 1, 1, 2, 3, 5, 11, 11, 11, 11, 11, 11, 11, 11 };

// These constant array's define the lexicon contained in the ADTDAWG.
unsigned int CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '};
unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS] = {0, BOGUS, 1, 2, 3, BOGUS, 4, BOGUS, 5, BOGUS, BOGUS, 6, 7, 8, 9, 10, BOGUS, 11, 12, 13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};
unsigned long int CHILD_LETTER_BIT_MASKS[SIZE_OF_CHARACTER_SET] = {1, 6, 24, 224, 1792, 14336, 114688, 1966080, 31457280, 503316480, 8053063680, 128849018880, 2061584302080, 32985348833280};
unsigned int CHILD_LETTER_BIT_SHIFTS[SIZE_OF_CHARACTER_SET] = {0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41};

typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The structure for a Boggle board is defined in this section.

// The "square" struct will represent one position in a Boggle board.
// A square also requires a flag to indicate use in the current word being formed, the number of valid neighbours, and the index of the letter showing on its face.
struct square {
	// The Used flag will indicate if a square is being used in constructing the current word, and hence to remove the used square from further inclusion in the same word.
	Bool Used;
	unsigned int  NumberOfLivingNeighbours;
	struct square *LivingNeighbourSquarePointerArray[NEIGHBOURS];
	unsigned int LetterIndex;
};

// Define the "Square" type.
typedef struct square Square;
typedef Square* SquarePtr;

// This Function initializes ThisSquare when passed its row and column position on the board.
// Important note:  The function is going to use the low level C concept of pointer arithmatic to fill the LivingNeighbourSquarePointerArray, which will be filled from the top-left, clockwise.
void SquareInit(SquarePtr ThisSquare, unsigned int RowPosition, unsigned int ColPosition){
	unsigned int X;
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

inline char SquareLetterIndex(SquarePtr ThisSquare){
	return ThisSquare->LetterIndex;
}

inline Bool SquareUsed(SquarePtr ThisSquare){
	return ThisSquare->Used;
}

// This function has been converted into a macro, because it is used in the often called "BoardPopulate" routine, and it needs to be fast.
#define SQUARE_CHANGE_LETTER_INDEX(thissquare, newindex) ((thissquare)->LetterIndex = (newindex))

// A function used exclusively for debugging purposes, display the members of "ThisSquare".
void SquareOutput( SquarePtr ThisSquare, unsigned int Row, unsigned int Col){
	unsigned int X;
	printf("{%c} at [%d,%d] |TL...Clockwise Living Neighbours|-|", CHARACTER_SET[ThisSquare->LetterIndex], Row, Col);
	for ( X = 0; X < NEIGHBOURS; X++ ) printf("%c|", (ThisSquare->LivingNeighbourSquarePointerArray[X] == NULL)? '*': (CHARACTER_SET[(ThisSquare->LivingNeighbourSquarePointerArray[X])->LetterIndex]));
	printf(" - |%s|\n", ((ThisSquare->Used) == FALSE)? "UNUSED": "USED");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A board is defined as simply a static 2 dimensional array of Squares.
struct board {
	Square Block[MAX_ROW][MAX_COL];
};

typedef struct board Board;
typedef Board* BoardPtr;

// This initialization function sets the neighbour array for all of the Squares in ThisBoard's Block (this only needs to be done once).
// The Letter index for each square will be a blank space ' ', and they will all be not Used.
void BoardInit(BoardPtr ThisBoard){
	unsigned int Row;
	unsigned int Col;
	for ( Row = MAX_ROW; Row-- > 0; ) {
		for ( Col = MAX_COL; Col-- > 0; ) {
			SquareInit(&(ThisBoard->Block[Row][Col]), Row, Col);
		}
	}
}

// This function simply transcribes the BoardString data into ThisBoard using the correct format.
// A major optimization has taken place at this level because the ADTDAWG's direct property enforces the "Order Does Not Matter," paradigm, and thus, no sorting is required.
void BoardPopulate(BoardPtr ThisBoard, char *BoardString){
	unsigned int Row;
	unsigned int Col;
	for ( Row = MAX_ROW; Row-- > 0; ) {
		for ( Col = MAX_COL; Col-- > 0; ) {
			SQUARE_CHANGE_LETTER_INDEX(&((ThisBoard->Block)[Row][Col]), CHARACTER_LOCATIONS[BoardString[Row * MAX_COL + Col] - 'A']);
		}
	}
}

// This function will be used to debug the program.  It will not be used when the analysis of a large number of boards is required.
void BoardOutput(BoardPtr ThisBoard){
	unsigned int Row;
	unsigned int Col;
	char PrintLine[2*(MAX_COL) + 2];
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
// These are the global variables needed.

// Each worker thread will have it's own time stamping unsigned int array for all of the words in the lexicon.
unsigned int *LexiconTimeStamps[NUMBER_OF_WORKER_THREADS];

// These are the pointers to the global immutable lexicon data structure.  The ADTDAWG is well advanced and beyond the scope of the high level search algorithm.
// Since these variables are branded as "Read Only," they can be utilized globally without passing pointers.
unsigned int *PartOneArray;
unsigned long int *PartTwoArray;
unsigned int *PartThreeArray;
unsigned int PartThreeFourTransition;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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

// This is where the recursion-replacement stack is implemented.
// Using macros allows the programmer to change the value of an argument directly.
// The stack needs room for the NULL position 0, (MAX_STRING_LENGTH - 1) square positions, and a buffer for the top pointer.
#define DISCOVERY_STACK_SIZE (MAX_STRING_LENGTH + 1)
#define DISCOVERY_STACK_PUSH(top, square, indie, fcindie, len, next, workon, second) (((top->TheSquareNow = (square)), (top->LexiconPartOneIndex = (indie)), (top->FirstChildIndex = (fcindie)), (top->WordLengthNow = (len)), (top->NextMarkerNow = (next)), (top->TheChildToWorkOn = (workon)), (top->TheSecondPartNow = (second)), ++top))
#define	DISCOVERY_STACK_POP(square, indie, fcindie, len, next, workon, second, top) ((--top, (square = top->TheSquareNow), (indie = top->LexiconPartOneIndex), (fcindie = top->FirstChildIndex), (len = top->WordLengthNow), (next = top->NextMarkerNow), (workon = top->TheChildToWorkOn), (second = top->TheSecondPartNow)))
#define	DISCOVERY_STACK_NOT_EMPTY (TheDiscoveryStack < TheTop)

DiscoveryStackNode TheDiscoveryStack[DISCOVERY_STACK_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The sequential board scoring functions are found here for the new ADTDAWG.

// This is the central piece of code in the BIG Boggle board analysis scheme.
// An explicit stack is used to traverse the neighbours of "BeginSquare," regardless of alphabetical order.  It updates the global "LexiconTimeStamps" to eliminate the counting of identical words.
// The algorithm is inherently recursive, and this drawback has been excised.
// Every letter on the board must be contained in the lexicon character set.

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
	DiscoveryStackNodePtr TheTop = TheDiscoveryStack + 1;
	while ( DISCOVERY_STACK_NOT_EMPTY ) {
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
		// If "WorkingSquare" has children, visit the next one from ("NumberOfLivingNeighbours" - 1) to zero.  There will be no scrolling through a list of children in the lexicon data structure when using the ADTDAWG.
		DoWeNeedToPop = TRUE;
		if ( WorkingChildIndex ) {
			WorkingNeighbourList = WorkingSquare->LivingNeighbourSquarePointerArray;
			if ( FirstTime == TRUE ) {
				WorkingNextMarker += (WorkingMarker - PartThreeArray[WorkingChildIndex]);
				WorkingSecondPart = PartTwoArray[(PartOneArray[WorkingIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT];
				WorkOnThisChild = WorkingSquare->NumberOfLivingNeighbours;
			}
			for ( X = WorkOnThisChild; X-- > 0; ) {
				if ( (WorkingNeighbourList[X])->Used == FALSE ) {
					TheChosenLetterIndex = (WorkingNeighbourList[X])->LetterIndex;
					if ( WorkingOffset = ( WorkingSecondPart & CHILD_LETTER_BIT_MASKS[TheChosenLetterIndex]) ) {
						WorkingOffset >>= CHILD_LETTER_BIT_SHIFTS[TheChosenLetterIndex];
						WorkingOffset -= 1;
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
			// We have now finished using "WorkingSquare", so set its "Used" element to FALSE.
			WorkingSquare->Used = FALSE;
			// Pop the top of the stack into the function and pick up where we left off at that particular square.
			DISCOVERY_STACK_POP(WorkingSquare, WorkingIndex, WorkingChildIndex, WorkingNumberOfChars, WorkingNextMarker, WorkOnThisChild, WorkingSecondPart, TheTop);
			FirstTime = FALSE;
		}
	}
	return Result;
}

// The function returns the Boggle score for "ThisBoard."  I uses the global time stamps.
unsigned int BoardSquareWordDiscover(BoardPtr ThisBoard, unsigned int TheTimeNow, unsigned int CallingThread){
	unsigned int TheScoreTotal = 0;
	unsigned int CurrentRow;
	unsigned int CurrentCol;
	unsigned int CurrentPartOneIndex;
	// Add up all the scores that originate from each square in the board.
	for ( CurrentRow = 0; CurrentRow < MAX_ROW; CurrentRow++ ) {
		for ( CurrentCol = 0; CurrentCol < MAX_COL; CurrentCol++ ) {
			CurrentPartOneIndex = (((ThisBoard->Block)[CurrentRow][CurrentCol]).LetterIndex + 1);
			TheScoreTotal += SquareWordDiscoverStack(&((ThisBoard->Block)[CurrentRow][CurrentCol]), CurrentPartOneIndex, PartThreeArray[CurrentPartOneIndex], TheTimeNow, CallingThread);
		}
	}
	return TheScoreTotal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Converts the two digit integer X into a string that tacks onto board strings to indicate the last square to be altered.
void ConvertSquareNumberToString( char *TheThreeString, unsigned int X ){
	if ( X > 19 ) {
		TheThreeString[0] = '2';
		TheThreeString[1] = ('0' + (X - 20));
	}
	else if ( X > 9 ) {
		TheThreeString[0] = '1';
		TheThreeString[1] = ('0' + (X - 10));
	}
	else {
		TheThreeString[0] = '0';
		TheThreeString[1] = '0' + X;
	}
	TheThreeString[2] = '\0';
}

int LoadDictionary() {
	FILE *PartOne = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "rb");
	FILE *PartTwo = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "rb");
	FILE *PartThree = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "rb");
	FILE *PartFour = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "rb");

	unsigned char TempReadIn;
	unsigned int SizeOfPartOne;
	unsigned long int SizeOfPartTwo;
	unsigned int SizeOfPartThree;
	unsigned int SizeOfPartFour;

	// Read in the size of each data file.
	if ( fread(&SizeOfPartOne, 4, 1, PartOne) != 1 ) return 0;
	if ( fread(&SizeOfPartTwo, 8, 1, PartTwo) != 1 ) return 0;
	if ( fread(&SizeOfPartThree, 4, 1, PartThree) != 1 ) return 0;
	PartThreeFourTransition = SizeOfPartThree + 1;
	SizeOfPartFour = SizeOfPartOne - SizeOfPartThree;

	// Print out the lexicon size values.
	printf("\n");
	printf("SizeOfPartOne |%d|\n", SizeOfPartOne);
	printf("SizeOfPartTwo |%ld|\n", SizeOfPartTwo);
	printf("SizeOfPartThree |%d|\n", SizeOfPartThree);
	printf("Transition |%d|\n", PartThreeFourTransition);
	printf("SizeOfPartFour |%d|\n", SizeOfPartFour);
	printf("\n");

	// Allocate memory for the ADTDAWG.
	PartOneArray = (unsigned int *)malloc((SizeOfPartOne + 1) * sizeof(int));
	PartTwoArray = (unsigned long int *)malloc(SizeOfPartTwo * sizeof(long int));
	PartThreeArray = (unsigned int *)malloc((SizeOfPartOne + 1) * sizeof(int));

	// Read in the data files into global arrays of basic integer types.
	// The zero position in "PartOneArray" is the NULL node.
	PartOneArray[0] = 0;
	if ( fread(PartOneArray + 1, 4, SizeOfPartOne, PartOne) != SizeOfPartOne ) return 0;
	if ( fread(PartTwoArray, 8, SizeOfPartTwo, PartTwo) != SizeOfPartTwo ) return 0;
	// The Zero position in "PartThreeArray" maps to the NULL node in "PartOneArray".
	PartThreeArray[0] = 0;
	if ( fread(PartThreeArray + 1, 4, SizeOfPartThree, PartThree) != SizeOfPartThree ) return 0;
	// Part Four has been replaced by encoding the Part Four WTEOBL values as 32 bit integers for speed.  The size of the data structure is small enough as it is.
	for ( int X = (SizeOfPartThree + 1); X <= SizeOfPartOne; X++ ) {
		if ( fread(&TempReadIn, 1, 1, PartFour) != 1 ) return 0;
		PartThreeArray[X] = TempReadIn;
	}

	fclose(PartOne);
	fclose(PartTwo);
	fclose(PartThree);
	fclose(PartFour);
	return 1;
}

int main() {

	unsigned int X;
	int Y;
	unsigned int Z;
	unsigned int W;
	unsigned int WhatTime;
	unsigned int UniqueFours = 0;

	unsigned int InsertionSpot = 0;
	unsigned int CurrentScore;
	unsigned int BestScore = 0;
	unsigned int TheBestBoardIndex;

	unsigned int FirstOffLimitsLetterIndex;
	unsigned int SecondOffLimitsLetterIndex;

	char SeedBoardString[SQUARE_COUNT + 1];
	char TemporaryBoardString[BOARD_STRING_SIZE];

	char CurrentOffLimitsNumbers[5];

	char *DoubleDeed = (char*)malloc(sizeof(char)*NUMBER_OF_DOUBLE_DEVIATIONS*(BOARD_STRING_SIZE));

	BoardPtr WorkingBoard = (BoardPtr)malloc(sizeof(Board));

	double BeginWorkTime;
	double EndWorkTime;
	double TheRunTime;
	srand((unsigned int)time(NULL));

	BoardInit(WorkingBoard);

	// Allocate the set of lexicon time stamps as unsigned integers.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) LexiconTimeStamps[X] = (unsigned int*)malloc((TOTAL_WORDS_IN_LEXICON + 1)*sizeof(unsigned int));

	// Zero all of the global time stamps.  Notice how there are only time stamps for each word in the lexicon and not for intermediate nodes.
	// The memset() function promises a more efficient zeroing procedure than a for loop.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) memset(LexiconTimeStamps[X], 0, (TOTAL_WORDS_IN_LEXICON + 1)*sizeof(unsigned int));

	int result = LoadDictionary();
	if (result != 1) {
		return result;
	}
	printf("The four data files have been opened and read into memory.\n\n");

	strcpy(SeedBoardString, "RSLCSDEIAEGNTRPATESESMIDR");
	strcpy(TemporaryBoardString, SeedBoardString);
	TemporaryBoardString[SQUARE_COUNT] = '2';

	printf("Create all of the champion board's double deviations, |%s|.\n", SeedBoardString);
	BoardPopulate(WorkingBoard, SeedBoardString);
	BoardOutput(WorkingBoard);
	printf("The Seed Score |%d|\n\n", BoardSquareWordDiscover(WorkingBoard, 1, 0));

	// Fill a large array with all of the champion board's double deviations.
	for ( X = 0; X < SQUARE_COUNT; X++ ) {
		for ( Y = X + 1; Y < SQUARE_COUNT; Y++ ) {
			// Now we are in a position where X and Y represent the two locations to be deviated.
			FirstOffLimitsLetterIndex = CHARACTER_LOCATIONS[SeedBoardString[X] - 'A'];
			SecondOffLimitsLetterIndex = CHARACTER_LOCATIONS[SeedBoardString[Y] - 'A'];
			// Set the trailing, off limits numbers here.
			ConvertSquareNumberToString(CurrentOffLimitsNumbers, X);
			ConvertSquareNumberToString(&(CurrentOffLimitsNumbers[2]), Y);
			strcpy(&(TemporaryBoardString[SQUARE_COUNT + 1]), CurrentOffLimitsNumbers);
			for ( Z = 0; Z < SIZE_OF_CHARACTER_SET; Z++ ) {
				if ( Z == FirstOffLimitsLetterIndex ) continue;
				TemporaryBoardString[X] = CHARACTER_SET[Z];
				for ( W = 0; W < SIZE_OF_CHARACTER_SET; W++ ) {
					if ( W == SecondOffLimitsLetterIndex ) continue;
					TemporaryBoardString[Y] = CHARACTER_SET[W];
					strcpy(&(DoubleDeed[InsertionSpot*(BOARD_STRING_SIZE)]), TemporaryBoardString);
					// printf("%s\n", TemporaryBoardString);
					InsertionSpot += 1;
				}
			}
			strcpy(TemporaryBoardString, SeedBoardString);
			TemporaryBoardString[SQUARE_COUNT] = '2';
		}
	}

	printf("Total Number Of Good Boards Being Evaluated Now = |%d|, so please wait.\n", InsertionSpot);

	BeginWorkTime = (double)clock() / CLOCKS_PER_SEC;
	// The boards inside of "DoubleDeed" get evaluated inside of this loop.
	for (WhatTime = 0; WhatTime < InsertionSpot; WhatTime++) {
		BoardPopulate(WorkingBoard, &(DoubleDeed[WhatTime * (BOARD_STRING_SIZE)]));
		CurrentScore = BoardSquareWordDiscover(WorkingBoard, WhatTime + 2, 0);
		if (CurrentScore > BestScore) {
			BestScore = CurrentScore;
			TheBestBoardIndex = WhatTime;
		}
	}
	EndWorkTime = (double)clock() / CLOCKS_PER_SEC;
	TheRunTime = EndWorkTime - BeginWorkTime;

	BoardPopulate(WorkingBoard, &(DoubleDeed[TheBestBoardIndex * (BOARD_STRING_SIZE)]));
	printf("The Best Score Found Is|%d| At Position |%d| - |%s|\n\n", BestScore, TheBestBoardIndex, &(DoubleDeed[TheBestBoardIndex * (BOARD_STRING_SIZE)]));
	printf("The Running time for this operation is |%g| seconds. |%g| Boards per second.\n\n", TheRunTime, InsertionSpot / TheRunTime);
	BoardOutput(WorkingBoard);

	printf("\n-----------------------------------------------------------------------\n");
	printf("\nAnalyze a random board batch of the same size.\n");
	InsertionSpot = 0;
	for (X = 0; X < NUMBER_OF_DOUBLE_DEVIATIONS; X++) {
		for (Y = 0; Y < SQUARE_COUNT; Y++) TemporaryBoardString[Y] = CHARACTER_SET[rand() % SIZE_OF_CHARACTER_SET];
		TemporaryBoardString[SQUARE_COUNT] = '\0';
		strcpy(&(DoubleDeed[InsertionSpot * (BOARD_STRING_SIZE)]), TemporaryBoardString);
		InsertionSpot += 1;
	}

	printf("Total Number Of Random Boards Being Evaluated Now = |%d|, so please wait.\n", InsertionSpot);

	BeginWorkTime = (double)clock() / CLOCKS_PER_SEC;
	// Evaluate the random boards.
	// Zero the timestamps.
	for (X = 0; X < NUMBER_OF_WORKER_THREADS; X++) memset(LexiconTimeStamps[X], 0, (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(unsigned int));
	BestScore = 0;
	for (WhatTime = 0; WhatTime < InsertionSpot; WhatTime++) {
		BoardPopulate(WorkingBoard, &(DoubleDeed[WhatTime * (BOARD_STRING_SIZE)]));
		CurrentScore = BoardSquareWordDiscover(WorkingBoard, WhatTime + 2, 0);
		if (CurrentScore > BestScore) {
			BestScore = CurrentScore;
			TheBestBoardIndex = WhatTime;
		}
	}

	EndWorkTime = (double)clock() / CLOCKS_PER_SEC;
	TheRunTime = EndWorkTime - BeginWorkTime;

	BoardPopulate(WorkingBoard, &(DoubleDeed[TheBestBoardIndex * (BOARD_STRING_SIZE)]));
	printf("The Best Random Score Found Is|%d| At Position |%d| - |%s|\n\n", BestScore, TheBestBoardIndex, &(DoubleDeed[TheBestBoardIndex * (BOARD_STRING_SIZE)]));
	printf("The Running time for this operation is |%g| seconds. |%g| Boards per second.\n\n", TheRunTime, InsertionSpot / TheRunTime);
	BoardOutput(WorkingBoard);

	return 0;
}
