// The DeepSearch.c algorithm uses an in-depth investigation of neighbouring boards starting at a predefined "MASTER_SEED_BOARD".
// The constants that define the depth of the search...  NUMBER_OF_SEEDS_TO_RUN, ROUNDS, BOARDS_PER_ROUND

// Noteworthy Optimizations:
//
// 1) Maintaining up to date information about previously analyzed elements to eliminate redundant searching.  The trie data structure was chosen for insertion speed.
// 1) The use of an immutable compressed lexicon data structure with complete child node information, and word tracking that enables the use of time stamps (ADTDAWG).
// 3) Parallel batch processing using Posix PTHREADS, to reduce inter-thread communication lag.
// 2) The reduction of the character set to "SIZE_OF_CHARACTER_SET" to eliminate the consideration of characters with limited presence in the lexicon.
// 3) Use of GLIBC qsort code optimized with an explicit comparison macro, and memmove() for the final Insertion Sort pass.
// 4) Replacing frequently used recursive functions with an explicit stack implementation.
// 5) Selection of list size values so that a streamlined binary inseretion sort can be used
// 6) The majority of numbers in the program are unsigned integers to reduce arithmetic complexity.
// 7) Low level programming style caters to -O3 gcc optimization, such as loop unrolling.
// 8) Arrays replace conditional cascades.
// 9) Comprehensive documentation to isolate logical flaws, and to make the program readable, and accessible.  Strict conventions for variable names, and white space have been used.
// 10) Generalizations are used only when they will not noticably compromise the performance of the program.

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "adtdawg.h"
#include "board-evaluate.h"
#include "const.h"

// The ADTDAWG for Lexicon_14, a subset of TWL06, is located in the 4 data files listed below.
#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"

// General "Boggle" Constants.
#define MAX_ROW 5
#define MAX_COL 5
#define SQUARE_COUNT 25
#define BOARD_STRING_SIZE 28
#define NEIGHBOURS 8
#define NUMBER_OF_ENGLISH_LETTERS 26
#define SIZE_OF_CHARACTER_SET 14
#define MAX_STRING_LENGTH 15
#define BOGUS 99

// Constants that define the high level "DeepSearch.c" algorithm.
#define MASTER_SEED_BOARD "AGRIMODAOLSTECETISMNGPART"
#define SINGLE_DEVIATIONS 312
#define NUMBER_OF_SEEDS_TO_RUN 1000
#define ROUNDS 25
#define BOARDS_PER_ROUND 64
#define BOARDS_PER_THREAD (BOARDS_PER_ROUND/NUMBER_OF_WORKER_THREADS)

// Scoreboard list constants for streamlined binary insertion sort implementation.  Note that "BOARDS_PER_ROUND" needs to be a multiple of "NUMBER_OF_WORKER_THREADS", and should be the closest multiple less than "EVALUATE_LIST_SIZE".
#define MASTER_LIST_SIZE 1026
#define MAX_LOOP_SEARCH_DEPTH_MASTER 9
#define EVALUATE_LIST_SIZE 66
#define MAX_LOOP_SEARCH_DEPTH_EVALUATE 5

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic constructs and functions that will be useful.

// This function assumes that "TheNumberNotYet" is a 2 char string of digits between [0,9].  Do not pass it anything else.  It will return the integer equivalent.
inline unsigned int TwoCharStringToInt(char* TheNumberNotYet){
	return (TheNumberNotYet[0] - '0')*10 + (TheNumberNotYet[1] - '0');
}

// Converts the two digit integer X into the string "TheThreeString" that tacks onto board strings to indicate the last altered square.  This reduces redundant element consideration.
void ConvertSquareNumberToString( char *TheThreeString, int X ){
	if ( X < 10 ) {
		TheThreeString[0] = '0';
		TheThreeString[1] = '0' + X;
	}
	else if ( X < 20 ) {
		TheThreeString[0] = '1';
		TheThreeString[1] = ('0' + (X - 10));
	}
	else {
		TheThreeString[0] = '2';
		TheThreeString[1] = ('0' + (X - 20));
	}
	TheThreeString[2] = '\0';
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section covers the streamlined Binary Insertion Sort coding.

// This function inserts "ThisBoardString" into "TheList" which must have "MASTER_LIST_SIZE" elements. "TheList" will already be sorted, and a Binary Insertion Sort will be used.
// The return value is "TRUE" or "FALSE" depending on if "ThisScore" was high enough to make the cut.
Bool InsertBoardStringIntoMasterList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore){
	unsigned int X;
	unsigned int Left = 0;
	unsigned int Right = MASTER_LIST_SIZE - 1;
	unsigned int NextElement;
	char *TempBoardStringHolder;

	// "ThisScore" does not make the cut; it is too small.
	if ( ThisScore <= TheNumbers[Right] ) return FALSE;

	// "ThisScore" belongs at the end of the list.
	Right -= 1;
	if ( ThisScore <= TheNumbers[Right] ) {
		strcpy(TheList[MASTER_LIST_SIZE - 1], ThisBoardString);
		TheNumbers[MASTER_LIST_SIZE - 1] = ThisScore;
		return TRUE;
	}

	// "ThisScore" belongs at the first position in the list.
	if ( ThisScore >=  TheNumbers[Left] ) {
		TempBoardStringHolder = TheList[(MASTER_LIST_SIZE - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + 1, TheList, sizeof(char*)*(MASTER_LIST_SIZE - 1));
		TheList[Left] = TempBoardStringHolder;
		memmove(TheNumbers + 1, TheNumbers, sizeof(unsigned int)*(MASTER_LIST_SIZE - 1));
		TheNumbers[Left] = ThisScore;
		return TRUE;
	}

	// Set the initial midpoint.
	NextElement = Left + ((Right - Left)>>1);

	// This loop will be unwound by compiler optimization.
	for ( X = 0; X < MAX_LOOP_SEARCH_DEPTH_MASTER; X++ ) {
		// "NextElement" is the new "Left".
		if ( TheNumbers[NextElement] >  ThisScore ) {
			Left = NextElement;
		}
		// "NextElement" will become the new "Right".
		else if ( TheNumbers[NextElement] <  ThisScore ) {
			Right = NextElement;
		}
		// "NextElement" holds a value equal to "ThisScore", and is the insertion point.
		else {
			// memmove() is going to employ pointer arithmatic for pointers to internal array members.
			TempBoardStringHolder = TheList[(MASTER_LIST_SIZE - 1)];
			strcpy(TempBoardStringHolder, ThisBoardString);
			memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(MASTER_LIST_SIZE - 1 - NextElement));
			TheList[NextElement] = TempBoardStringHolder;
			memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(MASTER_LIST_SIZE - 1 - NextElement));
			TheNumbers[NextElement] = ThisScore;
			return TRUE;
		}
		// Advance the "NextElement";
		NextElement = Left + ((Right - Left)>>1);
	}

	// "NextElement" is now flanked by "Left" and "Right", and this is known with absolute certainty.
	// Since two cases will result in the insertion position being equal to "Right", we only need to make one comparison on the final iteration.
	if ( TheNumbers[NextElement] <  ThisScore ) {
		TempBoardStringHolder = TheList[(MASTER_LIST_SIZE - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(MASTER_LIST_SIZE - 1 -NextElement));
		TheList[NextElement] = TempBoardStringHolder;
		memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(MASTER_LIST_SIZE - 1 - NextElement));
		TheNumbers[NextElement] = ThisScore;
		return TRUE;
	}
	// "ThisScore" is smaller or equal to "TheNumbers[NextElement]", so the insertion position will be "Right".
	else {
		TempBoardStringHolder = TheList[(MASTER_LIST_SIZE - 1)];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + Right + 1, TheList + Right, sizeof(char*)*(MASTER_LIST_SIZE - 1 - Right));
		TheList[Right] = TempBoardStringHolder;
		memmove(TheNumbers + Right + 1, TheNumbers + Right, sizeof(unsigned int)*(MASTER_LIST_SIZE - 1 - Right));
		TheNumbers[Right] = ThisScore;
		return TRUE;
	}
}


