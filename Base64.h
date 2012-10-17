/*

Abstract: C header file which exposes functions to base64 encode / decode a stream of data.

Environment:
User mode only

*/

#include <string>

using namespace std;

class Base64 {
public:
	Base64(void);
	~Base64(void);

	static string Base64Encode(unsigned char const* bytes_to_encode, unsigned int in_len);
	static unsigned int Base64Decode(const char * encoded_string, unsigned char * bytes_decoded);
	static inline bool IsBase64(unsigned char c) {
		return (isalnum(c) || (c == '+') || (c == '/'));
	}
};