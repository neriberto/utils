#include "NamedPipeClient.h"
#include "StringUtil.h"

CNamedPipeClient* CNamedPipeClient::m_Instance = 0;

CNamedPipeClient::CNamedPipeClient()
	: hPipe(INVALID_HANDLE_VALUE)
{	
}

CNamedPipeClient::~CNamedPipeClient() 
{
}

CNamedPipeClient* CNamedPipeClient::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new CNamedPipeClient();
		m_Instance->init();
	}

	return m_Instance;
}

void CNamedPipeClient::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;  
		m_Instance = 0;
	}
}
bool CNamedPipeClient::init() 
{
	InitializeCriticalSection(&this->critsecPipe);
	return true;
}
void CNamedPipeClient::deinit()
{
	DeleteCriticalSection(&this->critsecPipe);
}

CNamedPipeClient::NPCode CNamedPipeClient::Connect(const wchar_t *wcsPipeName, const unsigned int &uiTimeoutSecs /* = 5 */)
{
	NPCode rc = NP_ERR_GENERIC;
	const unsigned int uiTimeoutMs = uiTimeoutSecs * 1000, uiTimeMsInterval = 1000;
	unsigned int uiTimeMsElapsed = 0;
	bool bTimesUp = false;

	EnterCriticalSection(&this->critsecPipe);

	// Multiple attempt to connect every second.
	while(true)
	{
		this->hPipe = CreateFileW(wcsPipeName, 
			GENERIC_READ | GENERIC_WRITE | FILE_FLAG_OVERLAPPED, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(this->hPipe==INVALID_HANDLE_VALUE)
		{
			DWORD dwLastError = GetLastError();
			// If time out period hasn't elapsed, sleep, and try again.
			if(uiTimeMsElapsed<uiTimeoutMs)
			{
				Sleep(uiTimeMsInterval);
				uiTimeMsElapsed += uiTimeMsInterval;
			}
			else goto end; // Time out and finish.
		}
		else break;
	}

	rc = NP_OK;
end:
	LeaveCriticalSection(&this->critsecPipe);
	return rc;
}

void CNamedPipeClient::Disconnect()
{
	EnterCriticalSection(&this->critsecPipe);
	if (this->hPipe!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(this->hPipe);
		this->hPipe = INVALID_HANDLE_VALUE;
	}
	LeaveCriticalSection(&this->critsecPipe);

}

CNamedPipeClient::NPCode CNamedPipeClient::SendMessage(const wchar_t *wcsMessage)
{
	// always read and write messages in unicode - ascii can be either singlebyte or multibyte sequences
	string sMessage = StringUtil::WStrToStr(wcsMessage);
	DWORD dwBytesWritten, dwLastErr;
	BOOL bSuccess;
	NPCode rc = NP_ERR_GENERIC;
	OVERLAPPED o;

	memset(&o, 0, sizeof(o));

	EnterCriticalSection(&this->critsecPipe);
	if(this->hPipe==INVALID_HANDLE_VALUE)
	{
		rc = NP_ERR_NOCONNECTION;
		goto end;
	}
	
	o.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

	bSuccess = WriteFile(this->hPipe, sMessage.c_str(), (DWORD) sMessage.length(), &dwBytesWritten, 
		&o // overlapped, non-blocking
		);
	dwLastErr = GetLastError();

	// Must be FALSE if asynchronous (unless it finishes it right away)
	if(!bSuccess && dwLastErr==ERROR_IO_PENDING)
	{
		DWORD dwWait = WaitForSingleObject(o.hEvent, 4000);
		if(dwWait!=WAIT_OBJECT_0)
		{
			// Times up or error ocurred, cancel the IO read
			CancelIo(hPipe);
			GetOverlappedResult(this->hPipe, &o, &dwBytesWritten, TRUE); // Wait until IO cancellation is finished
		}
		else
		{
			bSuccess = GetOverlappedResult(this->hPipe, &o, &dwBytesWritten, TRUE);
			if(sMessage.length()==dwBytesWritten)
				rc = NP_OK;
		}
	}

end:
	if(o.hEvent!=NULL)
		CloseHandle(o.hEvent);
	LeaveCriticalSection(&this->critsecPipe);
	return rc;
}