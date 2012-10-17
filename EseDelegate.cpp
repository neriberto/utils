#include "EseDelegate.h"
#include "ClientCodes.h"
#include "EseCodes.h"
#include "GearsCodes.h"
#include "StringUtil.h"
#include "RegUtil.h"

// Standard includes
#include <strsafe.h>
#include <vector>

using namespace std;
using namespace stdext;

#define CMD_PARAMKEY_USERNAME					1
#define CMD_PARAMKEY_PASSWORD					2
#define CMD_PARAMKEY_LICENSEKEY					3
#define CMD_PARAMKEY_USERKEY					4
#define CMD_PARAMKEY_COMPUTERKEY				5
#define CMD_PARAMKEY_LOGGEDONUSER				6
#define CMD_PARAMKEY_HTTPCODE					7
#define CMD_PARAMKEY_FILTERVERIFIED				8

#define CMD_DELIMITER						"{|}"

// Incoming:
//	CheckForUpdates
// Outgoing:
//	Success: <return_code>
#define CMD_STR_CHECKFORUPDATES				"CheckForUpdates"

// Incoming:
//  InstallUpdate
// Outgoing:
//  <return_code>
#define CMD_STR_INSTALLUPDATE				"InstallUpdate"

// Incoming:
//  SignalGuiIsAvailable
// Outgoing:
//  <return_code>
#define CMD_STR_SIGNALGUIISAVAILABLE		"SignalGuiIsAvailable"

// Incoming:
//  EnableWindowsStartup
// Outgoing:
//  <return_code>
#define CMD_STR_ENABLEWINDOWSSTARTUP		"EnableWindowsStartup"

// Incoming:
//  DisableWindowsStartup
// Outgoing:
//  <return_code>
#define CMD_STR_DISABLEWINDOWSSTARTUP		"DisableWindowsStartup"

// Incoming:
//  IsWindowsStartupEnabled
// Outgoing:
//  <return_code>
#define CMD_STR_ISWINDOWSSTARTUPENABLED		"IsWindowsStartupEnabled"

// Incoming:
//  IsTrayIconEnabled
// Outgoing:
//  <return_code>
#define CMD_STR_ISTRAYICONENABLED			"IsTrayIconEnabled"
bool CEseDelegate::Decode(string sMsg, Command& command, CommandParams& params)
{
	bool rc = false;
	size_t uiCmdLength = sMsg.find(CMD_DELIMITER);
	string sTmp, sParameters("");
	const size_t DELIMITER_LENGTH = strlen(CMD_DELIMITER);
	vector<string> vecParams;

	// Get the command
	if(uiCmdLength==sMsg.npos)
		uiCmdLength = sMsg.length();
	string wsCmd = sMsg.substr(0, uiCmdLength);

	// Store the rest of the string as parameters
	if (sMsg.length() > uiCmdLength + DELIMITER_LENGTH)
		sParameters = sMsg.substr(uiCmdLength + DELIMITER_LENGTH);

	// Just to make a "break-able" block of code.
	do
	{
		// Store parameters (if any) to a vector
		vecParams = StringUtil::StringToVector(sParameters, CMD_DELIMITER);

		// Get the parameters passed in for the command
		if(wsCmd.compare(CMD_STR_CHECKFORUPDATES)==0)
		{
			command = CMD_CHECKFORUPDATES;
		}
		else if(wsCmd.compare(CMD_STR_CHECKFORUPDATES)==0)
		{
			command = CMD_CHECKFORUPDATES;
		}
		else if(wsCmd.compare(CMD_STR_INSTALLUPDATE)==0)
		{
			command = CMD_INSTALLUPDATE;
		}
		else if(wsCmd.compare(CMD_STR_SIGNALGUIISAVAILABLE)==0)
		{
			command = CMD_SIGNALGUIISAVAILABLE;
		}
		else if(wsCmd.compare(CMD_STR_ENABLEWINDOWSSTARTUP)==0)
		{
			command = CMD_ENABLEWINDOWSSTARTUP;
		}
		else if(wsCmd.compare(CMD_STR_DISABLEWINDOWSSTARTUP)==0)
		{
			command = CMD_DISABLEWINDOWSSTARTUP;
		}
		else if(wsCmd.compare(CMD_STR_ISTRAYICONENABLED)==0)
		{
			command = CMD_ISTRAYICONENABLED;
		}
		else if(wsCmd.compare(CMD_STR_ISWINDOWSSTARTUPENABLED)==0)
		{
			command = CMD_ISWINDOWSSTARTUPENABLED;
		}
		else // Error; unrecognized command. 
			break;

		// Assume everything succeeded if we've gotten here.
		rc = true;

	} while(false);

	return rc;
}

