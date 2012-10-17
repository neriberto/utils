#pragma once

#include "WADefs.h"

#include <string>
#include <Windows.h>
#include <vector>
#ifdef _USE_WAUF_PROXY_EVENT_LOG_MESSAGES
#include "WAUFProxyEventLogMessages.h"
#else
// these are copied from "WAUFProxyEventLogMessages.h and are here to allow people who do not want to include
// the full WAUFProxyEventLogMessages project to compile without errors
// the entries in the windows event log will still complain that there is no way to decode the messages
// in order to fully use the WAUFProxyEventLogMessages you will need to include the WAUFProxyEventLogMessages project
// and define _USE_WAUF_PROXY_EVENT_LOG_MESSAGES (best at project level c++ defines) in your code AND register the dll with windows 
// you can see an example of how to do this in the KomodiaInstaller\Common\KomodiaSDK\URLFilterKomodiaSetup project
// last seen at svn://172.16.0.4/OES_Engine/ESP/URLFilterAgent/branches/KomodiaInstaller
// the monitoring for debug value changes are also off unless this code is activated - we will not see ANY debugging level settings 
#define STATUS                           0x00000001L
#define GENERAL                          0x000003E8L
#endif

// Sample custom data structure for threads to use.
// This is passed by void pointer so it can be any data type
// that can be passed using a single void pointer (LPVOID).
typedef struct LOGGING_LEVEL_THREAD_DATA {
    DWORD   dwThreadId;
    HANDLE  hThread; 
	HANDLE  hThreadTerminateSignal;
	TCHAR * tsRegistryName;
	TCHAR * tsDWORDName;
	DWORD	dwLoggingLevel;
	bool    bThreadRunning;
	DWORD	dwLastError;
	bool	bThreadExited;
} sLLTData, * psLLTData;


typedef enum LOGGING_LEVEL_ENUM
{
	LLAlways=(DWORD) 0x0,
	LLFunctionTracing=(DWORD) 0x00000001,
	LLFunctionDebug=(DWORD) 0x00000002,
	LLFunctionDebugVerbose=(DWORD) 0x00000004,
	LLFunctionDumpSecureSurfDNSData=(DWORD) 0x00000008
} eLoggingLevelEnum;

// Linker Dependencies: version.lib

class FileUtil
{
public:
#ifdef UNICODE
	typedef std::wstringstream tstringstream;
#else
	typedef std::stringstream tstringstream;
#endif
	static FileUtil* Instance();
	static void DestroyInstance();

	// TODO: Eventually, ctor & dtor must be privatized to make class completely Singleton.
	FileUtil(void);
	~FileUtil(void);

	static bool DeletePathRecursively(const wchar_t * wcsPath);
	static std::string FileContentsAsString(const char *csFileName);
	static int CopyFile(const TCHAR *src, const TCHAR *dst, bool allowOverwrite);
	static int GetFilesInDirectory(const TCHAR *csDir, std::vector<WAUtils::tstring> &vecFiles, bool bAbsolutePaths = false, bool bRecursive = false);
	static int GetCurrentDir(WAUtils::tstring &tsCurrentDir);
	static int GetTempDir(WAUtils::tstring &tsTempPath);
	static int GetWindowsTempDir(WAUtils::tstring &tsTempPath);
	static bool MoveFilesToDir(const wchar_t * wcsSrcDir, const wchar_t * wcsDestDir);
	static bool GetVersionOfFile(const TCHAR *csFileName, TCHAR *csVerBuf, size_t uiVerBufLength,
		TCHAR *csLangBuf, size_t uiLangBufLength);
	static bool IsFolderPath(const WAUtils::tstring & objPath);
	static bool ListFilesInDirectory( WAUtils::tstring &tsDirectory, std::vector<WAUtils::tstring> & vecFiles , bool bEnforceDirectoryCheck = true );
	/*	Description: Logs to Windows Event Viewer.
		Params:
			wType [in] Specifies what type of log it is. Should be one of the following:
						EVENTLOG_SUCCESS                
						EVENTLOG_ERROR_TYPE             
						EVENTLOG_WARNING_TYPE           
						EVENTLOG_INFORMATION_TYPE       
						EVENTLOG_AUDIT_SUCCESS          
						EVENTLOG_AUDIT_FAILURE          
			wcsMsg [in] Wide-character string containing the message that will be logged. Supports format specifiers.
	*/
	static void LogToWindowsEvent(WORD wType, const wchar_t *wcsMsg, ... );
	void LogToWindowsEventUsingLoggingLevels(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, LPCTSTR *lpStrings, WORD wNumStrings=1, WORD wCategory=STATUS, DWORD dwEventID=GENERAL, PSID lpUserSid=NULL, LPVOID lpRawData=NULL, DWORD dwDataSize=0);
	void LogToWindowsEventUsingLoggingLevels(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, const char * lpStrings);
	void LogToWindowsEventUsingLoggingLevelsWithFormatting(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, TCHAR *csMsg, ... );
	void LogToWindowsEventUsingLoggingLevelsCategoryEventIDAndFormatting(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, WORD wCategory, DWORD dwEventID, TCHAR *csMsg, ... );

	static int RemoveFile( const TCHAR *tcsFileName );
	bool SetupLoggingLevelMonitoring(TCHAR * tsRegistryName, TCHAR * tsDWORDName);
	bool BreakdownLoggingLevelMonitoring();
	psLLTData pmd;
	PTOKEN_USER pTokenUser;

private:
	static FileUtil* m_Instance;
	bool m_wasInitialized;

	bool init();
	void deinit();
};