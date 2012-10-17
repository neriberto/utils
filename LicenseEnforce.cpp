#include "LicenseEnforce.h"
#include "RegUtil.h"
#include "thorwac.h"
#include "StringUtil.h"
#include "EseCodes.h"
#include "RegistryHash.h"

using namespace WAUtils;

#include <process.h> // _beginthreadex
#include <time.h>
#include <strsafe.h>

using namespace std;

struct EseLeParams {
	CLicenseEnforce::ComponentType componentType;
	CLicenseEnforce::tstring wsComponentName;
};

CLicenseEnforce* CLicenseEnforce::m_Instance = 0;

CLicenseEnforce::CLicenseEnforce() 
:	m_hMonitorEvent(NULL),
m_bMonitoring(FALSE),
m_hThread(NULL)
{	
	m_mapTokenToLECode[THORWA_TK_AVAILABLE]			= LE_OK;
	m_mapTokenToLECode[THORWA_TK_OUTOFSTOCK]		= LE_ERR_LICENSEOUTOFTOKENS;
	// Even though THORWA_TK_UNKNOWN is exposed, it is actually only used internally by the Licenser. The user just needs to handle cases
	// for the THORWA_RC_UNKNOWN* return codes.
	//m_mapTokenToLECode[THORWA_TK_UNKNOWN]				= LE_ERR_LICENSEINVALID;

	ComponentInfo ci;
	ci.setLicenseState(LE_ERR_LICENSEUNKNOWN, TEXT(""));
	ci.wsLicense = TEXT("");
	ci.onLicenseStatusChange = NULL;
	ci.uiTrialPeriod = 0;
	ci.wsRegKey = TEXT("");
	ci.wsRegAttr = TEXT("");

	// All components start with an unknown license state.
	m_mapComponentInfo[Ese]							= ci;
	m_mapComponentInfo[Svd]							= ci;
	m_mapComponentInfo[Md4sa]						= ci;
	m_mapComponentInfo[UrlFiltering]				= ci;

	m_mapComponentInfo[Ese].wsRegKey				= REGKEY_ESE_PATH;
	m_mapComponentInfo[Ese].wsRegAttr				= REGKEY_PARAM_ACCOUNTID;
}

CLicenseEnforce::~CLicenseEnforce() 
{
	m_hMonitorEvent = NULL;
	m_bMonitoring = FALSE;
	m_hThread = NULL;
}

CLicenseEnforce* CLicenseEnforce::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new CLicenseEnforce();
		m_Instance->init();
	}

	return m_Instance;
}

void CLicenseEnforce::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;  
		m_Instance = 0;
	}
}
void CLicenseEnforce::deinit()
{
	Stop();
	DeleteCriticalSection(&m_critsecLicense);
	CloseHandle(m_hMonitorEvent);
	m_mapComponentInfo.clear();
}
bool CLicenseEnforce::init() 
{ 
	InitializeCriticalSection(&m_critsecLicense);
	m_hMonitorEvent = CreateEvent(NULL, // No security attributes
		TRUE,   // Manual-reset event
		FALSE,  // Initial state is signaled
		NULL);
	return true;
}
CLicenseEnforce::LECode CLicenseEnforce::EnforceTrial(ComponentType ct, unsigned int uiTrialPeriod)
{
	if(m_mapComponentInfo.find(ct)!=m_mapComponentInfo.end())
	{
		m_mapComponentInfo[ct].uiTrialPeriod = uiTrialPeriod;
		return LE_OK;
	}
	else
		return LE_ERR_INVALIDARGUMENT;
}
CLicenseEnforce::LECode CLicenseEnforce::SetEventHandler(ComponentType componentType, OnLicenseStatusChange onLicenseStatusChange)
{
	LECode rc = LE_ERR_GENERIC;

	if(!onLicenseStatusChange)
		return LE_ERR_INVALIDARGUMENT;

	switch(componentType)
	{
	case Ese:
	case Svd:
		m_mapComponentInfo[componentType].onLicenseStatusChange = onLicenseStatusChange;
		rc = LE_OK;
		break;
	default:
		rc = LE_ERR_BADCOMPONENT;
		break;
	}

	return rc;
}

