#include "Configuration.h"
#include "SecureSurfDefines.h"
#include "FileUtil.h"
#include "StringUtil.h"
#include "RegUtil.h"

Configuration::Configuration(void)
{	
}

Configuration::~Configuration(void)
{
}
wstring Configuration::GetInstallDirWithPathEnding()
{
	wstring wsInstallDir;

	wsInstallDir = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_UFA_PATH, REGKEY_PARAM_INSTALLDIR);
	if(!wsInstallDir.empty() && wsInstallDir[wsInstallDir.length()-1]!=TEXT('\\'))
		wsInstallDir += TEXT("\\");

	return wsInstallDir;
}
wstring Configuration::GetESEDirWithPathEnding()
{
	wstring wsInstallDir;

	wsInstallDir = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_ESE_PATH, REGKEY_PARAM_INSTALLDIR);
	if(!wsInstallDir.empty() && wsInstallDir[wsInstallDir.length()-1]!=TEXT('\\'))
		wsInstallDir += TEXT("\\");

	return wsInstallDir;
}

BOOL Configuration::Is64BitOS()
{
	SYSTEM_INFO sysInfo;
	GetNativeSystemInfo(&sysInfo);

	switch (sysInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		return true;

	case PROCESSOR_ARCHITECTURE_INTEL:
		return false;

	// Unknown, let's just say it's 32-bit.
	default:
		return false;
	}
}