#include "NamedPipeServer.h"
#include "StringUtil.h"

// Standard headers
#include <strsafe.h>
#include <process.h> // _beginthreadex
#include <aclapi.h> // setentriesinacl

// String-safe types & functions
#ifdef _UNICODE
	#define NPBUF_MEMSET wmemset
	#define NPBUF_MEMCPY wmemcpy
#else
	#define NPBUF_MEMSET memset
	#define NPBUF_MEMCPY memcpy
#endif

const TCHAR *CNamedPipeServer::CONFIGKEY_GRANTALLPERMISSION	= TEXT("grantallpermission");
const TCHAR *CNamedPipeServer::CONFIGVALUE_TRUE				= TEXT("1");
const TCHAR *CNamedPipeServer::CONFIGVALUE_FALSE				= TEXT("0");

CNamedPipeServer::CNamedPipeServer()
	: MainThread(NULL), MainParams(NULL), IsRunning(FALSE), GrantAllPermissions(TRUE)
{
	InitializeCriticalSection(&critsecPipe);
}

CNamedPipeServer::~CNamedPipeServer() 
{
	DeleteCriticalSection(&critsecPipe);
}

CNamedPipeServer::NPCode CNamedPipeServer::Config(const TCHAR *csKey, const TCHAR *csValue)
{
	NPCode rc = NP_ERR_INVALIDARGS;

	if(_tcscmp(csKey, CONFIGKEY_GRANTALLPERMISSION)==0)
	{
		rc = NP_OK;

		if(_tcscmp(csValue, CONFIGVALUE_FALSE)==0)
			GrantAllPermissions = FALSE;
		else if(_tcscmp(csValue, CONFIGVALUE_TRUE)==0)
			GrantAllPermissions = TRUE;
		else
			rc = NP_ERR_INVALIDARGS;
	}

	return rc;
}

CNamedPipeServer::NPCode CNamedPipeServer::Start(const TCHAR *csPipeName, OnDataReceived onDataReceived, size_t uiMaxBufferLength)
{
	NPCode rc = NP_ERR_GENERIC;
	
	// Validate parameters
	if(	!csPipeName ||
		!onDataReceived ||
		uiMaxBufferLength == 0)
		return NP_ERR_INVALIDARGS;

	// Thread-safety
	EnterCriticalSection(&critsecPipe);
	if(IsRunning)
	{
		LeaveCriticalSection(&critsecPipe);
		return NP_ERR_INVALIDCALL;
	}
	MaxBufferLength = uiMaxBufferLength;
	HandleDataReceived = onDataReceived;
	NPBUF_MEMSET(PipeName, 0, _countof(PipeName));
	StringCbCopy(PipeName, _countof(PipeName) * sizeof(TCHAR), csPipeName);
	IsRunning = TRUE;
	LeaveCriticalSection(&critsecPipe);

	// Allocate parameters
	MainParams = new NamedPipeParams;
	MainParams->This = this;

	// for extra precaution, _beginthreadex is used instead of CreateThread (http://support.microsoft.com/kb/104641/en-us)
	MainThread = (HANDLE)_beginthreadex(NULL,	// security
		0,								// stack size
		(unsigned int(__stdcall*)(void*))thread_StartListening,		// le routine
		MainParams,						// arguments
		0,								// initial state: running
		NULL							// pointer to thread identifier 
		);

	if(MainThread!=0)
		rc = NP_OK;
	else
	{
		// Reset all allocation and initialization;
		delete MainParams;
		MainParams = NULL;
		IsRunning = FALSE;
	}

	return rc;
}

