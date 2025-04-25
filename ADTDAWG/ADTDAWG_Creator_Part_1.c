// Simply put, this program needs only 3 things to create an optimal 14 char DAWG for you.
// 1) A text file with a lexicon inside of it and the number of words is written on the very first line, in linux format, so no DOS CR.
// 2) Set the constant MAX below to the length of the longest word in the lexicon.  Modify if your shortest words are not of length 2.  Have fun.
// 3) Define the 14 characters that you have chosen, and that is it.

// The 14 Dawg data structure is important because each node will contain information about all of the child nodes below it.
// In this respect, traversal time will be reduced beyond any published DAWG implementation to my knowledge.

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#define MAX 15
#define NUMBER_OF_REDUCED_LETTERS 14
#define INPUT_LIMIT 30

// C requires a boolean variable type so use C's typedef concept to create one.
typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

// we will store the Dawg into a text file to verify the output.  Use The Init function this time
#define RAW_LEXICON "Lexicon_14.txt"
#define OPTIMAL_DAWG_DATA "Traditional_Dawg_For_Lexicon_14.dat"
#define OPTIMAL_DIRECT_DAWG_DATA_PART_ONE "Optimal_Part_One_Direct_Dawg_For_Lexicon_14.dat"
#define OPTIMAL_DIRECT_DAWG_DATA_PART_TWO "Optimal_Part_Two_Direct_Dawg_For_Lexicon_14.dat"
#define OPTIMAL_DAWG_TEXT_DATA "Traditional_Dawg_Text_For_Lexicon_14.txt"
#define END_OF_LIST_DATA "End_Of_List_Nodes_Direct_Dawg_For_Lexicon_14.dat"

int LetterArrayLocation[26] = {0, -999, 1, 2, 3, -999, 4, -999, 5, -999, -999, 6, 7, 8, 9, 10, -999, 11, 12, 13, -999, -999, -999, -999, -999, -999};
int BitShiftMe[26] = {0, -999, 1, 3, 5, -999, 8, -999, 11, -999, -999, 14, 17, 21, 25, 29, -999, 33, 37, 41, -999, -999, -999, -999, -999, -999};

inline void CutOffNewLine(char *String){
	String[strlen(String) - 1] = '\0';
}

// Returns the positive integer rerpresented by "TheNumberNotYet" string, and if not a number greater then zero, returns 0.
int StringToPositiveInt( char* TheNumberNotYet	) {
	int result = 0;
	int X;
	int Digits = strlen( TheNumberNotYet );
	for ( X = 0; X < Digits; X++ ) {
		printf("X =|%d|, TheNumberNotYet[X]=|%c|, result=|%d|, add|%d|\n", X, TheNumberNotYet[X], result, (int)((TheNumberNotYet[X] - '0')*pow( 10, Digits - X - 1 )));
		if ( !(TheNumberNotYet[X] >= '0' && TheNumberNotYet[X] <= '9') ) return 0;
		result += (int)((TheNumberNotYet[X] - '0')*pow( 10, Digits - X - 1 ));
	}
	printf("The result=|%d| is in.\n", result);
	return result;
}

