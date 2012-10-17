#pragma once

#include <Windows.h>
#include <string>
#include <map>

using namespace std;

/* Description: 
*/
class CNamedPipeClient
{
public:
	enum NPCode {
		NP_OK,
		NP_ERR_GENERIC,
		NP_ERR_NOCONNECTION
	};
	static CNamedPipeClient* Instance();
	static void DestroyInstance();

	/************************************************************************/
	// Public Member Functions
	/************************************************************************/
	/*	Description:  Connects to an existing Named Pipe server.
		Returns: NP_OK, NP_ERR_GENERIC */
	NPCode Connect(const wchar_t *wcsPipeName, const unsigned int &uiTimeoutSecs = 5);

	/* Description:  */
	void Disconnect();

	/*	Description: 
		Returns: NP_OK, NP_ERR_GENERIC, NP_ERR_NOCONNECTION */
	NPCode SendMessage(const wchar_t *wcsMessage);

private:
	CNamedPipeClient();
	~CNamedPipeClient();

	bool init();
	void deinit();

private:
	static CNamedPipeClient* m_Instance;

	HANDLE hPipe;

	CRITICAL_SECTION critsecPipe;
};