CNamedPipeServer::NPCode CNamedPipeServer::Stop()
{
	NPCode rc = NP_ERR_INVALIDCALL;

	if(MainThread!=0)
	{
		TerminateThread(MainThread, NP_OK);
		CloseHandle(MainThread);
		MainThread = NULL;
		rc = NP_OK;
	}

	if(MainParams!=NULL)
	{
		delete MainParams;
		MainParams = NULL;
	}

	IsRunning = FALSE;

	return rc;
}
CNamedPipeServer::NPCode CNamedPipeServer::thread_StartListening(void *vpParams)
{
	NPCode rc = NP_ERR_GENERIC;
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread;
	CNamedPipeServer *pThis = ((NamedPipeParams*)vpParams)->This;

	while(pThis->IsRunning)
	{
		hPipe = CreateNamedPipe(pThis->PipeName,
			PIPE_ACCESS_DUPLEX | WRITE_DAC,					// read/write access
			PIPE_TYPE_MESSAGE |					// message type pipe
			PIPE_READMODE_MESSAGE |				// message-read mode
			PIPE_WAIT,							// blocking mode
			PIPE_UNLIMITED_INSTANCES,			// max instances
			(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// output buffer size
			(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// input buffer size
			0,									// client time-out
			NULL);								// default security attribute

		if(hPipe == INVALID_HANDLE_VALUE)
		{
			// Keep retrying
			Sleep(1000);
			continue;
		}

		if(pThis->GrantAllPermissions)
			pipeGrantAccessForLowerPrivilegedUsers(hPipe);

		// Wait for the client to connect; if it succeeds, 
		// the function returns a nonzero value. If the function
		// returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
		if(ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			// Set parameters to pass to each pipe connection.
			NamedPipeParams *params = new NamedPipeParams;
			params->PipeHandle = hPipe;
			params->This = pThis;

			// _beginthreadex returns 0 on an error, in which case errno and _doserrno are set.
			hThread = (HANDLE)_beginthreadex(NULL,				// security
				0,								// stack size
				(unsigned int(__stdcall*)(void*))thread_ProcessMessage, 
				params,							// arguments
				0,								// initial state: running
				NULL							// pointer to thread identifier 
				);

			// NOTE: Closing a thread handle does not terminate the associated thread or remove the thread object. 
			if(hThread!=0)
				CloseHandle(hThread);
			else
			{
				// Thread creation failed; Free resources
				CloseHandle(hPipe);
				delete params;
			}
		}
		else
		{
			// Pipe connection failed; Free resources.
			CloseHandle(hPipe);
		}
	}

	pThis->IsRunning = FALSE;

	// Not required to explicitly call _endthreadex() but helps ensure allocated resources are recovered.
	_endthreadex(rc);
	return rc;
}

CNamedPipeServer::NPCode CNamedPipeServer::thread_ProcessMessage(void *vpParams)
{
	NPCode rc = NP_ERR_GENERIC;
	NamedPipeParams *params = (NamedPipeParams*)vpParams;
	BOOL bSuccess;
	DWORD dwBytesProcessed, dwLastError;
	size_t uiMemOffset = 0;
	WCHAR *csData, *csTmp;
	size_t uiBufferLength;

	// Response variables
	TCHAR * csResponse;
	size_t uiResponseLength;

	// Extra error-checking
	if(	!params ||
		!(params->PipeHandle) )
	{
		_endthreadex(rc);
	}

	// Class instance
	CNamedPipeServer *pThis = params->This;

	uiBufferLength = pThis->MaxBufferLength;
	csData = new WCHAR[uiBufferLength];
	csTmp = new WCHAR[uiBufferLength];

	NPBUF_MEMSET(csData, 0, uiBufferLength);
	NPBUF_MEMSET(csTmp, 0, uiBufferLength);

	HANDLE hPipe = params->PipeHandle;

	while(TRUE)
	{
		bSuccess = ReadFile( 
			hPipe,									// handle to pipe 
			csTmp,									// buffer to receive data 
			(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// size in bytes of buffer 
			&dwBytesProcessed,						// number of bytes read 
			NULL);									// not overlapped I/O 

		if(!bSuccess || dwBytesProcessed==0)
		{
			dwLastError = GetLastError();

			// More data to come, keep reading.
			if (dwLastError==ERROR_MORE_DATA)
			{
				// Append data to final buffer
				NPBUF_MEMCPY(&csData[uiMemOffset], csTmp, dwBytesProcessed);
				uiMemOffset += dwBytesProcessed;
				NPBUF_MEMSET(csTmp, 0, uiBufferLength);
				continue;
			}

			// Unhandled error; Abort
			break;
		}

		// Got entire message
		NPBUF_MEMCPY(&csData[uiMemOffset], csTmp, dwBytesProcessed);
		uiMemOffset += dwBytesProcessed;
		break;
	}

	// No data received
	if(uiMemOffset==0)
		goto end;

	// Invoke callback
	csResponse = new WCHAR[uiBufferLength];
	NPBUF_MEMSET(csResponse, 0, uiBufferLength);
	pThis->HandleDataReceived(csData, uiMemOffset, &csResponse, &uiResponseLength);

	// Don't send a response if not specified.
	if(uiResponseLength > 0)
	{
		// Only write the maximum allowed
		if(uiResponseLength > pThis->MaxBufferLength)
			uiResponseLength = pThis->MaxBufferLength;

		// Send response
		bSuccess = WriteFile( 
			hPipe,								// handle to pipe 
			csResponse,							// buffer to write from 
			(DWORD)(uiResponseLength * sizeof(TCHAR)),	// number of bytes to write 
			&dwBytesProcessed,					// number of bytes written 
			NULL);								// not overlapped I/O 
	
		if(bSuccess)
			rc = NP_OK;
	}
	else rc = NP_OK; // Response is not required.

	delete [] csResponse;

end:
	// Free resources
	CloseHandle(hPipe);
	delete params;
	delete [] csData;
	delete [] csTmp;

	// Not required to explicitly call _endthreadex() but helps ensure allocated resources are recovered.
	_endthreadex(rc);
	return rc;
}

void CNamedPipeServer::pipeGrantAccessForLowerPrivilegedUsers(HANDLE hPipe)
{
	// An ACE must be added to pipe's DACL so that client processes
	// running under low-privilege accounts are also able to change state
	// of client end of this pipe to Non-Blocking and Message-Mode.
	PACL pACL = NULL;
	PACL pNewACL = NULL;
	EXPLICIT_ACCESS explicit_access_list[1];
	TRUSTEE trustee[1];
	HRESULT hr;

	hr = GetSecurityInfo(hPipe, 
		SE_KERNEL_OBJECT,					// Type of object
		DACL_SECURITY_INFORMATION,			// Type of security information to retrieve
		NULL, NULL,							// Pointer that receives owner SID & group SID
		&pACL,								// Pointer that receives a pointer to the DACL
		NULL, NULL							// Pointer that receives a pointer to the SACL & security descriptr of the object
		);
	if(FAILED(hr)) goto end;

	// Identifies the user, group, or program to which the access control entry (ACE) applies
	trustee[0].TrusteeForm = TRUSTEE_IS_NAME;
	trustee[0].TrusteeType = TRUSTEE_IS_GROUP;
	trustee[0].ptstrName = TEXT("Everyone");
	trustee[0].MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	trustee[0].pMultipleTrustee = NULL;

	ZeroMemory(&explicit_access_list[0], sizeof(EXPLICIT_ACCESS));
	explicit_access_list[0].grfAccessMode = GRANT_ACCESS;
	explicit_access_list[0].grfAccessPermissions = GENERIC_READ | GENERIC_WRITE; // Make sure to use exact same permissions for client!
	explicit_access_list[0].grfInheritance = NO_INHERITANCE;
	explicit_access_list[0].Trustee = trustee[0];

	// Merge access control information to the existing ACL
	hr = SetEntriesInAcl(_countof(explicit_access_list),		// Size of array passed in the next argument
		explicit_access_list,							// Pointer to array that describe the access control information
		pACL,											// Pointer to existing ACL
		&pNewACL										// Pointer that receives a pointer to the new ACL
		);
	if(FAILED(hr)) goto end;

	// Sets specified security information in the security descriptor of a specified object
	hr = SetSecurityInfo(hPipe, 
		SE_KERNEL_OBJECT,					// Type of object
		DACL_SECURITY_INFORMATION,			// Type of security information to set
		NULL, NULL,							// Pointer that identifies the owner SID & group SID
		pNewACL,							// Pointer to the new DACL
		NULL								// Pointer to the new SACL
		);
	if(FAILED(hr)) goto end;

end:
	if(pNewACL!=NULL)
		LocalFree(pNewACL);
}