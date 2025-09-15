#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BYTE_WIDTH 8
#define INT_WIDTH 32
#define TWO_UP_EIGHT 256

// Store the computed table values in a data file to be used in the Bytewise CRC calculation program.
#define OUTPUT_FILE "CRC-32.dat"

const unsigned int Poly = 0X04C11DB7;
const unsigned int MSB = 0X80000000;
const unsigned int PowersOfTwo[INT_WIDTH] = { 0X1, 0X2, 0X4, 0X8, 0X10, 0X20, 0X40, 0X80, 0X100, 0X200, 0X400, 0X800,
 0X1000, 0X2000, 0X4000, 0X8000, 0X10000, 0X20000, 0X40000, 0X80000, 0X100000, 0X200000, 0X400000, 0X800000, 0X1000000,
 0X2000000, 0X4000000, 0X8000000, 0X10000000, 0X20000000, 0X40000000, 0X80000000 };

// Feed this function "TableIndex" and it will return the corresponding Lookup-Value for CRC-8.
unsigned int GenerateLookupTableValue(unsigned char TableIndex){
	unsigned int X;
	unsigned int TheRegister = TableIndex<<24;
	printf("|%d|\n", TheRegister);
	// Shift a Byte worth of zeros into the register, and when a "1" is shifted out, perform the required "XOR" operation with "Poly".
	for ( X = BYTE_WIDTH; X; X-- ) {
		if ( MSB & TheRegister ) TheRegister = (TheRegister << 1) ^ Poly;
		else TheRegister <<= 1;
	}
	return TheRegister;
}

// Simply print out "ThisByte" using "1"s and "0"s.
void PrintIntInBinary(unsigned int ThisByte){
	unsigned int X;
	char HoldOut[INT_WIDTH + 1];
	for ( X = 0; X < INT_WIDTH; X++ ) HoldOut[X] = (ThisByte & PowersOfTwo[INT_WIDTH - 1 - X])? '1': '0';
	HoldOut[INT_WIDTH] = '\0';
	printf("%s", HoldOut);
}

int main(){
	int X;
	int Y;
	unsigned int CalculatedValues[TWO_UP_EIGHT];
	FILE* DataOut;
	DataOut = fopen(OUTPUT_FILE, "wb");
	for ( X = 0; X < TWO_UP_EIGHT; X++ ) {
		CalculatedValues[X] = GenerateLookupTableValue((unsigned char)X);
		printf("|%3u| - Value-|%2X| - Binary|", X, CalculatedValues[X]);
		PrintIntInBinary(CalculatedValues[X]);
		printf("|\n");
	}
	
	// Verify that each value in the lookup table is unique.
	// Each value can be the seen as the remainder when a 2-Byte message is divided by "Poly".
	// The second Byte is always "W" zeros, so only the first byte changes, making it impossible for two remainders to be the same.
	for ( X = 1; X < TWO_UP_EIGHT; X++ ) {
		for ( Y = (X - 1); Y >= 0; Y-- ) {
			if ( CalculatedValues[X] == CalculatedValues[Y] ) printf("Duplicate values found at |%d|, and |%d|\n", X, Y);
		}
	}
	printf("The search for duplicate values in the table is complete.\n");
	
	fwrite(CalculatedValues, sizeof(unsigned int), TWO_UP_EIGHT, DataOut);
	fclose(DataOut);
	return 0;
}