/* Description: License activation for a specific component specified by the user.
*/
CLicenseEnforce::LECode CLicenseEnforce::ActivateLicense(ComponentType componentType, const TCHAR *wcsLicenseKey)
{
	int iRes, iTokenState;
	LECode rc = LE_ERR_GENERIC;
	tstring wsRegKey, wsRegAttr;	

	if(!wcsLicenseKey)
		return LE_ERR_INVALIDARGUMENT;

	// Activate license for specified component
	switch(componentType)
	{
	case Ese:
		iRes = thorwac_il(THORWA_COMP_ESE, util_strsafe_convert_tcs(wcsLicenseKey).c_str(), &iTokenState, NULL);
		wsRegKey = m_mapComponentInfo[Ese].wsRegKey;
		wsRegAttr = m_mapComponentInfo[Ese].wsRegAttr;
		break;
	default:
		rc = LE_ERR_BADCOMPONENT;
		break;
	}

	// If successful, store to the system (registry) and in memory (to keep track of license and license states)
	if(iRes==THORWA_RC_SUCCESS)
	{
		// Get the callback function for the component
		OnLicenseStatusChange onLicenseStatusChange = NULL;
		if(isCallbackAvailable(componentType))
			onLicenseStatusChange = m_mapComponentInfo[componentType].onLicenseStatusChange;
		// Convert token status to LECode
		LECode lecode = m_mapTokenToLECode[iTokenState];

		_ASSERTE(
			iTokenState==THORWA_TK_AVAILABLE ||
			iTokenState==THORWA_TK_OUTOFSTOCK);

		EnterCriticalSection(&m_critsecLicense);

		// Store the license key encrypted in the registry
		CRegistryHash regHash(wsRegKey);
		regHash.set(wsRegAttr, wcsLicenseKey);

		// If there is already a previous license state (for cases when activation is done more than once for one component)
		// In these cases, there is no need to invoke the callback if the license state is still the same.
		if( lecode!=m_mapComponentInfo[componentType].LicenseState() )
		{			
			// Store the license key in memory
			m_mapComponentInfo[componentType].wsLicense = wcsLicenseKey;

			// Get license state & info and store it to memory.
			m_mapComponentInfo[componentType].setLicenseState(lecode, getLicenseInfo(wcsLicenseKey));

			// Invoke callback to notify of new license state.
			if(onLicenseStatusChange!=NULL)
				onLicenseStatusChange(lecode);
		}

		LeaveCriticalSection(&m_critsecLicense);

		rc = lecode;
	}
	else
	{
		if(iRes==THORWA_RC_UNKNOWNACCOUNT)
			rc = LE_ERR_LICENSEINVALID;
		else if(iRes==THORWA_RC_UNKNOWNCOMPONENT)
			rc = LE_ERR_BADCOMPONENT;
		else if(iRes==THORWA_RC_SERVERNOTREACHABLE)
			rc = LE_ERR_NETWORK;
	}

	return rc;
}
CLicenseEnforce::LECode CLicenseEnforce::GetLicenseState(ComponentType componentType)
{
	LECode rc = LE_ERR_GENERIC;

	switch(componentType)
	{
	case Ese:
	case Svd:
		// Get the license state for the component
		rc = m_mapComponentInfo[componentType].LicenseState();
		break;
	default:
		rc = LE_ERR_BADCOMPONENT;
		break;
	}

	return rc;
}
CLicenseEnforce::LECode CLicenseEnforce::GetLicenseInfo(ComponentType componentType, tstring & wsLicenseInfo)
{
	LECode rc = LE_ERR_GENERIC;

	switch(componentType)
	{
	case Ese:
	case Svd:
		if(isLicenseInfoAvailable(componentType))
		{
			wsLicenseInfo = m_mapComponentInfo[componentType].LicenseInfo();

			rc = LE_OK;
		}

		break;
	default:
		rc = LE_ERR_BADCOMPONENT;
		break;
	}

	return rc;
}
void CLicenseEnforce::Start()
{			
	EseLeParams elp;
	EseLeParams arrayEseLeParams[] = {
		{ CLicenseEnforce::Ese, THORWA_COMP_ESE },
	};

	/* Notes on _beginthreadex():
	- _endthread or _endthreadex is called automatically when the thread returns from the routine passed as a parameter.
	- _endthread automatically closes the thread handle (whereas _endthreadex does not).
	- _beginthreadex returns 0 on error
	*/
	EnterCriticalSection(&m_critsecLicense);
	if(m_bMonitoring)
		goto end; // Already running

	if(m_hThread!=NULL)
	{
		// Close thread handle if it was running before
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// Check licenses that have already been activated and make sure they haven't expired.
	loadActivatedLicensesOnSystem();

	// Do a license check for all components before we enter the thread, for the sake of a synchronous license check and ensuring
	// that the license states are set when 'Start()' returns.
	for(int i=0; i<_countof(arrayEseLeParams); i++)
	{
		elp = arrayEseLeParams[i];

		doLicenseCheck(elp.componentType, elp.wsComponentName);		
	}

	if( (m_hThread=(HANDLE)_beginthreadex(NULL /* security */, 0 /* stack size */, (unsigned int(__stdcall*)(void*))thread_MonitorLicense, 
		(void*)this, 0 /* initial state: running */, NULL /* pointer to thread identifier */)) != 0 )
	{
		// Success
		m_bMonitoring = TRUE;
	}
end:
	LeaveCriticalSection(&m_critsecLicense);
}
void CLicenseEnforce::Stop()
{
	// Signal to stop the monitoring thread
	EnterCriticalSection(&m_critsecLicense);
	m_bMonitoring = FALSE;
	if(m_hThread!=NULL)
	{
		SetEvent(m_hMonitorEvent);
		WaitForSingleObject(m_hThread, 10000);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	// Reset signaled state.
	ResetEvent(m_hMonitorEvent);
	LeaveCriticalSection(&m_critsecLicense);
}

#define LE_HOUR_CHECK_INTERVAL				1
#define LE_HOUR_TO_MS_CONVERTER				60 * 60 * 1000
unsigned int CLicenseEnforce::thread_MonitorLicense(void* vpThis)
{
	CLicenseEnforce *pThis = (CLicenseEnforce*)vpThis;
	int rc = 0;
	DWORD dwResult;

	EseLeParams arrayEseLeParams[] = {
		{ CLicenseEnforce::Ese, THORWA_COMP_ESE },
	};

	// Check license every * hour(s)
	do
	{
		// Licenses are checked during Start() so we can sleep here.
		dwResult = WaitForSingleObject(pThis->m_hMonitorEvent, LE_HOUR_CHECK_INTERVAL * LE_HOUR_TO_MS_CONVERTER);		
		if( (dwResult == WAIT_TIMEOUT || dwResult == WAIT_OBJECT_0) && pThis->m_bMonitoring )
		{
			// Reset signaled state
			if(dwResult == WAIT_OBJECT_0)
				ResetEvent(pThis->m_hMonitorEvent);
			for(int i=0; i<_countof(arrayEseLeParams); i++)
			{
				EseLeParams elp = arrayEseLeParams[i];

				pThis->doLicenseCheck(elp.componentType, elp.wsComponentName);		
			}
		}
		else 
			break;
	} while(TRUE);

	pThis->m_bMonitoring = FALSE;
	// Not required to explicitly call _endthreadex() but helps ensure allocated resources are recovered.
	_endthreadex(rc);
	return rc;
}
void CLicenseEnforce::doLicenseCheck(ComponentType ct, tstring wsComponentName)
{
	OnLicenseStatusChange onLicenseStatusChange = NULL;
	LECode lecode;
	int iRes, iTokenState;
	ComponentInfo *pCi = &m_mapComponentInfo[ct];
	tstring wsLicenseInfo(TEXT(""));

	// If caller has set a notification callback with SetEventHandler(), set the callback function
	if(isCallbackAvailable(ct))
		onLicenseStatusChange = pCi->onLicenseStatusChange;

	lecode = pCi->LicenseState();

	// If user has entered a license for this component
	if(isLicenseAvailable(ct))
	{
		// Check license for any changes in state
		iRes = thorwac_il(util_strsafe_convert_tcs(wsComponentName.c_str()).c_str(), util_strsafe_convert_tcs(pCi->wsLicense.c_str()).c_str(), &iTokenState, NULL);
		wsLicenseInfo = getLicenseInfo(pCi->wsLicense.c_str());

		// If we got an error (network related, generic, etc.), default to unknown token status.
		if(iRes==THORWA_RC_SUCCESS)
		{
			// Note: If we got THORWA_RC_SUCCESS, it's assumed 'iTokenState' contains a valid token status!
			_ASSERTE(
				iTokenState==THORWA_TK_AVAILABLE ||
				iTokenState==THORWA_TK_OUTOFSTOCK);

			// Convert token to LECode
			lecode = m_mapTokenToLECode[iTokenState];
		}
		else if(iRes==THORWA_RC_UNKNOWNACCOUNT)
		{
			// If we got here, it means the license used to be valid (since the only reason we can only have a license to
			// check with if it has successfully been activated once), so it has either expired or was revoked.
			lecode = LE_ERR_LICENSEEXPIRED;

			// License is no longer valid, erase it from the system.
			CRegistryHash regHash(pCi->wsRegKey);
			regHash.erase(pCi->wsRegAttr);
		}
		// Else license validity could not be checked due to some error; License state doesn't change
	}
	else
	{
		// No license to check; Check if trial period is enforced
		if(pCi->uiTrialPeriod > 0)
		{
			CRegistryHash regHash(pCi->wsRegKey);
			tstring wsTrialStart = regHash.get(REGKEY_PARAM_TRIALSTART);
			TCHAR wcsTrialStart[32] = {0};

			// Get today's date
			time_t ttTrialStart, ttToday = time(NULL);
			unsigned int uiDaysRemaining;

			// If trial start date doesn't exist yet, add it to the registry.
			if(wsTrialStart.empty())
			{
				ttTrialStart = ttToday;
				_itow_s((int)ttTrialStart, wcsTrialStart, _countof(wcsTrialStart), 10);
				wsTrialStart = wcsTrialStart;
				regHash.set(REGKEY_PARAM_TRIALSTART, wsTrialStart.c_str());
			}
			else
				ttTrialStart = StringUtil::StrToInt(wsTrialStart);

			// Calculate the trial expiration date based on the trial start.
			if(isTrialExpired(ttTrialStart, ttToday, pCi->uiTrialPeriod, uiDaysRemaining))
				lecode = LE_ERR_LICENSETRIALEXPIRED;
			else 
			{
				// Trial license
				lecode = LE_ERR_LICENSETRIALMODE;

				wsLicenseInfo = StringUtil::IntToStr((int)uiDaysRemaining);
			}
		}
	}

	// If the new license state is different from the previous state
	if(	lecode!=pCi->LicenseState() )
	{
		// Track new license state
		pCi->setLicenseState(lecode, wsLicenseInfo);

		// Invoke callback to notify that there is a (new) license state (change)
		if(onLicenseStatusChange!=NULL)
			onLicenseStatusChange(lecode);
	}
	else if(!wsLicenseInfo.empty())
	{
		// If the license state hasn't changed, but license info was set, store it just in case it's different.
		pCi->setLicenseState(lecode, wsLicenseInfo);
	}
}
CLicenseEnforce::tstring CLicenseEnforce::getLicenseInfo(const TCHAR *wcsLicenseKey)
{
	TCHAR wcsBuffer[256] = { 0 };
	size_t uiBuffSize = _countof(wcsBuffer);

	// Get license info & store it.
	if(thorwac_gli(util_strsafe_convert_tcs(wcsLicenseKey).c_str(), (wchar_t*)util_strsafe_convert_tcs(wcsBuffer).c_str(), &uiBuffSize, NULL)==THORWA_RC_SUCCESS)
	{
		if(!tstring(wcsBuffer).empty())
			return wcsBuffer;
	}

	return TEXT("");
}
#define LE_DAYS_TO_SEC_CONVERTER				(60 * 60 * 24)
bool CLicenseEnforce::isTrialExpired(time_t ttTrialStart, time_t ttToday, unsigned int uiTrialPeriodInDays, unsigned int& uiDaysRemaining)
{
	if(uiTrialPeriodInDays==0)
		return false;

	unsigned int uiTrialPeriodInSeconds = uiTrialPeriodInDays * LE_DAYS_TO_SEC_CONVERTER;
	time_t ttExpirationDate = ttTrialStart + uiTrialPeriodInSeconds;

	// ttToday should before the start date of the trial; if not, the user must have tweaked the time.
	if(!(ttToday > ttExpirationDate) && (ttToday > ttTrialStart))
	{
		// Get the number of remaining days
		time_t ttTmp;
		ttTmp = ttExpirationDate - ttTrialStart;
		ttTmp = ttTmp - (ttToday - ttTrialStart);

		uiDaysRemaining = (unsigned int)(ttTmp / LE_DAYS_TO_SEC_CONVERTER);

		uiDaysRemaining++; // Round up.

		return false;
	}

	return true;
}
/* Description: This is called to load any activated accounts to memory (license keys stored in the registry).
*/
void CLicenseEnforce::loadActivatedLicensesOnSystem()
{
	tstring wsLicenseKey;
	struct EseLicenseParams {
		ComponentType componentType;
		tstring wsComponentName;
		tstring wsRegKey;
		tstring wsRegAttr;
	};

	EseLicenseParams arrayLicenseParams[] = {
		{ CLicenseEnforce::Ese, THORWA_COMP_ESE, REGKEY_ESE_PATH, REGKEY_PARAM_ACCOUNTID }
	};

	// Go through each component and get their license states
	for(int i=0; i<_countof(arrayLicenseParams); i++)
	{
		EseLicenseParams elp = arrayLicenseParams[i];

		try
		{
			// Retrieve license key (if it exists) for component
			CRegistryHash regHash(elp.wsRegKey);
			wsLicenseKey = regHash.get(elp.wsRegAttr);
		}
		catch (exception ex)
		{
			// It could throw an exception if the registry key has been tampered with (actually should be handled inside CRegistryHash)
			wsLicenseKey = TEXT("");
		}

		// If license exists in registry, store it in memory
		if(!wsLicenseKey.empty())
		{
			m_mapComponentInfo[elp.componentType].wsLicense = wsLicenseKey;
			// Assume license is valid if there is a license stored on the system.
			m_mapComponentInfo[elp.componentType].setLicenseState(LE_OK, TEXT(""));

			// Invoke callback to notify about valid license.
			if(isCallbackAvailable(elp.componentType))
				m_mapComponentInfo[elp.componentType].onLicenseStatusChange(LE_OK);
		}
	}
}
bool CLicenseEnforce::isCallbackAvailable(ComponentType ct)
{
	return (m_mapComponentInfo.find(ct) != m_mapComponentInfo.end() &&
		m_mapComponentInfo[ct].onLicenseStatusChange!=NULL);
}
bool CLicenseEnforce::isLicenseAvailable(ComponentType ct)
{
	return (m_mapComponentInfo.find(ct) != m_mapComponentInfo.end() &&
		!m_mapComponentInfo[ct].wsLicense.empty());
}
bool CLicenseEnforce::isLicenseInfoAvailable(ComponentType ct)
{
	return (m_mapComponentInfo.find(ct) != m_mapComponentInfo.end() &&
		!m_mapComponentInfo[ct].LicenseInfo().empty());
}
/* Description: String-safe method for copying a TCHAR buffer to a wstring.
* */
wstring CLicenseEnforce::util_strsafe_convert_tcs(const TCHAR *tcsSrc)
{
	wchar_t *csDest = NULL;
	size_t uiDestLength;
	wstring wsRet(L"");

#ifdef UNICODE
	uiDestLength = wcslen(tcsSrc);
#else
	uiDestLength = strlen(tcsSrc);
#endif

	if(uiDestLength <= 0)
		return wsRet;

	// Allocate buffer
	csDest = new wchar_t[++uiDestLength]; // include null
	wmemset(csDest, 0, uiDestLength);

#ifdef UNICODE
	if(FAILED(StringCbCopy(csDest, uiDestLength * sizeof(wchar_t), (wchar_t*)tcsSrc)))
		goto end;
#else
	// The function returns 0 if it does not succeed.
	if(MultiByteToWideChar(CP_UTF8, 0, tcsSrc, 
		-1, // ...this parameter can be set to -1 if the string is null-terminated.
		csDest, (int)uiDestLength)==0)
		goto end;
#endif

	wsRet = csDest;
end:
	if(csDest!=NULL)
		delete [] csDest;

	return wsRet;
}
/* Description: String-safe method for copying a char buffer to a TCHAR buffer.
* */
CLicenseEnforce::tstring CLicenseEnforce::util_strsafe_convert_mbs(const char *csSrc)
{
	TCHAR *csDest = NULL;
	size_t uiDestLength = strlen(csSrc);
	tstring tsRet(TEXT(""));

	if(uiDestLength <= 0)
		return tsRet;

	csDest = new TCHAR[++uiDestLength]; // add null

#ifdef UNICODE
	size_t uiConverted;

	wmemset(csDest, 0, uiDestLength);

	// Zero if successful, an error code on failure.
	if(mbstowcs_s(&uiConverted, csDest, uiDestLength, csSrc, _TRUNCATE)!=0)
		goto end;
#else
	memset(csDest, 0, uiDestLength);

	if(FAILED(StringCbCopy(csDest, uiDestLength * sizeof(TCHAR), (char*)csSrc)))
		goto end;
#endif

	tsRet = csDest;
end:
	if(csDest!=NULL)
		delete [] csDest;

	return tsRet;
}