#pragma once
#include <winsock2.h>
#include <Windows.h>

#include "AppRiverSecure2.h"
#include "WAUFRedirectorDLLControlInterface.h"

class CHttpFilter
{
public:
	static CHttpFilter* Instance();
	static void DestroyInstance();

	int IsFilterOn();
	int EnableFilter();
	int DisableFilter();
	int UpdatePayload();
	int SetDnsServer(ULONG ulIP, USHORT usPort);
private:
	CHttpFilter();
	~CHttpFilter();

	static CHttpFilter* m_Instance;
	bool m_wasInitialized;
	cWAUFRedirectorDLL * pWAUFRedirectorDLL;

	bool init();
	void deinit();
};