bool CEseDelegate::Encode(string & sMsg, const Command& command, const int& iReturnCode, CommandParams params)
{
	bool rc = false;
	string sFormat, sParams("");
	char csMsg[1024];

	do 
	{
		// The fact that the code compiled ensures that the command is valid.
		rc = true;

		// Add any code below to add arguments for any commands that requires passing parameters
		if(command==CMD_CHECKFORUPDATES)
		{
			if(iReturnCode==G_OK)
			{
				sParams = params[ESE_CMD_PARAMKEY_LATESTVERSION];
			}
		}
		
	} while (false);

	// Store return code and parameters (if any)
	sFormat = "%d";
	if(!sParams.empty())
		sFormat += CMD_DELIMITER "%s";

	StringCbPrintfA(csMsg, _countof(csMsg) * sizeof(char), sFormat.c_str(), iReturnCode, sParams.c_str());

	sMsg = csMsg;

	return rc;
}

string CEseDelegate::IsTrayIconEnabled(bool bEnableTrayIcon)
{
	string sResult;
	CommandParams params;
	int ret = (bEnableTrayIcon? G_TRUE : G_FALSE);

	if(!Encode(sResult, CMD_ISTRAYICONENABLED, ret, params))
		return "";

	return sResult;
}

/* Description: Checks whether the GUI is run on Windows login.
				The value is verified against our expectation to make sure its state.

For example:
SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run, Field: SecureSurf, Value: C:\Some\Invalid|Path\GUI.EXE

We make sure the path we get from the Value is the absolute GUI path we get from:

SOFTWARE\\OPSWAT\\GEARS, Field: InstallDir, Value: C:\Program Files (x86)\OPSWAT\GEARS 

That value, plus '\wagears.exe' will make the absolute path. If the first value doesn't match this, treat it
as the Windows startup as disabled.

 * */
string CEseDelegate::IsWindowsStartupEnabled()
{
	int ret = G_FALSE;
	string sTmp;
	CommandParams params;
	string sResult = "";

	// Get the absolute path to the GUI
	string sGuiPath = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_OSC, REGKEY_PARAM_INSTALLDIR);
	StringUtil::EnsurePathEnding(sGuiPath);
	if(!sGuiPath.empty())
	{
		sGuiPath.insert(0, "\"");
		sGuiPath += EXE_NAME_WAGEARS;
		sGuiPath += "\" /nogui";

		sTmp = RegUtil::ReadRegStrVal(HKEY_LOCAL_MACHINE, REGKEY_WINDOWSSTARTUP, REGKEY_PARAM_OSC);

		// If the absolute path matches the value in the registry, then startup on Windows logon is enabled
		ret = (sGuiPath==sTmp? G_TRUE : G_FALSE);
	}

	if(!Encode(sResult, CMD_ISWINDOWSSTARTUPENABLED, ret, params))
		return "";

	return sResult;
}

/************************************************************************/
// Helper functions
/************************************************************************/
string CEseDelegate::getParam(CommandParams params, const int& key)
{
	if(params.find(key)==params.end())
		return "";

	return params[key];
}