// The second part of the creation procedure is responsible for reording the lists, and deleting reduntant ones.

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define OPTIMAL_DIRECT_DAWG_DATA_PART_ONE "Optimal_Part_One_Direct_Dawg_For_Lexicon_14.dat"
#define OPTIMAL_DIRECT_DAWG_DATA_PART_TWO "Optimal_Part_Two_Direct_Dawg_For_Lexicon_14.dat"
#define END_OF_LIST_DATA "End_Of_List_Nodes_Direct_Dawg_For_Lexicon_14.dat"
#define DRUMPSTER "Numbered_Lexicon_14.txt"

#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_TEXT "Four_Part_DTDAWG_For_Lexicon_14.txt"

#define SIZE_OF_CHARACTER_SET 14
#define MAX_WORD_LENGTH 15
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15

typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

char CharacterSet[SIZE_OF_CHARACTER_SET] = {'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T'};
long int ChildLetterBitMasks[26] = {1, 0, 6, 24, 224, 0, 1792, 0, 14336, 0, 0, 114688, 1966080, 31457280, 503316480,
 8053063680, 0, 128849018880, 2061584302080, 32985348833280, 0, 0, 0, 0, 0, 0};
int ChildLetterBitShifts[26] = {0, -999, 1, 3, 5, -999, 8, -999, 11, -999, -999, 14, 17, 21, 25, 29, -999, 33, 37, 41, -999, -999, -999, -999, -999, -999};

FILE *WhatR;

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

// This function is used to make a text file for verification purposes.
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

void ExtractChildStringFromLongInt(long int TheOffset, char *TheChildren){
	int X;
	int FillThisSpot = 0;
	char CurrentChar;
	for ( X = 0; X < SIZE_OF_CHARACTER_SET; X++ ) {
		CurrentChar = CharacterSet[X];
		if ( TheOffset & ChildLetterBitMasks[CurrentChar - 'A'] ) {
			TheChildren[FillThisSpot] = CurrentChar;
			FillThisSpot += 1;
		}
	}
	TheChildren[FillThisSpot] = '\0';
}

