#ifndef CONST_H
#define CONST_H

#ifdef __cplusplus
extern "C" {
#endif

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

// C requires a boolean variable type so use C's typedef concept to create one.
typedef enum { TRUE = 1, FALSE = 0} Bool;
typedef Bool* BoolPtr;

// Provides the Boggle score associated with words of length equal to the array index.  Fifteen is the maximum word length of the TWL06 Tournament Scrabble Word List.
extern unsigned int THE_SCORE_CARD[MAX_STRING_LENGTH + 1];

// These constant arrays define the lexicon contained in the ADTDAWG.
extern char CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1];
extern unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS];
extern unsigned long int CHILD_LETTER_BIT_MASKS[SIZE_OF_CHARACTER_SET];
extern unsigned int CHILD_LETTER_BIT_SHIFTS[SIZE_OF_CHARACTER_SET];

// Constants that define the high level "DeepSearch.c" algorithm.
// #define NUMBER_OF_WORKER_THREADS	4
#define NUMBER_OF_WORKER_THREADS	1

// Constants that are lexicon specific.
#define TOTAL_WORDS_IN_LEXICON 44220
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15

#define DISCOVERY_STACK_SIZE (MAX_STRING_LENGTH + 1)

#define BOARDS_PER_ROUND 64

// This function assumes that "TheNumberNotYet" is a 2 char string of digits between [0,9].  Do not pass it anything else.  It will return the integer equivalent.
unsigned int TwoCharStringToInt(char* TheNumberNotYet);
void ConvertSquareNumberToString( char *TheThreeString, int X );

#ifdef __cplusplus
}
#endif

#endif // CONST_H
