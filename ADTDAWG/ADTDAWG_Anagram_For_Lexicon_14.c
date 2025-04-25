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

typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

char CharacterSet[SIZE_OF_CHARACTER_SET] = {'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T'};
char CharacterLocations[26] = {0, BOGUS, 1, 2, 3, BOGUS, 4, BOGUS, 5, BOGUS, BOGUS, 6, 7, 8, 9, 10, BOGUS, 11, 12, 13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};
long int ChildLetterBitMasks[26] = {1, 0, 6, 24, 224, 0, 1792, 0, 14336, 0, 0, 114688, 1966080, 31457280, 503316480, 8053063680, 0, 128849018880, 2061584302080, 32985348833280, 0, 0, 0, 0, 0, 0};
int ChildLetterBitShifts[26] = {0, BOGUS, 1, 3, 5, BOGUS, 8, BOGUS, 11, BOGUS, BOGUS, 14, 17, 21, 25, 29, BOGUS, 33, 37, 41, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};

// qsort char comparison function.
int CharCompare(const void *a, const void *b){
	const char *A = (const char *)a;
	const char *B = (const char *)b;
	return (int)(*A - *B);
}


// This function converts any lower case letters in the string "RawWord," into all capitals, so that the whole string is made of capital letters.
// It is useful for when a user enters a string to be anagrammed.
void MakeMeAllCapital(char* RawWord){
	int X;
	for( X = 0; X < strlen( RawWord ); X++ ) {
		if( RawWord[X] >= 'a' && RawWord[X] <= 'z' ) RawWord[X] = RawWord[X] - LOWERIT;
	}
}

// This function will place the letters in the string "TheString," into alphabetical order.
void AlphabetizeMe(char *TheString){
	qsort(TheString, strlen(TheString), sizeof(char), CharCompare);
}

// This function will remove any characters in the string "TheString" that are not capital Letters contained in the chosen character set.
void RidMeOfBadCharacters(char *TheString){
	int X;
	int Y;
	for ( X = 0; TheString[X] != '\0'; X++ ) {
		if ( TheString[X] < 'A' || TheString[X] > 'Z' ) {
			for ( Y = X; TheString[Y] != '\0'; Y++) TheString[Y] = TheString[Y + 1];
			X -= 1;
		}
		else if ( ChildLetterBitShifts[TheString[X] - 'A'] == BOGUS ) {
			for ( Y = X; TheString[Y] != '\0'; Y++) TheString[Y] = TheString[Y + 1];
			X -= 1;
		}
	}
}

inline int WordsToEndOfBranchList(int *ThirdPart, unsigned char *FourthPart, int TheIndexInQuestion, int TheCutoff){
	return (TheIndexInQuestion < TheCutoff)? ThirdPart[TheIndexInQuestion]: (int)FourthPart[TheIndexInQuestion - TheCutoff];
}


