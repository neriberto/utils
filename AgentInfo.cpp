#include "AgentInfo.h"
#include "SecureSurfDefines.h"
#include "RegistryHash.h"
#include "StringUtil.h"
#include "RegUtil.h"
#include "FileUtil.h"
#include "WinUtils.h"

using namespace WAUtils;
using namespace std;

/// <summary>System User User Key (provided by AppRiver) DAD217F0-6903-48C2-922C-6DA02C8E0E5E</summary>
// Use this hard-defined UserKey when there are multiple users logged on at the same time or there are no users logged on.
const TCHAR CAgentInfo::SUUK[] = {	TEXT('D'), TEXT('A'), TEXT('D'), TEXT('2'), TEXT('1'), TEXT('7'), TEXT('F'), TEXT('0'), TEXT('-'), 
						TEXT('6'), TEXT('9'), TEXT('0'), TEXT('3'), TEXT('-'), 
						TEXT('4'), TEXT('8'), TEXT('C'), TEXT('2'), TEXT('-'), 
						TEXT('9'), TEXT('2'), TEXT('2'), TEXT('C'), TEXT('-'), 
						TEXT('6'), TEXT('D'), TEXT('A'), TEXT('0'), TEXT('2'), TEXT('C'), TEXT('8'), TEXT('E'), TEXT('0'), TEXT('E'), TEXT('5'), TEXT('E'), 0 };

bool CAgentInfo::FilterEnabled = false;

bool CAgentInfo::GetLicenseKey(tstring& tsLicenseKey)
{
	CRegistryHash regHash(REGKEY_UFA_PATH);
	tsLicenseKey = regHash.get(REGKEY_PARAM_LICENSEKEY);
	
	if(tsLicenseKey.empty())
		return false;

	return true;
}
bool CAgentInfo::StoreLicenseKey(tstring& tsLicenseKey)
{
	CRegistryHash regHash(REGKEY_UFA_PATH);
	regHash.set(REGKEY_PARAM_LICENSEKEY, tsLicenseKey.c_str());

	return true;
}

bool CAgentInfo::GetUserKey(tstring& tsUserKey)
{
	// Look up key (name of the currently logged on user)
	vector<tstring> vecLoggedOnUsers;
	bool rc = false;

	// Find logged on user(s)
	if(!CWinUtils::GetLoggedOnUsers(vecLoggedOnUsers))
	{
		// Should NEVER come here, but set to default System User Key as a last resort.
		tsUserKey = SUUK;
		return true;
	}

	// If either multiple or zero users are logged on, use the default system UserKey.
	if(vecLoggedOnUsers.size()!=1)
		tsUserKey = SUUK;
	else
	{
		// Got username, look up UserKey for user.
		vecLoggedOnUsers[0] = replaceBackslashes(vecLoggedOnUsers[0], TEXT("_"));
		tstring tsRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + vecLoggedOnUsers[0];
		CRegistryHash regHash(tsRegPath.c_str());
		tsUserKey = regHash.get(REGKEY_PARAM_USERKEY);
	}

	// Set System UserKey since user has not authenticated yet.
	if(tsUserKey.empty())
		tsUserKey = SUUK;

	rc = true;

	return rc;
}
bool CAgentInfo::StoreUserKey(const tstring &tsLoggedOnUser, tstring &tsUserKey)
{
	if(tsLoggedOnUser.empty())
		return false;

	tstring tsFormatted = replaceBackslashes(tsLoggedOnUser, TEXT("_"));
	tstring tsRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + tsFormatted;
	CRegistryHash regHash(tsRegPath.c_str());
	regHash.set(REGKEY_PARAM_USERKEY, tsUserKey.c_str());

	return true;
}
bool CAgentInfo::EraseUserKey(const tstring &tsLoggedOnUser)
{
	tstring tsFormatted = replaceBackslashes(tsLoggedOnUser, TEXT("_"));
	tstring tsUserRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + tsFormatted;

	// Remove user authentication information
	RegUtil::DeleteRegVal(HKEY_LOCAL_MACHINE, tsUserRegPath.c_str(), REGKEY_PARAM_USERKEY);

	return true;
}
bool CAgentInfo::GetComputerKey(tstring& tsComputerKey)
{
	CRegistryHash regHash(REGKEY_UFA_PATH);
	tsComputerKey = regHash.get(REGKEY_PARAM_COMPUTERKEY);

	if(tsComputerKey.empty())
		return false;

	return true;
}
bool CAgentInfo::StoreComputerKey(tstring& tsComputerKey)
{
	CRegistryHash regHash(REGKEY_UFA_PATH);
	regHash.set(REGKEY_PARAM_COMPUTERKEY, tsComputerKey.c_str());

	return true;
}

