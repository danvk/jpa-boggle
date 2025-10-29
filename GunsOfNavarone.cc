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

//               3          2         1
//              1098 7654 3210 9876 5432 1098 7654 3210
// child_mask:                       111 1111 1111 1111
// offset_index:       11 1111 1111 1000 0000 0000 0000
// end_of_word:       100 0000 0000 0000 0000 0000 0000
// blank:       1111 1

struct Node {
  // this is an index into the part three array
  unsigned int child_mask : 15;  // bits 0-14
  // this is an index into the part two array
  unsigned int offset_index : 11;  // bits 15-25
  unsigned int end_of_word : 1;    // bit 26
  int blank : 5;
};

// Constants that define the high level algorithm.
#define NUMBER_OF_WORKER_THREADS 1

uint32_t SCORES[MAX_STRING_LENGTH + 1] = {
    0, 0, 0, 1, 1, 2, 3, 5, 11, 11, 11, 11, 11, 11, 11, 11
};

// These constant array's define the lexicon contained in the ADTDAWG.
uint32_t CHARACTER_SET[SIZE_OF_CHARACTER_SET + 1] = {
    'A', 'C', 'D', 'E', 'G', 'I', 'L', 'M', 'N', 'O', 'P', 'R', 'S', 'T', ' '
};
uint32_t CHARACTER_LOCATIONS[NUMBER_OF_ENGLISH_LETTERS] = {
    0, BOGUS, 1,  2,     3,  BOGUS, 4,  BOGUS, 5,     BOGUS, BOGUS, 6,     7,
    8, 9,     10, BOGUS, 11, 12,    13, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS, BOGUS
};
uint64_t CHILD_LETTER_BIT_MASKS[SIZE_OF_CHARACTER_SET] = {
    //    4         3         2         1
    // 32109876543210987654321098765432109876543210
    0b000000000000000000000000000000000000000000001,  // A
    0b000000000000000000000000000000000000000000110,  // C
    0b000000000000000000000000000000000000000011000,  // D
    0b000000000000000000000000000000000000011100000,  // E
    0b000000000000000000000000000000000011100000000,  // G
    0b000000000000000000000000000000011100000000000,  // I
    0b000000000000000000000000000011100000000000000,  // L
    0b000000000000000000000000111100000000000000000,  // M
    0b000000000000000000001111000000000000000000000,  // N
    0b000000000000000011110000000000000000000000000,  // O
    0b000000000000111100000000000000000000000000000,  // P
    0b000000001111000000000000000000000000000000000,  // R
    0b000011110000000000000000000000000000000000000,  // S
    0b111100000000000000000000000000000000000000000   // T
};
uint32_t CHILD_LETTER_BIT_SHIFTS[SIZE_OF_CHARACTER_SET] = {
    0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41
};
// each part two entry uses 45 bits

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The structure for a Boggle board is defined in this section.

// The "square" struct will represent one position in a Boggle board.
// A square also requires a flag to indicate use in the current word being formed, the
// number of valid neighbours, and the index of the letter showing on its face.
struct Square {
  // The Used flag will indicate if a square is being used in constructing the current
  // word, and hence to remove the used square from further inclusion in the same word.
  bool used;
  uint32_t letter_idx;
  uint32_t num_neighbors;
  Square *neighbors[NEIGHBOURS];
};

