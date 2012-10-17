#pragma once

#include "WADefs.h"

#include <Windows.h>
#include <string>

// Library Dependencies
// Kernel32.lib on Windows 7 and Windows Server 2008 R2;
// Psapi.lib if PSAPI_VERSION=1 on Windows 7 and Windows Server 2008 R2;
// Psapi.lib on Windows Server 2008, Windows Vista, Windows Server 2003, and Windows XP
class ProcUtil
{
public:
#ifdef UNICODE
	typedef std::wstringstream tstringstream;
#else
	typedef std::stringstream tstringstream;
#endif

	ProcUtil(void);
	~ProcUtil(void);

	static bool CloseApp(const wchar_t * wcsExeName, const DWORD dwPatienceInMilliseconds);
	static int CreateProcessWithInputAndOutputRedirection( const WAUtils::tstring & cmdLine, const WAUtils::tstring & input, WAUtils::tstring & output);
	static int CreateChildProcess(const WAUtils::tstring & cmdLine, bool waitForChildToFinish = true, HANDLE * pChildProcessHandle = NULL,
		HANDLE * pChildStdinRd = NULL, HANDLE * pChildStdoutWr = NULL);
	static bool GetProcessIdForExe(LPTSTR pstrProcessName, DWORD &dwProcessId);
	static int ReadFromPipe(HANDLE & input, WAUtils::tstring & output);
	static int RunProcessAsUser(const TCHAR *tcsExePath, const TCHAR *tcsParams);
	static bool Terminate(const wchar_t * wcsExeName);
	static bool TerminateA(const char * csExeName);
	static int WriteToPipe(const WAUtils::tstring & input, HANDLE & output);

private:
	static BOOL CALLBACK callback_CloseWindow(HWND hwnd, LPARAM lParam);
};