// The BinaryNode string must be at least 32 + 5 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
void ConvertIntNodeToBinaryString( int TheNode, char* BinaryNode){
	int X;
	int Bit;
	BinaryNode[0] = '|';
	// Bit 31, the leftmost bit is the sign bit.
	BinaryNode[1] = (TheNode < 0)?'1':'0';
	// Bit 30 to bit 27 are unused bits in a 32 bit node.
	BinaryNode[2] = (TheNode & (int)pow(2,30))?'1':'0';
	BinaryNode[3] = (TheNode & (int)pow(2,29))?'1':'0';
	BinaryNode[4] = (TheNode & (int)pow(2,28))?'1':'0';
	BinaryNode[5] = (TheNode & (int)pow(2,27))?'1':'0';
	BinaryNode[6] = '|';
	// Bit 26 is the boolean EOW flag for end of word.
	BinaryNode[7] = (TheNode & (int)pow(2,26))?'1':'0';
	BinaryNode[8] = '|';
	// Bit 25 to bit 15 represent the child offset index.
	Bit = 25;
	for ( X = 9; X <= 19; X++ ) {
		BinaryNode[X] = (TheNode & (int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryNode[20] = '|';
	// The first child index that requires 15 bits will finish off the 32 bit int.
	Bit = 14;
	for ( X = 21; X <= 35; X++ ) {
		BinaryNode[X] = (TheNode & (int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryNode[36] = '|';
	BinaryNode[37] = '\0';
}

// The BinaryOffset string must be at least 64 + 16 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
void ConvertOffsetLongIntToBinaryString( long int TheOffset, char* BinaryOffset){
	int X;
	int Bit;
	BinaryOffset[0] = '|';
	// Bit 63, the leftmost bit is the sign bit.
	BinaryOffset[1] = (TheOffset < 0)?'1':'0';
	// Bit 62 to bit 45 are not used with the 14 char set chosen for this DAWG.  18 Unused bits.  All bits including the sign bit will be used with an 18 character set.
	Bit = 62;
	for ( X = 2; X <= 19; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[20] = '|';
	// The seven letters that require a 4 bit offset encoding start here ---------------------------------------------------
	// Bit [44,41] represent the "T" child.
	Bit = 44;
	for ( X = 21; X <= 24; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[25] = '|';
	// Bit [40,37] represent the "S" child.
	Bit = 40;
	for ( X = 26; X <= 29; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[30] = '|';
	// Bit [36,33] represent the "R" child.
	Bit = 36;
	for ( X = 31; X <= 34; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[35] = '|';
	// Bit [32,29] represent the "P" child.
	Bit = 32;
	for ( X = 36; X <= 39; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[40] = '|';
	// Bit [28,25] represent the "O" child.
	Bit = 28;
	for ( X = 41; X <= 44; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[45] = '|';
	// Bit [24,21] represent the "N" child.
	Bit = 24;
	for ( X = 46; X <= 49; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[50] = '|';
	// Bit [20,17] represent the "M" child.
	Bit = 20;
	for ( X = 51; X <= 54; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[55] = '|';
	// The four letters that require a 3 bit offset encoding start here ---------------------------------------------------
	// Bit [16,14] represent the "L" child.
	Bit = 16;
	for ( X = 56; X <= 58; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[59] = '|';
	// Bit [13,11] represent the "I" child.
	Bit = 13;
	for ( X = 60; X <= 62; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[63] = '|';
	// Bit [10,8] represent the "G" child.
	Bit = 10;
	for ( X = 64; X <= 66; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[67] = '|';
	// Bit [7,5] represent the "E" child.
	Bit = 7;
	for ( X = 68; X <= 70; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[71] = '|';
	// The two letters that require a 2 bit offset encoding start here ---------------------------------------------------
	// Bit [4,3] represent the "G" child.
	Bit = 4;
	for ( X = 72; X <= 73; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[74] = '|';
	// Bit [2,1] represent the "E" child.
	Bit = 2;
	for ( X = 75; X <= 76; X++ ) {
		BinaryOffset[X] = (TheOffset & (long int)pow(2,Bit))?'1':'0';
		Bit -= 1;
	}
	BinaryOffset[77] = '|';
	// Bit 0 represent the "A" child.
	BinaryOffset[78] = (TheOffset & (long int)pow(2,0))?'1':'0';
	BinaryOffset[79] = '|';
	BinaryOffset[80] = '\0';
}

/*This Function converts any lower case letters in the string "RawWord," into all capitals, so that the whole string is made of capital letters. */
void MakeMeAllCapital(char *RawWord){
	int count = 0;
	for ( count=0; count < strlen(RawWord); count++ ){
		if (RawWord[count] >= 97 && RawWord[count] <= 122 ) RawWord[count] = RawWord[count] - 32;
	}
}

/*Trie to Dawg TypeDefs*/
struct tnode {
	struct tnode* Next;
	struct tnode* Child;
	struct tnode* ParentalUnit;
	// When populating the array, you must know the indices of only the child nodes.  Hence we Store ArrayIndex in every node so that we can mine the information from the reduced trie.
	int ArrayIndex;
	char DirectChild;
	char Letter;
	char MaxChildDepth;
	char Level;
	char NumberOfChildren;
	char Dangling;
	char EndOfWordFlag;
};

typedef struct tnode Tnode;
typedef Tnode* TnodePtr;

int TnodeArrayIndex( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->ArrayIndex;
}

char TnodeDirectChild( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->DirectChild;
}

TnodePtr TnodeNext( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->Next;
}

TnodePtr TnodeChild ( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->Child;
}

TnodePtr TnodeParentalUnit( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->ParentalUnit;
}

char TnodeLetter( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->Letter;
}

char TnodeMaxChildDepth( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->MaxChildDepth;
}

char TnodeNumberOfChildren( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->NumberOfChildren;
}

char TnodeEndOfWordFlag( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->EndOfWordFlag;
}

char TnodeLevel ( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->Level;
}

char TnodeDangling ( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	return ThisTnode->Dangling;
}

TnodePtr TnodeInit( char Chap, TnodePtr OverOne, char WordEnding, char Leveler, int StarterDepth, TnodePtr Parent, char IsaChild ) {
	TnodePtr Result = (TnodePtr)malloc( sizeof(Tnode) );
	assert( Result != NULL );
	Result->Letter = Chap;
	Result->ArrayIndex = 0;
	Result->NumberOfChildren = 0;
	Result->MaxChildDepth = StarterDepth;
	Result->Next = OverOne;
	Result->Child = NULL;
	Result->ParentalUnit = Parent;
	Result->Dangling = FALSE;
	Result->EndOfWordFlag = WordEnding;
	Result->Level = Leveler;
	Result->DirectChild = IsaChild;
	return Result;
}

void TnodeOutput( TnodePtr ThisTnode ){
	assert( ThisTnode != NULL );
	printf( "Letter|%c|, DirectChild|%d|, ArrayIndex|%d|, Level|%d|, MaxChildDepth|%d|, Dangling|%d|, NumberOfChildren|%d|, EndOfWordFlag|%d|\n", ThisTnode->Letter, ThisTnode->DirectChild, ThisTnode->ArrayIndex, ThisTnode->Level, ThisTnode->MaxChildDepth, ThisTnode->Dangling, ThisTnode->NumberOfChildren, ThisTnode->EndOfWordFlag );
}

void TnodeSetArrayIndex( TnodePtr ThisTnode, int TheWhat ){
	assert( ThisTnode != NULL );
	ThisTnode->ArrayIndex = TheWhat;
}

void TnodeSetChild( TnodePtr ThisTnode, TnodePtr Nexis ){
	assert( ThisTnode != NULL );
	ThisTnode->Child = Nexis;
}
	
void TnodeSetNext ( TnodePtr ThisTnode, TnodePtr Nexi ){
	assert( ThisTnode != NULL );
	ThisTnode->Next = Nexi;
}

void TnodeSetParentalUnit ( TnodePtr ThisTnode, TnodePtr Parent ){
	assert( ThisTnode != NULL );
	ThisTnode->ParentalUnit = Parent;
}

void TnodeSetMaxChildDepth( TnodePtr ThisTnode, int NewDepth ){
	assert( ThisTnode != NULL );
	ThisTnode->MaxChildDepth = NewDepth;
}

void TnodeSetDirectChild( TnodePtr ThisTnode, char Status ){
	assert( ThisTnode != NULL );
	ThisTnode->DirectChild = Status;
}

void TnodeFlyEndOfWordFlag ( TnodePtr ThisTnode ){
	assert( ThisTnode != NULL );
	ThisTnode->EndOfWordFlag = TRUE;
}

// This function Dangles a node, but also recursively dangles every node under it as well, that way nodes that are not direct children do hit the chopping block.  The function returns the total number of nodes dangled as a result.
int TnodeDangle( TnodePtr ThisTnode ){
	assert( ThisTnode != NULL );
	if ( ThisTnode->Dangling == TRUE ) return 0;
	int Result = 0;
	if ( (ThisTnode->Next) != NULL ) Result += TnodeDangle( ThisTnode->Next );
	if ( (ThisTnode->Child) != NULL ) Result += TnodeDangle( ThisTnode->Child);
	ThisTnode->Dangling = TRUE;
	Result += 1;
	return Result;
}

// This function returns the pointer to the Tnode in a parallel list of nodes with the letter "ThisLetter", and returns NULL if the Tnode does not exist.  In that case an insertion is required.
TnodePtr TnodeFindParaNode( TnodePtr ThisTnode, char ThisLetter ) {
	if ( ThisTnode == NULL ) return NULL;
	TnodePtr Result = ThisTnode;
	if ( Result->Letter == ThisLetter ) return Result;
	while ( Result->Letter < ThisLetter ){
		Result = Result->Next;
		if ( Result == NULL ) return NULL;
	}
	if ( Result->Letter == ThisLetter ) return Result;
	else return NULL;
}

// This function inserts a new Tnode before a larger letter node or at the end of a para list If the list does not esist then it is put at the beginnung.  
// The new node has ThisLetter in it.  AboveTnode is the node 1 level above where the new node will be placed.
// This function should never be passed a NULL pointer.  ThisLetter should never exist in the next child para list.
void TnodeInsertParaNode( TnodePtr AboveTnode, char ThisLetter, char WordEnder, int StartDepth){
	assert( AboveTnode != NULL );
	AboveTnode->NumberOfChildren += 1;
	TnodePtr Holder = NULL;
	TnodePtr Currently = NULL;
	// Case 1: ParaList does not exist yet so start it.
	if ( AboveTnode->Child == NULL ) AboveTnode->Child = TnodeInit( ThisLetter, NULL, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, TRUE );
	// Case 2: ThisLetter should be the first in the ParaList.
	else if ( ((AboveTnode->Child)->Letter) > ThisLetter ) {
		Holder = AboveTnode->Child;
		// The holder node is no longer a direct child so set it as such.
		TnodeSetDirectChild( Holder, FALSE );
		AboveTnode->Child = TnodeInit( ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, TRUE );
		// The parent node needs to be changed on what used to be the child. it is the Tnode in "Holder".
		Holder->ParentalUnit = AboveTnode->Child;
	}
	// Case 3: The ParaList exists and ThisLetter is not first in the list.
	else if ( (AboveTnode->Child)->Letter < ThisLetter ) {
		Currently = AboveTnode->Child;
		while ( Currently->Next !=NULL ){
			if ( TnodeLetter( Currently->Next ) > ThisLetter ){
				break;
			}
			Currently = Currently->Next;
		}
		Holder = Currently->Next;
		Currently->Next = TnodeInit( ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, Currently, FALSE );
		if ( Holder != NULL ) Holder->ParentalUnit = Currently->Next;
	}
}

// The MaxChildDepth of the two nodes can not be assumed equal due to the recursive nature of this function, so we must check for equivalence.
char TnodeAreWeTheSame( TnodePtr FirstNode, TnodePtr SecondNode){
	if ( FirstNode == SecondNode ) return TRUE;
	if ( FirstNode == NULL || SecondNode == NULL ) return FALSE;
	if ( FirstNode->Letter != SecondNode->Letter ) return FALSE;
	if ( FirstNode->MaxChildDepth != SecondNode->MaxChildDepth ) return FALSE;
	if ( FirstNode->NumberOfChildren != SecondNode->NumberOfChildren ) return FALSE;
	if ( FirstNode->EndOfWordFlag != SecondNode->EndOfWordFlag ) return FALSE;
	if ( TnodeAreWeTheSame( FirstNode->Child, SecondNode->Child ) == FALSE ) return FALSE;
	if ( TnodeAreWeTheSame( FirstNode->Next, SecondNode->Next ) == FALSE ) return FALSE;
	else return TRUE;
}

struct dawg {
	int NumberOfTotalWords;
	int NumberOfTotalNodes;
	TnodePtr First;
};

typedef struct dawg Dawg;
typedef Dawg* DawgPtr;

// Set up the parent nodes in the Dawg.
DawgPtr DawgInit (void){
	DawgPtr Result;
	Result = (DawgPtr)malloc( sizeof(Dawg));
	assert( Result != NULL );
	Result->NumberOfTotalWords = 0;
	Result->NumberOfTotalNodes = 0;
	Result->First = TnodeInit( '0', NULL, FALSE, 0, 0, NULL, FALSE );
	return Result;
}

// This function simply makes DawgAddWord look a hell of a lot smaller.  It returns the number of new nodes that it inserted.
int TnodeDawgAddWord( TnodePtr ParentNode, const char *Word ){
	assert( ParentNode != NULL );
	int Result = 0;
	int X, Y = 0;
	int WordLength = strlen( Word );
	TnodePtr HangPoint = NULL;
	TnodePtr Current = ParentNode;
	for ( X = 0; X < WordLength; X++){
		HangPoint = TnodeFindParaNode ( TnodeChild( Current ), Word[X] );
		if ( HangPoint == NULL ) {
			TnodeInsertParaNode( Current, Word[X], (X == WordLength - 1 ? TRUE : FALSE), WordLength - X - 1 );
			Result++;
			Current = TnodeFindParaNode ( TnodeChild( Current ), Word[X] );;
			for ( Y = X+1; Y < WordLength; Y++ ){
				TnodeInsertParaNode( Current, Word[Y], (Y == WordLength - 1 ? TRUE : FALSE), WordLength - Y - 1 );
				Result += 1;
				Current = TnodeChild( Current );
			}
			break;
		}
		else {
			if ( TnodeMaxChildDepth( HangPoint ) < WordLength - X - 1 ) TnodeSetMaxChildDepth( HangPoint, WordLength - X - 1 );
		}
		Current = HangPoint;
		// The path for the word that we are trying to insert already exists, so just make sure that the end flag is flying on the last node.  This should never happen if we are to add words in increasing word length, but hey, better safe than sorry.
		if ( X == WordLength - 1 ) TnodeFlyEndOfWordFlag( Current );
	}
	return Result;
}

void DawgAddWord ( DawgPtr ThisDawg, char * NewWord ) {
	assert( ThisDawg != NULL );
	ThisDawg->NumberOfTotalWords += 1;
	int WordLength = strlen( NewWord );
	int X = 0;
	char Temp;
	int Inter = 0;
	Inter = TnodeDawgAddWord( ThisDawg->First, NewWord );
	ThisDawg->NumberOfTotalNodes += Inter;
}

// This is a standard depth first preorder tree traversal, whereby the objective is to count nodes of various MaxChildDepths to allow for an optimal reduction process.
void TnodeGraphTabulateRecurse ( TnodePtr ThisTnode, int Level, int* Tabulator ) {
	assert( ThisTnode != NULL );
	if ( Level == 0 ) TnodeGraphTabulateRecurse( TnodeChild( ThisTnode ), Level + 1, Tabulator );
	else{
		if ( ThisTnode->Dangling == FALSE ) {
			Tabulator[ThisTnode->MaxChildDepth] += 1;
			// Go Down if possible.
			if ( ThisTnode->Child != NULL ) TnodeGraphTabulateRecurse( TnodeChild( ThisTnode ), Level + 1, Tabulator );
			// Go Right if possible.
			if ( ThisTnode->Next != NULL && ThisTnode->Dangling == FALSE ) TnodeGraphTabulateRecurse( TnodeNext( ThisTnode ), Level, Tabulator );
		}
	}
}

// Count the nodes of each max child depth.
void DawgGraphTabulate( DawgPtr ThisDawg, int* Count ) {
	int Numbers[MAX];
	int X = 0;
	for ( X = 0; X < MAX; X++ ) Numbers[X] = 0;
	if ( ThisDawg->NumberOfTotalWords > 0){
		TnodeGraphTabulateRecurse( ThisDawg->First, 0, Numbers );
		for ( X = 0; X < MAX; X++ ) {
			Count[X] = Numbers[X];
		}
	}
}
// This function can never be called after a trie has been mowed because this will result in pointers being freed twice resulting in a core dump!
void FreeTnodeRecurse( TnodePtr ThisTnode ) {
	assert( ThisTnode != NULL );
	if ( ThisTnode->Child != NULL ) FreeTnodeRecurse( ThisTnode->Child ) ;
	if ( ThisTnode->Next != NULL ) FreeTnodeRecurse( ThisTnode->Next );
	free(ThisTnode);
}

// This function can never be called after a trie has been mowed because this will result in pointers being freed twice resulting in a core dump!
void FreeDawg( DawgPtr ThisDawg ) {
	if ( ThisDawg->NumberOfTotalWords > 0 ){
		FreeTnodeRecurse( ThisDawg->First );
	}
	free( ThisDawg );
}

// An important function, this function returns the first node that is identical to ThisTnode.
TnodePtr TnodeMexicanEquivalent( TnodePtr ThisTnode, TnodePtr ** MaxChildDepthWist ) {
	assert ( ThisTnode != NULL );
	int Tall = ThisTnode->MaxChildDepth;
	int X = 0;
	while ( TnodeAreWeTheSame( ThisTnode, MaxChildDepthWist[Tall][X] ) == FALSE ) {
		X += 1;
	}
	return MaxChildDepthWist[Tall][X];
}

// Recursively replaces all redundant nodes in a trie with their first equivalent.
void TnodeLawnMowerRecurse( TnodePtr ThisTnode, TnodePtr ** HeightWits ) {
	assert (ThisTnode != NULL);
	if ( ThisTnode->Level == 0 ) TnodeLawnMowerRecurse( ThisTnode->Child, HeightWits );
	else {
		if ( ThisTnode->Next == NULL && ThisTnode->Child == NULL ) goto AndImDone;
		if ( ThisTnode->Child != NULL ) {
			// we have found a node that has been tagged to be mowed, so let us destroy it not literally and replace it with its first equivelant in the "HeightWits" list, and it won't be tagged.
			if ( (ThisTnode->Child)->Dangling == TRUE ) {
				ThisTnode->Child = TnodeMexicanEquivalent( ThisTnode->Child, HeightWits );
			}
			else TnodeLawnMowerRecurse( ThisTnode->Child, HeightWits );
		}
		if ( ThisTnode->Next != NULL ){
			if ( (ThisTnode->Next)->Dangling == TRUE ) {
				ThisTnode->Next = TnodeMexicanEquivalent( ThisTnode->Next, HeightWits );
			}
			else TnodeLawnMowerRecurse( ThisTnode->Next, HeightWits );
		}
	}
	AndImDone:;
}

// Replaces all redundant nodes in a trie with teir first equivalent.
void DawgLawnMower( DawgPtr ThisDawg, TnodePtr ** HeightWise ) {
	TnodeLawnMowerRecurse( ThisDawg->First, HeightWise );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A queue is required for breadth first traversal, and the rest is self evident.

struct breadthqueuenode {
	TnodePtr Element;
	struct breadthqueuenode *Next;
};

typedef struct breadthqueuenode BreadthQueueNode;
typedef BreadthQueueNode* BreadthQueueNodePtr;

void BreadthQueueNodeSetNext( BreadthQueueNodePtr ThisBreadthQueueNode, BreadthQueueNodePtr Nexit ) {
		assert ( ThisBreadthQueueNode != NULL );
		ThisBreadthQueueNode->Next = Nexit;
}

BreadthQueueNodePtr BreadthQueueNodeNext( BreadthQueueNodePtr ThisBreadthQueueNode ) {
		assert ( ThisBreadthQueueNode != NULL );
		return ThisBreadthQueueNode->Next;
}

TnodePtr BreadthQueueNodeElement( BreadthQueueNodePtr ThisBreadthQueueNode ) {
		assert ( ThisBreadthQueueNode != NULL );
		return ThisBreadthQueueNode->Element;
}

BreadthQueueNodePtr BreadthQueueNodeInit( TnodePtr NewElement ) {
	BreadthQueueNodePtr Result = (BreadthQueueNodePtr)malloc( sizeof( BreadthQueueNode ) );
	assert( Result != NULL );
	Result->Element = NewElement;
	Result->Next = NULL;
	return Result;
}

struct breadthqueue {
	BreadthQueueNodePtr Front;
	BreadthQueueNodePtr Back;
	int Size;
};

typedef struct breadthqueue BreadthQueue;
typedef BreadthQueue* BreadthQueuePtr;

BreadthQueuePtr BreadthQueueInit( void ) {
	BreadthQueuePtr Result = (BreadthQueuePtr)malloc( sizeof( BreadthQueue ) );
	assert( Result != NULL );
	Result->Front = NULL;
	Result->Back = NULL;
	Result->Size = 0;
}

void BreadthQueuePush( BreadthQueuePtr ThisBreadthQueue, TnodePtr NewElemental ) {
	assert( ThisBreadthQueue != NULL );
	assert( NewElemental != NULL );
	BreadthQueueNodePtr Noob = BreadthQueueNodeInit( NewElemental );
	assert( Noob != NULL );
	if ( (ThisBreadthQueue->Back) != NULL ) BreadthQueueNodeSetNext( ThisBreadthQueue->Back, Noob );
	else ThisBreadthQueue->Front = Noob;
	ThisBreadthQueue->Back = Noob;
	(ThisBreadthQueue->Size) += 1;
}

TnodePtr BreadthQueuePop( BreadthQueuePtr ThisBreadthQueue ) {
	assert( ThisBreadthQueue != NULL );
	if ( ThisBreadthQueue->Size == 0 ) return NULL;
	if ( ThisBreadthQueue->Size == 1 ) {
		ThisBreadthQueue->Back = NULL;
		ThisBreadthQueue->Size = 0;
		TnodePtr Result = (ThisBreadthQueue->Front)->Element;
		free( ThisBreadthQueue->Front );
		ThisBreadthQueue->Front = NULL;
		return Result;
	}
	TnodePtr Result = (ThisBreadthQueue->Front)->Element;
	BreadthQueueNodePtr Holder = ThisBreadthQueue->Front;
	ThisBreadthQueue->Front = (ThisBreadthQueue->Front)->Next;
	free( Holder );
	ThisBreadthQueue->Size -= 1;
	return Result;
}

void BreadthQueuePopulateReductionArray( BreadthQueuePtr ThisBreadthQueue, TnodePtr Root, TnodePtr **Holder ) {
	printf( "inside external function.\n" );
	int Taker[MAX];
	int X = 0;
	for ( X = 0; X < MAX; X++ ) Taker[X] = 0;
	int PopCount = 0;
	int CurrentMaxChildDepth = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while ( Current != NULL ) {
		BreadthQueuePush( ThisBreadthQueue, Current );
		Current = Current->Next;
	}
	// Initiate the pop followed by push all children loop.
	while ( (ThisBreadthQueue->Size) != 0 ) {
		Current = BreadthQueuePop( ThisBreadthQueue );
		PopCount += 1;
		CurrentMaxChildDepth = Current->MaxChildDepth;
		Holder[CurrentMaxChildDepth][Taker[CurrentMaxChildDepth]] = Current;
		Taker[CurrentMaxChildDepth] += 1;
		Current = TnodeChild( Current );
		while ( Current != NULL ) {
			BreadthQueuePush( ThisBreadthQueue, Current );
			Current = TnodeNext( Current );
		}
	}
	printf( "Completed Populating The Reduction Array.\n" );
}

int BreadthQueueUseToIndex( BreadthQueuePtr ThisBreadthQueue, TnodePtr Root ) {
	int IndexNow = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while ( Current != NULL ) {
		BreadthQueuePush( ThisBreadthQueue, Current );
		Current = Current->Next;
	}
	// Pop each element off of the queue and only push its children if has not been dangled yet.  Assign index if one has not been given to it yet.
	while ( (ThisBreadthQueue->Size) != 0 ) {
		Current = BreadthQueuePop( ThisBreadthQueue );
		if ( TnodeDangling( Current ) == FALSE && TnodeArrayIndex( Current ) == 0 ) {
			IndexNow += 1;
			TnodeSetArrayIndex( Current, IndexNow );
			Current = TnodeChild( Current );
			while ( Current != NULL ) {
				BreadthQueuePush( ThisBreadthQueue, Current );
				Current = Current->Next;
			}
		}
	}
	printf( "Completed Assigning Indices To Living Nodes.\n" );
	return IndexNow;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Next and Child become indices.
struct arraydnode{
	int Next;
	int Child;
	char Letter;
	char EndOfWordFlag;
	char Level;
};

typedef struct arraydnode ArrayDnode;
typedef ArrayDnode* ArrayDnodePtr;

void ArrayDnodeInit( ArrayDnodePtr ThisArrayDnode, char Chap, int Nextt, int Childd, char EndingFlag, char Breadth ){
	ThisArrayDnode->Letter = Chap;
	ThisArrayDnode->EndOfWordFlag = EndingFlag;
	ThisArrayDnode->Next = Nextt;
	ThisArrayDnode->Child = Childd;
	ThisArrayDnode->Level = Breadth;
}

void ArrayDnodeTnodeTranspose( ArrayDnodePtr ThisArrayDnode, TnodePtr ThisTnode ){
	ThisArrayDnode->Letter = ThisTnode->Letter;
	ThisArrayDnode->EndOfWordFlag = ThisTnode->EndOfWordFlag;
	ThisArrayDnode->Level = ThisTnode->Level;
	if ( ThisTnode->Next == NULL ) ThisArrayDnode->Next = 0;
	else ThisArrayDnode->Next = (ThisTnode->Next)->ArrayIndex;
	if ( ThisTnode->Child == NULL ) ThisArrayDnode->Child = 0;
	else ThisArrayDnode->Child = (ThisTnode->Child)->ArrayIndex;
}

int ArrayDnodeNext( ArrayDnodePtr ThisArrayDnode ){
	return ThisArrayDnode->Next;
}

int ArrayDnodeChild (ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->Child;
}

char ArrayDnodeLetter( ArrayDnodePtr ThisArrayDnode ){
	return ThisArrayDnode->Letter;
}

char ArrayDnodeEndOfWordFlag (ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->EndOfWordFlag;
}

int ArrayDnodeNumberOfChildrenPlusString(ArrayDnodePtr DoggieDog, int Index, char* FillThisString){
	int X;
	int CurrentArrayPosition;
	
	if ( (DoggieDog[Index]).Child == 0 ) {
		FillThisString[0] = '\0';
		return 0;
	}
	CurrentArrayPosition = (DoggieDog[Index]).Child;
	for ( X = 0; X < NUMBER_OF_REDUCED_LETTERS; X++ ) {
		FillThisString[X] = (DoggieDog[CurrentArrayPosition]).Letter;
		if ( (DoggieDog[CurrentArrayPosition]).Next == 0 ) {
			FillThisString[X+1] = '\0';
			return (X + 1);
		}
		CurrentArrayPosition = CurrentArrayPosition + 1;
	}
}

struct arraydawg {
	int NumberOfStrings;
	ArrayDnodePtr DawgArray;
	int First;
	char MinStringLength;
	char MaxStringLength;
};

typedef struct arraydawg ArrayDawg;
typedef ArrayDawg* ArrayDawgPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// This function is the core of the dawg creation procedure.  Pay close attention to the order of the steps involved.

ArrayDawgPtr ArrayDawgInit( char **Dictionary, int *SegmentLenghts, int MaxStringLength ){

	int X = 0;
	int Y = 0;
	int *ChildCount;
	char *ChildStrings;
	

	
	
	
	
	
	printf("step 0 - Allocate the framework for the array data structure.\n");
	/// Dynamically allocate the upper Data Structure.
	ArrayDawgPtr Result = (ArrayDawgPtr)malloc( sizeof( ArrayDawg ) );
	/// set MinStringLength, MaxStringLength, and NumberOfStrings.
	
	X = 0;
	while ( SegmentLenghts[X] == 0 ) X++;
	Result->MinStringLength = X;
	Result->MaxStringLength = MaxStringLength;
	Result->NumberOfStrings = 0;
	for ( X = Result->MinStringLength; X <= Result->MaxStringLength ; X++ ) Result->NumberOfStrings += SegmentLenghts[X];

	printf("step 1 - Create a temporary trie and begin filling it with the provided lexicon.\n");
	/// Create a Temp Trie structure and then feed in the given dictionary.
	DawgPtr TemporaryTrie = DawgInit();
	for ( Y = Result->MinStringLength; Y <= Result->MaxStringLength; Y++ ){
		for ( X = 0; X < SegmentLenghts[Y]; X++ ){
			DawgAddWord ( TemporaryTrie, &(Dictionary[Y][(Y + 1)*X]) );
		}
	}

	printf("step 2 - Finished adding words to the temporary trie.\n");
	/// Create the associated pointer array with all the nodes inside it.
	int *NodeNumberCounter = (int*)malloc( (Result->MaxStringLength)*sizeof(int) );
	for ( X = 0; X < Result->MaxStringLength; X++ ) NodeNumberCounter[X] = 0;
	int *NodeNumberCounterInit = (int*)malloc( (Result->MaxStringLength)*sizeof(int) );
	for ( X = 0; X < Result->MaxStringLength; X++ ) NodeNumberCounterInit[X] = 0;
	// Pass a fake pointer because we haven't counted the nodes yet, after that, set NodeNumberCounterInit.  Pass FALSE kuz we are not filling the array yet.

	printf("step 3 - Count the total number of nodes in the raw trie by height.\n");
	DawgGraphTabulate( TemporaryTrie, NodeNumberCounter );
	
	printf("step 4 - Initial counting of trie nodes complete, so display results.\n");
	int TotalNodeSum = 0;
	for ( X = 0; X < Result->MaxStringLength; X++ ){
	NodeNumberCounterInit[X] = NodeNumberCounter[X];
		TotalNodeSum += NodeNumberCounter[X];
	}
	for ( X = 0; X < Result->MaxStringLength; X++ ){
		printf( "Initial Count For MaxChildDepth |%d| is |%d|\n", X, NodeNumberCounterInit[X] );
	}
	printf("The Total Number Of Nodes In The Trie = |%d| \n", TotalNodeSum);
	// We will have exactly enough space for all of the Tnode pointers.

	printf("step 5 - Allocate a 2 dimensional array of Trie node pointers to minimize the trie into an optimal DAWG.\n");
	TnodePtr ** HolderOfAllTnodePointers = (TnodePtr **)malloc( (Result->MaxStringLength)*sizeof( TnodePtr * ) );
	for ( X = 0; X < MAX; X++ ) HolderOfAllTnodePointers[X] = (TnodePtr *)malloc(NodeNumberCounterInit[X]*sizeof(TnodePtr));
	// When populating the array we are going to need to go about doing so in a breadth first manner to make sure that the first time that we encounter something it will come first in the array structure.  Or maybe this is unnecessary, because we have literally replaced all references to obsolete nodes.

	printf("step 6 - Populate the 2 dimensional trie node pointer array.\n");
	// Let us use a breadth first traversal to populate the HolderOfAllTnodePointers array so that the first occurance of a group of identical nodes becomes the one that survives.
	BreadthQueuePtr Populator = BreadthQueueInit();
	BreadthQueuePopulateReductionArray( Populator, (TemporaryTrie->First)->Child, HolderOfAllTnodePointers );

	printf("step 7 - Population complete.\n");
	/// Flag all of the reduntant nodes. Flagging requires the node comparison function that will take a very long time for a big dictionary especially when comparing the nodes with small MaxChildDepths because there are so many, so start at the top. 
	int NumberDangled = 0;
	int TotalDangled = 0;
	int W = 0;
	
	// keep track of the number of nodes of each MaxChildDepth dangled recursively so we can check how many remaining nodes we need for the optimal array.
	int DangleCount[Result->MaxStringLength];
	for ( X = 0; X < Result->MaxStringLength; X++) DangleCount[X] = 0;

	printf("step 8 - Begin to tag redundant nodes as dangled - Use recursion because only direct children are considered for elimination to reduce node size.\n");
	// Start at the largest MaxChildDepth and work down from there for recursive reduction to take place.
	for ( Y = (Result->MaxStringLength - 1); Y >= 0 ; Y--){
		NumberDangled = 0;
		// Move through the holder array looking for any nodes that have not been dangled.
		for ( W = 0; W < (NodeNumberCounterInit[Y] - 1); W++ ){
			// The Node is already Dangling.  Note the this node need not be the first in a child line.  In order to eliminate the need for the next index all identical reduced nodes only must be direct children only.
			if ( TnodeDangling( HolderOfAllTnodePointers[Y][W] ) == TRUE ) {
				continue;
			}
			// Traverse the rest of the array looking for equivalent nodes that are both not dangled and are tagged as direct children.  Then dangle when found to be identical.
			for ( X = W + 1; X < NodeNumberCounterInit[Y]; X++ ){
				if ( TnodeDangling( HolderOfAllTnodePointers[Y][X] ) == FALSE && TnodeDirectChild( HolderOfAllTnodePointers[Y][X] ) == TRUE ) {
					if ( TnodeAreWeTheSame( HolderOfAllTnodePointers[Y][W], HolderOfAllTnodePointers[Y][X] ) == TRUE ){
						NumberDangled += TnodeDangle( HolderOfAllTnodePointers[Y][X] );
					}
				}
			}
		}
		printf( "Dangled |%d| Nodes In all, through recursion, for MaxChildDepth of |%d|\n", NumberDangled, Y );
		DangleCount[Y] = NumberDangled;
		TotalDangled += NumberDangled;
	}
	printf( "Total Number Of Dangled Nodes |%d|\n", TotalDangled );
	int NumberOfLivingNodes = TotalNodeSum - TotalDangled;
	printf( "Total Number Of Living Nodes |%d|\n", NumberOfLivingNodes );

	printf("step 9 - Count the number of living nodes in the trie before replacement so that we check our numbers.\n");
	DawgGraphTabulate( TemporaryTrie, NodeNumberCounter );
	for ( X = 0; X < Result->MaxStringLength; X++ ){
		printf( "Count for living nodes of MaxChildDepth |%d| is |%d|. It used to be |%d| and so the number dangled is |%d| \n", X, NodeNumberCounter[X], NodeNumberCounterInit[X], NodeNumberCounterInit[X] - NodeNumberCounter[X] );
	}
	int TotalDangledCheck = 0;
	for ( X = 0; X < MAX; X++ ) {
		TotalDangledCheck += (NodeNumberCounterInit[X] - NodeNumberCounter[X]);
	}
	assert( TotalDangled == TotalDangledCheck );

	printf("step 10 - Dangling is complete, so replace all dangled nodes with their mexican equivelants in the Trie to make a distributed DAWG.\n");
	// Node replacement has to take place before indices are set up so nothing points to redundant nodes. - This step is absolutely critical.  Mow The Lawn so to speak!  Then Index.
	DawgLawnMower( TemporaryTrie, HolderOfAllTnodePointers );

	printf("step 11 - Mowing of the lawn is now complete, so start to assign array indices to all living nodes.\nstep 11 - The assigning of array indices is accomplished with a breadth first queue so that all nodes with a Next refrence not equal to NULL will point to the next element of the array, this is after all an optimal DAWG.\n");
	BreadthQueuePtr OrderMatters = BreadthQueueInit();
	// Try to find out if the nodes we are setting are unique before we set them.
	int IndexCount = BreadthQueueUseToIndex( OrderMatters, HolderOfAllTnodePointers[MAX - 1][0] );
	printf( "Finish\n" );
	printf( "NumberOfLivingNodes from after the dangling process|%d|\n", NumberOfLivingNodes );
	printf( "IndexCount from the index handing out breadth first traversal |%d|\n", IndexCount );

	// We are going to likely need a FIFO queue to do a breadth first traversal but know that this will attempt to add the same nodes multiple times unless we test for it.

	// Assigning the array indices the way I have been doing to does not allow for the elimination of the next index reference, so a new method, using the trie structure is necessary.
	// Then assign array indices to the living nodes.  Keep track of the total number of living nodes.  The living nodes are now supposed to point to only living nodes.

	/// Allocate the space needed to store the "DawgArray".
	Result->DawgArray = (ArrayDnodePtr)calloc( (NumberOfLivingNodes + 1), sizeof( ArrayDnode ) );
	int IndexFollow = 0;
	int IndexFollower = 0;
	/// Roll through the pointer arrays and use the "ArrayDnodeTnodeTranspose" function to populate it.
	// Set the dummy entry at the beginning of the array.
	ArrayDnodeInit( &(Result->DawgArray[0]), 0, 0, 0, 0, 0 );
	Result->First = 1;

	printf("step 12 - Populate the new array dawg structure with each node being as small as possible for optimization.\n");
	// Traverse the whole 2D pointer array and look for undangled nodes, if so then transpose that node into the optimal DAWG array.
	for ( X = Result->MaxStringLength - 1; X >= 0; X-- ){
		for (W = 0; W < NodeNumberCounterInit[X]; W++ ){
			if ( TnodeDangling( HolderOfAllTnodePointers[X][W] ) == FALSE ){
				IndexFollow = TnodeArrayIndex( HolderOfAllTnodePointers[X][W] );
				ArrayDnodeTnodeTranspose( &(Result->DawgArray[IndexFollow]), HolderOfAllTnodePointers[X][W] );
				if ( IndexFollow > IndexFollower ) IndexFollower = IndexFollow;
			}
		}
	}
	printf( "IndexFollower, which is the largest index assigned in the array = |%d|\n", IndexFollower );
	printf( "NumberOfLivingNodes|%d|, assert that these two are equal because they must be.\n", NumberOfLivingNodes );
	assert ( IndexFollower == NumberOfLivingNodes );
	
	/// Do Garbage cleanup and free the whole Trie, which is no longer needed.  So write a free function.  But never use it here because after mowing it is rendered useless.  Free from the holding array.
	for ( X = 0; X < Result->MaxStringLength; X++ ) for ( W = 0; W < NodeNumberCounterInit[X]; W++ ) free( HolderOfAllTnodePointers[X][W] );
	free( TemporaryTrie );
	free( NodeNumberCounter );
	free( NodeNumberCounterInit );
	for ( X = 0; X < Result->MaxStringLength; X++ ) free( HolderOfAllTnodePointers[X] );
	free( HolderOfAllTnodePointers );
	
	printf("step 13 - Creation of dawg is complete, so store the optimal DAWG into a text file for verification as well as 32 and 64 bit binary files for use.\n");
	FILE *Text;
	Text = fopen( OPTIMAL_DAWG_TEXT_DATA,"w" );
	
	FILE *Short;
	FILE *Long;
	Short = fopen( OPTIMAL_DIRECT_DAWG_DATA_PART_ONE,"wb" );
	Long = fopen( OPTIMAL_DIRECT_DAWG_DATA_PART_TWO,"wb" );
	
	// The following variables will be used when setting up the child-offset 64 bit nodes.
	int CurrentNumberOfChildren = 0;
	
	char CurrentChildLetterString[NUMBER_OF_REDUCED_LETTERS + 1];
	CurrentChildLetterString[0] = '\0';
	char TheNodeInBinary[32+5+1];
	char TheOffsetInBinary[64+16+1];
	TheNodeInBinary[0] = '\0';
	
	long int ChildOffsetNumber[NUMBER_OF_REDUCED_LETTERS];
	for ( X = 0; X < NUMBER_OF_REDUCED_LETTERS; X++ ) ChildOffsetNumber[X] = 0;
	long int CurrentOffsetNumber;
	long int TotalOffsetNumber = 0;
	int CurrentOffsetNumberIndex;
	int CompleteThirtyTwoBitNode;
	fwrite( &NumberOfLivingNodes, 4, 1, Short );
	
	FILE *ListE;
	int EndOfListCount = 0;
	int EOLTracker = 0;
	int *EndOfListIndicies;
	ListE = fopen ( END_OF_LIST_DATA, "wb" );
	
	
	// Set up an array to hold all of the unique child strings for the reduced lexicon DAWG.  The empty placeholder will be all zeds.
	long int NumberOfUniqueChildStrings = 0;
	int InsertionPoint = 0;
	Bool IsSheUnique = TRUE;
	char **HolderOfUniqueChildStrings = (char**)malloc(NumberOfLivingNodes * sizeof(char*));
	for ( X = 0; X < NumberOfLivingNodes; X++ ) {
		HolderOfUniqueChildStrings[X] = (char*)malloc((NUMBER_OF_REDUCED_LETTERS + 1) * sizeof(char));
		strcpy(HolderOfUniqueChildStrings[X], "ZZZZZZZZZZZZZZ");
	}
	
	
	
	// Right here we will tabulate the child information so that it can be turned into a long int array and stored in a data file.
	// Also, 45 bits allows for an optimal child offset format, but tabulate how many unique values these bits hold.  For TWL06 and the 14 chars chosen it is 1505, requiring 11 bits.
	// The idea is that there are a very small number of actual values that these 45 bits will hold due to their format and the prevalent patterns in the English Language.
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CurrentNumberOfChildren = ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, X, CurrentChildLetterString);
		
		// Insert the CurrentChildLetterString into the HolderOfUniqueChildStrings if, and only if, it is unique.
		for ( Y = 0; Y < NumberOfUniqueChildStrings; Y++ ) {
			if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) == 0 ) {
				IsSheUnique = FALSE;
				InsertionPoint = 0;
				break;
			}
			if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) < 0 ) {
				IsSheUnique = TRUE;
				InsertionPoint = Y;
				break;
			}
			IsSheUnique = TRUE;
			InsertionPoint = Y + 1;
		}
		
		if ( IsSheUnique == TRUE ) {
			for ( Y = NumberOfUniqueChildStrings; Y > InsertionPoint; Y-- ) {
				strcpy(HolderOfUniqueChildStrings[Y], HolderOfUniqueChildStrings[Y-1]);
			}
			strcpy(HolderOfUniqueChildStrings[InsertionPoint], CurrentChildLetterString);
			NumberOfUniqueChildStrings += 1;
		}
	}
	
	
	long int *ParallelChildOffsetValues = (long int*)malloc(NumberOfUniqueChildStrings*sizeof(long int));
	// Convert the unique child strings into the equivalent bitwise offset numbers, and store these values in a parallel array.
	
	for ( X = 0; X < NumberOfUniqueChildStrings ; X++ ){
		strcpy(CurrentChildLetterString, HolderOfUniqueChildStrings[X]);
		CurrentNumberOfChildren = strlen(CurrentChildLetterString);
		
		for ( Y = 0; Y < CurrentNumberOfChildren; Y++ ) {
			CurrentOffsetNumber = Y + 1;
			CurrentOffsetNumber <<= BitShiftMe[(CurrentChildLetterString[Y] - 'A')];
			ChildOffsetNumber[LetterArrayLocation[(CurrentChildLetterString[Y] - 'A')]] = CurrentOffsetNumber;
		}
		
		for ( Y = 0; Y < NUMBER_OF_REDUCED_LETTERS; Y++ ) {
			TotalOffsetNumber += ChildOffsetNumber[Y];
			ChildOffsetNumber[Y] = 0;
		}
		ParallelChildOffsetValues[X] = TotalOffsetNumber;
		TotalOffsetNumber = 0;
	}
	
	// We are now ready to output to the 32 bit node data file and the text file.
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CurrentNumberOfChildren = ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, X, CurrentChildLetterString);
		
		// Get the correct offset index to store into the current node
		for ( Y = 0; Y < NumberOfUniqueChildStrings; Y++ ) {
				if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) == 0 ) {
					CurrentOffsetNumberIndex = Y;
					break;
				}
		}
		
		CompleteThirtyTwoBitNode = CurrentOffsetNumberIndex;
		CompleteThirtyTwoBitNode <<= 15;
		CompleteThirtyTwoBitNode += (Result->DawgArray)[X].Child;
		// Bits (0, 14) are for the first child index.  Bits (15, 25) contain the offset index.  Bit 26 then represents the EOW flag.
		if ( (Result->DawgArray)[X].EndOfWordFlag == 1 ) CompleteThirtyTwoBitNode += 67108864;
		fwrite( &CompleteThirtyTwoBitNode, 4, 1, Short );
		ConvertIntNodeToBinaryString(CompleteThirtyTwoBitNode, TheNodeInBinary);
		ConvertOffsetLongIntToBinaryString(ParallelChildOffsetValues[CurrentOffsetNumberIndex], TheOffsetInBinary);
		fprintf( Text, "N%d-%s,Lev|%d|,{'%c',%d,%d,%d},Childs|%d|-|%s|,Raw|%d|,Offset%s-|%ld|.\n", X, TheNodeInBinary, (Result->DawgArray)[X].Level, (Result->DawgArray)[X].Letter, (Result->DawgArray)[X].EndOfWordFlag, (Result->DawgArray)[X].Next, (Result->DawgArray)[X].Child, CurrentNumberOfChildren, CurrentChildLetterString, CompleteThirtyTwoBitNode, TheOffsetInBinary, ParallelChildOffsetValues[CurrentOffsetNumberIndex] );
		if ( CompleteThirtyTwoBitNode == 0 ) printf("The Fuck You Do\n");
		
		TotalOffsetNumber = 0;
	}
	fclose(Short);
	
	
	fwrite( &NumberOfUniqueChildStrings, 8, 1, Long );
	
	for ( X = 0; X < NumberOfUniqueChildStrings; X++ ) {
		fwrite( &ParallelChildOffsetValues[X], 8, 1, Long );
	}
	fclose(Long);
	
	for ( X = 0; X < NumberOfUniqueChildStrings; X++ ) {
		printf("#%d - |%s| - |%ld| - \n", X, HolderOfUniqueChildStrings[X], ParallelChildOffsetValues[X] );
	}
	
	fprintf(Text, "Number Of Living Nodes |%d| Plus The NULL Node.  Also, there are %ld child offset long ints.\n\n", NumberOfLivingNodes, NumberOfUniqueChildStrings);
	
	for ( X = 0; X < NumberOfUniqueChildStrings; X++ ) {
		fprintf(Text, "#%d - |%s| - |%ld|\n", X, HolderOfUniqueChildStrings[X], ParallelChildOffsetValues[X] );
	}
	
	// free all of the memory used to compile the two part DAWG encoding.
	for ( X = 0; X < NumberOfLivingNodes; X++ ) {
		free(HolderOfUniqueChildStrings[X]);
	}
	free(HolderOfUniqueChildStrings);
	free(ParallelChildOffsetValues);
	
	printf( "\nThe Number Of Unique Child Strings Has Been Found to be |%ld|.\n\n", NumberOfUniqueChildStrings );
	
	
	for ( X = 1; X <= NumberOfLivingNodes; X++ ) {
		if ( (Result->DawgArray)[X].Next == 0 ){
			EndOfListCount += 1;
		}
	}
	EndOfListIndicies = (int*)malloc(EndOfListCount * sizeof(int));
	fwrite( &EndOfListCount, 4, 1, ListE );
	
	for ( X = 1; X <= NumberOfLivingNodes; X++ ) {
		if ( (Result->DawgArray)[X].Next == 0 ){
			EndOfListIndicies[EOLTracker] = X;
			EOLTracker += 1;
		}
	}	
	
	printf("EndOfListCount |%d|\n", EndOfListCount);
	
	for ( X = 0; X < EndOfListCount; X++ ) {
		fwrite( &EndOfListIndicies[X], 4, 1, ListE );
	}
	
	fclose(ListE);
	
	fprintf(Text, "\nEndOfListCount |%d|\n\n", EndOfListCount);
	
	for ( X = 0; X < EndOfListCount; X++ ) {
		fprintf(Text, "#%d - |%d|\n", X, EndOfListIndicies[X]);
	}
	
	fclose(Text);
	
	printf( "Out of text, 32 bit, and 64 bit output to file clean.\n" );
	
	FILE *Data;
	Data = fopen( OPTIMAL_DAWG_DATA,"wb" );
	int CurrentNodeInteger = NumberOfLivingNodes;
	// It is critical especially in a binary file that the first integer written to the file be the number of nodes stored in the file.
	fwrite( &CurrentNodeInteger, 4, 1, Data );
	// Write the NULL node to the file first.
	CurrentNodeInteger = 0;
	fwrite( &CurrentNodeInteger, 4, 1, Data );
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CurrentNodeInteger = (Result->DawgArray)[X].Child;
		CurrentNodeInteger <<= 5;
		CurrentNodeInteger += ((Result->DawgArray)[X].Letter) - 'A';
		if ( (Result->DawgArray)[X].EndOfWordFlag == TRUE ) CurrentNodeInteger += 8388608;
		if ( (Result->DawgArray)[X].Next == 0 ) CurrentNodeInteger += 4194304;
		fwrite( &CurrentNodeInteger, 4, 1, Data );
	}
	fclose(Data);
	printf( "Out of 32 bit traditional data output to file clean.\n" );
	return Result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char ArrayDnodeWordSearchRecurse( ArrayDnodePtr DoggieDog, int Index, char *PotWord ){
	if ( Index == 0 ) return FALSE;
	if ( (DoggieDog[Index].Letter) == PotWord[0] ){
		printf( "Found first letter |%c| in |%s|\n", PotWord[0], PotWord );
		if ( PotWord[1] == '\0' && DoggieDog[Index].EndOfWordFlag == TRUE ) return TRUE;
	else {
		return ArrayDnodeWordSearchRecurse( DoggieDog, DoggieDog[Index].Child, &(PotWord[1]) );
		printf( "Not quite a match yet, continue.\n" );
	}
	}
	else {
		printf( "|%c| in dawg does not equal |%c| in the word\n", DoggieDog[Index].Letter, PotWord[0] );
		if ( (DoggieDog[Index].Letter) > PotWord[0] ){
			printf( "|%c| is past the letter we need |%c|\n", DoggieDog[Index].Letter, PotWord[0] );
		return FALSE;
	}
	else{
			printf("Keep looking down the line\n");
		return ArrayDnodeWordSearchRecurse( DoggieDog, DoggieDog[Index].Next, PotWord );
	}
	}
}

char ArrayDawgWordSearch( ArrayDawgPtr ThisArrayDawg, char *PotWord ){
	printf("in\n");
	return ArrayDnodeWordSearchRecurse( ThisArrayDawg->DawgArray, 1, PotWord );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main() {
	printf( "The main function lives\n" );
	int X = 0;
	// All of the words of similar length will be stored sequentially in the same array so that there will be (MAX + 1)  arrays in total.  The Smallest length of a string is assumed to be 2.
	char *AllWordsInEnglish[MAX + 1];
	for ( X = 0; X < (MAX + 1); X++ ) AllWordsInEnglish[X] = NULL;
	
	FILE *Input;
	Input = fopen( RAW_LEXICON,"r" );
	char ThisLine[100] = "\0";
	int FirstLineIsSize = 0;
	int LineLengthWithNl = 0;
	int LineLength = 0;
	fgets( ThisLine, 100, Input );
	// Correction is needed to get rid of the new line character that fgets appends to the word.
	LineLengthWithNl = strlen( ThisLine );
	LineLength = LineLengthWithNl - 1;
	ThisLine[LineLength] = '\0';
	FirstLineIsSize = StringToPositiveInt( ThisLine );
	
	printf( "FirstLineIsSize is |%d|\n", FirstLineIsSize );
	int KeepTracker[MAX + 1];
	for ( X = 0; X <= MAX; X++ ) KeepTracker[X] = 0;
	char **LexiconInRam = (char**)malloc( FirstLineIsSize*sizeof( char* ) ); 
	
	// The first line gives us the number of words so read in all of them to ram temporarily.
	for ( X = 0; X < FirstLineIsSize; X++ ) {
		fgets( ThisLine, 100, Input );
		LineLengthWithNl = strlen( ThisLine );
		LineLength = LineLengthWithNl - 1;
		ThisLine[LineLength] = '\0';
		//if ( LineLength <= MAX ) {
			if ( LineLength <= MAX ) KeepTracker[LineLength] += 1;
			LexiconInRam[X] = (char*)malloc( (LineLength + 1)*sizeof( char ) );
			strcpy( LexiconInRam[X], ThisLine );
		//}
	}
	printf( "Ram write complete\n" );
	for ( X = 0; X < (MAX + 1); X++ ) printf( "There are |%d| words of length |%d|\n", KeepTracker[X], X );
	// Allocate enough space to hold all of the words in strings so that we can add them to the trie by length.
	for ( X = 2; X < (MAX + 1); X++ ) AllWordsInEnglish[X] = (char*)malloc( (X + 1)*KeepTracker[X]*sizeof( char ) );
	printf( "Initial malloc() complete\n" );
	
	int CurrentTracker[MAX + 1];
	for ( X = 0; X < (MAX + 1); X++ ) CurrentTracker[X] = 0;
	int CurrentLength = 0;
	// Copy all of the strings into the halfway house 1.
	for ( X = 0; X < FirstLineIsSize; X++ ) {
		CurrentLength = strlen( LexiconInRam[X] );
		//printf("|%s| - |%d|\n", LexiconInRam[X], CurrentLength );
		// As convoluted as this command might seem, it simply copies a string from its temporary ram location to the array of length equivelant strings for processing in making the DAWG.
		if ( CurrentLength <= MAX ) strcpy( &((AllWordsInEnglish[CurrentLength])[(CurrentTracker[CurrentLength]*(CurrentLength + 1))]), LexiconInRam[X] );
		CurrentTracker[CurrentLength] += 1;
	}
	printf( "halfway house copy complete\n" );
	// Make sure that the counting has resulted in all of the strings being placed correctly.
	for ( X = 0; X < (MAX + 1); X++ ) assert( KeepTracker[X] == CurrentTracker[X] );
	
	// Free the initial ram read space
	for ( X = 0; X < FirstLineIsSize; X++ ) free( LexiconInRam[X] );
	free( LexiconInRam );
	
	clock_t start_clock = clock();
	
	int DictionarySizeIndex[MAX + 1];
	for ( X = 0; X <= MAX; X++ ) DictionarySizeIndex[X] = KeepTracker[X];
	
	
	printf( "I will now start the init function.\n" );
	ArrayDawgPtr Adoggy = ArrayDawgInit( AllWordsInEnglish, DictionarySizeIndex, MAX );
	printf( "The Init is over, so move on to a traditional DAWG word search.  This will clearly demonstrate the list-based DAWG's shortcomings.\n" );
	
	// Use an initial word to get into the search.
	char Query[INPUT_LIMIT +1] = "AGE";
	
	while ( strlen(Query) > 1 ){
		MakeMeAllCapital( Query );
		if ( ArrayDawgWordSearch( Adoggy, Query ) == TRUE ) printf( "\nIIIIIIIIII - Found |%s|\n\n", Query );
		else printf( "\nXXXXXXXXXX - |%s| NOT FOUND\n\n", Query );
		printf( "Enter the word that you want to look for...(<2 letters to exit):" );
		fgets(Query, INPUT_LIMIT, stdin);
		CutOffNewLine(Query);
	}
	char Outer[INPUT_LIMIT +1];
	printf( "Processor Time Used: %g sec... --press enter to exit--", (clock() - start_clock) / (double) CLOCKS_PER_SEC );
	fgets(Outer, INPUT_LIMIT, stdin);
	return 0;
}
