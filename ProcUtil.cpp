
#include "ProcUtil.h"
#include "WinUtils.h"
#include "ClientCodes.h"
#include "DebugUtils.h"

using namespace WAUtils;

#include <string>
#include <algorithm> // transform
#include <Psapi.h> // enumprocess

using namespace std;

ProcUtil::ProcUtil(void)
{
}

ProcUtil::~ProcUtil(void)
{
}
bool ProcUtil::CloseApp(const wchar_t * wcsExeName, const DWORD dwPatienceInMilliseconds)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	bool bJobDone = false;
	UINT uExitCode = 0;
	wchar_t szProcessName[MAX_PATH];
	HANDLE hProcess;
	HMODULE hMod;

	//Enumerate All Processes    
	if( EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);

		// Look for running instances of the process
		for( i = 0; i < cProcesses; i++ )
		{			
			memset( szProcessName, 0, sizeof(szProcessName) );

			// Get a handle to the process.
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, aProcesses[i]);

			// Get the process name.
			if(NULL != hProcess)
			{
				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded ) )
					GetModuleBaseNameW( hProcess, hMod, szProcessName, sizeof(szProcessName) );
			}
			else
				continue;

			// Is this the process?
			if( lstrcmpiW( wcsExeName, szProcessName ) != 0 )
			{
				CloseHandle(hProcess);
				continue;
			}

			// callback_CloseWindow() posts WM_CLOSE to all windows whose PID matches your process's.
			EnumWindows((WNDENUMPROC)callback_CloseWindow, (LPARAM)aProcesses[i]);

			// Wait on the handle. If it signals, great. If it times out, then you kill it.
			if(WaitForSingleObject(hProcess, dwPatienceInMilliseconds)!=WAIT_OBJECT_0)
				TerminateProcess(hProcess, uExitCode);

			// Job done, but keep enumerating and killing just in case there are multiple instances of the process running.
			bJobDone = true;

			CloseHandle(hProcess);
		}
	}

	return bJobDone;
}
BOOL CALLBACK ProcUtil::callback_CloseWindow(HWND hwnd, LPARAM lParam)
{
	DWORD dwID;

	GetWindowThreadProcessId(hwnd, &dwID);

	if(dwID == (DWORD)lParam)
		PostMessage(hwnd, WM_CLOSE, 0, 0);

	return TRUE;
}
//Part of GetProcessCommandLine
struct __PEB
{
	DWORD   dwFiller[4];
	//DWORD   dwInfoBlockAddress;
	LONG_PTR dwInfoBlockAddress;
};

struct __INFOBLOCK
{
	DWORD   dwFiller[16];
	WORD    wLength;
	WORD    wMaxLength;
	//DWORD   dwCmdLineAddress;
	LONG_PTR dwCmdLineAddress;
};