int TraverseTheDawgArrayRecurse(int *TheDawg, long int *TheOffsets, int *SuckOnIt, int CurrentIndex, char *TheWordSoFar,
 int FillThisPosition, char CurrentLetter, int *WordCounter, Bool PrintMe){
	int X;
	long int CurrentOffset;
	int CurrentChild;
	int WhatsBelowMe = 0;
	TheWordSoFar[FillThisPosition] = CurrentLetter;
	if ( CurrentChild = (TheDawg[CurrentIndex] & CHILD_MASK) ) {
		for ( X = SIZE_OF_CHARACTER_SET - 1; X >= 0 ; X-- ) {
			// This assignment statement inside of a conditional statement seems very convoluted but all that it does is find a number between 0 and 14
			// using two numbers that need bit masking and bit shifting.
			if ( CurrentOffset = (TheOffsets[(TheDawg[CurrentIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT] & ChildLetterBitMasks[CharacterSet[X] - 'A']) ) {
				CurrentOffset >>= ChildLetterBitShifts[CharacterSet[X] - 'A'];
				CurrentOffset -= 1;
				WhatsBelowMe += TraverseTheDawgArrayRecurse(TheDawg, TheOffsets, SuckOnIt, CurrentChild + (int)CurrentOffset, TheWordSoFar, FillThisPosition + 1,
				CharacterSet[X], WordCounter, PrintMe);
			}
		}
	}
	if ( TheDawg[CurrentIndex] & END_OF_WORD_FLAG ) {
		*WordCounter +=1;
		TheWordSoFar[FillThisPosition+ 1] = '\0';
		if ( PrintMe == TRUE ) printf("#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
		//if ( PrintMe == TRUE ) fprintf(What, "#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
		if ( PrintMe == FALSE ) fprintf(WhatR, "#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
		WhatsBelowMe += 1;
	}
	SuckOnIt[CurrentIndex] = WhatsBelowMe;
	return WhatsBelowMe;
}

void TraverseTheDawgArray(int *TheDawg, long int *TheOffsets, int *BelowingMe, Bool PrintToScreen){
	int X;
	int TheCounter = 0;
	char RunningWord[MAX_WORD_LENGTH +1];
	for ( X = SIZE_OF_CHARACTER_SET - 1; X >= 0 ; X-- ) {
		TraverseTheDawgArrayRecurse(TheDawg, TheOffsets, BelowingMe, X + 1, RunningWord, 0, CharacterSet[X], &TheCounter, PrintToScreen);
	}
}

// The Governing Equation of The Four Part DTDAWG: NextMarker = CurrentMarker - FCWTEOBL + NCWTEOBL - Flag;  WTEOBL means Words to end of branch list.  
// FC means FirstChild.  NC means NextChild, the one we want to go to.
void TraverseTheFourPartDawgArrayRecurse(int *One, long int *Two, int *Three, int CurrentIndex, char *TheWordSoFar, int FillThisPosition, char CurrentLetter, int CurrentMarker){
	int X;
	long int CurrentOffset;
	int CurrentChild;
	int NextMarker;
	int CurrentNCWTEOBL;
	TheWordSoFar[FillThisPosition] = CurrentLetter;
	if ( One[CurrentIndex] & END_OF_WORD_FLAG ) {
		TheWordSoFar[FillThisPosition+ 1] = '\0';
		printf("#|%d| - |%s|\n", CurrentMarker, TheWordSoFar);
	}
	if ( CurrentChild = (One[CurrentIndex] & CHILD_MASK) ) {
		// Begin the calculation of the NextMarker; the part that is common to all children.
		NextMarker = CurrentMarker - Three[CurrentChild];
		if ( One[CurrentIndex] & END_OF_WORD_FLAG ) NextMarker -= 1;
		for ( X = 0; X < SIZE_OF_CHARACTER_SET ; X++ ) {
			// This assignment statement inside of an if statement seems very convoluted but all that it does, with the help of the next line,
			// is find a number between 0 and 14, using two numbers that need bit masking and bit shifting.
			if ( CurrentOffset = (Two[(One[CurrentIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT] & ChildLetterBitMasks[CharacterSet[X] - 'A']) ) {
				CurrentOffset >>= ChildLetterBitShifts[CharacterSet[X] - 'A'];
				CurrentOffset -= 1;
				CurrentNCWTEOBL = Three[CurrentChild + (int)CurrentOffset];
				NextMarker += CurrentNCWTEOBL;
				TraverseTheFourPartDawgArrayRecurse(One, Two, Three, CurrentChild + (int)CurrentOffset, TheWordSoFar, FillThisPosition + 1, CharacterSet[X], NextMarker);
				NextMarker -= CurrentNCWTEOBL;
			}
		}
	}
}

void TraverseTheFourPartDawgArray(int *First, long int *Second, int *Third){
	int X;
	char RunningWord[MAX_WORD_LENGTH +1];
	for ( X = 0; X < SIZE_OF_CHARACTER_SET ; X++ ) {
		TraverseTheFourPartDawgArrayRecurse(First, Second, Third, (X + 1), RunningWord, 0, CharacterSet[X], Third[X + 1]);
	}
}

int main(){
	int X;
	int Y;
	int NumberOfPartOneNodes;
	int NumberOfEndOfLists;
	long int NumberOfPartTwoNodes;
	int *PartOneArray;
	long int *PartTwoArray;
	int CurrentCount;
	int CurrentEndOfList;
	int FurthestBigNode = 0;
	WhatR = fopen(DRUMPSTER, "w");
	
	// We are going to need to know the number of words in and below each node, then we can compute number till the end of the child list which is of paramount importance in word tracking.
	// When we have calculated the NumberOfWordsToEndOfBranchList, the NumberOfWordsBelowMe can be calculated with max 1 subtraction.
	int *NumberOfWordsBelowMe;
	int *NumberOfWordsToEndOfBranchList;
	int *RearrangedNumberOfWordsToEndOfBranchList;
	
	// In order to calculate NumberOfWordsToEndOfBranchList we need to know the location of the closest EndOfList node.
	int* EndOfListLocations;
	
	FILE *PartOne;
	FILE *PartTwo;
	// The PartThree data file requires rearranging of the PartOne node data so that the most WordsToEndOfBranchList numbers can be housed in a single unsigned char.
	// Only lists with nodes containing a number greater then 255 will need more bits.
	// PartTwo will remain untouched.
	FILE *ListE;
	FILE *Text;
	
	PartOne = fopen(OPTIMAL_DIRECT_DAWG_DATA_PART_ONE, "rb");
	PartTwo = fopen(OPTIMAL_DIRECT_DAWG_DATA_PART_TWO, "rb");
	ListE = fopen(END_OF_LIST_DATA, "rb");
	Text = fopen(FOUR_PART_DTDAWG_14_TEXT, "w");
	
	fread(&NumberOfPartOneNodes, 4, 1, PartOne);
	fread(&NumberOfPartTwoNodes, 8, 1, PartTwo);
	fread(&NumberOfEndOfLists, 4, 1, ListE);
	
	PartOneArray = (int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	PartTwoArray = (long int *)malloc(NumberOfPartTwoNodes * sizeof(long int));
	NumberOfWordsBelowMe = (int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	EndOfListLocations = (int *)malloc(NumberOfEndOfLists * sizeof(int));
	NumberOfWordsToEndOfBranchList =(int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	RearrangedNumberOfWordsToEndOfBranchList =(int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	
	NumberOfWordsBelowMe[0] = 0;
	NumberOfWordsToEndOfBranchList[0] = 0;
	RearrangedNumberOfWordsToEndOfBranchList[0] = 0;
	PartOneArray[0] = 0;
	
	for ( X = 1; X <= NumberOfPartOneNodes; X++ ) {
		fread(&PartOneArray[X], 4, 1, PartOne);
	}
	
	for ( X = 0; X < NumberOfPartTwoNodes; X++ ) {
		fread(&PartTwoArray[X], 8, 1, PartTwo);
	}
	
	for ( X = 0; X < NumberOfEndOfLists; X++ ) {
		fread(&EndOfListLocations[X], 4, 1, ListE);
	}
	
	fclose(PartOne);
	fclose(PartTwo);
	fclose(ListE);
	
	// This function is run at this point in time to fill the NumberOfWordsBelowMe array.
	TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	CurrentEndOfList = 0;
	
	// This little piece of code compiles the NumberOfWordsToEndOfBranchList array.  The requirements are the NumberOfWordsBelowMe array and the EndOfListLocations array.
	for ( X = 1; X <= NumberOfPartOneNodes; X++ ) {
		CurrentCount = 0;
		for ( Y = X; Y <= EndOfListLocations[CurrentEndOfList]; Y++ ) {
			CurrentCount += NumberOfWordsBelowMe[Y];
		}
		NumberOfWordsToEndOfBranchList[X] = CurrentCount;
		//printf("Node|%d|, WordsBelowMe|%d|, WordsToEndOfBranch|%d|, EndOfBranch|%d|\n", X, NumberOfWordsBelowMe[X], NumberOfWordsToEndOfBranchList[X], EndOfListLocations[CurrentEndOfList]);
		if ( X ==  EndOfListLocations[CurrentEndOfList] )CurrentEndOfList +=1;
	}
	
	CurrentEndOfList = 0;
	
	// Now with preliminary analysis complete, it is time to rearrange the PartOne nodes and then set up PartThree.
	
	int ListSizeCounter[SIZE_OF_CHARACTER_SET + 1];
	int TotalNumberOfLists = 0;
	Bool AreWeInBigList = FALSE;
	int TheCurrentChild;
	int StartOfCurrentList = 1;
	int SizeOfCurrentList = EndOfListLocations[0];
	int EndOfCurrentList = EndOfListLocations[0];
	int InsertionPoint = 1;
	int CurrentlyCopyingThisList = 0;
	int *CurrentAdjustments;
	int *PartOneRearrangedArray;
	PartOneRearrangedArray = (int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	CurrentAdjustments = (int *)malloc((NumberOfPartOneNodes + 1) * sizeof(int));
	
	PartOneRearrangedArray[0] = 0;
	for ( X = 0; X <= NumberOfPartOneNodes; X++ ) CurrentAdjustments[X] = 0;
	
	for ( X = 0; X <= SIZE_OF_CHARACTER_SET; X++ ) ListSizeCounter[X] = 0;
	
	
	// This code is responsible for the rearrangement of the node lists inside of the DAWG int array.
	CurrentEndOfList = 0;
	for ( X = 1; X <= NumberOfPartOneNodes; X++ ) {
		if ( NumberOfWordsToEndOfBranchList[X] > 255 ) AreWeInBigList = TRUE;
		if ( X ==  EndOfCurrentList ) {
			ListSizeCounter[SizeOfCurrentList] += 1;
			// We are now at the end of a big list that must to be moved up to the InsertionPoint.  This also implies moving everything between its current location and its new one.
			if ( AreWeInBigList == TRUE ) {
				// First step is to copy the CurrentList into the new array at its correct position.
				for ( Y = 0; Y < SizeOfCurrentList; Y++ ) {
					PartOneRearrangedArray[InsertionPoint + Y] = PartOneArray[StartOfCurrentList + Y];
					RearrangedNumberOfWordsToEndOfBranchList[InsertionPoint + Y] = NumberOfWordsToEndOfBranchList[StartOfCurrentList + Y];
				}
				// The following steps are required when we are actually moving the position of a list.  The first set of lists will bypass these steps.
				if ( InsertionPoint !=  StartOfCurrentList ) {
					// Step 2 is to move all of the nodes between the original and final location, "SizeOfCurrentList" number of places back, starting from the end.
					for ( Y = EndOfCurrentList; Y >= (InsertionPoint + SizeOfCurrentList); Y-- ) {
						PartOneArray[Y] = PartOneArray[Y - SizeOfCurrentList];
						NumberOfWordsToEndOfBranchList[Y] = NumberOfWordsToEndOfBranchList[Y - SizeOfCurrentList];
					}
					// Step 3 is to copy the list we are moving up from the rearranged array back into the original.
					for ( Y = InsertionPoint; Y < (InsertionPoint + SizeOfCurrentList); Y++ ) {
						PartOneArray[Y] = PartOneRearrangedArray[Y];
						NumberOfWordsToEndOfBranchList[Y] = RearrangedNumberOfWordsToEndOfBranchList[Y];
					}
					// Step 4 is to fill the CurrentAdjustments array with the amount that each child references must be moved.  The two arrays are identical now up to the new insertion point.
					// At this stage, the CurrentAdjustments array is all zeros.
					for ( Y = 1; Y <= NumberOfPartOneNodes; Y++ ) {
						TheCurrentChild = (PartOneArray[Y] & CHILD_MASK);
						if ( (TheCurrentChild >= InsertionPoint) && (TheCurrentChild < StartOfCurrentList) ) CurrentAdjustments[Y] = SizeOfCurrentList;
						if ( (TheCurrentChild >= StartOfCurrentList) && (TheCurrentChild <= EndOfCurrentList) ) CurrentAdjustments[Y] = InsertionPoint - StartOfCurrentList;
					}
					// Step 5 is to fix all of the child reference values in both of the arrays.
					// Start with the rearranged array.
					for ( Y = 1; Y < (InsertionPoint + SizeOfCurrentList); Y++ ) {
						if ( CurrentAdjustments[Y] != 0 ) PartOneRearrangedArray[Y] += CurrentAdjustments[Y];
					}
					// Finish with the original array.  Make sure to zero all the values after the adjustments have been made to get ready for the next round.
					for ( Y = 1; Y <= NumberOfPartOneNodes; Y++ ) {
						if ( CurrentAdjustments[Y] != 0 ) {
							PartOneArray[Y] += CurrentAdjustments[Y];
							CurrentAdjustments[Y] = 0;
						}
					}
				}
				// Step 7 is to set the new InsertionPoint and change the EndOfListLocations, so that they reflect the shift.
				InsertionPoint += SizeOfCurrentList;
				// Shift all of the end of lists between the currently copying and the CurrentEndOfList.
				for ( Y = CurrentEndOfList; Y > CurrentlyCopyingThisList; Y-- ) {
					EndOfListLocations[Y] = EndOfListLocations[Y - 1] + SizeOfCurrentList;
				}
				EndOfListLocations[CurrentlyCopyingThisList] = InsertionPoint - 1;
				CurrentlyCopyingThisList += 1;
				
			}
			// Even when we are not in a big list, we still need to update the current list parameters.
			CurrentEndOfList +=1;
			SizeOfCurrentList = EndOfListLocations[CurrentEndOfList] - EndOfCurrentList;
			EndOfCurrentList = EndOfListLocations[CurrentEndOfList];
			StartOfCurrentList = X + 1;
			AreWeInBigList = FALSE;
		}
	}
	
	// Step 8 is to copy all of the small lists from the original array to the rearranged array.  All of the references should be properly adjusted at this point.
	for ( X = InsertionPoint; X <= NumberOfPartOneNodes; X++  ) {
		PartOneRearrangedArray[X] = PartOneArray[X];
		RearrangedNumberOfWordsToEndOfBranchList[X] = NumberOfWordsToEndOfBranchList[X];
	}
	
	// The rearrangement of the DAWG lists to reduce size of the PartThree data file is complete, so check if the new and old lists are identical, because they should be.
	for ( X = 1; X <= NumberOfPartOneNodes; X++  ) {
		if ( PartOneArray[X] != PartOneRearrangedArray[X] ) printf("What A Mistake!\n");
		assert(PartOneArray[X] == PartOneRearrangedArray[X]);
		assert(RearrangedNumberOfWordsToEndOfBranchList[X] == NumberOfWordsToEndOfBranchList[X]);
	}
	
	// The two arrays are now identical, so as a final precaution, traverse the rearranged array.
	TraverseTheDawgArray(PartOneRearrangedArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	// Check for duplicate lists.  It is now highly likely that there are duplicates
	
	printf("\n");
	
	// Add up the total number of lists.
	for ( X = 1; X <= SIZE_OF_CHARACTER_SET; X++ ) {
		TotalNumberOfLists += ListSizeCounter[X];
	}
	
	int **NodeListsBySize = (int **)malloc((SIZE_OF_CHARACTER_SET + 1) * sizeof(int *));
	int WhereWeAt[SIZE_OF_CHARACTER_SET + 1];
	for ( X = 0; X <= SIZE_OF_CHARACTER_SET; X++ ) WhereWeAt[X] = 0;
	
	for ( X = 1; X <= SIZE_OF_CHARACTER_SET; X++ ) {
		NodeListsBySize[X] = (int *)malloc(ListSizeCounter[X] * sizeof(int));
	}
	
	// We are now required to fill the NodeListsBySize array.  Simply copy over the correct EndOfList information.  
	// Note that the EndOfList information reflects the readjustment that just took place.
	
	CurrentEndOfList = 0;
	EndOfCurrentList = EndOfListLocations[0];
	SizeOfCurrentList = EndOfListLocations[0];
	for ( X = 0; X < NumberOfEndOfLists; X++ ) {
		(NodeListsBySize[SizeOfCurrentList])[WhereWeAt[SizeOfCurrentList]] = EndOfCurrentList;
		WhereWeAt[SizeOfCurrentList] += 1;
		CurrentEndOfList += 1;
		SizeOfCurrentList = EndOfListLocations[CurrentEndOfList] - EndOfCurrentList;
		EndOfCurrentList = EndOfListLocations[CurrentEndOfList];
	}
	
	int Z;
	int V;
	int W;
	int TheNewChild;
	int TotalNumberOfKilledLists = 0;
	int NewNumberOfKilledLists = -1;
	int InspectThisEndOfList;
	int MaybeReplaceWithThisEndOfList;
	int NumberOfListsKilledThisRound = -1;
	int CurrentNumberOfPartOneNodes = NumberOfPartOneNodes;
	Bool EliminateCurrentList = TRUE;
	
	// Keep attempting to kill lists until there are no more redundant lists. It turns out that a single run does the trick.
	
	//Without an explicit character member in each node, many of the lists are redundant, run a series of tests to exterminate these lists.
	while ( NumberOfListsKilledThisRound != 0 ) {
		printf("Run a redundant check:\n");
		NumberOfListsKilledThisRound = 0;
		for ( X = 1; X <= SIZE_OF_CHARACTER_SET; X++ ) {
			printf("Eliminate Lists of Size |%d|\n", X);
			while ( NewNumberOfKilledLists != 0 ) {
				NewNumberOfKilledLists = 0;
				for ( Y = 1; Y < ListSizeCounter[X]; Y++ ) {
					InspectThisEndOfList = (NodeListsBySize[X])[Y];
					for ( Z = 0; Z < Y; Z++ ) {
						MaybeReplaceWithThisEndOfList = (NodeListsBySize[X])[Z];
						for( W = 0; W < X; W++ ) {
							if ( PartOneArray[InspectThisEndOfList - W] != PartOneArray[MaybeReplaceWithThisEndOfList - W] ) {
								EliminateCurrentList = FALSE;
								break;
							}
						}
						// When eliminating a list, make sure to adjust the WTEOBL data.
						if ( EliminateCurrentList == TRUE ) {
							// Step 1 - replace all references to the duplicate list with the earlier equivalent.
							for( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
								TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
								if ( (TheCurrentChild > (InspectThisEndOfList - X)) && (TheCurrentChild <= InspectThisEndOfList) ) {
									TheNewChild = MaybeReplaceWithThisEndOfList - (InspectThisEndOfList - TheCurrentChild);
									PartOneArray[V] -= TheCurrentChild;
									PartOneArray[V] += TheNewChild;
								}
							}
							// Step 2 - eliminate the dupe list by moving the higher lists forward.
							for ( V = (InspectThisEndOfList - X + 1); V <= (CurrentNumberOfPartOneNodes - X); V++ ) {
								PartOneArray[V] = PartOneArray[V + X];
								RearrangedNumberOfWordsToEndOfBranchList[V] = RearrangedNumberOfWordsToEndOfBranchList[V + X];
							}
							// Step 3 - change CurrentNumberOfPartOneNodes.
							CurrentNumberOfPartOneNodes -= X;
							// Step 4 - lower all references to the nodes coming after the dupe list.
							for ( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
								TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
								if ( TheCurrentChild > InspectThisEndOfList ){
									PartOneArray[V] -= X;
								}
							}
							// Step 5 - readjust all of the lists after Y forward 1 and down X to the value, and lower ListSizeCounter[X] by 1.
							for( V = Y; V < (ListSizeCounter[X] - 1); V++ ) {
								(NodeListsBySize[X])[V] = (NodeListsBySize[X])[V + 1] - X;
							}
							ListSizeCounter[X] -= 1;
							// Step 6 - Lower any list, of any size, greater than (NodeListsBySize[X])[Y], down by X.
							for ( V = 1; V <= (X - 1); V++ ) {
								for ( W = 0; W < ListSizeCounter[V]; W++ ) {
									if ( (NodeListsBySize[V])[W] > InspectThisEndOfList ) (NodeListsBySize[V])[W] -= X;
								}
							}
							for ( V = (X + 1); V <= SIZE_OF_CHARACTER_SET; V++ ) {
								for ( W = 0; W < ListSizeCounter[V]; W++ ) {
									if ( (NodeListsBySize[V])[W] > InspectThisEndOfList ) (NodeListsBySize[V])[W] -= X;
								}
							}
							// Step 7 - lower Y by 1 and increase NewNumberOfKilledLists.
							Y -= 1;
							NewNumberOfKilledLists += 1;
							break;
						}
						EliminateCurrentList = TRUE;
					}
				}
				printf("NewNumberOfKilledLists |%d|.\n", NewNumberOfKilledLists);
				TotalNumberOfKilledLists += NewNumberOfKilledLists;
				NumberOfListsKilledThisRound += NewNumberOfKilledLists;
			}
			NewNumberOfKilledLists = -1;
		}
	}
	printf("\nOriginalNumberOfPartOneNodes|%d|\n", NumberOfPartOneNodes);
	printf("CurrentNumberOfPartOneNodes|%d|\n\n", CurrentNumberOfPartOneNodes);
	printf("The Total Number Of Killed Lists Is |%d|\n", TotalNumberOfKilledLists);
	printf("The Total Number Of Killed Nodes Is |%d|\n\n", NumberOfPartOneNodes - CurrentNumberOfPartOneNodes);
	
	TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	// Now that we have proved how a large number of additional nodes become redundant, set up and write the original,
	// the first, the only, 3 part direct tracking directed acyclic word graph.
	// Test it, write an anagram function, and finally a boggle scoring parallel method using 14 worker threads and a main thread, using OTS time stamp paradigm.
	// It is of critical importance that each thread is started only once during program execution.
	
	// RearrangedNumberOfWordsToEndOfBranchList has been kept up to date during the reworking of the DAWG
	// and there are CurrentNumberOfPartOneNodes meaningful value in the array.
	
	// EndOfListLocations needs to be recompiled from what is left from the NodeListsBySize arrays.
	
	TotalNumberOfLists = 0;
	for ( X = 1; X <= SIZE_OF_CHARACTER_SET; X++ ) {
		TotalNumberOfLists += ListSizeCounter[X];
		printf("List Size|%d| - Number Of Lists|%d|\n", X, ListSizeCounter[X]);
	}
	
	// Set all of the values that we are going to use to something very high.
	for ( X = 0; X < TotalNumberOfLists; X++ ) {
		EndOfListLocations[X] = 100000;
	}
	
	// Filter all of the living EndOfList values into the EndOfListLocations array.
	
	int TempValue;
	
	for ( X = SIZE_OF_CHARACTER_SET; X >= 1; X-- ) {
		for ( Y = 0; Y < ListSizeCounter[X]; Y++ ) {
			EndOfListLocations[TotalNumberOfLists - 1] = (NodeListsBySize[X])[Y];
			// The new list has been placed at the end of the list, now filter it up to where it should be.
			for ( Z = (TotalNumberOfLists - 1); Z > 0; Z-- ) {
				if ( EndOfListLocations[Z - 1] > EndOfListLocations[Z] ) {
					TempValue = EndOfListLocations[Z - 1];
					EndOfListLocations[Z - 1] = EndOfListLocations[Z];
					EndOfListLocations[Z] = TempValue;
				}
				else break;
			}
		}
	}
	
	// Test for logical errors in the list elimination procedure.
	for ( X = 0; X < (TotalNumberOfLists - 1); X++ ) {
		if ( EndOfListLocations[X] == EndOfListLocations[X + 1] ) printf("No Two Lists Can End On The Same Node. |%d|\n", EndOfListLocations[X]);
	}
	
	// Find out the final index number that requires an integer greater in size than a byte to hold it.  This will become the pivot point between part 3 and part 4.
	CurrentEndOfList = 0;
	FurthestBigNode = 0;
	int FirstSmallNode;
	for ( X = 1; X <= CurrentNumberOfPartOneNodes; X++ ) {if ( RearrangedNumberOfWordsToEndOfBranchList[X] > 255 ) FurthestBigNode = X;
		if ( X ==  EndOfListLocations[CurrentEndOfList] ) CurrentEndOfList += 1;
	}
	
	FirstSmallNode = FurthestBigNode + 1;
	
	printf("\nThe first node that requires only one byte for its WTEOBL is |%d|, it is the transition between PartThree And PartFour.\n\n", FirstSmallNode);
	
	//for ( X = 0; X < TotalNumberOfLists; X++ ) {
	//	printf("EOL |%d|- |%d|\n", X, EndOfListLocations[X]);
	//}
	TraverseTheFourPartDawgArray(PartOneArray, PartTwoArray, RearrangedNumberOfWordsToEndOfBranchList);
	
	// All of the information required to store the DTDAWG into data files has been compiled, so proceed to save the data to disk.
	
	FILE *FinalPartOneData = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "wb");
	FILE *FinalPartTwoData = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "wb");
	FILE *FinalPartThreeData = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "wb");
	FILE *FinalPartFourData = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "wb");
	
	char PartOneNodeInBinary[38];
	char PartTwoNodeInBinary[81];
	char ChildString[SIZE_OF_CHARACTER_SET + 1];
	
	// The final part one data is located in "PartOneArray" and has size "CurrentNumberOfPartOneNodes".  Remember that the 0 node is the NULL node, it will not be written to file.
	fwrite(&CurrentNumberOfPartOneNodes, 4, 1, FinalPartOneData);
	fprintf(Text, "Part One\n\n");
	for ( X = 1; X <= CurrentNumberOfPartOneNodes; X++ ) {
		fwrite(&(PartOneArray[X]), 4, 1, FinalPartOneData);
		ConvertIntNodeToBinaryString(PartOneArray[X], PartOneNodeInBinary);
		ExtractChildStringFromLongInt(PartTwoArray[(PartOneArray[X] & OFFSET_INDEX_MASK)>>15], ChildString);
		fprintf(Text, "1Node#|%d|-|%d|-Binary%s-Offsets|%d|-Child|%d|-Children|%s|\n", X, PartOneArray[X], PartOneNodeInBinary, (PartOneArray[X] & OFFSET_INDEX_MASK)>>15, (PartOneArray[X] & CHILD_MASK), ChildString);
	}
	// The final part two data has not changed at all, so simply copy the exact data to the new file.  This array has no NULL value so start at zero.
	fwrite(&NumberOfPartTwoNodes, 8, 1, FinalPartTwoData);
	fprintf(Text, "\nPart Two\n\n");
	for ( X = 0; X < NumberOfPartTwoNodes; X++ ) {
		fwrite(&(PartTwoArray[X]), 8, 1, FinalPartTwoData);
		ConvertOffsetLongIntToBinaryString(PartTwoArray[X], PartTwoNodeInBinary);
		ExtractChildStringFromLongInt(PartTwoArray[X], ChildString);
		fprintf(Text, "2Node#|%d|-|%ld|-Binary%s-ChildString|%s|\n", X, PartTwoArray[X], PartTwoNodeInBinary, ChildString);
	}
	// The final part three array consists of the first "FurthestBigNode" number of values in the 
	// "RearrangedNumberOfWordsToEndOfBranchList" array, and note that the zero value is null so do not include it.
	// Also these values should use 32 bit integers, or 4 bytes.
	fwrite(&FurthestBigNode, 4, 1, FinalPartThreeData);
	fprintf(Text, "\nPart Three\n\n");
	for ( X = 1; X <= FurthestBigNode; X++ ) {
		fwrite(&(RearrangedNumberOfWordsToEndOfBranchList[X]), 4, 1, FinalPartThreeData);
		fprintf(Text, "3Node#|%d| - WTEOBL|%d|\n", X, RearrangedNumberOfWordsToEndOfBranchList[X]);
	}
	// The final part four array consists of the [FirstSmallNode, CurrentNumberOfPartOneNodes] values in the "RearrangedNumberOfWordsToEndOfBranchList" array.
	int TheSizeOfPartFour = CurrentNumberOfPartOneNodes - FurthestBigNode;
	unsigned char TheCurrentPartFourNode;
	printf("TheSizeOfPartFour|%d|\n", TheSizeOfPartFour);
	fprintf(Text, "\nPart Four\n\n");
	for ( X = FirstSmallNode; X <= CurrentNumberOfPartOneNodes; X++ ) {
		assert(RearrangedNumberOfWordsToEndOfBranchList[X] < 255);
		TheCurrentPartFourNode = RearrangedNumberOfWordsToEndOfBranchList[X];
		assert(TheCurrentPartFourNode == RearrangedNumberOfWordsToEndOfBranchList[X]);
		fwrite(&TheCurrentPartFourNode, 1, 1, FinalPartFourData);
		fprintf(Text, "4Node#|%d| - For|%d| - WTEOBL|%d|\n", X - FirstSmallNode, X, TheCurrentPartFourNode);
	}
	
	printf("The four data files required for the 4 part DATDAWG have now been written.  Inspect the text verification files.\n\n");
	
	return 0;
}