tstring CAgentInfo::replaceBackslashes(tstring tsInput, tstring tsNew)
{
	// Replace backslashes with underscores
	return StringUtil::Replace(tsInput, TEXT("\\"), tsNew);
}
bool CAgentInfo::GetUsername(tstring& tsUsername)
{
	vector<tstring> vecLoggedOnUsers;
	tstring tsRegPath;
	bool rc = false;

	// Find logged on user(s)
	if(!CWinUtils::GetLoggedOnUsers(vecLoggedOnUsers))
		return rc;

	// If either multiple or zero users are logged on, abort.
	if(vecLoggedOnUsers.size()!=1)
		return rc;

	vecLoggedOnUsers[0] = replaceBackslashes(vecLoggedOnUsers[0], TEXT("_"));
	tsRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + vecLoggedOnUsers[0];
	tsUsername = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, tsRegPath.c_str(), REGKEY_PARAM_USERNAME);

	if(tsUsername.empty())
		return rc;

	rc = true;

	return rc;
}
bool CAgentInfo::StoreUsername(const tstring& tsLoggedOnUser, tstring& tsUsername)
{
	if(tsLoggedOnUser.empty())
		return false;

	tstring tsFormatted = replaceBackslashes(tsLoggedOnUser, TEXT("_"));
	tstring tsRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + tsFormatted;
	RegUtil::SetRegStrVal(HKEY_LOCAL_MACHINE, tsRegPath.c_str(), REGKEY_PARAM_USERNAME, REG_SZ, tsUsername.c_str(), (DWORD)(tsUsername.length() * sizeof(TCHAR)));

	return true;
}
bool CAgentInfo::EraseComputerKey()
{
	// Remove computer key
	RegUtil::DeleteRegVal(HKEY_LOCAL_MACHINE, REGKEY_UFA_PATH, REGKEY_PARAM_COMPUTERKEY);

	return true;
}
bool CAgentInfo::EraseUsername(const tstring &tsLoggedOnUser)
{
	tstring tsFormatted = replaceBackslashes(tsLoggedOnUser, TEXT("_"));
	tstring tsUserRegPath = REGKEY_UFA_USERS_PATH TEXT("\\") + tsFormatted;

	// Remove user authentication information
	RegUtil::DeleteRegVal(HKEY_LOCAL_MACHINE, tsUserRegPath.c_str(), REGKEY_PARAM_USERNAME);

	return true;
}
bool CAgentInfo::IsFilterOn()
{
	return FilterEnabled;
}
bool CAgentInfo::SetFilterState(bool bNewState)
{
	FilterEnabled = bNewState;

	return true;
}
bool CAgentInfo::GetGuiInstallDirWithPathEnding(tstring& tsInstallDir)
{
	tsInstallDir = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_GUI_PATH, REGKEY_PARAM_INSTALLDIR);
	if(!tsInstallDir.empty() && tsInstallDir[tsInstallDir.length()-1]!=TEXT('\\'))
		tsInstallDir += TEXT("\\");

	if(tsInstallDir.empty())
		return false;

	return true;
}
bool CAgentInfo::GetAgentVersion(tstring& tsAgentVer)
{
	tstring ts;
	TCHAR tcsVersion[MAX_PATH + 1];

	ts = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_UFA_PATH, REGKEY_MSI_PACKAGE_VER);

	_tcscpy_s(tcsVersion, MAX_PATH + 1, ts.c_str());

	tsAgentVer = tcsVersion;

	return true;
}