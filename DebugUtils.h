#pragma once

#ifdef WA_VERBOSE

#include <time.h>
#define DEBUG_INIT(HANDLE, LOGFILE)				do{ _wfopen_s(&HANDLE, LOGFILE, L"w+, ccs=UTF-8"); } while(0)
#define DEBUG_LOG(HANDLE, MSG, ...)				do{ if(HANDLE!=NULL) { fwprintf_s(HANDLE, MSG, ## __VA_ARGS__); fflush(HANDLE); } } while(0)
#define DEBUG_DEINIT(HANDLE)					do{ if(HANDLE!=NULL) { fclose(HANDLE); HANDLE = NULL; } } while(0)

#include <conio.h>
#define DEBUG_PRINT(MSG, ...)					do{ _tcprintf_s(MSG, ## __VA_ARGS__); } while(0)

#else 

#define DEBUG_INIT(HANDLE, LOGFILE)
#define DEBUG_LOG(HANDLE, MSG, ...)
#define DEBUG_DEINIT(HANDLE)

#define DEBUG_PRINT(MSG, ...)

#endif