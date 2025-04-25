#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"

#define SIZE_OF_CHARACTER_SET 14
#define MAX_WORD_LENGTH 15
#define MIN_WORD_LENGTH 3
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15
#define MAX_INPUT_SIZE 100
#define LOWERIT 32
#define BOGUS -99
#define TOTAL_WORDS_IN_LEXICON 44220
#define NAME_OF_LEXICON "Lexicon_14"

typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

char CharacterSet[SIZE_OF_CHARACTER_SET] = {'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T'};
char CharacterLocations[26] = {0, BOGUS, 1, 2, 3, BOGUS, 4, BOGUS, 5, BOGUS, BOGUS, 6, 7, 8, 9, 10, BOGUS, 11, 12, 13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};
long int ChildLetterBitMasks[26] = {1, 0, 6, 24, 224, 0, 1792, 0, 14336, 0, 0, 114688, 1966080, 31457280, 503316480, 8053063680, 0, 128849018880, 2061584302080, 32985348833280, 0, 0, 0, 0, 0, 0};
int ChildLetterBitShifts[26] = {0, BOGUS, 1, 3, 5, BOGUS, 8, BOGUS, 11, BOGUS, BOGUS, 14, 17, 21, 25, 29, BOGUS, 33, 37, 41, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};


// This function converts any lower case letters in the string "RawWord," into all capitals, so that the whole string is made of capital letters.
// It is useful for when a user enters a string to be anagrammed.
void MakeMeAllCapital(char* RawWord){
	int X;
	for( X = 0; X < strlen( RawWord ); X++ ) {
		if( RawWord[X] >= 'a' && RawWord[X] <= 'z' ) RawWord[X] = RawWord[X] - LOWERIT;
	}
}

// This function tests for validity of "ThisWord" for potential inclusion in the lexicon.
Bool IsThisWordValid(char *ThisWord){
	int X;
	int ThisLength = strlen(ThisWord);
	if ( ThisLength < MIN_WORD_LENGTH ) return FALSE;
	if ( ThisLength > MAX_WORD_LENGTH ) return FALSE;
	for ( X = 0; X < ThisLength; X++ ) {
		if ( ThisWord[X] < 'A' || ThisWord[X] > 'Z') return FALSE;
		if ( CharacterLocations[ThisWord[X] - 'A'] == BOGUS) return FALSE;
	}
	return TRUE;
}

inline int WordsToEndOfBranchList(int *ThirdPart, unsigned char *FourthPart, int TheIndexInQuestion, int TheCutoff){
	return (TheIndexInQuestion < TheCutoff)? ThirdPart[TheIndexInQuestion]: (int)FourthPart[TheIndexInQuestion - TheCutoff];
}

