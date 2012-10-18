#include "WinUtils.h"
#include "StringUtil.h"
#include "DebugUtils.h"

// standard includes
#include "Psapi.h"

using namespace std;

bool CWinUtils::GetLoggedOnUsers(vector<tstring> &vecUsers)
{
	bool rc = false;
	vector<DWORD> vecSessionIds;
	tstring tsLoggedOnUser;

	vecUsers.clear();

	// Get active session(s)
	if(!GetCurrentSessions(vecSessionIds))
		return rc;

	// Get the name for each user of each session id
	for(size_t ui=0; ui<vecSessionIds.size(); ui++)
	{
		if(CWinUtils::GetUserName(vecSessionIds[ui], tsLoggedOnUser))
			vecUsers.push_back(tsLoggedOnUser);
	}

	rc = true;

	return rc;
}

bool CWinUtils::GetCurrentSessions(vector<DWORD> &vecSessionIds, WTS_CONNECTSTATE_CLASS state // = WTSActive
						   )
{
	bool rc = false;

	PWTS_SESSION_INFO pSessionInfo = 0;
	DWORD dwCount = 0;

	vecSessionIds.clear();

	// Get the list of all terminal sessions (If the function fails, the return value is zero.)
	if(!WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &dwCount))
		return rc;

	// look over obtained list in search of the active session
	for (DWORD i = 0; i < dwCount; ++i)
	{
		WTS_SESSION_INFO si = pSessionInfo[i];
		if (state == si.State)
		{
			// If the current session is active – store its ID
			vecSessionIds.push_back(si.SessionId);
		}
	}

	// Free session info
	WTSFreeMemory((PVOID)pSessionInfo);

	rc = true;

	return rc;
}

HANDLE CWinUtils::GetUserToken(DWORD dwSessionId)
{
	HANDLE hCurrentToken = 0;
	HANDLE hPrimaryToken = 0;

	// Get token of the logged in user by the active session ID
	BOOL bRet = WTSQueryUserToken(dwSessionId, &hCurrentToken);
	if (bRet == 0)
		return 0;

	bRet = DuplicateTokenEx(hCurrentToken, TOKEN_ASSIGN_PRIMARY | TOKEN_ALL_ACCESS, 0, SecurityImpersonation, TokenPrimary, &hPrimaryToken);
	if (bRet == 0)
		hPrimaryToken = 0;

	// Release handles
	CloseHandle(hCurrentToken);

	// Make sure to call CloseHandle(hPrimaryToken);
	return hPrimaryToken;
}

