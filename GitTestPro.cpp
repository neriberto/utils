// include me first
#include "Dns.h"
#include "ClientCodes.h"

// standard includes
#include <IPHlpApi.h>
#include <WinSock2.h>
#include <IPTypes.h>

// define structures necessary to send and recieve DNS requests
#pragma pack(push)
#pragma pack(1)

// we expect to see the following fields and subfields in the record - or to fill them in if we are sending it
// RR - Transaction ID
//    - Flags - Response?
//    - Optcode - standard query=0 
//    - Truncated?
//    - Recursion desired
//    - Recursion available (only on answers)
//    - reserved
//    - Answer Authenticated?
//    - Non-authenticated data: unacceptable?
//    - reply code - No Error = 0
//    - Question Count
//    - Answer RR count
//    - Authority RR count (no idea what this is)
//    - Additonal RR count (no idea on this one either)
// QUERY ... repeats according to question count above
//    - CNAME_LENGTH 
//    -  either a COMPRESSED or UNCOMPRESSED CNAME
//       - repeats in seqments until we hit a 0 count
//    TYPE_CLASS_TTL_DATALENGTH
//      Type
//      Class
// ANSWER ... repeats according to question count above
//    - CNAME_LENGTH 
//    -  either a COMPRESSED or UNCOMPRESSED CNAME
//       - repeats in seqments until we hit a 0 count
//    TYPE_CLASS_TTL_DATALENGTH
//      Type
//      Class
//      TTL if an answer
//      Data Length if an answer
//    Data if an answer
//  
// we do not care about anything else at this point

#define LITTLE_ENDIAN_INT16_ONE   0x0100
#define LITTLE_ENDIAN_INT16_ZERO  0x0000
inline unsigned __int16 flipbytes(unsigned __int16 ui16IN)
{
	// Some ugly non-PRO comments and typos
	nonpro int noprobelm;
	unsigned __int16 ui16Result;
	union NONFLIPPED
	{
		unsigned __int16 ui16;
		struct LH
		{
			unsigned __int8 ui8L;
			unsigned __int8 ui8H;
		} slh;
	} snf;
	union FLIPPED
	{
		unsigned __int16 ui16;
		struct LH
		{
			unsigned __int8 ui8H;
			unsigned __int8 ui8L;
		} shl;
	} sf;
	// flip byte order
	snf.ui16 = ui16IN;
	sf.shl.ui8L = snf.slh.ui8L;
	sf.shl.ui8H = snf.slh.ui8H;
	ui16Result = sf.ui16;
	return ui16Result;
}

#define REPLYCODE_NO_SUCH_NAME 0x03
#define REPLYCODE_SUCCESS 0x01
struct RR
{
	unsigned __int16 id; // unique id number - question and response must match
	struct BITFLAGS
	{
		// bit fields
		// bits are stacked by MS C++ right to left
		unsigned __int16  bfRecursionDesired:1;
		unsigned __int16  bfTruncated:1;
		unsigned __int16  bfAuthoritative:1;
		unsigned __int16  bfOpcode:4;
		unsigned __int16  bfResponse:1;
		unsigned __int16  bfReplyCode:4;
		unsigned __int16  bfNonAuthenticatedData:1;
		unsigned __int16  bfAnswerAuthenticated:1;
		unsigned __int16  bZReserved:1;
		unsigned __int16  bfRecursionAvailable:1;
	} sFlags;

	unsigned __int16 ui16QuestionCount;
	unsigned __int16 ui16AnswerCount;
	unsigned __int16 ui16AuthorityRRCount;
	unsigned __int16 ui16AdditonalRRCount;
} * pRR;


struct CNAME_LENGTH
{
	unsigned __int8 len;  // if first two bits are 11 then we are using "Message compression" and the next 14 bits define the actual offset from the start of the record
} * pCNAME;

struct UNCOMPRESSED
{
	// the following structure will continue until we get a zero length
	unsigned __int8 ui8Length;
	char cNameSection;
} *pUnCompressedQuery;

struct COMPRESSED
{
	struct CNAME
	{
		// bits are stacked by MS C++ right to left
		unsigned __int16 ubOffset:14; // offset from start of record
		unsigned __int16 ubCompress:2; // if this equals binary 11 then we are compressed
	} pCName;
} * pCompressedQuery;

