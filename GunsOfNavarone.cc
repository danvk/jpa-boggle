#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>  // Ensure this is included for clock()

#include <string>
#include <vector>

using namespace std;

// The ADTDAWG for Lexicon_14, a subset of TWL06, is located in the 4 data files listed
// below.
#define FOUR_PART_DTDAWG_14_PART_ONE "Four_Part_1_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_TWO "Four_Part_2_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_THREE "Four_Part_3_DTDAWG_For_Lexicon_14.dat"
#define FOUR_PART_DTDAWG_14_PART_FOUR "Four_Part_4_DTDAWG_For_Lexicon_14.dat"

// General "Boggle" Constants.
#define MAX_ROW 5
#define MAX_COL 5
#define SQUARE_COUNT 25
#define NEIGHBOURS 8
#define NUMBER_OF_ENGLISH_LETTERS 26
#define SIZE_OF_CHARACTER_SET 14
#define MAX_STRING_LENGTH 15
#define BOGUS 4294967197

// Constants that are lexicon specific.
#define TOTAL_WORDS_IN_LEXICON 44220
#define END_OF_WORD_FLAG 67108864
#define CHILD_MASK 32767
#define OFFSET_INDEX_MASK 67076096
#define OffSET_BIT_SHIFT 15

// Constants that define the high level algorithm.
#define NUMBER_OF_WORKER_THREADS 1

unsigned int THE_SCORE_CARD[MAX_STRING_LENGTH + 1] = {
    0, 0, 0, 1, 1, 2, 3, 5, 11, 11, 11, 11, 11, 11, 11, 11
};

