#include "tlg6_bit_stream.h"
#include "memory.h"

#define BLOCK_SIZE 4096*1024
TLG6BIT_STREAM* CreateTLG6BIT_STREAM(void)
{
	TLG6BIT_STREAM* ret =
		(TLG6BIT_STREAM*)MEM_ALLOC_FUNC(sizeof(*ret));

	(void)memset(ret, 0, sizeof(*ret));

	ret->out_stream = CreateMemoryStream(BLOCK_SIZE);
	ret->buffer = (unsigned char*)MEM_ALLOC_FUNC(BLOCK_SIZE);
	(void)memset(ret->buffer, 0, BLOCK_SIZE);
	ret->capacity = BLOCK_SIZE;

	InitGolombTable(&ret->table);

	return ret;
}

void DeleteTLG6BIT_STREAM(TLG6BIT_STREAM** stream)
{
	MEM_FREE_FUNC((*stream)->buffer);
	DeleteMemoryStream((*stream)->out_stream);
	MEM_FREE_FUNC(*stream);

	*stream = NULL;
}

void TLG6BitStreamFlush(TLG6BIT_STREAM* stream)
{
	if(stream->buffer != NULL
		&& (stream->buffer_bit_pos != 0 || stream->buffer_byte_pos != 0))
	{
		if(stream->buffer_bit_pos != 0)
		{
			stream->buffer_byte_pos++;
		}

		(void)MemWrite(stream->buffer, 1, stream->buffer_byte_pos,
			stream->out_stream);
		stream->buffer_byte_pos = 0;
		stream->buffer_bit_pos = 0;
	}

	MEM_FREE_FUNC(stream->buffer);
	stream->buffer = NULL;
	stream->capacity = 0;
}

void TLG6Put1Bit(TLG6BIT_STREAM* stream, int b)
{
	if(stream->buffer_byte_pos == stream->capacity)
	{	// バッファが満杯
		unsigned char* temp =
			(unsigned char*)MEM_ALLOC_FUNC(stream->capacity+BLOCK_SIZE);
		(void)memcpy(temp, stream->buffer, stream->capacity);
		(void)memset(&temp[stream->capacity], 0, BLOCK_SIZE);
		MEM_FREE_FUNC(stream->buffer);
		stream->buffer = temp;
		stream->capacity += BLOCK_SIZE;
	}

	if(b != 0)
	{
		stream->buffer[stream->buffer_byte_pos] |= 1 << stream->buffer_bit_pos;
	}
	stream->buffer_bit_pos++;

	if(stream->buffer_bit_pos == 8)
	{
		stream->buffer_bit_pos = 0;
		stream->buffer_byte_pos++;
	}
}

void TLG6PutGamma(TLG6BIT_STREAM* stream, int v)
{
	// Put a gamma code.
	// v must be larger than 0.
	int t = v;
	int cnt = 0;
	t >>= 1;

	while(t != 0)
	{
		TLG6Put1Bit(stream, 0);
		t >>= 1;
		cnt ++;
	}
	TLG6Put1Bit(stream, 1);
	while(cnt--)
	{
		TLG6Put1Bit(stream, v&1);
		v >>= 1;
	}
}

void TLG6PutInterleavedGamma(TLG6BIT_STREAM* stream ,int v)
{
	// Put a gamma code, interleaved.
	// interleaved gamma codes are:
	//     1 :                   1
	//   <=3 :                 1x0
	//   <=7 :               1x0x0
	//  <=15 :             1x0x0x0
	//  <=31 :           1x0x0x0x0
	// and so on.
	// v must be larger than 0.
		
	v --;
	while(v != 0)
	{
		v >>= 1;
		TLG6Put1Bit(stream, 0);
		TLG6Put1Bit(stream, v&1);
	}
	TLG6Put1Bit(stream, 1);
}

void TLG6PutNonzeroSigned(TLG6BIT_STREAM* stream, int v, int len)
{
	// Put signed value into the bit pool, as length of "len".
	// v must not be zero. abs(v) must be less than 257.
	if(v > 0) v--;
	while(len --)
	{
		TLG6Put1Bit(stream, v&1);
		v >>= 1;
	}
}

static int GetNonzeroSignedBitLength(int v)
{
	// Get bit (minimum) length where v is to be encoded as a non-zero signed value.
	// v must not be zero. abs(v) must be less than 257.
	if(v == 0) return 0;
	if(v < 0) v = -v;
	if(v <= 1) return 1;
	if(v <= 2) return 2;
	if(v <= 4) return 3;
	if(v <= 8) return 4;
	if(v <= 16) return 5;
	if(v <= 32) return 6;
	if(v <= 64) return 7;
	if(v <= 128) return 8;
	if(v <= 256) return 9;
	return 10;
}

void TLG6PutValue(TLG6BIT_STREAM* stream ,long v, int len)
{
	// put value "v" as length of "len"
	while(len --)
	{
		TLG6Put1Bit(stream, v&1);
		v >>= 1;
	}
}

int GetGammaBitLengthGeneric(int v)
{
	int needbits = 1;
	v >>= 1;
	
	while(v)
	{
		needbits += 2;
		v >>= 1;
	}
	
	return needbits;
}

int GetGammaBitLength(int v)
{
	// Get bit length where v is to be encoded as a gamma code.
	if(v<=1) return 1;    //                   1
	if(v<=3) return 3;    //                 x10
	if(v<=7) return 5;    //               xx100
	if(v<=15) return 7;   //             xxx1000
	if(v<=31) return 9;   //          x xxx10000
	if(v<=63) return 11;  //        xxx xx100000
	if(v<=127) return 13; //      xxxxx x1000000
	if(v<=255) return 15; //    xxxxxxx 10000000
	if(v<=511) return 17; //   xxxxxxx1 00000000
	return GetGammaBitLengthGeneric(v);
}
