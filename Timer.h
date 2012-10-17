#pragma once
#include <Windows.h>

namespace WAUtils
{
	class CTimer
	{
		struct TimerParams
		{
			CTimer *This;
		};

	public:
		CTimer();
		~CTimer();

		typedef void(*OnTimeElapsed)(const size_t uiTimeElapsed, DWORD UserContext);

		int SetTimer(size_t uiTimeInMilliseconds, OnTimeElapsed onTimeElapsed);
		int Start(DWORD dwUserContext);
		void Stop();
		BOOL IsRunning();
		size_t TimeRemainingInMilliseconds();

	private:
		OnTimeElapsed HandleTimeElapsed;
		HANDLE MainThread;
		TimerParams *MainParams;
		size_t TimeToWaitInMs;
		size_t TimeSleptInMs;
		BOOL Running;
		DWORD UserContext;

		static int thread_StartTimer(void *vpParams);
		void clear();
	};
}