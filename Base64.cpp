/*

Abstract: C functions to base64 encode / decode a stream of data.

Environment:
User mode only

*/


#include "Base64.h"

static const string base64_chars = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

// 63rd char used for Base64 code
static const char CHAR_63 = '+';

// 64th char used for Base64 code
static const char CHAR_64 = '/';

// Char used for padding
static const char CHAR_PAD = '=';

string Base64::Base64Encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	string ret = "";
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';

	}

	return ret;
}

unsigned int Base64::Base64Decode(const char * encoded_string, unsigned char * bytes_decoded)
{
	// used as temp 24-bits buffer
	union
	{
		unsigned char bytes[ 4 ];
		unsigned int block;
	} buffer;
	buffer.block = 0;

	// number of decoded bytes
	int j = 0;

	size_t codeLength = strlen(encoded_string);

	for( size_t i = 0; i < codeLength; i++ )
	{
		// position in temp buffer
		int m = i % 4;

		char x = encoded_string[i];
		int val = 0;

		// converts base64 character to six-bit value
		if( x >= 'A' && x <= 'Z' )
			val = x - 'A';
		else if( x >= 'a' && x <= 'z' )
			val = x - 'a' + 'Z' - 'A' + 1;
		else if( x >= '0' && x <= '9' )
			val = x - '0' + ( 'Z' - 'A' + 1 ) * 2;
		else if( x == CHAR_63 )
			val = 62;
		else if( x == CHAR_64 )
			val = 63;

		// padding chars are not decoded and written to output buffer
		if( x != CHAR_PAD )
			buffer.block |= val << ( 3 - m ) * 6;
		else
			m--;

		// temp buffer is full or end of code is reached
		// flushing temp buffer
		if( m == 3 || x == CHAR_PAD )
		{
			// writes byte from temp buffer (combined from two six-bit values) to output buffer
			bytes_decoded[ j++ ] = buffer.bytes[ 2 ];
			// more data left?
			if( x != CHAR_PAD || m > 1 )
			{
				// writes byte from temp buffer (combined from two six-bit values) to output buffer
				bytes_decoded[ j++ ] = buffer.bytes[ 1 ];
				// more data left?
				if( x != CHAR_PAD || m > 2 )
					// writes byte from temp buffer (combined from two six-bit values) to output buffer
					bytes_decoded[ j++ ] = buffer.bytes[ 0 ];
			}

			// restarts temp buffer
			buffer.block = 0;
		}

		// when padding char is reached it is the end of code
		if( x == CHAR_PAD )
			break;
	}

	return j;
}