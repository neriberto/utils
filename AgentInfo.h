#pragma once

#include "WADefs.h"

#include <string>
#include <tchar.h>

class CAgentInfo
{
public:
	// System User User Key (provided by AppRiver)
	// Use this hard-defined UserKey when there are multiple users logged on at the same time or there are no users logged on.
	static const TCHAR SUUK[];

	static bool GetLicenseKey(WAUtils::tstring& tsLicenseKey);
	static bool GetUserKey(WAUtils::tstring& tsUserKey);
	static bool GetComputerKey(WAUtils::tstring& tsComputerKey);
	static bool StoreLicenseKey(WAUtils::tstring& tsLicenseKey);
	static bool StoreUserKey(const WAUtils::tstring &tsLoggedOnUser, WAUtils::tstring &tsUserKey);
	static bool EraseComputerKey();
	static bool EraseUserKey(const WAUtils::tstring &tsLoggedOnUser);
	static bool StoreComputerKey(WAUtils::tstring& tsComputerKey);

	static bool GetUsername(WAUtils::tstring& tsUsername);
	static bool StoreUsername(const WAUtils::tstring& tsLoggedOnUser, WAUtils::tstring& tsUsername);
	static bool EraseUsername(const WAUtils::tstring& tsLoggedOnUser);

	static bool IsFilterOn();
	static bool SetFilterState(bool bNewState);

	static bool GetAgentVersion(WAUtils::tstring& tsAgentVer);
	static bool GetGuiInstallDirWithPathEnding(WAUtils::tstring& tsInstallDir);

private:
	static WAUtils::tstring replaceBackslashes(WAUtils::tstring tsInput, WAUtils::tstring tsNew);

	static bool FilterEnabled;
};