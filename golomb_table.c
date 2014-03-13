#include "golomb_table.h"

// golomb bit length table is compressed, so we need to
// decompress it.
static short int TVPTLG6GolombCompressed[TVP_TLG6_GOLOMB_N_COUNT][9] = {
		{3,7,15,27,63,108,223,448,130,},
		{3,5,13,24,51,95,192,384,257,},
		{2,5,12,21,39,86,155,320,384,},
		{2,3,9,18,33,61,129,258,511,},
	// Tuned by W.Dee, 2004/03/25
};

void InitGolombTable(GOLOMB_TABLE* table)
{
	int i, n;

	if(table->table_init != 0)
	{
		return;
	}

	for(n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++)
	{
		int a = 0;
		for(i = 0; i < 9; i++)
		{
			int j;
			for(j = 0; j < TVPTLG6GolombCompressed[n][i]; j++)
				table->GolombBitLengthTable[a++][n] = (char)i;
		}
		if(a != TVP_TLG6_GOLOMB_N_COUNT*2*128)
			*(char*)0 = 0;   // THIS MUST NOT BE EXECUETED!
				// (this is for compressed table data check)
	}
	table->table_init = 1;
}

/*
void TVPTLG6InitGolombTable()
{
	if(TVPTLG6GolombTableInit) return;
	for(int n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++)
	{
		for(int a = 0; a < TVP_TLG6_GOLOMB_N_COUNT * 2 * 128; a++)
		{
			int k;
			int nn = (TVP_TLG6_GOLOMB_N_COUNT*2-1) - n;
			for (k = 0; (nn << k) < a; k++);
			if(k > 8)
				k = 8;
			else if(k < 0)
				k = 0;
			TVPTLG6GolombBitLengthTable[a][n] = (char)k;
		}
	}
	TVPTLG6GolombTableInit = true;
}

*/