#define TYPE_A 0x0100
#define TYPE_HOST TYPE_A

#define CLASS_IN 0x0100

struct TYPE_CLASS
{
	unsigned __int16 ui16Type;
	unsigned __int16 ui16Class;
} * pTC; 

struct TYPE_CLASS_TTL_DATALENGTH
{
	TYPE_CLASS tc;
	unsigned __int32 ui32TTL;
	unsigned __int16 ui16DataLength;
} * pTCTD; 

struct IP4_ADDRESS
{
	unsigned __int8 n[4];
} * pIP4Address;

struct IP_ADDRESS
{
	// probably need IP4 and IP6 defines here...
	// for now we are only implementing IPv4 
	union IP_ADDRESS_TYPES
	{
		IP4_ADDRESS pIP4Address;
	} pIPAddressTypes;
} * pIPAddress;

typedef unsigned __int8 * QUERY, *QUESTION;

#pragma pack(pop)

using namespace std;

#define DEFAULT_PORT "53"
#define DEFAULT_BUFLEN 512
#define BUFFER_SIZE 4096
#define MAX_DNS_NAME_LENGTH 512 // 64 byte is the normal limit - lets allow 8 times that for the appriver expanded names
#define MAX_IP_ADDRESS_LENGTH 128 // lots of disagreement on max text size for IPv6 - have seen 39 and 45 - bettting 128 will be safe
Dns* Dns::m_Instance = 0;

Dns::Dns()
{
}

Dns::~Dns()
{
}

Dns* Dns::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new Dns();
		m_Instance->init();
	}

	return m_Instance;
}

void Dns::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;
		m_Instance = 0;
	}
}
void Dns::deinit()
{
}
bool Dns::init()
{
	m_wasInitialized = true;

	return true;
}
int Dns::GetSystemDnsServers(vector<tstring> &vsDnsServers)
{
	// Get DNS server list
	FIXED_INFO *pFixedInfo;
	ULONG ulOutBufLen;
	DWORD dwRetVal;
	IP_ADDR_STRING *pIPAddr;
	int rc = G_FAIL;

	pFixedInfo = (FIXED_INFO *)malloc(sizeof (FIXED_INFO));
	if (pFixedInfo == NULL)
	{
		// Error allocating memory needed to call GetNetworkParams
		return rc;
	}
	ulOutBufLen = sizeof (FIXED_INFO);

	// Make an initial call to GetNetworkParams to get the necessary size into the ulOutBufLen variable
	if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pFixedInfo);
		pFixedInfo = (FIXED_INFO *)malloc(ulOutBufLen);
		if (!pFixedInfo)
		{
			// Error allocating memory needed to call GetNetworkParams
			return rc;
		}
	}

	// Now get the network info
	if (dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen)!=NO_ERROR)
	{
		// Failed to retrieve network parameters for the local computer
		free(pFixedInfo);
		return rc;
	}

	// Iterate through each DNS in the list that we got
	pIPAddr = &(pFixedInfo->DnsServerList);
	while(pIPAddr!=NULL)
	{
		TCHAR tcsIpAddr[16] = { 0 }, tcsDnsAddr[16] = { 0 };

#ifdef UNICODE
		// Convert to a wchar_t*
		size_t origsize = strlen(pIPAddr->IpAddress.String) + 1;
		size_t convertedChars = 0;
		mbstowcs_s(&convertedChars, tcsDnsAddr, origsize, pIPAddr->IpAddress.String, _TRUNCATE);
#else
		tcsDnsAddr = pIPAddr->IpAddress.String;
#endif
		
		vsDnsServers.push_back(tcsDnsAddr);

		rc = G_OK;

		// Get the next one in the list
		pIPAddr = pIPAddr->Next;
	}

	// Free resources
	if(pFixedInfo!=NULL)
		free(pFixedInfo);

	return rc;
}

