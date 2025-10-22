#ifndef CONST_H
#define CONST_H

#ifdef __cplusplus
extern "C" {
#endif

// General "Boggle" Constants.
#define SQUARE_COUNT 25
#define NUMBER_OF_ENGLISH_LETTERS 26
#define SIZE_OF_CHARACTER_SET 14
#define BOGUS 99

// These constant arrays define the lexicon contained in the ADTDAWG.
extern char CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1];

// TODO: eliminate this
extern unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS];

#define BOARDS_PER_ROUND 64

#ifdef __cplusplus
}
#endif

#endif  // CONST_H
