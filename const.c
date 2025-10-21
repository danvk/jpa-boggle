#include "const.h"

char CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {
    'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '
};

unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS] = {
    0, BOGUS, 1,  2,     3,  BOGUS, 4,  BOGUS, 5,     BOGUS, BOGUS, 6,     7,
    8, 9,     10, BOGUS, 11, 12,    13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Basic constructs and functions that will be useful.
