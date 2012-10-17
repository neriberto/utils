#pragma once

#include <string>
#include <Windows.h>

class RegUtil
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

	RegUtil(void);
	~RegUtil(void);

	static bool IsRegKeyPresent(HKEY hPreDefined, const TCHAR * keyName, BOOL bForceNative = FALSE);
	static tstring		ReadRegStrVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, BOOL bForceNative = FALSE);
	static void			SetRegStrVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, DWORD dwRegType, const TCHAR * wcsVal, DWORD dwValSize, BOOL bForceNative = FALSE);
	static bool			DeleteRegKey(HKEY hPreDefined, const TCHAR * keyName, BOOL bForceNative = FALSE);
	static void			DeleteRegVal(HKEY hPreDefined, const TCHAR * keyName, const TCHAR * szRegAttr, BOOL bForceNative = FALSE);
	template <class RegValType> static void ReadRegVal(HKEY hPreDefined, 
													const wchar_t * keyName, 
													const wchar_t * szRegAttr, 
													RegValType *pVal,
													BOOL bForceNative = FALSE);
	template <class RegValType> static void SetRegVal(HKEY hPreDefined, 
													const wchar_t * keyName, 
													const wchar_t * szRegAttr, 
													DWORD dwRegType, 
													RegValType *pVal, 
													DWORD dwValSize,
													BOOL bForceNative = FALSE);
	HKEY startMonitoring(tstring regkey_name, tstring dword_name, HANDLE hEvent, BOOL bWatchSubtree=FALSE, DWORD dwNotifyFilter=REG_NOTIFY_CHANGE_LAST_SET, BOOL fAsynchronous=TRUE);
	HKEY startMonitoring(tstring regkey_name, tstring dword_name, HKEY hBase, HANDLE hEvent, BOOL bWatchSubtree, DWORD dwNotifyFilter, BOOL fAsynchronous);
	bool stopMonitoring(HKEY hKey);

private:
	static BOOL is64BitOS();
};

// Template functions must be in header file
template <class RegValType> void RegUtil::ReadRegVal(HKEY hPreDefined, const wchar_t * keyName, const wchar_t * szRegAttr, RegValType *pVal, BOOL bForceNative /* = FALSE */)
{
	HKEY hKey = NULL;
	LPBYTE pbyteKeyEn;
	long result;
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
			return;

		result = RegQueryValueEx(hKey, szRegAttr, NULL, NULL, pbyteKeyEn, &dwLen);
		*pVal = *((RegValType*)pbyteKeyEn);

		free(pbyteKeyEn);

		RegCloseKey(hKey);
	}
}

template <class RegValType> void RegUtil::SetRegVal(HKEY hPreDefined, const wchar_t * keyName, const wchar_t * szRegAttr, DWORD dwRegType, RegValType *pVal, DWORD dwValSize, BOOL bForceNative /* = FALSE */)
{
	REGSAM samDesired = KEY_ALL_ACCESS;
	DWORD dwDisposition; 
	HKEY hKey = NULL;

	if(is64BitOS())
	{
		if(bForceNative)
			samDesired |= KEY_WOW64_64KEY;
	}

	long cError = RegOpenKeyEx(hPreDefined, keyName, 0, KEY_WRITE, &hKey);
	if (cError != ERROR_SUCCESS) // Create a key if it did not exist
		cError = RegCreateKeyEx(hPreDefined, keyName, 0, NULL, REG_OPTION_NON_VOLATILE, samDesired, NULL, &hKey, &dwDisposition);

	if(cError == ERROR_SUCCESS)
	{
		RegSetValueEx(hKey, szRegAttr, 0, dwRegType, reinterpret_cast<BYTE *>(pVal), dwValSize);	
		RegCloseKey(hKey);
	}
}