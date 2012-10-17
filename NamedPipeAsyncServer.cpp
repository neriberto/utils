#include "NamedPipeAsyncServer.h"
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

#define PIPE_DEFAULT_MAX_INSTANCES				5

const TCHAR *CNamedPipeServer::CONFIGKEY_MAXPIPEINSTANCES	= TEXT("maxpipeinstances");
const TCHAR *CNamedPipeServer::CONFIGKEY_GRANTALLPERMISSION	= TEXT("grantallpermission");
const TCHAR *CNamedPipeServer::CONFIGVALUE_TRUE				= TEXT("1");
const TCHAR *CNamedPipeServer::CONFIGVALUE_FALSE				= TEXT("0");

CNamedPipeServer::CNamedPipeServer()
	: MainThread(NULL), MainParams(NULL), IsRunning(FALSE), GrantAllPermissions(TRUE)
{
	InitializeCriticalSection(&critsecPipe);

	PipeMaxInstances = PIPE_DEFAULT_MAX_INSTANCES;
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
	if(_tcscmp(csKey, CONFIGKEY_GRANTALLPERMISSION)==0)
	{
		PipeMaxInstances=StringUtil::StrToInt(csValue);
		if( PipeMaxInstances>=1 && PipeMaxInstances<=255 )
			rc = NP_OK;
		else
		{
			rc = NP_ERR_INVALIDARGS;
			PipeMaxInstances = PIPE_DEFAULT_MAX_INSTANCES;
		}
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

	// Create all pipe instances and spawn a thread for each one
	for(UINT32 i=0; i<pThis->PipeMaxInstances; i++)
	{
		hPipe = CreateNamedPipe(pThis->PipeName,
			PIPE_ACCESS_DUPLEX | WRITE_DAC | FILE_FLAG_OVERLAPPED,					// read/write access
			PIPE_TYPE_MESSAGE |					// message type pipe
			PIPE_READMODE_MESSAGE |				// message-read mode
			PIPE_WAIT,							// blocking mode
			pThis->PipeMaxInstances, //PIPE_UNLIMITED_INSTANCES,			// max instances
			(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// output buffer size
			(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// input buffer size
			0,									// client time-out
			NULL);								// default security attribute

		if(hPipe == INVALID_HANDLE_VALUE)
		{
			DWORD dw = GetLastError();
			// Keep retrying
			Sleep(1000);
			continue;
		}

		if(pThis->GrantAllPermissions)
			pipeGrantAccessForLowerPrivilegedUsers(hPipe);

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
			delete params;
			goto try_again;
		}

		continue;

try_again:
		// goto here when something fails; proper freeing of resources done here.
		CloseHandle(hPipe);
	}

	while(pThis->IsRunning)
		Sleep(1000);

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
	TCHAR *csData, *csTmp;
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
	csData = new TCHAR[uiBufferLength];
	csTmp = new TCHAR[uiBufferLength];

	NPBUF_MEMSET(csData, 0, uiBufferLength);
	NPBUF_MEMSET(csTmp, 0, uiBufferLength);

	HANDLE hPipe = params->PipeHandle;
	DWORD dwWait;

	// IO results will be stored here by the OS asynchronously
	OVERLAPPED o;
	o.hEvent = CreateEvent( 
		NULL,    // default security attribute 
		TRUE,    // manual-reset event 
		TRUE,    // initial state = signaled 
		NULL);   // unnamed event object
	_ASSERTE(o.hEvent!=NULL);

	while(TRUE)
	{
		// Wait for the client to connect; If it succeeds, overlapped function should return zero 
		// (only source of this is in the sample code, not documented otherwise) and 
		// GetLastError returns ERROR_PIPE_CONNECTED or ERROR_IO_PENDING.		
		BOOL bConnectResult = ConnectNamedPipe(hPipe, &o);
		DWORD dwError = GetLastError();

		// Non-zero for success
		if(bConnectResult)
		{
			// Keep trying
			goto next_client;
		}

		switch(dwError)
		{
		case ERROR_PIPE_CONNECTED:
			// Signal event that pipe connection has *already* been established prior to ConnectNamedPipe() call
			if(!SetEvent(o.hEvent))
				goto new_connection;
			// drop through to next case
		case ERROR_IO_PENDING:
			// Wait for pipe connection to establish
			dwWait = WaitForSingleObject(o.hEvent, INFINITE); // 2 minutes.
			if(dwWait!=WAIT_OBJECT_0)
				goto new_connection;

			// Pipe connection established, now listen for data
			o.Offset = 0;
			o.OffsetHigh = 0;
			bSuccess = ReadFile( 
				hPipe,									// handle to pipe 
				csTmp,									// buffer to receive data 
				(DWORD)(pThis->MaxBufferLength * sizeof(TCHAR)),	// size in bytes of buffer 
				&dwBytesProcessed,									// number of bytes read; NULL since this is asynchronous
				&o);									// overlapped I/O 
			dwLastError = GetLastError();

			// The read operation completed successfully. 
			if (bSuccess && dwBytesProcessed!=0) 
			{
				// Got entire message
				NPBUF_MEMCPY(&csData[uiMemOffset], csTmp, dwBytesProcessed);
				uiMemOffset += dwBytesProcessed;
				break;
			}

			// It may return 0 and get into a ERROR_IO_PENDING state for an asynchronous read, if data read is still pending.
			if(!bSuccess && dwLastError==ERROR_IO_PENDING)
			{
				// Wait for read IO to process
				dwWait = WaitForSingleObject(o.hEvent, 120000); // 2 minutes
				if(dwWait!=WAIT_OBJECT_0)
				{
					// Times up or error occurred, cancel the IO read
					// How to prevent stack corruption (see link):
					// http://blogs.msdn.com/b/oldnewthing/archive/2011/02/02/10123392.aspx
					CancelIo(hPipe);
					GetOverlappedResult(hPipe, &o, &dwBytesProcessed, TRUE); // Wait until IO cancellation is finished
					goto new_connection;
				}
				else
				{
					// Read complete, extract the result.
					bSuccess = GetOverlappedResult(
						hPipe,					// handle to pipe 
						&o,						// OVERLAPPED structure 
						&dwBytesProcessed,      // bytes transferred 
						TRUE);
					dwLastError = GetLastError();

					// Got the message
					if(bSuccess)
					{
						NPBUF_MEMCPY(&csData[uiMemOffset], csTmp, dwBytesProcessed);
						uiMemOffset += dwBytesProcessed;
						break;
					}
					else if (dwLastError==ERROR_MORE_DATA)
					{
						// Append data to final buffer
						NPBUF_MEMCPY(&csData[uiMemOffset], csTmp, dwBytesProcessed);
						uiMemOffset += dwBytesProcessed;
						NPBUF_MEMSET(csTmp, 0, uiBufferLength);
						// More data to come, keep reading.
						continue;
					}
				}
			} // if(!bSuccess && dwLastError==ERROR_IO_PENDING)
			// else // Unhandled error 			

			break;
		default:
			break;
		}

		// No data received
		if(uiMemOffset==0)
			goto new_connection;

		// Invoke callback
		csResponse = new TCHAR[uiBufferLength];
		NPBUF_MEMSET(csResponse, 0, uiBufferLength);
		pThis->HandleDataReceived((const TCHAR*)csData, uiMemOffset, &csResponse, &uiResponseLength);

		// Don't send a response if not specified.
		if(uiResponseLength > 0)
		{
			// Only write the maximum allowed
			if(uiResponseLength > pThis->MaxBufferLength)
				uiResponseLength = pThis->MaxBufferLength;

			// Send response
			o.Offset = 0;
			o.OffsetHigh = 0;
			bSuccess = WriteFile( 
				hPipe,								// handle to pipe 
				csResponse,							// buffer to write from 
				(DWORD)(uiResponseLength * sizeof(TCHAR)),	// number of bytes to write 
				&dwBytesProcessed,					// number of bytes written 
				&o);								// not overlapped I/O 
			dwLastError = GetLastError();

			// It may return 0 and get into a ERROR_IO_PENDING state for an asynchronous write, if data write is still pending.
			if(!bSuccess && dwLastError==ERROR_IO_PENDING)
			{
				// Wait for write IO to process
				dwWait = WaitForSingleObject(o.hEvent, 120000); // 2 minutes
				if(dwWait!=WAIT_OBJECT_0)
				{
					// Times up or error occurred, cancel the IO read
					// How to prevent stack corruption (see link):
					// http://blogs.msdn.com/b/oldnewthing/archive/2011/02/02/10123392.aspx
					CancelIo(hPipe);
					GetOverlappedResult(hPipe, &o, &dwBytesProcessed, TRUE); // Wait until IO cancellation is finished
					goto new_connection;
				}
				else
				{
					// Write complete, extract the result.
					bSuccess = GetOverlappedResult(
						hPipe,					// handle to pipe 
						&o,						// OVERLAPPED structure 
						&dwBytesProcessed,      // bytes transferred 
						TRUE);
					dwLastError = GetLastError();

					// Wrote the message
					if(bSuccess)
						rc = NP_OK;
				}
			} // if(!bSuccess && dwLastError==ERROR_IO_PENDING)
			// else // Unhandled error, unless it succeeded.
		}
		else rc = NP_OK; // Response is not required.

		delete [] csResponse;

		goto next_client;

new_connection:
		// Force close the server end of the pipe
		DisconnectNamedPipe(hPipe);

next_client:
		uiMemOffset = 0;

		// Must be reset every time pipe is reused.
		o.Offset = 0;
		o.OffsetHigh = 0;

		NPBUF_MEMSET(csData, 0, uiBufferLength);
		NPBUF_MEMSET(csTmp, 0, uiBufferLength);
	}

	// Free resources
	CloseHandle(o.hEvent);
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