int Dns::ResolveUrl(const TCHAR *tcsDnsServer, const TCHAR *tcsUrl, TCHAR *tcsResolvedAddr)
{
	// Validate input
	if(!tcsDnsServer || !tcsUrl)
		return G_INVALID_PARAM;

	// Do necessary TCHAR conversion
	char *csDnsServer, *csUrl;
#ifdef UNICODE
	size_t origsize = wcslen(tcsDnsServer) + 1;
	csDnsServer = new char[origsize];
	memset(csDnsServer, 0, origsize);
	WideCharToMultiByte(CP_ACP, 0, tcsDnsServer, -1, csDnsServer , (int)origsize, 0, 0);

	origsize = wcslen(tcsUrl) + 1;
	csUrl = new char[origsize];
	memset(csUrl, 0, origsize);
	WideCharToMultiByte(CP_ACP, 0, tcsUrl, -1, csUrl , (int)origsize, 0, 0);
#else
	csDnsServer = tcsDnsServer;
	csUrl = tcsUrl;
#endif

	static size_t stIPResultSize = 0;
	static size_t stIPProxySize = 0;
	static unsigned short usSequenceNumber = 0;
	int rc = G_FAIL;

	char szIPAddress[16];

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	char buffer [BUFFER_SIZE];
	char *p=buffer;

	int iResult = 0;
	sockaddr_in server;
	u_short port_number;
	SOCKET sd = INVALID_SOCKET;

	char *pUrlDots;
	char *pUrl;

	// Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		// Tell the user that we could not find a usable Winsock DLL.
		// printf("WSAStartup failed with error: %d\n", err);
		return rc;
	}

	// Confirm that the WinSock DLL supports 2.2.
	// Note that if the DLL supports versions greater    
	// than 2.2 in addition to 2.2, it will still return 
	// 2.2 in wVersion since that is the version we      
	// requested.
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
		goto end; // Tell the user that we could not find a usable WinSock DLL.

	// Clear out server struct
	memset((void *)&server, '\0', sizeof(struct sockaddr_in));

	// Set family and port
	server.sin_family = AF_INET;
	port_number = atoi(DEFAULT_PORT);
	server.sin_port = htons(port_number);

	// Get address of DNS server
	server.sin_addr.s_addr = inet_addr(csDnsServer);

	// Close previously opened socket
	if(sd!=INVALID_SOCKET)
	{
		closesocket(sd);
		sd = INVALID_SOCKET;
	}

	// Open a datagram socket
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == INVALID_SOCKET)
		goto end; // Failed to create socket

	// Set a recv() timeout on socket
	struct timeval tv;
	tv.tv_sec = 5000; // Though it is 'tv_sec', it seems to be in milliseconds based on testing.
	tv.tv_usec = 0;
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,  sizeof tv)==SOCKET_ERROR)
		goto end;

	// Connect to DNS server
	// If no error occurs, connect returns zero.
	if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr_in))==SOCKET_ERROR)
	{
		// Could not bind name to socket
		goto end;
	}

	// Now we want to create a simple query to send to the DNS server
	{
		// now setup the actual question section
		// transaction ID -- hmmm - guess?
		memset(buffer, 0, BUFFER_SIZE);
		p = buffer;
		pRR = (RR *) p;

		usSequenceNumber = 0;
		if (usSequenceNumber == 0)
		{
			time_t ttTemp;
			usSequenceNumber = (unsigned short) time(&ttTemp);
		}
		usSequenceNumber++;
		pRR->id = usSequenceNumber;

		pRR->sFlags.bfRecursionAvailable=0;
		pRR->sFlags.bZReserved=0;
		pRR->sFlags.bfAnswerAuthenticated=0;
		pRR->sFlags.bfNonAuthenticatedData=0;
		pRR->sFlags.bfReplyCode=0;
		pRR->sFlags.bfResponse=0;
		pRR->sFlags.bfOpcode=0;
		pRR->sFlags.bfAuthoritative=0;
		pRR->sFlags.bfTruncated=0;
		pRR->sFlags.bfRecursionDesired=1;  // the way the one in Wireshark was...

		pRR->ui16QuestionCount = LITTLE_ENDIAN_INT16_ONE;
		pRR->ui16AnswerCount = LITTLE_ENDIAN_INT16_ZERO;
		pRR->ui16AuthorityRRCount = LITTLE_ENDIAN_INT16_ZERO;
		pRR->ui16AdditonalRRCount = LITTLE_ENDIAN_INT16_ZERO;

		p += sizeof(RR);

		// we need to break the URL on the "."s into seperate pieces
		pUrlDots = NULL;
		pUrl = csUrl;
		do
		{
			// Return a pointer to the first occurrence of a search string in a string.
			pUrlDots = strstr(pUrl, ".");
			if (pUrlDots != NULL)
			{
				*pUrlDots = '\0';
			}

			size_t stSize = strlen(pUrl);
			pUnCompressedQuery = (UNCOMPRESSED *) p;
			pUnCompressedQuery->ui8Length = stSize;
			memcpy(&pUnCompressedQuery->cNameSection, pUrl, stSize);
			p += sizeof(pUnCompressedQuery->ui8Length)+stSize;

			pUrl = pUrlDots+1;
		}
		while (pUrlDots !=NULL);

		pUnCompressedQuery = (UNCOMPRESSED *) p;
		pUnCompressedQuery->ui8Length = 0;
		p += sizeof(pUnCompressedQuery->ui8Length);

		pTC = (TYPE_CLASS *) p;
		pTC->ui16Type = TYPE_A; // HOST ADDRESS
		pTC->ui16Class = CLASS_IN; // IN

		p += sizeof(TYPE_CLASS);

		// done - send packet
		int client_length = (int)sizeof(struct sockaddr_in);
		int nBufferLen = p-buffer;
		if (sendto(sd, buffer, 
			nBufferLen, 0, 
			(struct sockaddr *)&server, client_length) != 
			nBufferLen)
		{
			// Error sending datagram.
			goto end;
		}		

		// now read the response
		// Receive bytes from client
		int bytes_received = recv(sd, buffer, BUFFER_SIZE, 0);
		if (bytes_received < 0)
		{
			// Could not receive datagram.
			goto end;
		}

		// Extract & Check the IP address
		if(bExtractIPAddress(buffer, szIPAddress))
		{
#ifdef UNICODE
			// Convert to a wchar_t*
			origsize = strlen(szIPAddress) + 1;
			size_t convertedChars = 0;
			mbstowcs_s(&convertedChars, tcsResolvedAddr, origsize, szIPAddress, _TRUNCATE);
#else
			strcpy_s(tcsResolvedAddr, strlen(szIPAddress)+1, szIPAddress);
#endif
			rc = G_OK;
		}
	}