int ProcUtil::CreateProcessWithInputAndOutputRedirection( const tstring & cmdLine, const tstring & input, tstring & output )
{
	//based on ms-help://MS.VSCC.v80/MS.MSDN.v80/MS.WIN32COM.v10.en/dllproc/base/creating_a_child_process_with_redirected_input_and_output.htm
	//or if you don't have VC8 MSDN installed: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/creating_a_child_process_with_redirected_input_and_output.asp
	SECURITY_ATTRIBUTES saAttr;
	int rc;
	BOOL fSuccess;
	HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup,
		hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup, hChildProcessHandle;

	output = TEXT("");

	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT.
	if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
		//error creating pipe
		rc = G_FAIL;
		goto end;
	}

	// Create non inheritable read handle and close the inheritable read
	// handle.

	fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
		GetCurrentProcess(), &hChildStdoutRdDup , 0,
		FALSE,
		DUPLICATE_SAME_ACCESS);
	if( !fSuccess ) {
		//DuplicateHandle failed
		rc = G_FAIL;
		goto end;
	}

	// Create a pipe for the child process's STDIN.

	if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) {
		//Stdin pipe creation failed
		rc = G_FAIL;
		goto end;
	}
	// Duplicate the write handle to the pipe so it is not inherited.

	fSuccess = DuplicateHandle(GetCurrentProcess(), hChildStdinWr,
		GetCurrentProcess(), &hChildStdinWrDup, 0,
		FALSE,                  // not inherited
		DUPLICATE_SAME_ACCESS);
	if (! fSuccess) {
		//Duplication failed
		rc = G_FAIL;
		goto end;
	}

	CloseHandle(hChildStdinWr);
	hChildStdinWr = NULL;

	// Now create the child process.
	if (G_FAILURE(rc = CreateChildProcess(cmdLine, false, &hChildProcessHandle, &hChildStdinRd, &hChildStdoutWr))) {
		//Create process failed
		rc = G_FAIL;
		goto end;
	}

	// Write input to pipe that is the standard input for a child process.
	if (G_FAILURE(rc = WriteToPipe(input, hChildStdinWrDup))) {
		goto end;
	}

	if( WaitForSingleObject(hChildProcessHandle, INFINITE)  !=  WAIT_OBJECT_0 )
		goto end;

	// Close the pipe handle.
	if (!CloseHandle(hChildStdinWrDup))  {
		hChildStdinWrDup = NULL;
		//Close pipe failed
		rc = G_FAIL;
		goto end;
	}
	hChildStdinWrDup = NULL;

	// Close the write end of the pipe before reading from the
	// read end of the pipe.
	if (!CloseHandle(hChildStdoutWr)) {
		hChildStdoutWr = NULL;
		//Close pipe failed
		rc = G_FAIL;
		goto end;
	}
	hChildStdoutWr = NULL;


	// Read from pipe that is the standard output for child process.

	if (G_FAILURE(rc = ReadFromPipe(hChildStdoutRdDup, output))) {
		goto end;
	}

end:
	//close all relevant handles
	if(hChildProcessHandle!=NULL) CloseHandle(hChildProcessHandle);
	if(hChildStdinRd!=NULL) CloseHandle(hChildStdinRd);
	if(hChildStdinWr!=NULL) CloseHandle(hChildStdinWr);
	if(hChildStdinWrDup!=NULL) CloseHandle(hChildStdinWrDup);
	if(hChildStdoutRd!=NULL) CloseHandle(hChildStdoutRd);
	if(hChildStdoutWr!=NULL) CloseHandle(hChildStdoutWr);
	if(hChildStdoutRdDup!=NULL) CloseHandle(hChildStdoutRdDup);

	return rc;
}

int ProcUtil::CreateChildProcess(const tstring & cmdLine,
	bool waitForChildToFinish /* = true */,
	HANDLE * pChildProcessHandle /* = NULL*/,
	HANDLE * pChildStdinRd /* = NULL*/, HANDLE * pChildStdoutWr /* = NULL*/)
{
	PROCESS_INFORMATION piProcInfo;
	STARTUPINFO siStartInfo;
	int rc;

	// Set up members of the PROCESS_INFORMATION structure.

	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure.

	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = (pChildStdoutWr != NULL)?(*pChildStdoutWr):0;
	siStartInfo.hStdOutput = (pChildStdoutWr != NULL)?(*pChildStdoutWr):0;
	siStartInfo.hStdInput = (pChildStdinRd != NULL)?(*pChildStdinRd):0;
	if (pChildStdinRd!=NULL || pChildStdoutWr!=NULL) {
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
	}

	// Create the child process.

	if (!CreateProcess(NULL,
		(LPTSTR)cmdLine.c_str(),	// command line
		NULL,          // process security attributes
		NULL,          // primary thread security attributes
		TRUE,          // handles are inherited
		CREATE_NO_WINDOW,             // creation flags
		NULL,          // use parent's environment
		NULL,          // use parent's current directory
		&siStartInfo,  // STARTUPINFO pointer
		&piProcInfo))  // receives PROCESS_INFORMATION
	{
		//createProcess failed
		rc = G_FAIL;
	}
	else
	{
		if (waitForChildToFinish) {
			WaitForSingleObject(piProcInfo.hProcess, INFINITE);
		}
		if (pChildProcessHandle != NULL) {
			*pChildProcessHandle = piProcInfo.hProcess;
		}
		else {
			//we don't need the child handle so close it here
			CloseHandle(piProcInfo.hProcess);
		}
		CloseHandle(piProcInfo.hThread);
		rc = G_OK;
	}
	return rc;
}

