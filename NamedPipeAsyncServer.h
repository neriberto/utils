#pragma once

#include <Windows.h>
#include <tchar.h>
#include <string>
#include <map>

using namespace std;

/* Description: 
		Thread-Safe? Yes
*/
class CNamedPipeServer
{
	struct NamedPipeParams
	{
		CNamedPipeServer *This;
		HANDLE PipeHandle;
	};

public:
	enum NPCode {
		NP_OK,
		NP_ERR_GENERIC,
		NP_ERR_NOCONNECTION,
		NP_ERR_INVALIDCALL,
		NP_ERR_INVALIDARGS
	};

	static const TCHAR *CONFIGKEY_MAXPIPEINSTANCES;
	static const TCHAR *CONFIGKEY_GRANTALLPERMISSION;
	static const TCHAR *CONFIGVALUE_TRUE;
	static const TCHAR *CONFIGVALUE_FALSE;

	typedef void(*OnDataReceived)(const TCHAR* csData, const size_t uiDataSize, TCHAR** pcsResponse, size_t* puiResponseSize);

	/************************************************************************/
	// Public Member Functions
	/************************************************************************/
	NPCode Config(const TCHAR *csKey, const TCHAR *csValue);

	NPCode Start(const TCHAR *csPipeName, OnDataReceived onDataReceived, size_t uiMaxBufferSize);

	/*	Description: Stops listening and closes the opened named-pipe server.
		Returns: NP_OK, NP_ERR_INVALIDCALL */
	NPCode Stop();

	CNamedPipeServer();
	~CNamedPipeServer();

private:
	static NPCode thread_StartListening(void *vpParams);
	static NPCode thread_ProcessMessage(void *vpParams);
	static void pipeGrantAccessForLowerPrivilegedUsers(HANDLE hPipe);

private:
	static CNamedPipeServer* m_Instance;

	BOOL IsRunning;
	BOOL GrantAllPermissions;
	// The entire pipe name string can be up to 256 characters long. Pipe names are not case sensitive.
	TCHAR PipeName[257];
	UINT32 PipeMaxInstances;
	HANDLE MainThread;
	NamedPipeParams *MainParams;
	OnDataReceived HandleDataReceived;
	size_t MaxBufferLength;

	CRITICAL_SECTION critsecPipe;
};