// This function inserts "ThisBoardString" into "TheList" which must have "EVALUATE_LIST_SIZE" elements. "TheList" will already be sorted, and a Binary Insertion Sort will be used.
// The return value is "TRUE" or "FALSE" depending on if "ThisScore" was high enough to make the cut.
Bool InsertBoardStringIntoEvaluateList(char **TheList, unsigned int *TheNumbers, const char *ThisBoardString, unsigned int ThisScore){
	unsigned int X;
	unsigned int Left = 0;
	unsigned int Right = EVALUATE_LIST_SIZE - 1;
	unsigned int NextElement;
	char *TempBoardStringHolder;

	// "ThisScore" does not make the cut; it is too small.
	if ( ThisScore <= TheNumbers[Right] ) return FALSE;

	// "ThisScore" belongs at the end of the list.
	Right -= 1;
	if ( ThisScore <= TheNumbers[Right] ) {
		strcpy(TheList[EVALUATE_LIST_SIZE - 1], ThisBoardString);
		TheNumbers[EVALUATE_LIST_SIZE - 1] = ThisScore;
		return TRUE;
	}

	// "ThisScore" belongs at the first position in the list.
	if ( ThisScore >=  TheNumbers[Left] ) {
		TempBoardStringHolder = TheList[EVALUATE_LIST_SIZE - 1];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + 1, TheList, sizeof(char*)*(EVALUATE_LIST_SIZE - 1));
		TheList[Left] = TempBoardStringHolder;
		memmove(TheNumbers + 1, TheNumbers, sizeof(unsigned int)*(EVALUATE_LIST_SIZE - 1));
		TheNumbers[Left] = ThisScore;
		return TRUE;
	}

	// Set the initial midpoint.
	NextElement = Left + ((Right - Left)>>1);

	// This loop will be unwound by compiler optimization.
	for ( X = 0; X < MAX_LOOP_SEARCH_DEPTH_EVALUATE; X++ ) {
		// "NextElement" is the new "Left".
		if ( TheNumbers[NextElement] >  ThisScore ) {
			Left = NextElement;
		}
		// "NextElement" will become the new "Right".
		else if ( TheNumbers[NextElement] <  ThisScore ) {
			Right = NextElement;
		}
		// "NextElement" holds a value equal to "ThisScore", and is the insertion point.
		else {
			// memmove() is going to employ pointer arithmatic for pointers to internal array members.
			TempBoardStringHolder = TheList[EVALUATE_LIST_SIZE - 1];
			strcpy(TempBoardStringHolder, ThisBoardString);
			memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(EVALUATE_LIST_SIZE - 1 - NextElement));
			TheList[NextElement] = TempBoardStringHolder;
			memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(EVALUATE_LIST_SIZE - 1 - NextElement));
			TheNumbers[NextElement] = ThisScore;
			return TRUE;
		}
		// Advance the "NextElement";
		NextElement = Left + ((Right - Left)>>1);
	}

	// "NextElement" is now flanked by "Left" and "Right", and this is known with absolute certainty.
	// Since two cases will result in the insertion position being equal to "Right", we only need to make one comparison on the final iteration.
	if ( TheNumbers[NextElement] <  ThisScore ) {
		TempBoardStringHolder = TheList[EVALUATE_LIST_SIZE - 1];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + NextElement + 1, TheList + NextElement, sizeof(char*)*(EVALUATE_LIST_SIZE - 1 -NextElement));
		TheList[NextElement] = TempBoardStringHolder;
		memmove(TheNumbers + NextElement + 1, TheNumbers + NextElement, sizeof(unsigned int)*(EVALUATE_LIST_SIZE - 1 - NextElement));
		TheNumbers[NextElement] = ThisScore;
		return TRUE;
	}
	// "ThisScore" is smaller or equal to "TheNumbers[NextElement]", so the insertion position will be "Right".
	else {
		TempBoardStringHolder = TheList[EVALUATE_LIST_SIZE - 1];
		strcpy(TempBoardStringHolder, ThisBoardString);
		memmove(TheList + Right + 1, TheList + Right, sizeof(char*)*(EVALUATE_LIST_SIZE - 1 - Right));
		TheList[Right] = TempBoardStringHolder;
		memmove(TheNumbers + Right + 1, TheNumbers + Right, sizeof(unsigned int)*(EVALUATE_LIST_SIZE - 1 - Right));
		TheNumbers[Right] = ThisScore;
		return TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This sector of the program will hold the minimal trie content.  No end of word flag is required because every board must be SQUARE_COUNT chars in length
// It is a reduced trie for holding board strings of identical size, that way boards already evaluated can be avoided for future evaluation.

struct minboardtnode {
	struct minboardtnode* Next;
	struct minboardtnode* Child;
	char Letter;
};

typedef struct minboardtnode MinBoardTnode;
typedef MinBoardTnode* MinBoardTnodePtr;

MinBoardTnodePtr MinBoardTnodeNext( MinBoardTnodePtr ThisMinBoardTnode ) {
	return ThisMinBoardTnode->Next;
}

#define MIN_BOARD_TNODE_CHILD(thisminboardtnode) ((thisminboardtnode)->Child)

MinBoardTnodePtr MinBoardTnodeInit(char Chap, MinBoardTnodePtr OverOne){
	MinBoardTnodePtr Result = (MinBoardTnode*)malloc(sizeof(MinBoardTnode));
	Result->Letter = Chap;
	Result->Next = OverOne;
	Result->Child = NULL;
	return Result;
}

struct minboardtrie {
	MinBoardTnodePtr First;
	unsigned int NumberOfBoards;
};

typedef struct minboardtrie MinBoardTrie;
typedef MinBoardTrie* MinBoardTriePtr;

// Set up the parent node in the MinBoardTrie.
MinBoardTriePtr MinBoardTrieInit(void){
	MinBoardTriePtr Result;
	Result = (MinBoardTrie*)malloc(sizeof(MinBoardTrie));
	Result->NumberOfBoards = 0;
	Result->First = MinBoardTnodeInit('*', NULL);
	return Result;
}

unsigned int MinBoardTrieSize(MinBoardTriePtr ThisMinBoardTrie){
	return ThisMinBoardTrie->NumberOfBoards;
}

// Search for "WonderBoard" under "ThisMinBoardTnode", and return TRUE or FALSE.
Bool MinBoardTnodeBoardSearchRecurse(MinBoardTnodePtr ThisMinBoardTnode, char *WonderBoard, unsigned int CheckThisPosition){
	MinBoardTnodePtr Current = ThisMinBoardTnode;
	for ( ; ; ) {
		if ( WonderBoard[CheckThisPosition] == Current->Letter ) {
			if ( CheckThisPosition == (SQUARE_COUNT - 1) ) return TRUE;
			else if ( Current->Child != NULL ) return MinBoardTnodeBoardSearchRecurse(Current->Child, WonderBoard, CheckThisPosition + 1);
			else return FALSE;
		}
		else if ( WonderBoard[CheckThisPosition] > Current->Letter ) {
			Current = Current->Next;
			if ( Current == NULL ) return FALSE;
		}
		else return FALSE;
	}
}

// Searches for the word "QuanderBoard" in "ThisMinBoardTrie", returns TRUE, or FALSE accordingly.
Bool MinBoardTrieBoardSearch(MinBoardTriePtr ThisMinBoardTrie, char * QuanderBoard){
	if ( MIN_BOARD_TNODE_CHILD(ThisMinBoardTrie->First) == NULL ) return FALSE;
	else return MinBoardTnodeBoardSearchRecurse(MIN_BOARD_TNODE_CHILD(ThisMinBoardTrie->First), QuanderBoard, 0);
}

// This function adds a board to the MinBoardTrie and returns 1, if the board already exists, it returns 0.
unsigned int MinBoardTnodeAddBoardRecurse(MinBoardTnodePtr ParentNode, const char *Board, unsigned int CheckThisPosition){
	unsigned int Y;
	MinBoardTnodePtr CurrentParent = ParentNode;
	MinBoardTnodePtr Current = ParentNode->Child;
	MinBoardTnodePtr NewNode;
	// We are now dealing with the case where the new nodes will start a line as a direct child.
	if ( Current == NULL) {
		for ( Y = CheckThisPosition; Y < SQUARE_COUNT; Y++ ) {
			NewNode = MinBoardTnodeInit(Board[Y], NULL);
			CurrentParent->Child = NewNode;
			CurrentParent = NewNode;
		}
		return 1;
	}
	// We are now going to add a new line of nodes at the beginning of a list that already exists.  The distinction is the setting of the Next pointer of the first new node.
	if ( Current->Letter > Board[CheckThisPosition] ) {
		NewNode = MinBoardTnodeInit(Board[CheckThisPosition], Current);
		CurrentParent->Child = NewNode;
		CurrentParent = NewNode;
		for ( Y = (CheckThisPosition + 1); Y < SQUARE_COUNT; Y++ ) {
			NewNode = MinBoardTnodeInit(Board[Y], NULL);
			CurrentParent->Child = NewNode;
			CurrentParent = NewNode;
		}
		return 1;
	}
	// Use iteration to roll through the next list, then recurse, return 0, or start a new line at the right place.
	for ( ; ; ) {
		if ( Board[CheckThisPosition] == Current->Letter ) {
			if ( CheckThisPosition == (SQUARE_COUNT - 1) ) return 0;
			// We have reached the node that we should be at but it is not the final letter so recurse and incrament the BoggleScoreBelowMe by the ReturnedValue.
			else return MinBoardTnodeAddBoardRecurse(Current, Board, CheckThisPosition + 1);
		}
		// Current node's letter is smaller than the one we are looking for.  It will never be larger at this point because we would have caught it already.
		else {
			// Add a new line at the end of the list.
			if ( Current->Next == NULL ) {
				NewNode = MinBoardTnodeInit(Board[CheckThisPosition], NULL);
				Current->Next = NewNode;
				CurrentParent = NewNode;
				for ( Y = (CheckThisPosition + 1); Y < SQUARE_COUNT; Y++ ) {
					NewNode = MinBoardTnodeInit( Board[Y], NULL );
					CurrentParent->Child = NewNode;
					CurrentParent = NewNode;
				}
				return 1;
			}
			// Insert the node line directly after the Current node.
			if ( Board[CheckThisPosition] < (Current->Next)->Letter ) {
				NewNode = MinBoardTnodeInit(Board[CheckThisPosition], Current->Next);
				Current->Next = NewNode;
				CurrentParent = NewNode;
				for ( Y = (CheckThisPosition + 1); Y < SQUARE_COUNT; Y++ ) {
					NewNode = MinBoardTnodeInit(Board[Y], NULL);
					CurrentParent->Child = NewNode;
					CurrentParent = NewNode;
				}
				return 1;
			}
			// If we make it to here, keep looking on down the line.
			Current = Current->Next;
		}
	}
}


// This function adds "NewBoard" to "ThisMinBoardTrie" if it doesn't exist already.  Returns "1" or "0" accordingly.
unsigned int MinBoardTrieAddBoard(MinBoardTriePtr ThisMinBoardTrie, const char * NewBoard){
	unsigned int ReturnValue;
	ReturnValue = MinBoardTnodeAddBoardRecurse(ThisMinBoardTrie->First, NewBoard, 0);
	ThisMinBoardTrie->NumberOfBoards += ReturnValue;
	return ReturnValue;
}

// This is a standard depth first preorder tree traversal, whereby the objective is to print all of the boards contained in a MinBoardTrie alphabetically, for debugging only.
void MinBoardTnodeTreeOutputRecurse(MinBoardTnodePtr ThisMinBoardTnode, char *RunnerBoard, unsigned int PositionImpossible){
	// We are at a valid node, so add the Letter to the RunnerBoard at the PositionImpossible.
	RunnerBoard[PositionImpossible] = ThisMinBoardTnode->Letter;
	// If we have arrived at a word, then print it.
	if ( PositionImpossible == (SQUARE_COUNT - 1) ) {
		RunnerBoard[SQUARE_COUNT] = '\0';
		printf( "|%s|\n", RunnerBoard );
	}
	// Move to the Child first, and then Move to the Next node in the current list.
	if ( ThisMinBoardTnode->Child != NULL ) MinBoardTnodeTreeOutputRecurse(ThisMinBoardTnode->Child, RunnerBoard, PositionImpossible + 1);
	if ( ThisMinBoardTnode->Next != NULL ) MinBoardTnodeTreeOutputRecurse(ThisMinBoardTnode->Next, RunnerBoard, PositionImpossible);
}


void MinBoardTrieOutput(MinBoardTriePtr ThisMinBoardTrie){
	char Mercury[SQUARE_COUNT + 1];
	// Make sure that we start at the first node under the blank root node First.
	printf( "This Min Board Trie Contains |%d| Boards.\n", ThisMinBoardTrie->NumberOfBoards );
	if ( MIN_BOARD_TNODE_CHILD(ThisMinBoardTrie->First) != NULL ) MinBoardTnodeTreeOutputRecurse(MIN_BOARD_TNODE_CHILD(ThisMinBoardTrie->First), Mercury, 0);
}

void FreeMinBoardTnodeRecurse(MinBoardTnodePtr ThisMinBoardTnode){
	if ( ThisMinBoardTnode->Child != NULL ) FreeMinBoardTnodeRecurse(ThisMinBoardTnode->Child) ;
	if ( ThisMinBoardTnode->Next != NULL ) FreeMinBoardTnodeRecurse(ThisMinBoardTnode->Next);
	free(ThisMinBoardTnode);
}

void FreeMinBoardTrie(MinBoardTriePtr ThisMinBoardTrie){
	FreeMinBoardTnodeRecurse(ThisMinBoardTrie->First);
	free(ThisMinBoardTrie);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This section is dedicated to sorting large lists of evaluated boards and scores.  It represents an adaptation of the GLIBC qsort code and an explicit stack documented here http://www.corpit.ru/mjt/qsort.html
// I will thank Michael Tokarev for an excellent web page, and a very useful recursion elimination technique.

// The BoardData pseudo-class will use macros for its associated functionality.
struct boarddata {
	char TheBoardString[SQUARE_COUNT + 2 + 1];
	unsigned int TheBoardScore;
};

typedef struct boarddata BoardData;
typedef BoardData* BoardDataPtr;

#define BOARD_DATA_THE_BOARD_STRING(thisboarddata) ((thisboarddata)->TheBoardString)

#define BOARD_DATA_THE_BOARD_SCORE(thisboarddata) (thisboarddata->TheBoardScore)

#define BOARD_DATA_SET_THE_BOARD_STRING(thisboarddata, newstring) (strcpy(thisboarddata->TheBoardString, newstring))

#define BOARD_DATA_SET_THE_BOARD_SCORE(thisboarddata, newscore) (thisboarddata->TheBoardScore = (newscore))

#define BOARD_DATA_SET(thisboarddata, string, score) ((strcpy(thisboarddata->TheBoardString, string)),(thisboarddata->TheBoardScore = (score)))

// This statement evaluates to TRUE when the board at "First" has a higher score than the board at "Second".
#define COMPARE_BOARD_DATA(First, Second) ( (*First)->TheBoardScore > (*Second)->TheBoardScore )? TRUE: FALSE

// Swap two items pointed to by "a" and "b" using temporary buffer "T".
#define BOARD_DATA_SWAP(a, b, T) ((void)((T = *a), (*a = *b), (*b = T)))

// When a list gets this small, standard insertion sort is a faster method.
#define SORT_TRANSITION_SIZE 4

// define the size of the list being sorted.  This number represents the total number of boards analyzed per thread, per round.
#define LIST_SIZE (BOARDS_PER_THREAD*SINGLE_DEVIATIONS)

// A qsort stack only needs to save two values.
struct qsortstacknode {
	BoardDataPtr *Lo;
	BoardDataPtr *Hi;
};

typedef struct qsortstacknode QsortStackNode;
typedef QsortStackNode* QsortStackNodePtr;

// This is where the recursion-replacement stack is implemented.
// Using macros allows the programmer to change the value of an argument directly.
#define QSORT_STACK_SIZE (8 * sizeof(unsigned int))
#define QSORT_STACK_PUSH(top, low, high) (((top->Lo = (low)), (top->Hi = (high)), ++top))
#define	QSORT_STACK_POP(low, high, top) ((--top, (low = top->Lo), (high = top->Hi)))
#define	QSORT_STACK_NOT_EMPTY(identity, top) ((TheQsortStacks[identity]) < top)

// Each worker thread will require its own qsort() stack.
QsortStackNode *TheQsortStacks[NUMBER_OF_WORKER_THREADS];

void BoardDataExplicitStackQuickSort(BoardDataPtr *Base, unsigned int Size, unsigned int CallingThread){
	BoardDataPtr Temp;
	if ( Size > SORT_TRANSITION_SIZE ) {
		BoardDataPtr *Low = Base;
		BoardDataPtr *High = Low + Size - 1;
		QsortStackNodePtr TheTop = TheQsortStacks[CallingThread] + 1;
		// Declaring "Left", "Right", and "Mid" inside of this while() loop is a valid choice.  "Low", and "High", on the other hand, require a larger scope.
		while ( QSORT_STACK_NOT_EMPTY(CallingThread, TheTop) ) {
			BoardDataPtr *Left;
			BoardDataPtr *Right;
			// Select median value from among "Low", "Mid", and "High".
			// Shift "Low" and "High" so the three values are sorted.
			// This lowers the probability of picking a bad pivot value.
			// Also a comparison is skipped for both "Left" and "Right" in the while loops.
			BoardDataPtr *Mid = Low + ((High - Low) >> 1);
			if ( COMPARE_BOARD_DATA(Mid, Low) ) BOARD_DATA_SWAP(Mid, Low, Temp);
			if ( COMPARE_BOARD_DATA(High, Mid) ) {
				BOARD_DATA_SWAP(Mid, High, Temp);
				if ( COMPARE_BOARD_DATA(Mid, Low) ) BOARD_DATA_SWAP(Mid, Low, Temp);
			}
			// The values at positions Low and High are already known to be in the correct partition.
			Left = Low + 1;
			Right = High - 1;
			// This section of Qsort collapses the walls of "Left" and "Right" until they cross over each other.
			do {
				while ( COMPARE_BOARD_DATA(Left, Mid) ) ++Left;
				while ( COMPARE_BOARD_DATA(Mid, Right) ) --Right;
				// Swap the elements at "Left" and "Right", but make sure to maintain the Mid pointer, because it might get in the way.
				if ( Left < Right) {
					BOARD_DATA_SWAP(Left, Right, Temp);
					if ( Mid == Left) Mid = Right;
					else if ( Mid == Right ) Mid = Left;
					++Left;
					--Right;
				}
				// When "Left" is equal to "Right", make them cross over each other.
				else if ( Left == Right ) {
					++Left;
					--Right;
					break;
				}
			} while ( Left <= Right);
			// Set up pointers for the next partitions, and push the larger partition onto the stack if its size exceeds "SORT_TRANSITION_SIZE".
			// By always pushing the larger of the two partitiona onto the stack, the stack size has an absolute limit of LOG base 2 (Size).
			// Continue sorting the smaller partition if its size exceeds "SORT_TRANSITION_SIZE".
			if ( (Right - Low) <= SORT_TRANSITION_SIZE ) {
				// Ignore both small partitions.
				if ( (High - Left) <= SORT_TRANSITION_SIZE ) QSORT_STACK_POP(Low, High, TheTop);
				// Ignore small left partition.
				else Low = Left;
			}
			// Ignore small right partition.
			else if ( (High - Left) <= SORT_TRANSITION_SIZE ) High = Right;
			// Push the larger left partition indices.
			else if ( (Right - Low) > (High - Left) ) {
				QSORT_STACK_PUSH(TheTop, Low, Right);
				Low = Left;
			}
			// Push the larger right partition indices.
			else {
				QSORT_STACK_PUSH(TheTop, Left, High);
				High = Right;
			}
		}
	}

	// The Base array of BoardData is now partitioned into an ordered sequence of small unsorted blocks.
	// Insertion sort will be used to sort the whole array, because it is faster when shifting over short distances.
	{
		BoardDataPtr *End = Base + Size - 1;
		BoardDataPtr *TempPointer = Base;
		register BoardDataPtr *RunPointer;
		BoardDataPtr *Threshold;

		Threshold = Base + SORT_TRANSITION_SIZE;
		if ( Threshold > End) Threshold = End;

		// Find largest element in first "Threshold" and place it at the beginning of the array.
		// This is the largest array element, and the operation speeds up insertion sort's inner loop.

		for ( RunPointer = TempPointer + 1; RunPointer <= Threshold; ++RunPointer ) if ( COMPARE_BOARD_DATA(RunPointer, TempPointer) ) TempPointer = RunPointer;

		if ( TempPointer != Base ) BOARD_DATA_SWAP(TempPointer, Base, Temp);

		// This is a modification of the GLIBC code that seems to be a shifting optimization.
		// Insertion sort, running from left-hand-side up to right-hand-side.
		// Everything to the left as we go through the array is sorted and the maximum insertion displacement will be SORT_TRANSITION_SIZE
		RunPointer = Base + 1;
		while ( ++RunPointer <= End ) {
			TempPointer = RunPointer - 1;
			// When "RunPointer" needs to be moved, set "TempPointer" to the insertion position.
			while ( COMPARE_BOARD_DATA(RunPointer, TempPointer) ) --TempPointer;
			++TempPointer;
			if ( TempPointer != RunPointer ) {
				// Save the element at position "RunPointer" into "Temp".
				Temp = *RunPointer;
				// Move the elements from the range ("TempPointer" up to just before "RunPointer"), one position towards the end of the array.
				memmove(TempPointer + 1, TempPointer, sizeof(BoardDataPtr)*(RunPointer - TempPointer));
				// Fill the insertion position at "TempPointer" with the "Temp" value.
				*TempPointer = Temp;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The Global array of pointers to arrays of "BoardDataPtr"s.  All of the associated "BoardData" with this array will thus never move around.
// The Main thread will allocate the space required to store the actual "BoardData".
// The thread identities and attributes are also defined here.
BoardDataPtr *WorkingBoardScoreTallies[NUMBER_OF_WORKER_THREADS];
pthread_t Threads[NUMBER_OF_WORKER_THREADS];
pthread_attr_t ThreadAttribute;
char ThreadBoardStringsToAnalyze[NUMBER_OF_WORKER_THREADS][BOARDS_PER_ROUND/NUMBER_OF_WORKER_THREADS][BOARD_STRING_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The POSIX thread inter-thread communication section.

// Two sets of inter-thread communication variables are defined below.  Each "condition variable" requires an associated "mutex".
// It is the responsibility of the programmer to ensure that the global variables connected to a "mutex" are only modified under a lock condition.

// Set one.
pthread_mutex_t StartWorkMutex;
pthread_cond_t StartWorkCondition;

// The "StartWorkCondition" sends a boolean flag to request the worker threads to "WorkOn" or terminate, and a value indicating the current "Round".
Bool WorkOn = FALSE;
// This set of variables will coordinate the dispersed use of the lexicon time stamps by the main thread during round 0, when 25x(SIZE_OF_CHARACTER_SET - 1) boards are evaluated to begin the following deviation rounds.
unsigned int Round = 0;
Bool HandOffRecievedByWorker = TRUE;
unsigned int HandOffThread = 0;
unsigned int TimeStampHandOff = 1;

// Set two.
pthread_mutex_t CompleteMutex;
pthread_cond_t CompleteCondition;

// The worker threads need to let the main thread know which one has just sent the "CompleteCondition" signal, so it can coordinate the correct data set.
// The main thread needs to let the worker threads know when it is waiting for a signal and this is communicated using "MainThreadWaiting".
Bool MainThreadWaiting = FALSE;
unsigned int TheCompletedBatch = BOGUS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// POSIX thread function, which ends up processing large batches of computation undisturbed.

// This Thread function will complete an entire round of board single-deviations and analysis when it recieves the "StartWorkCondition" broadcast.
// Then it will wait for the next round to be coordinated.
// It is also responsible for sorting the board analysis results.
void *ThreadForSetOfBoardsAnalysis(void *ThreadArgument){
	unsigned int ThreadIndex;
	char *ThisBoardStringTwoArray;
	char TempBoardString[BOARD_STRING_SIZE];
	char TheNewOffLimitSquareString[3];
	BoardPtr WorkingBoard = (Board*)malloc(sizeof(Board));
	unsigned int X, Y, Z;
	unsigned int OffLimitSquare;
	unsigned int OffLimitLetterIndex;
	unsigned int InsertionSpot;
	unsigned int CurrentStringIndex;
	unsigned int TheLocalTime = 0;

	ThreadIndex = (unsigned long int)UINT_MAX & (unsigned long int)ThreadArgument;
	ThisBoardStringTwoArray = &(ThreadBoardStringsToAnalyze[ThreadIndex][0][0]);

	//printf( "We are now inside of Thread |%d|\n", TaskIdentity );
	BoardDataPtr *WorkingBoardScoreTally = WorkingBoardScoreTallies[ThreadIndex];

	// Zero all of the time stamps for the words associated with this "ThreadIndex".  The stamps are 32-bit integers so for now they will only need to be zeroed once.  2^32 -1 = 4,294,967,295 represents a very deep search.
	memset(LexiconTimeStamps[ThreadIndex], 0, (TOTAL_WORDS_IN_LEXICON + 1)*sizeof(unsigned int));

	BoardInit(WorkingBoard);

	// Enter a waiting state for the "StartWorkCondition".
	pthread_mutex_lock(&StartWorkMutex);
	pthread_cond_wait(&StartWorkCondition, &StartWorkMutex);
	while ( TRUE ) {
		// It is necessary to unlock the "StartWorkMutex" before thread termination, so that the other worker threads can also terminate.
		if ( WorkOn == FALSE ) {
			pthread_mutex_unlock(&StartWorkMutex);
			pthread_exit(NULL);
		}
		// When the round is zero, we have to check if the main thread has used this thread's time stamps to evaluate all the seed board deviations.
		if ( Round == 0 ) {
			if ( HandOffRecievedByWorker == FALSE ) {
				// Indicate that the hand-off has now been made, and read the new value into "TheLocalTime".
				if ( HandOffThread == ThreadIndex ) {
					HandOffRecievedByWorker = TRUE;
					TheLocalTime = TimeStampHandOff;
					// Set "HandOffThread" to the next thread inline so that the main thread can adjust its local time value, after all the work is done.
					HandOffThread = (HandOffThread + 1)%NUMBER_OF_WORKER_THREADS;
				}
			}
		}
		pthread_mutex_unlock(&StartWorkMutex);
		// All of the work will be carried out here.

		// Fill "WorkingBoardScoreTally" with all single deviations of the boards located in "ThisBoardStringTwoArray".
		InsertionSpot = 0;
		for ( X = 0; X < BOARDS_PER_THREAD; X++ ) {
			strcpy(TempBoardString, &(ThisBoardStringTwoArray[X*BOARD_STRING_SIZE]));
			OffLimitSquare = TwoCharStringToInt(&(TempBoardString[SQUARE_COUNT]));
			for ( Y = 0; Y < SQUARE_COUNT; Y++ ) {
				if ( Y == OffLimitSquare ) continue;
				// Y will now represent the placement of the off limits Square so set it as such.
				ConvertSquareNumberToString( TheNewOffLimitSquareString, Y );
				TempBoardString[SQUARE_COUNT] = TheNewOffLimitSquareString[0];
				TempBoardString[SQUARE_COUNT + 1] = TheNewOffLimitSquareString[1];
				OffLimitLetterIndex = CHARACTER_LOCATIONS[TempBoardString[Y] - 'A'];
				for ( Z = 0; Z < SIZE_OF_CHARACTER_SET; Z++ ) {
					if ( Z == OffLimitLetterIndex ) continue;
					TempBoardString[Y] = CHARACTER_SET[Z];
					BOARD_DATA_SET_THE_BOARD_STRING(WorkingBoardScoreTally[InsertionSpot], TempBoardString);
					InsertionSpot += 1;
				}
				TempBoardString[Y] = CHARACTER_SET[OffLimitLetterIndex];
			}
		}

		// Evaluate all of the single deviation boards and store the scores into the BoardData in "WorkingBoardScoreTallly".
		for ( X = 0; X < LIST_SIZE; X++ ) {
			TheLocalTime += 1;
			BoardPopulate(WorkingBoard, BOARD_DATA_THE_BOARD_STRING(WorkingBoardScoreTally[X]));
			// Insert the board score into the "WorkingBoardScoreTally" array.
			BOARD_DATA_SET_THE_BOARD_SCORE(WorkingBoardScoreTally[X], BoardSquareWordDiscover(WorkingBoard, TheLocalTime, ThreadIndex));
		}

		// We are now going to use an explicit stack, optimized qsort to arrange "WorkingBoardScoreTally.
		BoardDataExplicitStackQuickSort(WorkingBoardScoreTally, LIST_SIZE, ThreadIndex);

		// Get a lock on "CompleteMutex" and make sure that the main thread is waiting, then set "TheCompletedBatch" to "ThreadIndex".  Set "MainThreadWaiting" to "FALSE".
		// If the main thread is not waiting, continue trying to get a lock on "CompleteMutex" unitl "MainThreadWaiting" is "TRUE".
		while ( TRUE ) {
			pthread_mutex_lock(&CompleteMutex);
			if ( MainThreadWaiting == TRUE ) {
				// While this thread still has a lock on the "CompleteMutex", set "MainThreadWaiting" to "FALSE", so that the next thread to maintain a lock will be the main thread.
				MainThreadWaiting = FALSE;
				break;
			}
			pthread_mutex_unlock(&CompleteMutex);
		}
		TheCompletedBatch = ThreadIndex;
		// Lock the "StartWorkMutex" before we send out the "CompleteCondition" signal.
		// This way, we can enter a waiting state for the next round before the main thread broadcasts the "StartWorkCondition".
		pthread_mutex_lock(&StartWorkMutex);
		//printf("Thread-%u: Completed Batch %d\n", ThreadIndex, Round);
		pthread_cond_signal(&CompleteCondition);
		pthread_mutex_unlock(&CompleteMutex);
		// Coordinate the hand-off of the current time to the main thread if we are on the final round.
		if ( Round == (ROUNDS - 1) ) {
			if ( HandOffRecievedByWorker == TRUE ) {
				// Send out this thread's "TheLocalTime" to the main thread.
				if ( HandOffThread == ThreadIndex ) {
					TimeStampHandOff = TheLocalTime;
				}
			}
			else exit(EXIT_FAILURE);
		}
		// Wait for the Main thread to send us the next "StartWorkCondition" broadcast.
		// Be sure to unlock the corresponding mutex immediately so that the other worker threads can exit their waiting state as well.
		pthread_cond_wait(&StartWorkCondition, &StartWorkMutex);
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main () {
	// for() loop counter variables.
	unsigned int X, Y, T, S;

	// Variables used in coordinating pthreads.
	long unsigned int Identity;
	void *Status;
	int ReturnCode;
	unsigned int InsertionSlot;
	unsigned int SendToThread;
	unsigned int TheReturn;

	// A string to use fgets() with at the end of the program for monitoring purposes.
	char ExitString[BOARD_STRING_SIZE];

	// The seed board selection process is beyond the scope of this program.
	// For now, choose seeds that are as different as possible and verify that they all produce the same results.
	char SeedBoard[BOARD_STRING_SIZE] = MASTER_SEED_BOARD;

	// The evaluation lists.
	char *TopEvaluationBoardList[EVALUATE_LIST_SIZE];
	unsigned int TopEvaluationBoardScores[EVALUATE_LIST_SIZE];

	// The Master List.
	char *MasterResultsBoardList[MASTER_LIST_SIZE];
	unsigned int MasterResultsBoardScores[MASTER_LIST_SIZE];

	// Holders used for the seed board single deviations before the deviation rounds begin.
	BoardPtr InitialWorkingBoard;
	unsigned int UseTheseTimeStamps = 0;
	char TemporaryBoardString[BOARD_STRING_SIZE];
	unsigned int TemporaryBoardScore;
	char SquareNumberString[3];
	char TheSeedLetter;
	unsigned int TheCurrentTime = 0;

	// These "MinBoardTrie"s will maintain information about the search so that new boards will continue to be evaluated.  This is an important construct to a search algorithm.
	MinBoardTriePtr CurrentBoardsConsideredThisRound;
	MinBoardTriePtr AllEvaluatedBoards = MinBoardTrieInit();
	MinBoardTriePtr ChosenSeedBoards = MinBoardTrieInit();
	MinBoardTriePtr WhatMadeTheMasterList = MinBoardTrieInit();

	// The ADTDAWG lexicon is stored inside of four files, and then read into three arrays for speed.  This is the case because the data structure is extremely small.
	FILE *PartOne = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "rb");
	FILE *PartTwo = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "rb");
	FILE *PartThree = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "rb");
	FILE *PartFour = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "rb");
	unsigned int SizeOfPartOne;
	unsigned long int SizeOfPartTwo;
	unsigned int SizeOfPartThree;
	unsigned int SizeOfPartFour;
	unsigned char TempReadIn;

	// The global variables required by the worker threads to do their work are allocated here.

	// Allocate the arrays of "BoardDataPtr"s.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) WorkingBoardScoreTallies[X] = (BoardDataPtr*)malloc(LIST_SIZE*sizeof(BoardDataPtr));
	// Allocate the actual "BoardData" that the arrays of pointers will point to.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) for ( Y = 0; Y < LIST_SIZE; Y++ ) (WorkingBoardScoreTallies[X])[Y] = (BoardData*)malloc(sizeof(BoardData));
	// Allocate the "NUMBER_OF_WORKER_THREADS" Qsort stacks.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) TheQsortStacks[X] = (QsortStackNode*)malloc(QSORT_STACK_SIZE*sizeof(QsortStackNode));
	// Allocate the explicit discovery stacks for each worker thread.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) TheDiscoveryStacks[X] = (DiscoveryStackNode*)malloc((DISCOVERY_STACK_SIZE)*sizeof(DiscoveryStackNode));
	// Allocate the set of lexicon time stamps as unsigned integers.
	for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) LexiconTimeStamps[X] = (unsigned int*)malloc((TOTAL_WORDS_IN_LEXICON + 1)*sizeof(unsigned int));

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
	for ( X = (SizeOfPartThree + 1); X <= SizeOfPartOne; X++ ) {
		if ( fread(&TempReadIn, 1, 1, PartFour) != 1 ) return 0;
		PartThreeArray[X] = TempReadIn;
	}

	// Close the four files.
	fclose(PartOne);
	fclose(PartTwo);
	fclose(PartThree);
	fclose(PartFour);

	// Print out the high level algorithm variables for this run of the DeepSearch.
	printf("ADTDAWG Read of Lexicon_14 is Complete.\n\n");
	printf("DoubleUp.c Variables - Chain Seeds |%d|, Single Deviation Rounds |%d|, Full Evaluations Per Round |%d|.\n\n", NUMBER_OF_SEEDS_TO_RUN, ROUNDS, BOARDS_PER_ROUND);

	// Populate the "InitialWorkingBoard" with the original seed board.
	InitialWorkingBoard = (Board*)malloc(sizeof(Board));
	BoardInit(InitialWorkingBoard);
	BoardPopulate(InitialWorkingBoard, SeedBoard);

	// Allocate and zero the master list.
	for ( X = 0; X < MASTER_LIST_SIZE; X++ ) {
		MasterResultsBoardList[X] = (char*)calloc(BOARD_STRING_SIZE, sizeof(char));
		MasterResultsBoardScores[X] = 0;
	}
	// Allocate the evaluation list.
	for ( X = 0; X < EVALUATE_LIST_SIZE; X++ ) {
		TopEvaluationBoardList[X] = (char*)calloc(BOARD_STRING_SIZE, sizeof(char));
	}

	// Initialize all of the thread related objects.
	pthread_t Threads[NUMBER_OF_WORKER_THREADS];
  pthread_attr_t attr;

	pthread_mutex_init(&CompleteMutex, NULL);
  pthread_cond_init (&CompleteCondition, NULL);

	pthread_mutex_init(&StartWorkMutex, NULL);
  pthread_cond_init (&StartWorkCondition, NULL);

  /* For portability, explicitly create threads in a joinable state */
  pthread_attr_init(&ThreadAttribute);
  pthread_attr_setdetachstate(&ThreadAttribute, PTHREAD_CREATE_JOINABLE);

	// Create all of the worker threads here.
	for ( Identity = 0; Identity < NUMBER_OF_WORKER_THREADS; Identity++ ) pthread_create(&Threads[Identity], &ThreadAttribute, ThreadForSetOfBoardsAnalysis, (void *)Identity);

	// Allow time for the worker threads to start up and wait for the signal that is going to be sent out soon.
	printf("Wait for 3 second for the %d worker threads to enter a waiting state.\n\n", NUMBER_OF_WORKER_THREADS);
	sleep(3);

	// The very first task is to insert the original seed board into the master list.
	TheCurrentTime += 1;
	TemporaryBoardScore = BoardSquareWordDiscover(InitialWorkingBoard, TheCurrentTime, UseTheseTimeStamps);
	MinBoardTrieAddBoard(WhatMadeTheMasterList, SeedBoard);
	InsertBoardStringIntoMasterList(MasterResultsBoardList, MasterResultsBoardScores, SeedBoard, TemporaryBoardScore);

	printf( "This is the original seed board that will be used...  It is worth |%d| points.  Sleep for 2 seconds to look at it\n\n", TemporaryBoardScore );
	BoardOutput( InitialWorkingBoard );
	sleep(2);


	// This loop represents the chain seeds cascade.
	for( S = 0; S < NUMBER_OF_SEEDS_TO_RUN; S++ ) {
		// The main thread will alternate its use of the time stamps so that the time stamps get equal use, and the most evaluation can be run on "NUMBER_OF_WORKER_THREADS"xMax(unsigned 32-bit int), without timestamp zeroing.
		UseTheseTimeStamps = S%NUMBER_OF_WORKER_THREADS;

		pthread_mutex_lock(&StartWorkMutex);
		// Inherit the "TimeStampHandOff" from the "UseTheseTimeStamps" worker thread.
		if ( HandOffRecievedByWorker == TRUE ) {
			// Pick up the "UseTheseTimeStamps" thread time value.
			if ( HandOffThread == UseTheseTimeStamps ) {
				TheCurrentTime = TimeStampHandOff;
			}
		}
		else exit(EXIT_FAILURE);
		pthread_mutex_unlock(&StartWorkMutex);

		// Before checking the "AllEvaluatedBoards" Trie, test if the score is high enough to make the list.
		// The scores attached to this list needs to be reset every time that we start a new seed, the important remaining list is the master list.
		memset(TopEvaluationBoardScores, 0, EVALUATE_LIST_SIZE*sizeof(unsigned int));

		for ( X = 0; X < MASTER_LIST_SIZE; X++ ) {
			if ( MinBoardTrieBoardSearch( ChosenSeedBoards, MasterResultsBoardList[X] ) == FALSE ) {
				strcpy( SeedBoard, MasterResultsBoardList[X] );
				TemporaryBoardScore = MasterResultsBoardScores[X];
				break;
			}
		}

		SeedBoard[SQUARE_COUNT] = '\0';
		printf( "For the |%d|'th run the seed board is |%s| worth |%d| points.\n", S + 1, SeedBoard, TemporaryBoardScore );
		MinBoardTrieAddBoard( ChosenSeedBoards, SeedBoard );
		MinBoardTrieAddBoard( AllEvaluatedBoards, SeedBoard );
		// Populate the evaluate list for the first round of boards based on the best solitary deviations of the current seed board.  Add these boards to the Evaluate and Master lists.  They Have not been fully evaluated yet.
		// These boards will not get evaluated in the threads, so evaluate them here.  Add them to the master list if they qualify.
		strcpy(TemporaryBoardString, SeedBoard);
		for ( X = 0; X < SQUARE_COUNT; X++ ) {
			if ( X > 0 ) TemporaryBoardString[X - 1] = SeedBoard[X - 1];
			ConvertSquareNumberToString(SquareNumberString, X);
			strcpy(TemporaryBoardString + SQUARE_COUNT, SquareNumberString);
			TheSeedLetter = SeedBoard[X];
			for ( Y = 0; Y < SIZE_OF_CHARACTER_SET; Y++ ) {
				// This statement indicates that less new boards are generated for each evaluation board, as in one square will be off limits.  This is how we arrive at the number "SOLITARY_DEVIATIONS".
				if ( TheSeedLetter == CHARACTER_SET[Y]) continue;
				TemporaryBoardString[X] = CHARACTER_SET[Y];
				BoardPopulate(InitialWorkingBoard, TemporaryBoardString);
				TheCurrentTime += 1;
				TemporaryBoardScore = BoardSquareWordDiscover(InitialWorkingBoard, TheCurrentTime, UseTheseTimeStamps);
				// Try to add each board to the "MasterResultsBoardList", and the "TopEvaluationBoardList".  Do this in sequence.  Only the "WhatMadeTheMasterList" MinBoardTrie will be augmented.
				if ( MinBoardTrieBoardSearch(WhatMadeTheMasterList, TemporaryBoardString) == FALSE ) {
					if ( InsertBoardStringIntoMasterList(MasterResultsBoardList, MasterResultsBoardScores, TemporaryBoardString, TemporaryBoardScore) == TRUE ) {
						MinBoardTrieAddBoard(WhatMadeTheMasterList, TemporaryBoardString);
						// printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", 0, TemporaryBoardString, TemporaryBoardScore);
					}
				}
				if ( MinBoardTrieBoardSearch(AllEvaluatedBoards, TemporaryBoardString) == FALSE ) InsertBoardStringIntoEvaluateList(TopEvaluationBoardList, TopEvaluationBoardScores, TemporaryBoardString, TemporaryBoardScore);
			}
		}

		// This Loop Represents the rounds cascade.
		for ( T = 0; T < ROUNDS; T++ ) {
			// Initiate a "MinBoardTrie" to keep track of the round returns.
			CurrentBoardsConsideredThisRound = MinBoardTrieInit();
			// Lock the "StartWorkMutex" to set the global work coordination variables.  Be sure to hand off "TheCurrentTime" to the right thread.
			pthread_mutex_lock(&StartWorkMutex);
			// Set the WorkOn to TRUE, the current Round, and "TheCurrentTime" HandOff to the right thread.
			WorkOn = TRUE;
			Round = T;
			if ( T == 0 ) {
				HandOffRecievedByWorker = FALSE;
				HandOffThread = UseTheseTimeStamps;
				TimeStampHandOff = TheCurrentTime;
			}
			// Here is where we have to transcripe the board strings in "TopEvaluationBoardList" into the global "ThreadBoardStringsToAnalyze".
			InsertionSlot = 0;
			for( X = 0; X < BOARDS_PER_ROUND; X++ ) {
					InsertionSlot = X/NUMBER_OF_WORKER_THREADS;
					SendToThread = X%NUMBER_OF_WORKER_THREADS;
					strcpy(&(ThreadBoardStringsToAnalyze[SendToThread][InsertionSlot][0]), TopEvaluationBoardList[X]);
			}
			// Now that the "TopEvaluationBoardList" has been sent over to the global array, add the board strings to the "AllEvaluatedBoards" trie.
			for ( X = 0; X < BOARDS_PER_ROUND; X++) {
				MinBoardTrieAddBoard(AllEvaluatedBoards, TopEvaluationBoardList[X]);
			}
			// The boards on the evaluate list in round zero have already been added to the master list.
			if ( T != 0 ) {
				for ( X = 0; X < EVALUATE_LIST_SIZE; X++ ) {
					if ( TopEvaluationBoardScores[X] > MasterResultsBoardScores[MASTER_LIST_SIZE - 1]) {
						if ( MinBoardTrieBoardSearch( WhatMadeTheMasterList, TopEvaluationBoardList[X] ) == FALSE ) {
							InsertBoardStringIntoMasterList(MasterResultsBoardList, MasterResultsBoardScores, TopEvaluationBoardList[X], TopEvaluationBoardScores[X]);
							MinBoardTrieAddBoard( WhatMadeTheMasterList, TopEvaluationBoardList[X] );
							// printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", T, TopEvaluationBoardList[X], TopEvaluationBoardScores[X]);
						}
					}
					// As soon as a board is reached that doesn't make the master list, get the fuck out of here.
					else break;
				}
			}
			// Even if nothing qualifies for the master list on this round, print out the best result for the round to keep track of the progress.
			printf( "\nRound|%d|, Best Board|%s|, Best Score|%d|\n", T, TopEvaluationBoardList[0], TopEvaluationBoardScores[0] );
			printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
			for ( X = 0; X < 10; X++ ) {
				printf("#%4d -|%5d|-|%s|\n", X + 1, MasterResultsBoardScores[X], MasterResultsBoardList[X]);
			}
			// Zero the scores on the evaluation board list so we can fill it with the next round boards.
			memset(TopEvaluationBoardScores, 0, EVALUATE_LIST_SIZE*sizeof(unsigned int));
			// The Work broadcast signal is now ready to be sent out to the worker threads.
			//printf("Main: Broadcast Signal To Start Batch |%d|\n", Round);
			// Lock the "CompleteMutex" so we can start waiting for completion before any of the worker threads finish their batch.
			pthread_mutex_lock(&CompleteMutex);
			pthread_cond_broadcast(&StartWorkCondition);
			pthread_mutex_unlock(&StartWorkMutex);

			// This construct is to recieve and coordinate the results as they come in, one by one.
			for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) {
				// Before entering a waiting state, set "MainThreadWaiting" to "TRUE" while we still have a lock on the "CompleteMutex".
				// Worker threads will be waiting for this condition to be met before sending "CompleteCondition" signals.
				MainThreadWaiting = TRUE;
				pthread_cond_wait(&CompleteCondition, &CompleteMutex);
				TheReturn = TheCompletedBatch;
				//printf("Main: Complete Signal Recieved From Thread-%d\n", TheCompletedBatch);
				// This is where partial work on the batch data coordination will happen.  All of the worker threads will have to finish before we can start the next batch.
				for ( Y = 0; Y < LIST_SIZE; Y++ ) {
					// Because the list is sorted, once we find a board that doesn't make this evaluation round, get the fuck out.
					if ( BOARD_DATA_THE_BOARD_SCORE((WorkingBoardScoreTallies[TheReturn])[Y]) > TopEvaluationBoardScores[EVALUATE_LIST_SIZE - 1] ) {
						if ( MinBoardTrieAddBoard(CurrentBoardsConsideredThisRound, BOARD_DATA_THE_BOARD_STRING((WorkingBoardScoreTallies[TheReturn])[Y])) == 1 ) {
							if ( MinBoardTrieBoardSearch(AllEvaluatedBoards, BOARD_DATA_THE_BOARD_STRING((WorkingBoardScoreTallies[TheReturn])[Y])) == FALSE ) {
								InsertBoardStringIntoEvaluateList(TopEvaluationBoardList, TopEvaluationBoardScores, BOARD_DATA_THE_BOARD_STRING((WorkingBoardScoreTallies[TheReturn])[Y]), BOARD_DATA_THE_BOARD_SCORE((WorkingBoardScoreTallies[TheReturn])[Y]));
							}
						}
					}
					else break;
				}

			}
			pthread_mutex_unlock(&CompleteMutex);

			// This Point represents the end of the current round so the current boards min trie needs to be freed.
			FreeMinBoardTrie( CurrentBoardsConsideredThisRound );
		}

		// Print to screen all of the new boards that qualified for the "MasterResultsBoardList" on the final round.
		for ( X = 0; X < EVALUATE_LIST_SIZE; X++ ) {
			if ( TopEvaluationBoardScores[X] > MasterResultsBoardScores[MASTER_LIST_SIZE - 1]) {
				if ( MinBoardTrieBoardSearch(WhatMadeTheMasterList, TopEvaluationBoardList[X]) == FALSE ) {
					InsertBoardStringIntoMasterList(MasterResultsBoardList, MasterResultsBoardScores, TopEvaluationBoardList[X], TopEvaluationBoardScores[X]);
					MinBoardTrieAddBoard(WhatMadeTheMasterList, TopEvaluationBoardList[X]);
					// printf("Round |%d|Pop - New On Master |%s| Score |%d|\n", T, TopEvaluationBoardList[X], TopEvaluationBoardScores[X]);
				}
			}
			// As soon as a board is reached that doesn't make the master list, get the fuck out of here.
			else break;
		}
		// Even if nothing qualifies for the master list on this round, print out the best result for the round to keep track of the progress.
		printf("\nRound|%d|, Best Board|%s|, Best Score|%d|\n", T, TopEvaluationBoardList[0], TopEvaluationBoardScores[0] );
		// The last round is now complete, so we have to get ready for the next seed.
		printf("\nThe Top 10 Off The Master List After Round |%d|.\n", T);
		for ( X = 0; X < 10; X++ ) {
			printf("#%4d -|%5d|-|%s|\n", X + 1, MasterResultsBoardScores[X], MasterResultsBoardList[X]);
		}
		printf("\n");

		printf( "At this point, |%d| boards have been placed on the evaluation queue, and have been singularly deviated.\n", MinBoardTrieSize ( AllEvaluatedBoards ) );

		// Print the current time and date.
		time_t now = time(NULL);
		struct tm *local = localtime(&now);
		printf("%02d-%02d-%04d %02d:%02d:%02d\n",
			   local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
			   local->tm_hour, local->tm_min, local->tm_sec);

		// Print out everything on the master results list after running each chain seed.
		printf( "\nThe Master List After Seed |%d|.\n", S + 1 );
		for ( X = 0; X < MASTER_LIST_SIZE; X++ ){
			printf( "#%4d -|%5d|-|%s|\n", X + 1, MasterResultsBoardScores[X], MasterResultsBoardList[X] );
		}
	}

	pthread_mutex_lock(&StartWorkMutex);
	// Set the GAME OVER condition.
	WorkOn = FALSE;
	printf("Main: Broadcast The Termination Signal\n");
	pthread_cond_broadcast(&StartWorkCondition);
	pthread_mutex_unlock(&StartWorkMutex);

  // Wait for all threads to complete, and then join with them.
  for ( X = 0; X < NUMBER_OF_WORKER_THREADS; X++ ) {
		pthread_join(Threads[X], NULL);
		printf("Main: Thread[%d] Has Been Joined And Terminated.\n", X);
	}

	// Produce a list of the boards used as seeds when done, and wait for the user to look at, and store the results if they want to.
	printf( "The boards used as seed boards are as follows:..\n" );
	MinBoardTrieOutput( ChosenSeedBoards );
	free( InitialWorkingBoard );
	FreeMinBoardTrie( AllEvaluatedBoards );
	FreeMinBoardTrie( ChosenSeedBoards );
	FreeMinBoardTrie( WhatMadeTheMasterList );
	printf( "Done... Press enter to exit...:");
	if ( fgets(ExitString, BOARD_STRING_SIZE - 1, stdin ) == NULL ) return 0;
	// Clean up and exit.
  pthread_attr_destroy(&ThreadAttribute);
  pthread_mutex_destroy(&CompleteMutex);
  pthread_cond_destroy(&CompleteCondition);
	pthread_mutex_destroy(&StartWorkMutex);
  pthread_cond_destroy(&StartWorkCondition);
  pthread_exit (NULL);
}

// This is a deterministic way to find the top 10 Boggle boards beyond a reasonable doubt.  This is one solution that works.  That is all.
