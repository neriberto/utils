// include me first
#include "Gui.h"
#include "NamedPipeClient.h"
#include "StringUtil.h"

// standard includes

/************************************************************************/
// Pipe Configuration
/************************************************************************/
#define PIPE_NAME_OSC						L"\\\\.\\pipe\\OscNamedPipe"

/************************************************************************/
// Pipe Command Format
// <name of command>|<parameter 1>|<parameters 2>|... <parameter n>
/************************************************************************/
#define PIPE_CMD_PARAM_DELIMITER			L"|"

/*	Description: Notify the GUI whether or not filtering is verified by the SecureSurf DNS server.
Parameters: 
state: 0 - filtering is not verified, 1 - filtering has been verified. */
#define PIPE_CMD_NOTIFY_FILTERVERIFIED		L"notify_filterverified";

/*	Description: Notify the GUI whether or not URL filtering is verified by the SecureSurf DNS server.
Parameters: 
state: 0 - URL filtering is not verified, 1 - URL filtering has been verified. */
#define PIPE_CMD_NOTIFY_HTTP_FILTERVERIFIED	L"notify_HTTP_filterverified";

Gui* Gui::m_Instance = 0;

Gui::Gui()
{
}

Gui::~Gui()
{
}

Gui* Gui::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new Gui();
		m_Instance->init();
	}

	return m_Instance;
}

void Gui::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;
		m_Instance = 0;
	}
}
void Gui::deinit()
{
}
bool Gui::init()
{
	m_wasInitialized = true;

	return true;
}
bool Gui::sendMessageToGui(const wchar_t *wcsMessage, unsigned int iTimeoutSecs /* = 5 */)
{
	bool bResult=false;
	if (CNamedPipeClient::Instance()->Connect(PIPE_NAME_OSC, iTimeoutSecs)==CNamedPipeClient::NP_OK)
	{
		if (CNamedPipeClient::Instance()->SendMessage(wcsMessage) == CNamedPipeClient::NP_OK)
		{
			bResult = true;
		}
		CNamedPipeClient::Instance()->Disconnect();
	}
	return bResult;
}
bool Gui::NotifyFilterVerified(bool bVerified, unsigned int iTimeoutSecs)
{
	wstring wsMsg;
	wsMsg = PIPE_CMD_NOTIFY_FILTERVERIFIED;
	wsMsg += PIPE_CMD_PARAM_DELIMITER;
	wsMsg += (bVerified? L"1" : L"0");
	bool bResult=sendMessageToGui(wsMsg.c_str(), iTimeoutSecs);
	return bResult;
}
bool Gui::NotifyURLFilterVerified(bool bVerified, unsigned int iTimeoutSecs)
{
	wstring wsMsg;
	wsMsg = PIPE_CMD_NOTIFY_HTTP_FILTERVERIFIED;
	wsMsg += PIPE_CMD_PARAM_DELIMITER;
	wsMsg += (bVerified? L"1" : L"0");
	bool bResult=sendMessageToGui(wsMsg.c_str(), iTimeoutSecs);
	return bResult;
}