// Read from a string and write its contents to a pipe.
int ProcUtil::WriteToPipe(const tstring & input, HANDLE & output)
{
	DWORD dwToWrite, dwWritten;
	INT8 * buff;
	unsigned int i, len;
	int ret = G_OK;

	len = (unsigned int)input.length();
	buff = new INT8[len];
	for (i = 0; i<input.length(); i++) {
		buff[i] = (INT8)input[i];
	}

	dwToWrite = len;

	if (!WriteFile(output, buff, dwToWrite, &dwWritten, NULL)) {
		//failed to write
		ret = G_FAIL;
	}
	else if (dwWritten != dwToWrite) {
		//byte count mismatch
		ret = G_FAIL;
	}

	return ret;
}

#define BUFSIZE	1024
int ProcUtil::ReadFromPipe(HANDLE & input, tstring & output)
{
	DWORD dwRead;
	INT8 chBuf[BUFSIZE];
	unsigned int i;

	// Read output from the child process, and write to string
	for (;;)
	{
		if( !ReadFile( input, chBuf, BUFSIZE * sizeof(INT8), &dwRead,
			NULL) || dwRead == 0) break;
		for (i = 0; i<dwRead; i++)
			output += (TCHAR)chBuf[i];
	}

	return G_OK;
}
/* Description: Retrieves the process ID for the executable specified. If only a process name is given, it will match
with the first running processes with the same name. If an absolute path is specified, it will only look for 
running processes that originated in the same location.
*/
bool ProcUtil::GetProcessIdForExe(LPTSTR pstrProcessName, DWORD &dwProcessId)
{
	DWORD adwProcesses[4096];
	DWORD cbNeededForProcesses, cProcesses;
	bool bFound = false;
	bool bVerifyAbsolutePath = false;

	// Append the process name to the current directory
	tstring wsProcessToFind = pstrProcessName;

	if(wsProcessToFind.find(TEXT("\\"), 0)!=wsProcessToFind.npos)
		bVerifyAbsolutePath = true;

	transform(wsProcessToFind.begin(), wsProcessToFind.end(), wsProcessToFind.begin(), toupper);

	// Enumerate through running processes and find the process we're looking for.
	if ( EnumProcesses( adwProcesses, sizeof(adwProcesses), &cbNeededForProcesses ) )
	{
		cProcesses = cbNeededForProcesses / sizeof( DWORD );
		for ( DWORD i = 0; i < cProcesses; i++ )
		{
			if( adwProcesses[i] != 0 )
			{
				HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE,
					adwProcesses[i] );
				if ( NULL != hProcess )
				{
					HMODULE hMods[4096];
					DWORD cbNeededForModules;

					if ( EnumProcessModules( hProcess, hMods, sizeof(hMods), &cbNeededForModules) )
					{
						TCHAR szProcessName[MAX_PATH];
						if( GetModuleFileNameEx( hProcess, hMods[0], szProcessName, sizeof(szProcessName) ) )
						{
							tstring wsProcessName = szProcessName;
							transform(wsProcessName.begin(), wsProcessName.end(), wsProcessName.begin(), toupper);

							if( (wsProcessToFind.compare(0, wsProcessToFind.length(), wsProcessName)==0) ||
								(!bVerifyAbsolutePath &&
								wsProcessToFind.compare(0, wsProcessToFind.length(), 
								wsProcessName.substr(wsProcessName.rfind(TEXT("\\"))+1))==0) 
								)
							{
								dwProcessId = adwProcesses[i];
								bFound = true;
							}
						}
					}
					CloseHandle(hProcess);
				}
			}
			if(bFound)
				break;
		}
	}

	return bFound;
}
int ProcUtil::RunProcessAsUser(const TCHAR *tcsExePath, const TCHAR *tcsParams)
{
	int rc = G_FAIL;
	HANDLE hToken = NULL, hProc = NULL;
	vector<DWORD> vecSessionIds;
	BOOL bRet = 0;
	tstring tsCmd(TEXT(""));
	DWORD adwProcesses[4096];
	DWORD cbNeededForProcesses, cProcesses, dwPid, dwSid, dwActiveUserSessionId;
	tstring tsActiveUsername(TEXT(""));

	// In case the executable path contains spaces, wrap it in quotes
	tsCmd = TEXT("\"");
	tsCmd += tcsExePath;
	tsCmd += TEXT("\"");

	// Arguments if provided.
	if(tcsParams!=NULL)
	{
		tsCmd += TEXT(" ");
		tsCmd += tcsParams;
	}

	// Get active session(s)
	if(!CWinUtils::GetCurrentSessions(vecSessionIds))
	{
		DEBUG_PRINT(TEXT("Failed to get current active sessions."));
		goto end;
	}

	// Function will not succeed if we cannot enumerate running processes
	// We want to find a process that was started by the same user of the current active session id
	if (EnumProcesses(adwProcesses, sizeof(adwProcesses), &cbNeededForProcesses) == 0)
	{
		DEBUG_PRINT(TEXT("Failed to get enumerate running processes."));
		goto end;
	}

	// Number of processes
	cProcesses = cbNeededForProcesses / sizeof( DWORD );

	// Attempt to impersonate a user and start the application until it is successful.
	for (size_t i = 0; i < vecSessionIds.size(); i++)
	{		
		dwActiveUserSessionId = vecSessionIds[i];

		// Get the logged-on username of the active session id
		if(!CWinUtils::GetUserName(dwActiveUserSessionId, tsActiveUsername))
			continue;

		// Iterate through processes
		for ( DWORD j = 0; j < cProcesses; j++ )
		{
			dwPid = adwProcesses[j];

			// System Idle Process Id == 0
			if( dwPid == 0 )
				continue;

			// Get session id of the process
			if(ProcessIdToSessionId(dwPid, &dwSid)==0)
				continue;

			// If the session id of this process matches the active session id, get the token from it.
			if(dwSid != dwActiveUserSessionId)
				continue;

			// Get a handle to this process
			hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
			if (hProc) 
			{
				PTOKEN_USER ptiUser = NULL;
				SID_NAME_USE snu;
				DWORD cbti, dwUsernameSize = 64, dwDomainSize = 64;
				TCHAR szUsername[64];
				TCHAR szDomain[64];
				tstring tsUsernameOfProcess(TEXT(""));

				hToken = NULL;

				// Obtain process token
				if(OpenProcessToken(hProc, TOKEN_ALL_ACCESS, &hToken)==0)
				{
					DEBUG_PRINT(TEXT("Failed to get process token for process id %d (%u)", dwPid, GetLastError())); 
					goto nextprocess;
				}

				// Obtain the size of the user information in the token.
				if(GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti)==0)
				{
					// Call should have failed due to zero-length buffer.
					if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					{
						DEBUG_PRINT(TEXT("A. Failed to look up token info for process id %d (%u)", dwPid, GetLastError())); 
						goto nextprocess;
					}
				}

				// Allocate buffer for user information in the token.
				ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);

				// Retrieve the user information from the token.
				if(GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)==0)
				{
					DEBUG_PRINT(TEXT("B. Failed to look up token info for process id %d (%u)", dwPid, GetLastError())); 
					goto nextprocess;
				}

				// Retrieve user name and domain name based on user's SID.
				if(LookupAccountSid(NULL, ptiUser->User.Sid, szUsername, &dwUsernameSize, szDomain, &dwDomainSize, &snu)==0)
				{
					DEBUG_PRINT(TEXT("Failed to look up user for sid (%u)", GetLastError())); 
					goto nextprocess;
				}

				// Construct full name
				tsUsernameOfProcess = szDomain;
				if(!tsUsernameOfProcess.empty())
					tsUsernameOfProcess += TEXT("\\");
				tsUsernameOfProcess += szUsername;

				// If the user associated with the process matches the logged-on user
				if (tsUsernameOfProcess == tsActiveUsername) 
				{
					// Impersonate the user using the user token
					if(ImpersonateLoggedOnUser(hToken))
					{
						// Launch GUI
						STARTUPINFO         sui;
						PROCESS_INFORMATION pi;

						ZeroMemory(&sui, sizeof(sui));
						sui.cb = sizeof (STARTUPINFO);
						bRet = CreateProcessAsUser(hToken,
							NULL,
							(LPTSTR)tsCmd.c_str(),
							NULL,
							NULL,
							FALSE,
							CREATE_NEW_PROCESS_GROUP,
							NULL,
							NULL,
							&sui,
							&pi );

						if(bRet==0)
						{
							DEBUG_PRINT(TEXT("Failed to launch application {%ls} with process id %d, user context {%ls}, and session id %d (%u)", 
								tsCmd.c_str(), dwPid, tsActiveUsername.c_str(), dwActiveUserSessionId, GetLastError()));
						}
						else
						{
							// Log success
							DEBUG_PRINT(TEXT("Started application %d using process id %d and session id %d.", pi.dwProcessId, dwPid, dwActiveUserSessionId));

							rc = G_OK;
						}

						// Revert to SYSTEM context (assuming this function was called when the installer was run from the SYSTEM's context).
						RevertToSelf();	
					}
					else DEBUG_PRINT(TEXT("Failed to impersonate user."));
				}
				else DEBUG_PRINT(TEXT("User of process id %d does not match the logged-on user", dwPid));

nextprocess:
				// Clean up
				if (ptiUser)
					HeapFree(GetProcessHeap(), 0, ptiUser);

				if(hToken)
					CloseHandle(hToken);

				CloseHandle(hProc);
			}
			else DEBUG_PRINT(TEXT("Failed to get handle to process %u.", dwPid));

			// Break out if application started successfully
			if(bRet)
			{
				break;
			}
		} // for loop for processes

		// Break out if application started successfully
		if(bRet)
			break;
	} // for loop for session id's