end:

	if (sd!=INVALID_SOCKET)
		closesocket(sd);

	WSACleanup();

#ifdef UNICODE
	if(csDnsServer!=NULL)
		delete [] csDnsServer;
	if(csUrl!=NULL)
		delete [] csUrl;
#endif

	return rc;
}
/* Description: Extract the IP address from the Answer portion of the DNS response.
 */
bool Dns::bExtractIPAddress(char *buffer, char *csIPAddress)
{
	RR *pRR = (RR*)buffer;
	char * p;
	bool bFoundAnswer = false; // so we take the first answer we find

	// now enumerate the questions (queries)
	int nQuestionCount = flipbytes(pRR->ui16QuestionCount);
	unsigned __int8 uc8Name[MAX_DNS_NAME_LENGTH];  
	memset(uc8Name, 0, MAX_DNS_NAME_LENGTH);
	p = buffer + sizeof(RR);

	// Move the pointer past all the questions
	for (int i=0; i<nQuestionCount; i++)
	{
		// Even if we don't need the name, we have to call this to move the pointer
		if (bExtractName(&p, buffer, uc8Name, MAX_DNS_NAME_LENGTH) != true)
		{
			// name extraction failed; abort
			return bFoundAnswer;
		}

		p += sizeof(TYPE_CLASS);
	}

	// now enumerate the answers
	unsigned __int8 uc8IP[MAX_IP_ADDRESS_LENGTH]; 
	int nAnswerCount = flipbytes(pRR->ui16AnswerCount);	
	memset(uc8Name, 0, MAX_DNS_NAME_LENGTH);
	for (int j=0; j<nAnswerCount; j++)
	{
		if (!bExtractName(&p, buffer, uc8Name, MAX_DNS_NAME_LENGTH))
		{
			// name extraction failed; abort
			return bFoundAnswer;
		}

		// each one also has two final values - four if a response
		pTCTD = (TYPE_CLASS_TTL_DATALENGTH *) p;
		p += sizeof(TYPE_CLASS_TTL_DATALENGTH);
		// and we have some data - hopefully an ip4 payload
		pIPAddress = (IP_ADDRESS *) p;
		pTCTD->ui16DataLength = flipbytes(pTCTD->ui16DataLength);
		p += pTCTD->ui16DataLength;
		if (pTCTD->ui16DataLength == sizeof(IP4_ADDRESS))
		{
			pIP4Address = &pIPAddress->pIPAddressTypes.pIP4Address; // simpler with shorter name pointer
			char * pTemp = (char *) uc8IP;
			memset(pTemp, 0, MAX_IP_ADDRESS_LENGTH);
			size_t nRemaining = MAX_IP_ADDRESS_LENGTH;
			for (int i=0; i<4; i++)
			{
				_itoa_s(pIP4Address->n[i], pTemp, nRemaining, 10);
				int nLen=(int)strlen(pTemp);
				if (i!=3)
				{
					strcat_s(pTemp, nRemaining-nLen, ".");
					nLen++;
				}
				nRemaining-=nLen;
				pTemp+=nLen;
			}

			if (!bFoundAnswer)
			{
				// Store IP address into string
				bFoundAnswer = true;
				size_t st=strlen((char *)uc8IP)+1;				
				strcpy_s(csIPAddress, st, (char *) uc8IP);
				break;
			}
		}
	}

	return bFoundAnswer;
}
bool Dns::bExtractName(char **p, char *pStartOfPacket, unsigned __int8 *uc8Name, unsigned __int16 nMaxLength)
{
	bool bResult=true;
	unsigned __int8 * pCNameBuffer = (unsigned __int8 *) *p;
	bool bCompressed=false;
	unsigned __int16 ui16Temp;
	// try the compress version first - if it is not then we can switch to the uncompress
	// byte order issue here - copy to temp and swap bytes first
	ui16Temp = flipbytes(*((unsigned __int16 *)*p));
	pCompressedQuery = (COMPRESSED *)&ui16Temp;
	if (pCompressedQuery->pCName.ubCompress == 0x03) 
	{
		// we have a compressed name ("Message compression")
		pCNameBuffer = (unsigned __int8 *) pStartOfPacket+pCompressedQuery->pCName.ubOffset;
		// advance the 
		*p+=sizeof(COMPRESSED);
		bCompressed=true;
	}
	// now we need to decode  the CNAME - it is stored at offset pCName as a series of lengths and characters - a zero length ends the list
	unsigned __int8 uc8NameLength = 0;
	unsigned __int8 ui8SegmentLen = 0;
	bool bFirst=true;
	unsigned __int8 * pvCName = (unsigned __int8 *) uc8Name;
	do
	{
		pCNameBuffer+=ui8SegmentLen;
		pUnCompressedQuery = (UNCOMPRESSED *) pCNameBuffer;
		ui8SegmentLen = pUnCompressedQuery->ui8Length;
		if (ui8SegmentLen > 0)
		{
			if (!bFirst)
			{
				if (uc8NameLength+ui8SegmentLen < nMaxLength)
				{
					*pvCName = '.';
					uc8NameLength++;
					pvCName++;
				}
				else
				{
					// errror - what to do 
					return false;
				}
			}
			else
			{
				bFirst = false;
			}

			if (uc8NameLength+ui8SegmentLen < nMaxLength)
			{
				memcpy(pvCName, &pUnCompressedQuery->cNameSection, ui8SegmentLen);
				uc8NameLength+=ui8SegmentLen;
				pvCName+=ui8SegmentLen;
			}
			else
			{
				// error - what to do?
				return false;
			}
			// add in the length value length too
			ui8SegmentLen+=sizeof(pUnCompressedQuery->ui8Length);
			if (!bCompressed)
			{
				// we need to advance the current buffer since we are parsing actual bytes
				*p+=ui8SegmentLen;
			}
		}

	}
	while (ui8SegmentLen > 0);
	if (!bCompressed)
	{
		*p+=sizeof(pUnCompressedQuery->ui8Length);
	}
	// we should now have the full name in the buffer
	return bResult;
}