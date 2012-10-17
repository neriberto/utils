/** \file CWinUtils.h
\brief Defines utilities for handling storage. */
#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <tchar.h>
#include <Wtsapi32.h>

// Dependent Libraries: Wtsapi32.lib; Psapi.lib
class CWinUtils
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

	static bool GetLoggedOnUsers(std::vector<tstring> & vecUsers);
	static bool GetCurrentSessions(std::vector<DWORD> &vecSessionIds, WTS_CONNECTSTATE_CLASS state = WTSActive);
	static HANDLE GetUserToken(DWORD dwSessionId);
	static bool GetUserName(DWORD dwSessionId, tstring &tsLoggedOnUser);
	static BOOL Is64BitOS();
	static bool RunProcessAsLoggedOnUser(const TCHAR* csProcess, const TCHAR* csParams);
};