bool CWinUtils::GetUserName(DWORD dwSessionId, tstring &tsLoggedOnUser)
{
	LPTSTR	pBuffer = NULL;
	DWORD	dwBufferLen;
	bool rc = false;

	tsLoggedOnUser = TEXT("");

	// Get domain name (if applicable)
	BOOL bRes = WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSDomainName, &pBuffer, &dwBufferLen);
	if (bRes)
	{
		tsLoggedOnUser = pBuffer;
		tsLoggedOnUser += TEXT("\\");
		WTSFreeMemory(pBuffer);
	}

	// Get user name
	bRes = WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, dwSessionId, WTSUserName, &pBuffer, &dwBufferLen);
	if (bRes == FALSE)
		return rc;
	tsLoggedOnUser += pBuffer;
	WTSFreeMemory(pBuffer);

	if(tsLoggedOnUser.empty())
		return rc;

	rc = true;

	return rc;
}
BOOL CWinUtils::Is64BitOS()
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
bool CWinUtils::RunProcessAsLoggedOnUser(const TCHAR* csProcess, const TCHAR* csParams)
{
	bool rc = false;

	HANDLE hToken = NULL, hProc = NULL;
	vector<DWORD> vecSessionIds;
	BOOL bRet = 0;
	CWinUtils::tstring sProcExe(TEXT(""));
	DWORD adwProcesses[4096];
	DWORD cbNeededForProcesses, cProcesses, dwPid, dwSid, dwActiveUserSessionId;
	CWinUtils::tstring sActiveUsername(TEXT(""));

	if(_tcscmp(csProcess, TEXT(""))==0)
		return false;

	sProcExe = csProcess;

	// Wrap the exe with double quotes
	// Ref: See 'Security Remarks' for CreateProcessAsUser in msdn
	if(sProcExe[0]!=TEXT('"') && sProcExe[sProcExe.length()-1]!=TEXT('"'))
	{
		sProcExe = TEXT("\"");
		sProcExe += csProcess;
		sProcExe += TEXT("\"");
	}

	if(csParams!=NULL && _tcslen(csParams)>0)
	{
		sProcExe += TEXT(" ");
		sProcExe += csParams;
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
		if(!CWinUtils::GetUserName(dwActiveUserSessionId, sActiveUsername))
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
				tstring sUsernameOfProcess(TEXT(""));

				hToken = NULL;

				// Obtain process token
				if(OpenProcessToken(hProc, TOKEN_ALL_ACCESS, &hToken)==0)
				{
					DEBUG_PRINT(TEXT("Failed to get process token for process id %d (%u)"), dwPid, GetLastError()); 
					goto nextprocess;
				}

				// Obtain the size of the user information in the token.
				if(GetTokenInformation(hToken, TokenUser, NULL, 0, &cbti)==0)
				{
					// Call should have failed due to zero-length buffer.
					if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
					{
						DEBUG_PRINT(TEXT("A. Failed to look up token info for process id %d (%u)"), dwPid, GetLastError()); 
						goto nextprocess;
					}
				}

				// Allocate buffer for user information in the token.
				ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);

				// Retrieve the user information from the token.
				if(GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti)==0)
				{
					DEBUG_PRINT(TEXT("B. Failed to look up token info for process id %d (%u)"), dwPid, GetLastError()); 
					goto nextprocess;
				}

				// Retrieve user name and domain name based on user's SID.
				if(LookupAccountSid(NULL, ptiUser->User.Sid, szUsername, &dwUsernameSize, szDomain, &dwDomainSize, &snu)==0)
				{
					DEBUG_PRINT(TEXT("Failed to look up user for sid (%u)"), GetLastError()); 
					goto nextprocess;
				}

				// Construct full name
				sUsernameOfProcess = szDomain;
				if(!sUsernameOfProcess.empty())
					sUsernameOfProcess += TEXT("\\");
				sUsernameOfProcess += szUsername;

				// If the user associated with the process matches the logged-on user
				if (sUsernameOfProcess == sActiveUsername) 
				{
					// Impersonate the user using the user token
					if(ImpersonateLoggedOnUser(hToken))
					{
						// Launch GUI
						STARTUPINFO         sui;
						PROCESS_INFORMATION pi;

						ZeroMemory(&sui, sizeof(sui));
						sui.cb = sizeof (STARTUPINFO);
						ZeroMemory(&pi, sizeof(pi));
						bRet = CreateProcessAsUser(hToken,
							NULL,
							(LPTSTR)sProcExe.c_str(),
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
							DEBUG_PRINT(TEXT("Failed to launch application {%ls} with process id %d, user context {%ls}, and session id %d (%u)\r\n"), 
								sProcExe.c_str(), dwPid, sActiveUsername.c_str(), dwActiveUserSessionId, GetLastError());
						}
						else
						{
							// Log success
							DEBUG_PRINT(TEXT("Started application %d using process id %d and session id %d.\r\n"), pi.dwProcessId, dwPid, dwActiveUserSessionId);
						}

						// Revert to SYSTEM context (assuming this function was called when the installer was run from the SYSTEM's context).
						RevertToSelf();	
					}
					else DEBUG_PRINT(TEXT("Failed to impersonate user."));
				}
				//else WcaLog(LOGMSG_STANDARD, "User of process id %d does not match the logged-on user", dwPid);

nextprocess:
				// Clean up
				if (ptiUser)
					HeapFree(GetProcessHeap(), 0, ptiUser);

				if(hToken)
					CloseHandle(hToken);

				CloseHandle(hProc);
			}
			//else WcaLog(LOGMSG_STANDARD, "Failed to get handle to process %u.", dwPid);

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