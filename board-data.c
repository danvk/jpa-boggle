#include "board-data.h"

#include <string.h>

void BoardDataExplicitStackQuickSort(BoardDataPtr *Base, unsigned int Size, unsigned int CallingThread){
	BoardDataPtr Temp;
	if ( Size > SORT_TRANSITION_SIZE ) {
		BoardDataPtr *Low = Base;
		BoardDataPtr *High = Low + Size - 1;
		QsortStackNodePtr TheTop = TheQsortStacks[CallingThread] + 1;
		// Declaring "Left", "Right", and "Mid" inside of this while() loop is a valid choice.  "Low", and "High", on the other hand, require a larger scope.
		while ( QSORT_STACK_NOT_EMPTY(CallingThread, TheTop) ) {
			BoardDataPtr *Left;
			BoardDataPtr *Right;
			// Select median value from among "Low", "Mid", and "High".
			// Shift "Low" and "High" so the three values are sorted.
			// This lowers the probability of picking a bad pivot value.
			// Also a comparison is skipped for both "Left" and "Right" in the while loops.
			BoardDataPtr *Mid = Low + ((High - Low) >> 1);
			if ( COMPARE_BOARD_DATA(Mid, Low) ) BOARD_DATA_SWAP(Mid, Low, Temp);
			if ( COMPARE_BOARD_DATA(High, Mid) ) {
				BOARD_DATA_SWAP(Mid, High, Temp);
				if ( COMPARE_BOARD_DATA(Mid, Low) ) BOARD_DATA_SWAP(Mid, Low, Temp);
			}
			// The values at positions Low and High are already known to be in the correct partition.
			Left = Low + 1;
			Right = High - 1;
			// This section of Qsort collapses the walls of "Left" and "Right" until they cross over each other.
			do {
				while ( COMPARE_BOARD_DATA(Left, Mid) ) ++Left;
				while ( COMPARE_BOARD_DATA(Mid, Right) ) --Right;
				// Swap the elements at "Left" and "Right", but make sure to maintain the Mid pointer, because it might get in the way.
				if ( Left < Right) {
					BOARD_DATA_SWAP(Left, Right, Temp);
					if ( Mid == Left) Mid = Right;
					else if ( Mid == Right ) Mid = Left;
					++Left;
					--Right;
				}
				// When "Left" is equal to "Right", make them cross over each other.
				else if ( Left == Right ) {
					++Left;
					--Right;
					break;
				}
			} while ( Left <= Right);
			// Set up pointers for the next partitions, and push the larger partition onto the stack if its size exceeds "SORT_TRANSITION_SIZE".
			// By always pushing the larger of the two partitiona onto the stack, the stack size has an absolute limit of LOG base 2 (Size).
			// Continue sorting the smaller partition if its size exceeds "SORT_TRANSITION_SIZE".
			if ( (Right - Low) <= SORT_TRANSITION_SIZE ) {
				// Ignore both small partitions.
				if ( (High - Left) <= SORT_TRANSITION_SIZE ) QSORT_STACK_POP(Low, High, TheTop);
				// Ignore small left partition.
				else Low = Left;
			}
			// Ignore small right partition.
			else if ( (High - Left) <= SORT_TRANSITION_SIZE ) High = Right;
			// Push the larger left partition indices.
			else if ( (Right - Low) > (High - Left) ) {
				QSORT_STACK_PUSH(TheTop, Low, Right);
				Low = Left;
			}
			// Push the larger right partition indices.
			else {
				QSORT_STACK_PUSH(TheTop, Left, High);
				High = Right;
			}
		}
	}

	// The Base array of BoardData is now partitioned into an ordered sequence of small unsorted blocks.
	// Insertion sort will be used to sort the whole array, because it is faster when shifting over short distances.
	{
		BoardDataPtr *End = Base + Size - 1;
		BoardDataPtr *TempPointer = Base;
		register BoardDataPtr *RunPointer;
		BoardDataPtr *Threshold;

		Threshold = Base + SORT_TRANSITION_SIZE;
		if ( Threshold > End) Threshold = End;

		// Find largest element in first "Threshold" and place it at the beginning of the array.
		// This is the largest array element, and the operation speeds up insertion sort's inner loop.

		for ( RunPointer = TempPointer + 1; RunPointer <= Threshold; ++RunPointer ) if ( COMPARE_BOARD_DATA(RunPointer, TempPointer) ) TempPointer = RunPointer;

		if ( TempPointer != Base ) BOARD_DATA_SWAP(TempPointer, Base, Temp);

		// This is a modification of the GLIBC code that seems to be a shifting optimization.
		// Insertion sort, running from left-hand-side up to right-hand-side.
		// Everything to the left as we go through the array is sorted and the maximum insertion displacement will be SORT_TRANSITION_SIZE
		RunPointer = Base + 1;
		while ( ++RunPointer <= End ) {
			TempPointer = RunPointer - 1;
			// When "RunPointer" needs to be moved, set "TempPointer" to the insertion position.
			while ( COMPARE_BOARD_DATA(RunPointer, TempPointer) ) --TempPointer;
			++TempPointer;
			if ( TempPointer != RunPointer ) {
				// Save the element at position "RunPointer" into "Temp".
				Temp = *RunPointer;
				// Move the elements from the range ("TempPointer" up to just before "RunPointer"), one position towards the end of the array.
				memmove(TempPointer + 1, TempPointer, sizeof(BoardDataPtr)*(RunPointer - TempPointer));
				// Fill the insertion position at "TempPointer" with the "Temp" value.
				*TempPointer = Temp;
			}
		}
	}
}
