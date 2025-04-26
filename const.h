#ifndef CONST_H
#define CONST_H

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
unsigned int THE_SCORE_CARD[MAX_STRING_LENGTH + 1] = { 0, 0, 0, 1, 1, 2, 3, 5, 11, 11, 11, 11, 11, 11, 11, 11 };

// These constant arrays define the lexicon contained in the ADTDAWG.
char CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '};
unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS] = {0, BOGUS, 1, 2, 3, BOGUS, 4, BOGUS, 5, BOGUS, BOGUS, 6, 7, 8, 9, 10, BOGUS, 11, 12, 13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS};
unsigned long int CHILD_LETTER_BIT_MASKS[SIZE_OF_CHARACTER_SET] = {1, 6, 24, 224, 1792, 14336, 114688, 1966080, 31457280, 503316480, 8053063680, 128849018880, 2061584302080, 32985348833280};
unsigned int CHILD_LETTER_BIT_SHIFTS[SIZE_OF_CHARACTER_SET] = {0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41};

// Constants that define the high level "DeepSearch.c" algorithm.
#define NUMBER_OF_WORKER_THREADS	4

// Constants that are lexicon specific.
#define TOTAL_WORDS_IN_LEXICON 44220
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15

#define DISCOVERY_STACK_SIZE (MAX_STRING_LENGTH + 1)

#endif // CONST_H
