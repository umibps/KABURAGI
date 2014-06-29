#ifndef GolombTableH
#define GolombTableH

// Table for 'k' (predicted bit length) of golomb encoding
#define TVP_TLG6_GOLOMB_N_COUNT 4

typedef struct _GOLOMB_TABLE
{
	char GolombBitLengthTable[TVP_TLG6_GOLOMB_N_COUNT*2*128][TVP_TLG6_GOLOMB_N_COUNT];
	int table_init;
} GOLOMB_TABLE;

extern void InitGolombTable(GOLOMB_TABLE* table);

#endif