// These constant array's define the lexicon contained in the ADTDAWG.
unsigned int CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {
    'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '
};
unsigned int CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS] = {
    0, BOGUS, 1,  2,     3,  BOGUS, 4,  BOGUS, 5,     BOGUS, BOGUS, 6,     7,
    8, 9,     10, BOGUS, 11, 12,    13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS
};
unsigned long int CHILD_LETTER_BIT_MASKS[SIZE_OF_CHARACTER_SET] = {
    1,
    6,
    24,
    224,
    1792,
    14336,
    114688,
    1966080,
    31457280,
    503316480,
    8053063680,
    128849018880,
    2061584302080,
    32985348833280
};
unsigned int CHILD_LETTER_BIT_SHIFTS[SIZE_OF_CHARACTER_SET] = {
    0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The structure for a Boggle board is defined in this section.

// The "square" struct will represent one position in a Boggle board.
// A square also requires a flag to indicate use in the current word being formed, the
// number of valid neighbours, and the index of the letter showing on its face.
struct Square {
  // The Used flag will indicate if a square is being used in constructing the current
  // word, and hence to remove the used square from further inclusion in the same word.
  bool used;
  unsigned int letter_idx;
  unsigned int num_neighbors;
  Square *neighbors[NEIGHBOURS];
};

// This Function initializes ThisSquare when passed its row and column position on the
// board. Important note:  The function is going to use the low level C concept of
// pointer arithmatic to fill the LivingNeighbourSquarePointerArray, which will be
// filled from the top-left, clockwise.
void SquareInit(Square *square, unsigned int row, unsigned int col) {
  square->letter_idx = SIZE_OF_CHARACTER_SET;
  square->used = false;
  for (int i = 0; i < NEIGHBOURS; i++) {
    (square->neighbors)[i] = NULL;
  }
  if (row == 0) {
    // ThisSquare is in the top-left position.
    if (col == 0) {
      square->num_neighbors = 3;
      (square->neighbors)[0] = square + 1;
      (square->neighbors)[1] = square + MAX_COL + 1;
      (square->neighbors)[2] = square + MAX_COL;
    }
    // ThisSquare is in the top-right position.
    else if (col == (MAX_COL - 1)) {
      square->num_neighbors = 3;
      (square->neighbors)[0] = square + MAX_COL;
      (square->neighbors)[1] = square + MAX_COL - 1;
      (square->neighbors)[2] = square - 1;
    }
    // ThisSquare is in a top-middle position.
    else {
      square->num_neighbors = 5;
      (square->neighbors)[0] = square + 1;
      (square->neighbors)[1] = square + MAX_COL + 1;
      (square->neighbors)[2] = square + MAX_COL;
      (square->neighbors)[3] = square + MAX_COL - 1;
      (square->neighbors)[4] = square - 1;
    }
  } else if (row == (MAX_ROW - 1)) {
    // ThisSquare is in the bottom-left position.
    if (col == 0) {
      square->num_neighbors = 3;
      (square->neighbors)[0] = square - MAX_COL;
      (square->neighbors)[1] = square - MAX_COL + 1;
      (square->neighbors)[2] = square + 1;
    }
    // ThisSquare is in the bottom-right position.
    else if (col == (MAX_COL - 1)) {
      square->num_neighbors = 3;
      (square->neighbors)[0] = square - MAX_COL - 1;
      (square->neighbors)[1] = square - MAX_COL;
      (square->neighbors)[2] = square - 1;
    }
    // ThisSquare is in a bottom-middle position.
    else {
      square->num_neighbors = 5;
      (square->neighbors)[0] = square - MAX_COL - 1;
      (square->neighbors)[1] = square - MAX_COL;
      (square->neighbors)[2] = square - MAX_COL + 1;
      (square->neighbors)[3] = square + 1;
      (square->neighbors)[4] = square - 1;
    }
  }
  // ThisSquare is in a middle-left position.
  else if (col == 0) {
    square->num_neighbors = 5;
    (square->neighbors)[0] = square - MAX_COL;
    (square->neighbors)[1] = square - MAX_COL + 1;
    (square->neighbors)[2] = square + 1;
    (square->neighbors)[3] = square + MAX_COL + 1;
    (square->neighbors)[4] = square + MAX_COL;
    (square->neighbors)[5] = NULL;
    (square->neighbors)[6] = NULL;
    (square->neighbors)[7] = NULL;
  }
  // ThisSquare is in a middle-right position.
  else if (col == (MAX_COL - 1)) {
    square->num_neighbors = 5;
    (square->neighbors)[0] = square - MAX_COL - 1;
    (square->neighbors)[1] = square - MAX_COL;
    (square->neighbors)[2] = square + MAX_COL;
    (square->neighbors)[3] = square + MAX_COL - 1;
    (square->neighbors)[4] = square - 1;
  }
  // ThisSquare is in a middle-middle position.
  else {
    square->num_neighbors = NEIGHBOURS;
    (square->neighbors)[0] = square - MAX_COL - 1;
    (square->neighbors)[1] = square - MAX_COL;
    (square->neighbors)[2] = square - MAX_COL + 1;
    (square->neighbors)[3] = square + 1;
    (square->neighbors)[4] = square + MAX_COL + 1;
    (square->neighbors)[5] = square + MAX_COL;
    (square->neighbors)[6] = square + MAX_COL - 1;
    (square->neighbors)[7] = square - 1;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A board is defined as simply a static 2 dimensional array of Squares.
struct Board {
  Square Block[MAX_ROW][MAX_COL];
};

// This initialization function sets the neighbour array for all of the Squares in
// ThisBoard's Block (this only needs to be done once). The Letter index for each square
// will be a blank space ' ', and they will all be not Used.
void BoardInit(Board *ThisBoard) {
  unsigned int Row;
  unsigned int Col;
  for (Row = MAX_ROW; Row-- > 0;) {
    for (Col = MAX_COL; Col-- > 0;) {
      SquareInit(&(ThisBoard->Block[Row][Col]), Row, Col);
    }
  }
}

// This function simply transcribes the BoardString data into ThisBoard using the
// correct format. A major optimization has taken place at this level because the
// ADTDAWG's direct property enforces the "Order Does Not Matter," paradigm, and thus,
// no sorting is required.
void BoardPopulate(Board *bd, const char *letters) {
  for (unsigned int Row = MAX_ROW; Row-- > 0;) {
    for (unsigned int Col = MAX_COL; Col-- > 0;) {
      (bd->Block)[Row][Col].letter_idx =
          CHARACTER_LOCATIONS[letters[Row * MAX_COL + Col] - 'A'];
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are the global variables needed.

// Each worker thread will have it's own time stamping unsigned int array for all of the
// words in the lexicon.
unsigned int *LexiconMarks;

// These are the pointers to the global immutable lexicon data structure.  The ADTDAWG
// is well advanced and beyond the scope of the high level search algorithm. Since these
// variables are branded as "Read Only," they can be utilized globally without passing
// pointers.
unsigned int *PartOneArray;
unsigned long int *PartTwoArray;
unsigned int *PartThreeArray;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DiscoveryStackNode {
  Square *TheSquareNow;
  unsigned int LexiconPartOneIndex;
  unsigned int FirstChildIndex;
  unsigned int WordLengthNow;
  unsigned int NextMarkerNow;
  unsigned int TheChildToWorkOn;
  unsigned long int TheSecondPartNow;
};

// This is where the recursion-replacement stack is implemented.
// Using macros allows the programmer to change the value of an argument directly.
// The stack needs room for the NULL position 0, (MAX_STRING_LENGTH - 1) square
// positions, and a buffer for the top pointer.
#define DISCOVERY_STACK_SIZE (MAX_STRING_LENGTH + 1)
#define DISCOVERY_STACK_PUSH(top, square, indie, fcindie, len, next, workon, second) \
  (((top->TheSquareNow = (square)),                                                  \
    (top->LexiconPartOneIndex = (indie)),                                            \
    (top->FirstChildIndex = (fcindie)),                                              \
    (top->WordLengthNow = (len)),                                                    \
    (top->NextMarkerNow = (next)),                                                   \
    (top->TheChildToWorkOn = (workon)),                                              \
    (top->TheSecondPartNow = (second)),                                              \
    ++top))
#define DISCOVERY_STACK_POP(square, indie, fcindie, len, next, workon, second, top) \
  ((--top,                                                                          \
    (square = top->TheSquareNow),                                                   \
    (indie = top->LexiconPartOneIndex),                                             \
    (fcindie = top->FirstChildIndex),                                               \
    (len = top->WordLengthNow),                                                     \
    (next = top->NextMarkerNow),                                                    \
    (workon = top->TheChildToWorkOn),                                               \
    (second = top->TheSecondPartNow)))
#define DISCOVERY_STACK_NOT_EMPTY (TheDiscoveryStack < TheTop)

DiscoveryStackNode TheDiscoveryStack[DISCOVERY_STACK_SIZE];

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The sequential board scoring functions are found here for the new ADTDAWG.

// This is the central piece of code in the BIG Boggle board analysis scheme.
// Recursion is used to traverse the neighbours of the starting square, regardless of
// alphabetical order. It updates the global "LexiconTimeStamps" to eliminate the
// counting of identical words. Every letter on the board must be contained in the
// lexicon character set.

int ScoreSquare(
    Square *square,
    unsigned int part1_idx,
    unsigned int lexicon_idx,
    unsigned int mark,
    unsigned int num_chars
) {
  unsigned int score = 0;
  square->used = true;

  // Get the child index from the lexicon
  unsigned int child_idx = (PartOneArray[part1_idx] & CHILD_MASK);
  int lexicon_offset = 0;

  // Check if we have arrived at a new word, and if so, add the correct score
  if (PartOneArray[part1_idx] & END_OF_WORD_FLAG) {
    if (LexiconMarks[lexicon_idx] < mark) {
      score += THE_SCORE_CARD[num_chars];
      LexiconMarks[lexicon_idx] = mark;
    }
    lexicon_offset -= 1;
  }

  // If this node has children in the lexicon, explore the neighbors
  if (child_idx) {
    lexicon_offset -= PartThreeArray[child_idx];
    unsigned long int part_two =
        PartTwoArray[(PartOneArray[part1_idx] & OFFSET_INDEX_MASK) >> OffSET_BIT_SHIFT];

    Square **neighbors = square->neighbors;

    // Loop through all neighbors
    for (unsigned int X = 0; X < square->num_neighbors; X++) {
      if (neighbors[X]->used == false) {
        unsigned int letter_idx = (neighbors[X])->letter_idx;
        unsigned long int offset64 = (part_two & CHILD_LETTER_BIT_MASKS[letter_idx]);

        if (offset64) {
          offset64 >>= CHILD_LETTER_BIT_SHIFTS[letter_idx];
          offset64 -= 1;

          auto offset32 = (unsigned int)offset64;

          // Recursive call to explore this neighbor
          score += ScoreSquare(
              neighbors[X],
              child_idx + offset32,
              lexicon_idx + lexicon_offset + PartThreeArray[child_idx + offset32],
              mark,
              num_chars + 1
          );
        }
      }
    }
  }

  // Unmark this square (backtrack)
  square->used = false;

  return score;
}

// The function returns the Boggle score for "ThisBoard."
unsigned int ScoreBoard(Board *ThisBoard, unsigned int mark) {
  auto &block = ThisBoard->Block;
  unsigned int score = 0;
  // Add up all the scores that originate from each square in the board.
  for (unsigned int row = 0; row < MAX_ROW; row++) {
    for (unsigned int col = 0; col < MAX_COL; col++) {
      unsigned int part1_idx = block[row][col].letter_idx + 1;
      score +=
          ScoreSquare(&block[row][col], part1_idx, PartThreeArray[part1_idx], mark, 1);
    }
  }
  return score;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int LoadDictionary() {
  FILE *PartOne = fopen(FOUR_PART_DTDAWG_14_PART_ONE, "rb");
  FILE *PartTwo = fopen(FOUR_PART_DTDAWG_14_PART_TWO, "rb");
  FILE *PartThree = fopen(FOUR_PART_DTDAWG_14_PART_THREE, "rb");
  FILE *PartFour = fopen(FOUR_PART_DTDAWG_14_PART_FOUR, "rb");

  unsigned char TempReadIn;
  unsigned int SizeOfPartOne;
  unsigned long int SizeOfPartTwo;
  unsigned int SizeOfPartThree;

  // Read in the size of each data file.
  if (fread(&SizeOfPartOne, 4, 1, PartOne) != 1) return 0;
  if (fread(&SizeOfPartTwo, 8, 1, PartTwo) != 1) return 0;
  if (fread(&SizeOfPartThree, 4, 1, PartThree) != 1) return 0;

  // Allocate memory for the ADTDAWG.
  size_t bytes_one = (SizeOfPartOne + 1) * sizeof(int);
  size_t bytes_two = SizeOfPartTwo * sizeof(long int);
  size_t bytes_three = (SizeOfPartOne + 1) * sizeof(int);
  PartOneArray = (unsigned int *)malloc(bytes_one);
  PartTwoArray = (unsigned long int *)malloc(bytes_two);
  PartThreeArray = (unsigned int *)malloc(bytes_three);

  printf(
      "bytes for arrays: %zu + %zu + %zu = %zu\n",
      bytes_one,
      bytes_two,
      bytes_three,
      bytes_one + bytes_two + bytes_three
  );

  // Read in the data files into global arrays of basic integer types.
  // The zero position in "PartOneArray" is the NULL node.
  PartOneArray[0] = 0;
  if (fread(PartOneArray + 1, 4, SizeOfPartOne, PartOne) != SizeOfPartOne) return 0;
  if (fread(PartTwoArray, 8, SizeOfPartTwo, PartTwo) != SizeOfPartTwo) return 0;
  // The Zero position in "PartThreeArray" maps to the NULL node in "PartOneArray".
  PartThreeArray[0] = 0;
  if (fread(PartThreeArray + 1, 4, SizeOfPartThree, PartThree) != SizeOfPartThree)
    return 0;
  // Part Four has been replaced by encoding the Part Four WTEOBL values as 32 bit
  // integers for speed.  The size of the data structure is small enough as it is.
  for (int X = (SizeOfPartThree + 1); X <= SizeOfPartOne; X++) {
    if (fread(&TempReadIn, 1, 1, PartFour) != 1) return 0;
    PartThreeArray[X] = TempReadIn;
  }

  fclose(PartOne);
  fclose(PartTwo);
  fclose(PartThree);
  fclose(PartFour);
  return 1;
}

int main(int argc, char *argv[]) {
  unsigned int X;
  unsigned int CurrentScore;
  unsigned int BoardCount = 0;

  char BoardString[SQUARE_COUNT + 1];
  Board *WorkingBoard = (Board *)malloc(sizeof(Board));

  double BeginWorkTime;
  double EndWorkTime;
  double TheRunTime;

  FILE *input_file;

  // Check for command-line argument
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <board_file>\n", argv[0]);
    return 1;
  }

  BoardInit(WorkingBoard);

  // Allocate the set of lexicon time stamps as unsigned integers.
  size_t bytes_marks = (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(unsigned int);
  LexiconMarks = (unsigned int *)malloc(bytes_marks);
  printf("bytes for marks: %zu\n", bytes_marks);

  // Zero all of the global time stamps.
  memset(LexiconMarks, 0, (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(unsigned int));

  int result = LoadDictionary();
  if (result != 1) {
    return result;
  }

  // Open the input file
  input_file = fopen(argv[1], "r");
  if (input_file == NULL) {
    fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
    return 1;
  }

  // Read the boards in advance to avoid measuring I/O time.
  vector<string> boards;

  BeginWorkTime = (double)clock() / CLOCKS_PER_SEC;

  // Read boards from file and score them
  while (fscanf(input_file, "%25s", BoardString) == 1) {
    BoardString[SQUARE_COUNT] = '\0';
    for (int i = 0; BoardString[i]; i++) {
      BoardString[i] = toupper(BoardString[i]);
    }
    boards.push_back(BoardString);
  }
  fclose(input_file);

  unsigned int total_score = 0;
  for (const auto &board : boards) {
    BoardPopulate(WorkingBoard, board.c_str());
    CurrentScore = ScoreBoard(WorkingBoard, BoardCount + 1);
    total_score += CurrentScore;
    BoardCount++;
  }

  EndWorkTime = (double)clock() / CLOCKS_PER_SEC;
  TheRunTime = EndWorkTime - BeginWorkTime;

  printf("Evaluated %zu boards\nTotal score: %u\n", boards.size(), total_score);
  // printf("sizeof(long int) = %zu\n", sizeof(long int));  // 8

  // Report performance to stderr
  fprintf(
      stderr,
      "Scored %u boards in %.3f seconds (%.2f boards/second)\n",
      BoardCount,
      TheRunTime,
      BoardCount / TheRunTime
  );

  // Clean up
  free(WorkingBoard);
  free(LexiconMarks);
  free(PartOneArray);
  free(PartTwoArray);
  free(PartThreeArray);

  return 0;
}
