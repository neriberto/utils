#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vector>
#include <string>

// Some not-so-PRO comments

class Dns
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

	static Dns* Instance();
	static void DestroyInstance();

	static int GetSystemDnsServers(std::vector<tstring> &vsDnsServers);
	static int ResolveUrl(const TCHAR *tcsDnsServer, const TCHAR *tcsUrl, TCHAR *tcsResolvedAddr);

private:
	Dns();
	~Dns();

	static Dns* m_Instance;
	bool m_wasInitialized;

	bool init();
	void deinit();

	static bool dnsQuery(const TCHAR *csUrl);
	static bool bExtractIPAddress(char *buffer, char *csIPAddress);
	static bool bExtractName(char **p, char *pStartOfPacket, unsigned __int8 *uc8Name, unsigned __int16 nMaxLength);
};