end:

	return rc;
}
bool ProcUtil::Terminate(const wchar_t * wcsExeName)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	bool bJobDone = false;

	//Enumerate All Processes    
	if( EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);

		// Look for running instances of the process
		for( i = 0; i < cProcesses; i++ )
		{
			wchar_t szProcessName[MAX_PATH];
			memset( szProcessName, 0, sizeof(szProcessName) );

			// Get a handle to the process.
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, aProcesses[i] );

			// Get the process name.
			if( NULL != hProcess )
			{
				HMODULE hMod;

				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded ) )
					GetModuleBaseNameW( hProcess, hMod, szProcessName, sizeof(szProcessName) );
			}

			// Is this the process?
			if( lstrcmpiW( wcsExeName, szProcessName ) == 0 )
			{
				UINT uExitCode = 0;

				// Kill It
				TerminateProcess( hProcess, uExitCode );

				// Job done, but keep enumerating and killing just in case there are multiple instances of the process running.
				bJobDone = true;
			}

			CloseHandle( hProcess );
		}
	}

	return bJobDone;
}
bool ProcUtil::TerminateA(const char * wcsExeName)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;
	bool bJobDone = false;

	//Enumerate All Processes    
	if( EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
	{
		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);

		// Look for running instances of the process
		for( i = 0; i < cProcesses; i++ )
		{
			char szProcessName[MAX_PATH];
			memset( szProcessName, 0, sizeof(szProcessName) );

			// Get a handle to the process.
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, false, aProcesses[i] );

			// Get the process name.
			if( NULL != hProcess )
			{
				HMODULE hMod;

				if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded ) )
					GetModuleBaseNameA( hProcess, hMod, szProcessName, sizeof(szProcessName) );
			}

			// Is this the process?
			if( lstrcmpiA( wcsExeName, szProcessName ) == 0 )
			{
				UINT uExitCode = 0;

				// Kill It
				TerminateProcess( hProcess, uExitCode );

				// Job done, but keep enumerating and killing just in case there are multiple instances of the process running.
				bJobDone = true;
			}

			CloseHandle( hProcess );
		}
	}

	return bJobDone;
}