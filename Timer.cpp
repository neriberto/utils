// include me first
#include "Timer.h"
#include "ClientCodes.h"

// standard includes
#include <process.h> // _beginthreadex

namespace WAUtils
{
CTimer::CTimer()
{
	MainThread = NULL;
	MainParams = NULL;

	clear();
}

CTimer::~CTimer()
{
	clear();
}

void CTimer::clear()
{
	// Internal variables
	Running = FALSE;	
	TimeSleptInMs = 0;

	// Clear configurations
	HandleTimeElapsed = NULL;
	TimeToWaitInMs = 0;

	if(MainThread!=NULL)
	{
		TerminateThread(MainThread, G_OK);
		CloseHandle(MainThread);
		MainThread = NULL;
	}

	if(MainParams!=NULL)
	{
		delete MainParams;
		MainParams = NULL;
	}
}
BOOL CTimer::IsRunning()
{
	return Running;
}
int CTimer::SetTimer(size_t uiTimeInMilliseconds, OnTimeElapsed onTimeElapsed)
{
	int rc = G_FAIL;

	if(!onTimeElapsed ||
		uiTimeInMilliseconds == 0)
		return G_INVALID_PARAM;

	TimeToWaitInMs = uiTimeInMilliseconds;

	HandleTimeElapsed = onTimeElapsed;

	rc = G_OK;

	return rc;
}

int CTimer::Start(DWORD dwUserContext)
{
	int rc = G_FAIL;

	// Allocate parameters
	MainParams = new TimerParams;
	MainParams->This = this;

	UserContext = dwUserContext;

	// We set the running state here, not in the thread where it would be asynchronous.
	Running = TRUE;

	// for extra precaution, _beginthreadex is used instead of CreateThread (http://support.microsoft.com/kb/104641/en-us)
	MainThread = (HANDLE)_beginthreadex(NULL,	// security
		0,								// stack size
		(unsigned int(__stdcall*)(void*))thread_StartTimer,		// le routine
		MainParams,						// arguments
		0,								// initial state: running
		NULL							// pointer to thread identifier 
		);

	if(MainThread!=0)
		rc = G_OK;
	else
	{
		Running = FALSE;

		// Reset all allocation and initialization;
		delete MainParams;
		MainParams = NULL;
	}	

	return rc;
}

void CTimer::Stop()
{
	// We don't call clear() because we don't want to reset the configurations.
	Running = FALSE;
	TimeSleptInMs = 0;

	if(MainThread!=NULL)
	{
		TerminateThread(MainThread, G_OK);
		CloseHandle(MainThread);
		MainThread = NULL;
	}

	if(MainParams!=NULL)
	{
		delete MainParams;
		MainParams = NULL;
	}
}

int CTimer::thread_StartTimer(void *vpParams)
{
	int rc = G_FAIL;
	CTimer *pThis = ((TimerParams *)vpParams)->This;	
	size_t uiIntervalMs = 500;

	pThis->TimeSleptInMs = 0;

	while(pThis->TimeSleptInMs < pThis->TimeToWaitInMs)
	{
		Sleep(uiIntervalMs);

		pThis->TimeSleptInMs += uiIntervalMs;
	}
	pThis->Running = FALSE;

	// Signal callback now that specified time has elapsed.
	if(pThis->TimeSleptInMs >= pThis->TimeToWaitInMs)
	{
		pThis->HandleTimeElapsed(pThis->TimeSleptInMs, pThis->UserContext);
	}
	
	pThis->Stop();

	// Not required to explicitly call _endthreadex() but helps ensure allocated resources are recovered.
	_endthreadex(rc);
	return rc;
}
size_t CTimer::TimeRemainingInMilliseconds()
{
	if(TimeToWaitInMs > TimeSleptInMs)
		return TimeToWaitInMs - TimeSleptInMs;

	return 0;
}
} // namespace WAUtils