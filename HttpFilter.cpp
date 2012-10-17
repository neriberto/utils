// include me first
#include "HttpFilter.h"
#include "ClientCodes.h"
#include "SecureSurfDefines.h"
#include <malloc.h>
#include "Configuration.h"


// standard includes

CHttpFilter* CHttpFilter::m_Instance = 0;

CHttpFilter::CHttpFilter()
{
}

CHttpFilter::~CHttpFilter()
{
}

CHttpFilter* CHttpFilter::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new CHttpFilter();
		if (!m_Instance->init())
		{
			delete m_Instance;
			m_Instance=NULL;
		}
	}

	return m_Instance;
}

void CHttpFilter::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;
		m_Instance = 0;
	}
}
void CHttpFilter::deinit()
{
	try
	{
		delete  pWAUFRedirectorDLL;
		pWAUFRedirectorDLL = NULL;
	}
	catch(...)
	{
	}
}
bool CHttpFilter::init()
{
	bool bResult = false;
	m_wasInitialized = true;
#ifndef _DEBUG
	try
#endif
	{
		pWAUFRedirectorDLL = NULL;
		wchar_t * wszDirectorName=NULL;
		// need to detemine which exe was used for the WAUFProxy service so we can get the full path to the dll
		wchar_t * wszServiceFullpath = cWAUFRedirectorDLL::DoQuerySvc(L"WAUFProxy");
		if (wszServiceFullpath != NULL)
		{
			bool b64Bit = true;		
			if (wcsstr(wszServiceFullpath, L"64") == NULL)
			{
				b64Bit = false;
			}
			size_t stNeeded = (wcslen(wszServiceFullpath)+1)*sizeof(wchar_t)*2;
			wszDirectorName = (wchar_t *) malloc(stNeeded);
			// now we need to use the path for the redirector dll
			size_t stDestSize = stNeeded/sizeof(wchar_t);
			wchar_t * pch;
			wchar_t *next_token = NULL;
			pch = wcstok_s (wszServiceFullpath, L"\\", &next_token);
			// we need the count of these 
			int nCount=0;
			while (pch != NULL)
			{
				nCount++;
				pch = wcstok_s (NULL,  L"\\", &next_token);
			}
			nCount-=2; // we skip the last directory and the executable name
			wszServiceFullpath = cWAUFRedirectorDLL::DoQuerySvc(L"WAUFProxy");
			pch = wcstok_s (wszServiceFullpath, L"\\", &next_token);
			wszDirectorName[0]=L'\0';
			int nCurrent=0;
			while (pch != NULL)
			{
				if (nCurrent<nCount)
				{
					wcscat_s(wszDirectorName, stDestSize, pch);
					wcscat_s(wszDirectorName, stDestSize, L"\\");
				}
				pch = wcstok_s (NULL,  L"\\", &next_token);
				nCurrent++;
			}
			// we only care how this module has been compiled - it must call the same bitness dll for this to work
			// not that this means the bKomodiaSetRedirectorDLLFile function will not necessarly work - the bitness of the loaded
			// dll MUST match the bitness of the WAUFProxy service for that call to work.  The rest of the calls will work ok either way.
#ifdef _WIN64
			{
				wcscat_s(wszDirectorName, stDestSize, L"x64\\WAUFRedirector64.dll");
			}
#else
			{
				wcscat_s(wszDirectorName, stDestSize, L"x86\\WAUFRedirector32.dll");
			}
#endif
			pWAUFRedirectorDLL = new cWAUFRedirectorDLL(wszDirectorName);
			free(wszDirectorName);
			wszDirectorName=NULL;

			bResult = pWAUFRedirectorDLL->Init();
			if (bResult)
			{
				cWAUFRedirectorDLL::DoQuerySvc(NULL); // frees memory used to pass back wszServiceFullpath
			}
			else
			{
				delete pWAUFRedirectorDLL;
				pWAUFRedirectorDLL=NULL;
			}

		}
	}
#ifndef _DEBUG
	catch(...)
	{
	}
#endif

	return bResult;
}
int CHttpFilter::IsFilterOn()
{
	int nResult = G_FAIL;

	try
	{
		if (this != NULL && pWAUFRedirectorDLL != NULL)
		{
			if (pWAUFRedirectorDLL->bGetFilterState())
			{
				nResult = G_TRUE;
			}
			else
			{
				nResult = G_FALSE;
			}
		}

	}
	catch(...)
	{
	}

	return nResult;
}
int CHttpFilter::EnableFilter()
{
	int nResult = G_FAIL;
	try
	{
		if (this != NULL && pWAUFRedirectorDLL != NULL)
		{
			if (pWAUFRedirectorDLL->bTurnHTTPFilteringOn())
			{
				nResult = G_OK;
			}
			else
			{
				nResult = G_FAIL;
			}
		}
	}
	catch(...)
	{
	}
	return nResult;
}
int CHttpFilter::DisableFilter()
{
	int nResult = G_FAIL;
	try
	{
		if (this != NULL && pWAUFRedirectorDLL != NULL)
		{
			if (pWAUFRedirectorDLL->bTurnHTTPFilteringOff())
			{
				nResult = G_OK;
			}
			else
			{
				nResult = G_FAIL;
			}
		}
	}
	catch(...)
	{
	}
	return nResult;
}
int CHttpFilter::SetDnsServer(ULONG ulIP, USHORT usPort)
{
	int nResult = G_FAIL;
	try
	{
		if (this != NULL && pWAUFRedirectorDLL != NULL)
		{
			if (pWAUFRedirectorDLL->bSetDNSServer(ulIP, usPort))
			{
				nResult = G_OK;
			}
			else
			{
				nResult = G_FAIL;
			}
		}
	}
	catch(...)
	{
	}
	return nResult;
}
int CHttpFilter::UpdatePayload()
{
	int nResult = G_FAIL;
	try
	{
		if (this != NULL && pWAUFRedirectorDLL != NULL)
		{
			if (pWAUFRedirectorDLL->bUpdatePayload())
			{
				nResult = G_OK;
			}
			else
			{
				nResult = G_FAIL;
			}
		}
	}
	catch(...)
	{
	}
	return nResult;
}
