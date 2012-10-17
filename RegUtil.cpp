
#include "RegUtil.h"
#include "ClientCodes.h"
#include <tchar.h>
RegUtil::RegUtil(void)
{
}

RegUtil::~RegUtil(void)
{
}

BOOL RegUtil::is64BitOS()
{
	SYSTEM_INFO sysInfo;
	GetNativeSystemInfo(&sysInfo);

	switch (sysInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_IA64:
	case PROCESSOR_ARCHITECTURE_AMD64:
		return TRUE;

	case PROCESSOR_ARCHITECTURE_INTEL:
		return FALSE;

		// Unknown, let's just say it's 32-bit.
	default:
		return FALSE;
	}
}


RegUtil::tstring RegUtil::ReadRegStrVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, BOOL bForceNative /* = FALSE */)
{
	tstring sRet = TEXT("");
	HKEY hKey = NULL;
	LPBYTE pbyteKeyEn;
	long result, result2;
	DWORD dwLen = 1;
	BYTE byteKeyEnTemp;
	REGSAM samDesired = KEY_READ;

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, samDesired, &hKey);
	if(cError== ERROR_SUCCESS)
	{
		result = RegQueryValueEx(hKey, szRegAttr, NULL, NULL, &byteKeyEnTemp, &dwLen);
		if(result == ERROR_MORE_DATA)
			pbyteKeyEn = (BYTE*)malloc(dwLen);
		else
			return TEXT("");

		result2 = RegQueryValueEx(hKey, szRegAttr, NULL, NULL, pbyteKeyEn, &dwLen);
		sRet = (TCHAR*)pbyteKeyEn;

		free(pbyteKeyEn);

		RegCloseKey(hKey);
	}

	return sRet;
}
void RegUtil::SetRegStrVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, DWORD dwRegType, const TCHAR * wcsVal, DWORD dwValSize, BOOL bForceNative /* = FALSE */)
{
	REGSAM samDesired = KEY_ALL_ACCESS;
	DWORD dwDisposition; 
	HKEY hKey = NULL;
	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, KEY_WRITE, &hKey);

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	if (cError != ERROR_SUCCESS) // Create a key if it did not exist
		cError = RegCreateKeyEx(hPreDefined, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired, NULL, &hKey, &dwDisposition);

	if(cError == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, szRegAttr, 0, dwRegType, (LPBYTE)wcsVal, dwValSize);
		RegCloseKey(hKey);
	}
}
bool RegUtil::IsRegKeyPresent(HKEY hPreDefined, const TCHAR * keyName, BOOL bForceNative /* = FALSE */)
{
	HKEY hKey = NULL;
	REGSAM samDesired = KEY_READ;

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, samDesired, &hKey);
	if (cError != ERROR_SUCCESS) // Return if it doesn't exist
		return false;

	RegCloseKey(hKey);

	return true;
}
bool RegUtil::DeleteRegKey(HKEY hPreDefined, const TCHAR * keyName, BOOL bForceNative /* = FALSE */)
{
	HKEY hKey = NULL;
	REGSAM samDesired = KEY_WRITE;

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, samDesired, &hKey);
	if (cError != ERROR_SUCCESS) // Return if it doesn't exist
		return true;

	if(cError == ERROR_SUCCESS)
	{
		cError = RegDeleteKeyEx(hPreDefined, keyName, samDesired, 0);
		RegCloseKey(hKey);
		if(cError==ERROR_SUCCESS)
			return true;
	}

	return false;
}
void RegUtil::DeleteRegVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, BOOL bForceNative /* = FALSE */)
{
	HKEY hKey = NULL;
	REGSAM samDesired = KEY_WRITE;

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, samDesired, &hKey);
	if (cError != ERROR_SUCCESS) // Return if it doesn't exist
		return;

	if(cError == ERROR_SUCCESS)
	{
		RegDeleteValue(hKey, szRegAttr);
		RegCloseKey(hKey);
	}
}
HKEY RegUtil::startMonitoring(tstring regkey_name, tstring dword_name, HANDLE hEvent, BOOL bWatchSubtree, DWORD dwNotifyFilter, BOOL fAsynchronous)
{
	// the purpose of this function is to setup notifcation via a semaphore when a registry key has 
	// been modified 
	// its original purpose was to let us change the logging levels without having to poll the registry keys
	HKEY hKey=NULL;
	HKEY hKeyBASE=NULL;
	REGSAM samDesired;
	samDesired = KEY_NOTIFY;

	hKeyBASE=HKEY_LOCAL_MACHINE;
 
	hKey = startMonitoring(regkey_name, dword_name, hKeyBASE, hEvent, bWatchSubtree, dwNotifyFilter, fAsynchronous);

	return hKey;
}
HKEY RegUtil::startMonitoring(tstring regkey_name, tstring dword_name, HKEY hBase, HANDLE hEvent, BOOL bWatchSubtree, DWORD dwNotifyFilter, BOOL bAsynchronous)
{
	// the purpose of this function is to setup notifcation via a semaphore when a registry key has 
	// been modified 
	// its original purpose was to let us change the logging levels without having to poll the registry keys
	bool bResult=false;
	HKEY hKey=NULL;
	LONG  lResult=0L;

	REGSAM samDesired;
	samDesired = KEY_NOTIFY;

	// this is a bit odd but bear with me
	// we try to open the key normally - this normally will vary depending on if we are a win32 or win64 bit program
	// if we can open it we are happy - otherwise we try it explicitly as win64 and win32
	// this is the part of the key we can always count on - if we are installed

	lResult= RegOpenKeyEx(hBase,
							_T("SOFTWARE\\OPSWAT\\URL Filtering Agent"),
							0,
							samDesired,
							&hKey);

	if (lResult==ERROR_SUCCESS)
	{
		lResult = RegNotifyChangeKeyValue(
			hKey, // since this does not seem to work with keys from the 64bit hive I suspect we are seeing an issue caused by reflection issues
			TRUE,
			REG_NOTIFY_CHANGE_NAME|REG_NOTIFY_CHANGE_ATTRIBUTES|REG_NOTIFY_CHANGE_LAST_SET|REG_NOTIFY_CHANGE_SECURITY,
			hEvent,
			bAsynchronous
			);
		if (lResult==ERROR_SUCCESS)
		{
			bResult = true;
		}
	}
	return hKey;
}
bool RegUtil::stopMonitoring(HKEY hKey)
{
	bool bResult=false;

	RegCloseKey(hKey);
	
	return bResult;
}
