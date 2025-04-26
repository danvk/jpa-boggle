#include <stdlib.h>
#include <stdio.h>

#include "const.h"
#include "min-board-trie.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This sector of the program will hold the minimal trie content.  No end of word flag is required because every board must be SQUARE_COUNT chars in length
// It is a reduced trie for holding board strings of identical size, that way boards already evaluated can be avoided for future evaluation.

MinBoardTnodePtr MinBoardTnodeNext( MinBoardTnodePtr ThisMinBoardTnode ) {
	return ThisMinBoardTnode->Next;
}

MinBoardTnodePtr MinBoardTnodeInit(char Chap, MinBoardTnodePtr OverOne){
	MinBoardTnodePtr Result = (MinBoardTnode*)malloc(sizeof(MinBoardTnode));
	Result->Letter = Chap;
	Result->Next = OverOne;
	Result->Child = NULL;
	return Result;
}

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
