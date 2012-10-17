#pragma once

/************************************************************************/
// Status Types
/************************************************************************/
#define ESE_COMAPI_STATUSTYPE_HEALTH				L"HEALTH"
#define ESE_COMAPI_STATUSTYPE_PRODUCTINFO			L"PRODUCTINFO"

/************************************************************************/
// Status Codes
/************************************************************************/
#define ESE_VERIFY_FAILED							0
#define ESE_VERIFY_DONE								1
#define ESE_VERIFY_INPROGRESS						2
#define ESE_VERIFY_DISABLED							3

#define ESE_OK_GENERIC								100
#define ESE_ERR_GENERIC								101
#define ESE_ERR_BADARGUMENT							102
#define ESE_ERR_NOTSUPPORTED						103
#define ESE_ERR_LICENSEINVALID						110
#define ESE_ERR_NETWORK								120

#define ESE_UPDATE_PROMPTCONFIG_NEVER				0
#define ESE_UPDATE_PROMPTCONFIG_BEFORECHECK			1
#define ESE_UPDATE_PROMPTCONFIG_BEFOREINSTALL		2
#define ESE_UPDATE_PROMPTCONFIG_ALWAYS				3

// License States
#define ESE_LICENSESTATE_VALID						0
#define ESE_LICENSESTATE_OUTOFTOKENS				1
#define ESE_LICENSESTATE_TRIALEXPIRED				2
#define ESE_LICENSESTATE_EXPIRED					3
#define ESE_LICENSESTATE_UNKNOWN					4

/************************************************************************/
// Check Names
/************************************************************************/
#define CHECKNAME_AV				"AV"
#define CHECKNAME_AS				"AS"
#define CHECKNAME_FW				"FW"
#define CHECKNAME_P2P				"P2P"
#define CHECKNAME_AV_W				L"AV"
#define CHECKNAME_AS_W				L"AS"
#define CHECKNAME_FW_W				L"FW"
#define CHECKNAME_P2P_W				L"P2P"

/************************************************************************/
// Global Names
/************************************************************************/
#define OSC_MSI_PRODUCTCODE			"{A65DF44A-F53B-44d4-A27B-FEA14AAAB5AC}"

#define SVCNAME_ESE_SHORT			TEXT("EseService")
#define SVC_NAME_SHORT				"EseService"
#define SVC_NAME_SHORT_W			L"EseService"

#define EXE_NAME_SVC				"EseService.exe"
#define EXE_NAME_SVC_CAPS			"ESESERVICE.EXE"
#define EXE_NAME_SVC_W				L"EseService.exe"

#define SVC_NAME_W					L"OPSWAT Endpoint Security Service"
#define SVC_NAME_SHORT_W			L"EseService"

#define LOG_ESE_FILENAME			"ese.log"

/************************************************************************/
// Registry Keys
/************************************************************************/
#define REGKEY_WINDOWSSTARTUP			"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REGKEY_UFA_PATH					TEXT("SOFTWARE\\OPSWAT\\URL Filtering Agent")
#define REGKEY_MSI_PACKAGE_VER			TEXT("MSIPackageVersion")

#define REGKEY_PARAM_ACCOUNTID			TEXT("ai")
#define REGKEY_PARAM_TRIALSTART			TEXT("ts")
#define REGKEY_PARAM_ESEINSTALLDIR		TEXT("installdir")
#define REGKEY_ESE_PATH					TEXT("SOFTWARE\\OPSWAT\\ESE")

// Custom Service Commands (128-256)
#define SVC_CMD_CHECKFORUPDATES			128
#define SVC_CMD_INSTALLUPDATE			129
#define SVC_CMD_SIGNALGUIISAVAILABLE	130
#define SVC_CMD_ENABLEWINDOWSSTARTUP	131
#define SVC_CMD_DISABLEWINDOWSSTARTUP	132
#define SVC_CMD_ISTRAYICONENABLED		133

/************************************************************************/
// Named Pipe Server
/************************************************************************/
#define NAMEDPIPESERVER_FULLNAME				"\\\\.\\pipe\\WA_ESE_NamedPipe"

#define NAMEDPIPESERVER_MAXBUFFERLENGTH				1024
