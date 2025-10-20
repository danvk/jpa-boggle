#ifndef ADTDAWG_H
#define ADTDAWG_H

#include "const.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are the global variables needed for the immutable lexicon, and for the
// solution to the duplicate word problem.

// Time stamping array for all of the words in the lexicon.
unsigned int *LexiconTimeStamps;

// These are the pointers to the global immutable lexicon data structure.  The
// ADTDAWG is well advanced and beyond the scope of the high level search
// algorithm. Since these variables are branded as "Read Only," they can be
// utilized globally without passing pointers.
unsigned int *PartOneArray;
unsigned long int *PartTwoArray;
unsigned int *PartThreeArray;
unsigned int PartThreeFourTransition;

#endif // ADTDAWG_H
