#include "EseUtil.h"
#include "EseCodes.h"
#include "FileUtil.h"
#include "ProcUtil.h"
#include "StringUtil.h"
#include "RegUtil.h"

CEseUtil::CEseUtil(void)
{	
}

CEseUtil::~CEseUtil(void)
{
}
wstring CEseUtil::GetInstallDirWithPathEnding()
{
	wstring wsInstallDir;

	wsInstallDir = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_ESE_PATH, REGKEY_PARAM_ESEINSTALLDIR);
	if(!wsInstallDir.empty() && wsInstallDir[wsInstallDir.length()-1]!=TEXT('\\'))
		wsInstallDir += TEXT("\\");

	return wsInstallDir;
}

BOOL CEseUtil::Is64BitOS()
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