// This Function initializes ThisSquare when passed its row and column position on the
// board. Important note:  The function is going to use the low level C concept of
// pointer arithmatic to fill the LivingNeighbourSquarePointerArray, which will be
// filled from the top-left, clockwise.
void SquareInit(Square *square, uint32_t row, uint32_t col) {
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
  uint32_t Row;
  uint32_t Col;
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
  for (uint32_t Row = MAX_ROW; Row-- > 0;) {
    for (uint32_t Col = MAX_COL; Col-- > 0;) {
      (bd->Block)[Row][Col].letter_idx =
          CHARACTER_LOCATIONS[letters[Row * MAX_COL + Col] - 'A'];
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// These are the global variables needed.

// Each worker thread will have it's own time stamping uint32_t array for all of the
// words in the lexicon.
uint32_t *LexiconMarks;

// These are the pointers to the global immutable lexicon data structure.  The ADTDAWG
// is well advanced and beyond the scope of the high level search algorithm. Since these
// variables are branded as "Read Only," they can be utilized globally without passing
// pointers.
Node *Nodes;
uint64_t *ChildOffsets;
uint32_t *PartThreeArray;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The sequential board scoring functions are found here for the new ADTDAWG.

// This is the central piece of code in the BIG Boggle board analysis scheme.
// Recursion is used to traverse the neighbours of the starting square, regardless of
// alphabetical order. It updates the global "LexiconTimeStamps" to eliminate the
// counting of identical words. Every letter on the board must be contained in the
// lexicon character set.

int ScoreSquare(
    Square *square,
    uint32_t node_idx,
    uint32_t lexicon_idx,
    uint32_t mark,
    uint32_t num_chars
) {
  uint32_t score = 0;
  square->used = true;
  const auto &node = Nodes[node_idx];
  uint32_t child_idx = node.child_mask;

  // Check if we have arrived at a new word, and if so, add the correct score
  if (node.end_of_word) {
    if (LexiconMarks[lexicon_idx] < mark) {
      score += SCORES[num_chars];
      LexiconMarks[lexicon_idx] = mark;
    }
    lexicon_idx -= 1;
  }

  // If this node has children in the lexicon, explore the neighbors
  if (child_idx) {
    lexicon_idx -= PartThreeArray[child_idx];
    uint64_t part_two = ChildOffsets[node.offset_index];

    Square **neighbors = square->neighbors;

    // Loop through all neighbors
    for (int i = 0; i < square->num_neighbors; i++) {
      auto n = neighbors[i];
      if (n->used) {
        continue;
      }
      auto letter_idx = n->letter_idx;
      uint64_t offset64 = part_two & CHILD_LETTER_BIT_MASKS[letter_idx];

      if (offset64) {
        offset64 >>= CHILD_LETTER_BIT_SHIFTS[letter_idx];
        offset64 -= 1;
        auto offset32 = (uint32_t)offset64;
        auto next_idx = child_idx + offset32;

        score += ScoreSquare(
            n, next_idx, lexicon_idx + PartThreeArray[next_idx], mark, num_chars + 1
        );
      }
    }
  }

  square->used = false;
  return score;
}

// The function returns the Boggle score for "ThisBoard."
uint32_t ScoreBoard(Board *ThisBoard, uint32_t mark) {
  auto &block = ThisBoard->Block;
  uint32_t score = 0;
  // Add up all the scores that originate from each square in the board.
  for (uint32_t row = 0; row < MAX_ROW; row++) {
    for (uint32_t col = 0; col < MAX_COL; col++) {
      uint32_t part1_idx = block[row][col].letter_idx + 1;
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
  uint32_t SizeOfPartOne;
  uint64_t SizeOfPartTwo;
  uint32_t SizeOfPartThree;

  // Read in the size of each data file.
  if (fread(&SizeOfPartOne, 4, 1, PartOne) != 1) return 0;
  if (fread(&SizeOfPartTwo, 8, 1, PartTwo) != 1) return 0;
  if (fread(&SizeOfPartThree, 4, 1, PartThree) != 1) return 0;

  // Allocate memory for the ADTDAWG.
  size_t bytes_one = (SizeOfPartOne + 1) * sizeof(uint32_t);
  size_t bytes_two = SizeOfPartTwo * sizeof(uint64_t);
  size_t bytes_three = (SizeOfPartOne + 1) * sizeof(uint32_t);
  Nodes = (Node *)malloc(bytes_one);
  ChildOffsets = (uint64_t *)malloc(bytes_two);
  PartThreeArray = (uint32_t *)malloc(bytes_three);

  printf(
      "bytes for arrays: %zu + %zu + %zu = %zu\n",
      bytes_one,
      bytes_two,
      bytes_three,
      bytes_one + bytes_two + bytes_three
  );

  // Read in the data files into global arrays of basic integer types.
  // The zero position in "PartOneArray" is the NULL node.
  Nodes[0].child_mask = 0;
  Nodes[0].end_of_word = 0;
  Nodes[0].offset_index = 0;
  if (fread(Nodes + 1, 4, SizeOfPartOne, PartOne) != SizeOfPartOne) return 0;
  if (fread(ChildOffsets, 8, SizeOfPartTwo, PartTwo) != SizeOfPartTwo) return 0;
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
  uint32_t X;
  uint32_t CurrentScore;
  uint32_t BoardCount = 0;

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

  // Allocate the set of lexicon time stamps as uint32_tegers.
  size_t bytes_marks = (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(uint32_t);
  LexiconMarks = (uint32_t *)malloc(bytes_marks);
  printf("bytes for marks: %zu\n", bytes_marks);
  printf("sizeof(Node): %zu\n", sizeof(Node));

  // Zero all of the global time stamps.
  memset(LexiconMarks, 0, (TOTAL_WORDS_IN_LEXICON + 1) * sizeof(uint32_t));

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

  uint32_t total_score = 0;
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
  free(Nodes);
  free(ChildOffsets);
  free(PartThreeArray);

  return 0;
}