// This function returns the lexicon position of "TheWord" if found, and BOGUS otherwise.
int TrackingWordSearchWithFourPartDawgArrayRecurse(const char *TheWord, int *One, long int *Two, int *Three, unsigned char *Four
, int BeginFourAt, int CurrentIndex, int TheCurrentPosition, char CurrentLetter, int CurrentMarker, int SizeOfTheWord, int *StampingSet, int CurrentTime){
	long int CurrentOffset;
	int CurrentChild;
	int NextMarker = 0;
	
	if ( One[CurrentIndex] & END_OF_WORD_FLAG ) {
		if ( (TheCurrentPosition + 1) == SizeOfTheWord ) {
			printf("Found |%s| At Position |%d|.  Old Stamp |%d| - New Stamp |%d|\n", TheWord, CurrentMarker, StampingSet[CurrentMarker], CurrentTime);
			StampingSet[CurrentMarker] = CurrentTime;
			return CurrentMarker;
		}
		NextMarker -= 1;
	}
	if ( (TheCurrentPosition + 1) == SizeOfTheWord ) return BOGUS;
	
	// If the current node has a child list, then check for the next letter in TheWord.
	if ( CurrentChild = (One[CurrentIndex] & CHILD_MASK) ) {
		if ( CurrentOffset = (Two[(One[CurrentIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT] & ChildLetterBitMasks[TheWord[TheCurrentPosition + 1] - 'A']) ) {
			CurrentOffset >>= ChildLetterBitShifts[TheWord[TheCurrentPosition + 1] - 'A'];
			CurrentOffset -= 1;
			NextMarker += (CurrentMarker - WordsToEndOfBranchList(Three, Four, CurrentChild, BeginFourAt)+ WordsToEndOfBranchList(Three, Four, (CurrentChild + (int)CurrentOffset), BeginFourAt));
			return TrackingWordSearchWithFourPartDawgArrayRecurse(TheWord, One, Two, Three, Four, BeginFourAt, (CurrentChild + (int)CurrentOffset)
			, (TheCurrentPosition + 1), TheWord[TheCurrentPosition + 1], NextMarker, SizeOfTheWord, StampingSet, CurrentTime);
		}
	}
	return BOGUS;
}

// The characters inside of TheRawWord should all be contained in the specified character set.
// TheRawWord should have min length MIN_WORD_LENGTH,and max length MAX_WORD_LENGTH , if these conditions are not met, return BOGUS.
// If theRawWord does exist, return the lexicon position integer.
int TrackingWordSearchWithFourPartDawgArray(char *TheRawWord, int *First, long int *Second, int *Third, unsigned char *Fourth, int StartOfFourth, int *TheTimeStamps, int TheTime){
	if ( IsThisWordValid(TheRawWord) == FALSE ) return BOGUS;
	TrackingWordSearchWithFourPartDawgArrayRecurse(TheRawWord, First, Second, Third, Fourth, StartOfFourth
	, (CharacterLocations[TheRawWord[0] - 'A'] + 1), 0, TheRawWord[0], Third[CharacterLocations[TheRawWord[0] - 'A'] + 1], strlen(TheRawWord), TheTimeStamps, TheTime);
}


int main(){
	FILE *PartOne = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "rb");
	FILE *PartTwo = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "rb");
	FILE *PartThree = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "rb");
	FILE *PartFour = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "rb");
	
	int SizeOfPartOne;
	long int SizeOfPartTwo;
	int SizeOfPartThree;
	int SizeOfPartFour;
	
	int *PartOneArray;
	long int *PartTwoArray;
	int *PartThreeArray;
	unsigned char *PartFourArray;
	int WhatRoundAreWeOn = 1;
	
	int X;
	int PartThreeFourTransition;
	char TheUserInput[MAX_INPUT_SIZE + 1];
	char Question[MAX_INPUT_SIZE + 1];
	char DOM = '\0';
	Bool ShouldWeContinue = TRUE;
	int LexiconTimeStamps[TOTAL_WORDS_IN_LEXICON + 1];
	int CurrentPosition;
	int InputLength;
	
	for ( X = 0; X <= TOTAL_WORDS_IN_LEXICON; X++ ) LexiconTimeStamps[X] = 0;
	
	fread(&SizeOfPartOne, 4, 1, PartOne);
	fread(&SizeOfPartTwo, 8, 1, PartTwo);
	fread(&SizeOfPartThree, 4, 1, PartThree);
	PartThreeFourTransition = SizeOfPartThree + 1;
	SizeOfPartFour = SizeOfPartOne - SizeOfPartThree;
	
	printf("\n");
	printf("SizeOfPartOne |%d|\n", SizeOfPartOne);
	printf("SizeOfPartTwo |%ld|\n", SizeOfPartTwo);
	printf("SizeOfPartThree |%d|\n", SizeOfPartThree);
	printf("Transition |%d|\n", PartThreeFourTransition);
	printf("SizeOfPartFour |%d|\n", SizeOfPartFour);
	printf("\n");
	
	PartOneArray = (int *)malloc((SizeOfPartOne + 1) * sizeof(int));
	PartTwoArray = (long int *)malloc(SizeOfPartTwo * sizeof(long int));
	PartThreeArray = (int *)malloc((SizeOfPartThree + 1) * sizeof(int));
	PartFourArray = (unsigned char *)malloc(SizeOfPartFour * sizeof(char));
	
	PartOneArray[0] = 0;
	for ( X = 1; X <= SizeOfPartOne; X++ ) {
		fread(&(PartOneArray[X]), 4, 1, PartOne);
	}
	
	for ( X = 0; X < SizeOfPartTwo; X++ ) {
		fread(&(PartTwoArray[X]), 8, 1, PartTwo);
	}
	
	PartThreeArray[0] = 0;
	for ( X = 1; X <= SizeOfPartThree; X++ ) {
		fread(&(PartThreeArray[X]), 4, 1, PartThree);
	}
	
	for ( X = 0; X < SizeOfPartFour; X++ ) {
		fread(&(PartFourArray[X]), 1, 1, PartFour);
	}
	
	fclose(PartOne);
	fclose(PartTwo);
	fclose(PartThree);
	fclose(PartFour);
	
	printf("The four data files have been opened and read into memory.\n\n");
	
	while ( ShouldWeContinue == TRUE ) {
		printf("Type in the word that you want to search for in %s:", NAME_OF_LEXICON);
		fgets(TheUserInput, MAX_INPUT_SIZE, stdin);
		// Get rid of the new line char that fgets always reads in.
		InputLength = strlen(TheUserInput) - 1;
		TheUserInput[InputLength] = '\0';
		
		MakeMeAllCapital(TheUserInput);
		printf("We are now going to search for this word |%s|.\n\n", TheUserInput);
		CurrentPosition = TrackingWordSearchWithFourPartDawgArray(TheUserInput, PartOneArray, PartTwoArray, PartThreeArray, PartFourArray, PartThreeFourTransition, LexiconTimeStamps, WhatRoundAreWeOn);
		if ( CurrentPosition == BOGUS) printf("This word |%s| simply does not exist in %s\n", TheUserInput, NAME_OF_LEXICON);
		while ( !(DOM == 'Y' || DOM == 'y' || DOM == 'N' || DOM == 'n') ) {
			printf("\nDo you want to quit(y/n)?");
			fgets(Question, MAX_INPUT_SIZE, stdin);
			DOM = Question[0];
		}
		if ( DOM == 'Y' || DOM == 'y' ) ShouldWeContinue = FALSE;
		DOM = '\0';
		WhatRoundAreWeOn += 1;
	}	
	printf("\nYou have chosen to exit the program after |%d| rounds.\n\n", (WhatRoundAreWeOn - 2));
	
	return 0;
}