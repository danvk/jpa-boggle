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
// child_index:                      111 1111 1111 1111
// offset_index:       11 1111 1111 1000 0000 0000 0000
// end_of_word:       100 0000 0000 0000 0000 0000 0000
// blank:       1111 1

struct Node {
  // this is the index of the first child of this node (zero for leaf)
  unsigned int child_index : 15;  // bits 0-14
  // this is an index into the child offsets array
  unsigned int offset_index : 11;  // bits 15-25
  unsigned int is_word : 1;        // bit 26
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
uint64_t CHILD_MASKS[SIZE_OF_CHARACTER_SET] = {
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
uint32_t CHILD_SHIFTS[SIZE_OF_CHARACTER_SET] = {
    0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41
};
// each part two entry uses 45 bits

uint32_t used;
int letter_idxs[25];

// This function simply transcribes the BoardString data into ThisBoard using the
// correct format. A major optimization has taken place at this level because the
// ADTDAWG's direct property enforces the "Order Does Not Matter," paradigm, and thus,
// no sorting is required.
void BoardPopulate(const char *letters) {
  for (int i = 0; i < 25; i++) {
    letter_idxs[i] = CHARACTER_LOCATIONS[letters[i] - 'A'];
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
uint32_t *Tracking;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The sequential board scoring functions are found here for the new ADTDAWG.

// This is the central piece of code in the BIG Boggle board analysis scheme.
// Recursion is used to traverse the neighbours of the starting square, regardless of
// alphabetical order. It updates the global "LexiconTimeStamps" to eliminate the
// counting of identical words. Every letter on the board must be contained in the
// lexicon character set.

#define REC(idx)                                                                    \
  do {                                                                              \
    if ((used & (1 << idx)) == 0) {                                                 \
      letter_idx = letter_idxs[idx];                                                \
      offset64 = child_offsets & CHILD_MASKS[letter_idx];                           \
      if (offset64) {                                                               \
        offset64 >>= CHILD_SHIFTS[letter_idx];                                      \
        next_idx = child_idx + offset64 - 1;                                        \
        next_lexicon_idx =                                                          \
            lexicon_idx + Tracking[next_idx] - Tracking[child_idx] - is_word;       \
        score += ScoreSquare(idx, next_idx, next_lexicon_idx, mark, num_chars + 1); \
      }                                                                             \
    }                                                                               \
  } while (0)

#define REC3(a, b, c) \
  REC(a);             \
  REC(b);             \
  REC(c)

#define REC5(a, b, c, d, e) \
  REC3(a, b, c);            \
  REC(d);                   \
  REC(e)

#define REC8(a, b, c, d, e, f, g, h) \
  REC5(a, b, c, d, e);               \
  REC3(f, g, h)

int ScoreSquare(
    int square,
    uint32_t node_idx,
    uint32_t lexicon_idx,
    uint32_t mark,
    uint32_t num_chars
) {
  uint32_t score = 0;
  used ^= (1 << square);
  const auto &node = Nodes[node_idx];

  // Check if we have arrived at a new word, and if so, add the correct score
  auto is_word = node.is_word;
  if (is_word) {
    if (LexiconMarks[lexicon_idx] < mark) {
      score += SCORES[num_chars];
      LexiconMarks[lexicon_idx] = mark;
    }
  }

  // If this node has children in the lexicon, explore the neighbors
  uint32_t child_idx = node.child_index;
  if (child_idx) {
    uint64_t child_offsets = ChildOffsets[node.offset_index];

    uint32_t letter_idx, next_idx, next_lexicon_idx;
    uint64_t offset64;

    // clang-format off
    switch(square) {
      case 0: REC3(1, 5, 6); break;
      case 1: REC5(0, 2, 5, 6, 7); break;
      case 2: REC5(1, 3, 6, 7, 8); break;
      case 3: REC5(2, 4, 7, 8, 9); break;
      case 4: REC3(3, 8, 9); break;
      case 5: REC5(0, 1, 6, 10, 11); break;
      case 6: REC8(0, 1, 2, 5, 7, 10, 11, 12); break;
      case 7: REC8(1, 2, 3, 6, 8, 11, 12, 13); break;
      case 8: REC8(2, 3, 4, 7, 9, 12, 13, 14); break;
      case 9: REC5(3, 4, 8, 13, 14); break;
      case 10: REC5(5, 6, 11, 15, 16); break;
      case 11: REC8(5, 6, 7, 10, 12, 15, 16, 17); break;
      case 12: REC8(6, 7, 8, 11, 13, 16, 17, 18); break;
      case 13: REC8(7, 8, 9, 12, 14, 17, 18, 19); break;
      case 14: REC5(8, 9, 13, 18, 19); break;
      case 15: REC5(10, 11, 16, 20, 21); break;
      case 16: REC8(10, 11, 12, 15, 17, 20, 21, 22); break;
      case 17: REC8(11, 12, 13, 16, 18, 21, 22, 23); break;
      case 18: REC8(12, 13, 14, 17, 19, 22, 23, 24); break;
      case 19: REC5(13, 14, 18, 23, 24); break;
      case 20: REC3(15, 16, 21); break;
      case 21: REC5(15, 16, 17, 20, 22); break;
      case 22: REC5(16, 17, 18, 21, 23); break;
      case 23: REC5(17, 18, 19, 22, 24); break;
      case 24: REC3(18, 19, 23); break;
    }
    // clang-format on
  }

  used ^= (1 << square);
  return score;
}

// The function returns the Boggle score for "ThisBoard."
uint32_t ScoreBoard(uint32_t mark) {
  uint32_t score = 0;
  used = 0;
  // Add up all the scores that originate from each square in the board.
  for (int i = 0; i < SQUARE_COUNT; i++) {
    uint32_t part1_idx = letter_idxs[i] + 1;
    score += ScoreSquare(i, part1_idx, Tracking[part1_idx], mark, 1);
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
  Tracking = (uint32_t *)malloc(bytes_three);

  printf(
      "bytes for arrays: %zu + %zu + %zu = %zu\n",
      bytes_one,
      bytes_two,
      bytes_three,
      bytes_one + bytes_two + bytes_three
  );

  // Read in the data files into global arrays of basic integer types.
  // The zero position in "PartOneArray" is the NULL node.
  Nodes[0].child_index = 0;
  Nodes[0].is_word = 0;
  Nodes[0].offset_index = 0;
  if (fread(Nodes + 1, 4, SizeOfPartOne, PartOne) != SizeOfPartOne) return 0;
  if (fread(ChildOffsets, 8, SizeOfPartTwo, PartTwo) != SizeOfPartTwo) return 0;
  // The Zero position in "PartThreeArray" maps to the NULL node in "PartOneArray".
  Tracking[0] = 0;
  if (fread(Tracking + 1, 4, SizeOfPartThree, PartThree) != SizeOfPartThree) return 0;
  // Part Four has been replaced by encoding the Part Four WTEOBL values as 32 bit
  // integers for speed.  The size of the data structure is small enough as it is.
  for (int X = (SizeOfPartThree + 1); X <= SizeOfPartOne; X++) {
    if (fread(&TempReadIn, 1, 1, PartFour) != 1) return 0;
    Tracking[X] = TempReadIn;
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

  double BeginWorkTime;
  double EndWorkTime;
  double TheRunTime;

  FILE *input_file;

  // Check for command-line argument
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <board_file>\n", argv[0]);
    return 1;
  }

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

  for (int i = 0; i < 100; i++) {
    auto c = ChildOffsets[i];
    printf("%d: %llu\n", i, c);
    for (int j = 0; j < SIZE_OF_CHARACTER_SET; j++) {
      printf(" %2llu", (c & CHILD_MASKS[j]) >> CHILD_SHIFTS[j]);
    }
    printf("\n");
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
    BoardPopulate(board.c_str());
    CurrentScore = ScoreBoard(BoardCount + 1);
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
  free(LexiconMarks);
  free(Nodes);
  free(ChildOffsets);
  free(Tracking);

  return 0;
}