// The introduction of "tracking," and "order doesn't matter," has made the basic recursive anagram function look more complex than what is actually go on.
// The Governing Equation of The Four Part DTDAWG: NextMarker = CurrentMarker - FCWTEOBL + NCWTEOBL - Flag;  WTEOBL means Words to end of branch list.
// FC means FirstChild.  NC means NextChild, the one we want to go to.
int TrackingAnagramWithFourPartDawgArrayRecurse(char *RawLetters, int *One, long int *Two, int *Three, unsigned char *Four, int BeginFourAt
, int CurrentIndex, char *TheWordSoFar, int FillThisPosition, char CurrentLetter, int CurrentMarker, int LettersRemaining, int *StampingSet, int CurrentTime){
	int X;
	int Y;
	char TheChosenLetter;
	long int CurrentOffset;
	int CurrentChild;
	int NextMarker = 0;
	int CurrentNCWTEOBL;
	int Count = 0;;
	Bool LettersUsedSoFar[SIZE_OF_CHARACTER_SET];
	
	//printf("We are in anagram recurse. CurrentLetter |%c|, CurrentIndex |%d|, LettersRemaining |%d|, RawLetters |%s|, CurrentMarker |%d|\n"
	//, CurrentLetter, CurrentIndex, LettersRemaining, RawLetters, CurrentMarker);
	
	for ( X = 0; X < SIZE_OF_CHARACTER_SET; X++ ) LettersUsedSoFar[X] = FALSE;
	TheWordSoFar[FillThisPosition] = CurrentLetter;
	if ( One[CurrentIndex] & END_OF_WORD_FLAG ) {
		TheWordSoFar[FillThisPosition+ 1] = '\0';
		printf("#Word|%d| Is |%s| - Old Stamp |%d| - New Stamp |%d|\n", CurrentMarker, TheWordSoFar, StampingSet[CurrentMarker], CurrentTime);
		StampingSet[CurrentMarker] = CurrentTime;
		NextMarker -= 1;
		Count += 1;
	}
	// If we have zero RawLetters left, then do not waste processor time trying to move further down the graph.
	if ( LettersRemaining ) {
	// If the current node has a child list, then go to the RawLetters nodes.
		if ( CurrentChild = (One[CurrentIndex] & CHILD_MASK) ) {
			//printf("CurrentChild |%d|\n", CurrentChild);
			// Begin the calculation of the NextMarker; the part that is common to all children.
			NextMarker += (CurrentMarker - WordsToEndOfBranchList(Three, Four, CurrentChild, BeginFourAt));
			for ( X = 0; X < LettersRemaining ; X++ ) {
				TheChosenLetter = RawLetters[X];
				if ( LettersUsedSoFar[CharacterLocations[TheChosenLetter - 'A']] == FALSE ) {
					if ( CurrentOffset = (Two[(One[CurrentIndex] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT] & ChildLetterBitMasks[TheChosenLetter - 'A']) ) {
						// If we have made it to this point "TheChosenLetter" has not been tried yet, and it does exist in the next child list
						CurrentOffset >>= ChildLetterBitShifts[RawLetters[X] - 'A'];
						CurrentOffset -= 1;
						//printf("We are using an offset of |%d| for |%c|", (int)CurrentOffset, TheChosenLetter);
						CurrentNCWTEOBL = WordsToEndOfBranchList(Three, Four, (CurrentChild + (int)CurrentOffset), BeginFourAt);
						NextMarker += CurrentNCWTEOBL;
						// First remove the letter that we are using for the primary position, then pass the remaining letters to the recursive anagrammer.
						for ( Y = X; Y < LettersRemaining; Y++ ) RawLetters[Y] = RawLetters[Y + 1];
						Count += TrackingAnagramWithFourPartDawgArrayRecurse(RawLetters, One, Two, Three, Four, BeginFourAt, (CurrentChild + (int)CurrentOffset)
						, TheWordSoFar, (FillThisPosition + 1), TheChosenLetter, NextMarker, (LettersRemaining - 1), StampingSet, CurrentTime);
						NextMarker -= CurrentNCWTEOBL;
						// Place the letter that we just used, back into TheRawLetters.
						for ( Y = LettersRemaining; Y > X; Y-- ) RawLetters[Y] = RawLetters[Y - 1];
						RawLetters[X] = TheChosenLetter;
					}
					LettersUsedSoFar[CharacterLocations[TheChosenLetter - 'A']] = TRUE;
				}
			}
		}
	}
	//printf("Out. CurrentLetter |%c|\n", CurrentLetter);
	return Count;
}


// The characters inside of TheRawLetters must all be contained in the specified character set, but the order does not matter.
int TrackingAnagramWithFourPartDawgArray(char *TheRawLetters, int *First, long int *Second, int *Third, unsigned char *Fourth
, int StartOfFourth, int *TheTimeStamps, int TheTime){
	int X;
	int Y;
	int Counter = 0;
	char TheChosenLetter;
	int NumberOfRawLetters = strlen(TheRawLetters);
	Bool LettersThatHaveBeenUsed[SIZE_OF_CHARACTER_SET];
	char RunningWord[MAX_WORD_LENGTH +1];
	
	for ( X = 0; X < SIZE_OF_CHARACTER_SET; X++ ) LettersThatHaveBeenUsed[X] = FALSE;
	//printf("We are in the first Anagram function.\n");
	for ( X = 0; X < NumberOfRawLetters ; X++ ) {
		TheChosenLetter = TheRawLetters[X];
		if ( LettersThatHaveBeenUsed[CharacterLocations[TheChosenLetter - 'A']] == FALSE ) {
			// First remove the letter that we are using for the primary position, then pass the remaining letters to the recursive anagrammer.
			for ( Y = X; Y < NumberOfRawLetters; Y++ ) TheRawLetters[Y] = TheRawLetters[Y + 1];
			printf("Run recurison on letter |%c| and pass on |%s|.\n", TheChosenLetter, TheRawLetters);
			Counter += TrackingAnagramWithFourPartDawgArrayRecurse(TheRawLetters, First, Second, Third, Fourth, StartOfFourth
			, (CharacterLocations[TheChosenLetter - 'A'] + 1), RunningWord, 0, TheChosenLetter, Third[CharacterLocations[TheChosenLetter - 'A'] + 1]
			, (NumberOfRawLetters - 1), TheTimeStamps, TheTime);
			// Place the letter that we just used, back into TheRawLetters.
			for ( Y = NumberOfRawLetters; Y > X; Y-- ) TheRawLetters[Y] = TheRawLetters[Y - 1];
			TheRawLetters[X] = TheChosenLetter;
			LettersThatHaveBeenUsed[CharacterLocations[TheChosenLetter - 'A']] = TRUE;
		}
	}
	return Counter;
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
	int CurrentCount;
	
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
		printf("Type in the letters that you want to anagram:");
		fgets(TheUserInput, MAX_INPUT_SIZE, stdin);
		if ( strlen(TheUserInput) < (MIN_WORD_LENGTH + 1) ) ShouldWeContinue = FALSE;
		MakeMeAllCapital(TheUserInput);
		RidMeOfBadCharacters(TheUserInput);
		if ( strlen(TheUserInput) < MIN_WORD_LENGTH ) ShouldWeContinue = FALSE;
		
		if ( ShouldWeContinue == TRUE ) {
			printf("TheUserInput-|%s|-|%d|.\n", TheUserInput, (int)strlen(TheUserInput));
			while ( !(DOM == 'Y' || DOM == 'y' || DOM == 'N' || DOM == 'n') ) {
				printf("Do you desire alphabetical order(y/n)?");
				fgets(Question, MAX_INPUT_SIZE, stdin);
				DOM = Question[0];
			}
			if ( DOM == 'Y' || DOM == 'y' ) AlphabetizeMe(TheUserInput);
			DOM = '\0';
			
			printf("We are now going to perform an anagramming on this |%s|.\n\n", TheUserInput);
			
			CurrentCount = TrackingAnagramWithFourPartDawgArray(TheUserInput, PartOneArray, PartTwoArray, PartThreeArray
			, PartFourArray, PartThreeFourTransition, LexiconTimeStamps, WhatRoundAreWeOn);
			
			while ( !(DOM == 'Y' || DOM == 'y' || DOM == 'N' || DOM == 'n') ) {
				printf("\n|%d| Words generated in round %d, do you want to quit(y/n)?", CurrentCount, WhatRoundAreWeOn);
				fgets(Question, MAX_INPUT_SIZE, stdin);
				DOM = Question[0];
			}
			if ( DOM == 'Y' || DOM == 'y' ) ShouldWeContinue = FALSE;
			DOM = '\0';
		}
		WhatRoundAreWeOn += 1;
	}	
	printf("\nYou have chosen to exit the program after |%d| rounds.\n\n", (WhatRoundAreWeOn - 2));
	
	return 0;
}