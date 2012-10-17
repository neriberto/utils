#pragma once

#include <string>
#include <hash_map>

#define ESE_CMD_PARAMKEY_LATESTVERSION						1

class CEseDelegate
{
public:
	enum Command {
		CMD_CHECKFORUPDATES,
		CMD_INSTALLUPDATE,
		CMD_SIGNALGUIISAVAILABLE,
		CMD_ISWINDOWSSTARTUPENABLED,
		CMD_ENABLEWINDOWSSTARTUP,
		CMD_DISABLEWINDOWSSTARTUP,
		CMD_ISTRAYICONENABLED
	};

	typedef stdext::hash_map<int, std::string> CommandParams;

	static bool Decode(std::string sMsg, Command& command, CommandParams& params);
	static bool Encode(std::string & sMsg, const Command& command, const int& iReturnCode, CommandParams params);

	static std::string IsTrayIconEnabled(bool bEnableTrayIcon);
	static std::string IsWindowsStartupEnabled();
	
private:
	CEseDelegate(void);
	~CEseDelegate(void);

	static std::string getParam(CommandParams params, const int& key);
};
