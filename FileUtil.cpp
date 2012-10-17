#include "FileUtil.h"
#include "StringUtil.h"
#include "RegUtil.h"
#include "ClientCodes.h"

using namespace WAUtils;

#include <tchar.h>
#include <Strsafe.h>
#include <fstream>
#include <errno.h>
#include <vector>

using namespace std;

FileUtil* FileUtil::m_Instance = 0;

FileUtil* FileUtil::Instance()
{
	if (!m_Instance)
	{
		m_Instance = new FileUtil();
		m_Instance->init();
	}

	return m_Instance;
}

void FileUtil::DestroyInstance()
{
	if (m_Instance)
	{
		m_Instance->deinit();
		delete m_Instance;
		m_Instance = 0;
	}
}
void FileUtil::deinit()
{
}
bool FileUtil::init()
{
	m_wasInitialized = true;

	return true;
}
FileUtil::FileUtil(void)
{
#ifndef _DEBUG
	try
	{
#endif
		pmd=NULL;
		pTokenUser = NULL;
		HANDLE hToken;
		HANDLE g_eventHandle = NULL;
		DWORD dwLength = 0;


		// in order to use ReportEvent we must first Register Event

		OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);

		// Get required buffer size and allocate the PTOKEN_USER buffer.

		if (!GetTokenInformation(
			hToken,         // handle to the access token

			TokenUser,    // get information about the token's groups

			(LPVOID) pTokenUser,   // pointer to TOKEN_USER buffer

			0,              // size of buffer

			&dwLength       // receives required buffer size

		))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				pTokenUser = (PTOKEN_USER)HeapAlloc(GetProcessHeap(),
					HEAP_ZERO_MEMORY, dwLength);

				if (pTokenUser != NULL)
				{
					// Get the token group information from the access token.

					if (!GetTokenInformation(
						hToken,         // handle to the access token

						TokenUser,    // get information about the token's groups

						(LPVOID) pTokenUser,   // pointer to TOKEN_USER buffer

						dwLength,       // size of buffer

						&dwLength       // receives required buffer size

					))
					{
						if (pTokenUser != NULL)
						{
							HeapFree(GetProcessHeap(), 0, (LPVOID)pTokenUser);
							pTokenUser = NULL;
						}    
					}
				}
			}
		}
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
}

FileUtil::~FileUtil(void)
{
#ifndef _DEBUG
	try
	{
#endif

#ifdef _USE_WAUF_PROXY_EVENT_LOG_MESSAGES
		if (pmd != NULL)
		{
			BreakdownLoggingLevelMonitoring();
		}
#endif
		// Free the buffer for the token .

		if (pTokenUser != NULL)
		{
			HeapFree(GetProcessHeap(), 0, (LPVOID)pTokenUser);
			pTokenUser = NULL;
		}
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
}
// Description: Copy a file
// Parameters:
//		src [in] Path to a file (absolute or just the file name) that is to be copied.
//		dst [in] Path where the file in 'src' is copied to.
//			- NULL or Empty string: 'src' is copied to the current working directory, which modifying the file name.
//			- A path ending with a backslash '\': This is treated as a directory, so 'src' will be copied to this path without modifying the file name.
//		bAllowOverwrite: 'true' if the destination file can be overwritten if it already exists; 'false' otherwise.
int FileUtil::CopyFile(const TCHAR *src, const TCHAR *dst, bool bAllowOverwrite)
{
	int rc = G_FAIL;
	tstring tsDst = TEXT("");
	tstring tsSrc = src;

	// Validations
	_ASSERT(!tsSrc.empty());
	_ASSERT(tsSrc[tsSrc.length()-1]!=TEXT('\\'));

	// If dst is empty, then it means copy the source file to the working directory without changing the file name.
	if(dst==NULL || _tcscmp(dst, TEXT(""))==0)
	{		
		size_t pos =0;
		if((pos=tsSrc.rfind(TEXT("\\")))!=tsSrc.npos)
			tsDst = tsSrc.substr(pos+1);
		else
			tsDst = tsSrc;
	}
	else if(dst[_tcslen(dst)-1]==TEXT('\\'))
	{
		// If dst ends with a backslash, then it means put the src file in that directory without changing the file name.
		tsDst = dst;

		size_t pos = 0;
		if((pos=tsSrc.rfind(TEXT("\\")))!=tsSrc.npos)
			tsDst += tsSrc.substr(pos+1);
		else // No backslash, tsSrc is only a filename.
			tsDst += tsSrc;
	}
	else
		tsDst = dst;

	if(::CopyFile(src, tsDst.c_str(), 
		!bAllowOverwrite	// If this parameter is TRUE and the new file specified by lpNewFileName already exists, the function fails. 
							//If this parameter is FALSE and the new file already exists, the function overwrites the existing file and succeeds.
		)==0)
	{
		goto end;
	}

	rc = G_OK;
end:
	return rc;
}
// Recursively moves files within 'wcsSrcDir' to 'wcsDestDir'.
bool FileUtil::MoveFilesToDir(const wchar_t * wcsSrcDir, const wchar_t * wcsDestDir) 
{
	wstring wsSrcDir = wcsSrcDir;
	wstring wsDestDir = wcsDestDir;
	wstring wsSearchStr;

	// Go over files in source dir
	HANDLE hSearch; 
	WIN32_FIND_DATAW FileData; 
	BOOL bFinished = FALSE;

	StringUtil::EnsurePathEndingW(wsSrcDir);
	StringUtil::EnsurePathEndingW(wsDestDir);	

	wsSearchStr = wsSrcDir;
	wsSearchStr += L"*.*";

	hSearch = FindFirstFileW(wsSearchStr.c_str(), &FileData); 
	if (hSearch == INVALID_HANDLE_VALUE) 
		return true;

	while (!bFinished) 
	{ 
		wstring wsFile = FileData.cFileName;

		if (wsFile != L"." && wsFile != L"..")
		{
			wstring::size_type pos = wsFile.find_last_of(L"\\/");
			if (pos != wstring::npos) 
			{
				//we need to strip the dir name
				wsFile = wsFile.substr(pos+1);
			}

			//is this another dir?
			if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
			{
				MoveFilesToDir((wsSrcDir+wsFile).c_str(), (wsDestDir+wsFile).c_str());
				RemoveDirectoryW((wsSrcDir+wsFile).c_str());
			}
			else 
			{
				// Delete dest file if it exists
				wstring destFile = wsDestDir + wsFile;
				wstring sourceFile = wsSrcDir + wsFile;
				DeleteFileW(destFile.c_str());
				if (!MoveFileW(sourceFile.c_str(), destFile.c_str())) {
					//if fails to move, delete src file.
					DeleteFileW(sourceFile.c_str());
					//return false;  //Problematic becuase NCL.dll fails.
				}
			}
		}
		if (!FindNextFileW(hSearch, &FileData)) 
		{
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
				bFinished = TRUE; 
			} 
			else 
			{ 
				//no error reporting on thi method so...
				bFinished = TRUE;
			}
		}
	} 

	// Close the search handle. 
	FindClose(hSearch);
	return true;
}
#define LONG_FILENAME_PREFIX L"\\\\?\\"
bool FileUtil::DeletePathRecursively(const wchar_t * wcsPath)
{
	if(wcslen(wcsPath) == 0)
		return false;

	wstring sPath = wcsPath;

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind;
	wstring fileName;
	bool ret = true;

	StringUtil::EnsurePathEndingW(sPath);

	wstring wsForLongFileName = LONG_FILENAME_PREFIX + sPath;
	hFind = FindFirstFileW(  ( wsForLongFileName + L"*" ).c_str(),  &findFileData  );
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do 
		{
			fileName = findFileData.cFileName;
			if(  fileName != L"."  &&  fileName != L".."  )
			{
				fileName.insert( 0, sPath);

				if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ||
					findFileData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
				{
					SetFileAttributesW(fileName.c_str(), findFileData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
				}


				if( (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)//file is a directory
				{
					ret &= DeletePathRecursively(fileName.c_str());
				}
				else//is a file 
				{
					wsForLongFileName = LONG_FILENAME_PREFIX + fileName;
					ret &= (DeleteFileW(wsForLongFileName.c_str()) != FALSE);
				}
			}
		} while(  FindNextFileW(hFind, &findFileData)  );

		FindClose(hFind);
	}

	wsForLongFileName = LONG_FILENAME_PREFIX + sPath;
	ret &= (RemoveDirectoryW(wsForLongFileName.c_str()) != FALSE);

	return ret;
}

string FileUtil::FileContentsAsString(const char *csFileName)
{
	string sContents = "";
	char* csFile = NULL;
	size_t uiSize = 0;

	// Opening and loading the file into the memory
	ifstream file;
	file.open(csFileName);

	if( !file.is_open() )
		return "";

	//Allocating the buffer and copy all file content there
	file.seekg(0, ios_base::end );

	uiSize = (size_t)file.tellg();
	if(uiSize==0)
	{
		file.close();
		return "";
	}
	sContents.reserve(uiSize+1);
	file.seekg(0, ios_base::beg);

	csFile = new char[uiSize+1];
	memset(csFile, 0, uiSize+1);
	file.read(csFile, uiSize); // Don't read to the last char because we want to reserve that for null.

	//copy buffer to string
	sContents = csFile;

	delete [] csFile;
	file.close();

	return sContents;
}
int FileUtil::GetFilesInDirectory(const TCHAR *csDir,
								vector<tstring> &vecFiles,
								bool bAbsolutePaths, // = false
								bool bRecursive // = false; currently not supported
	)
{
	tstring sDirSearchExpression = csDir;
	tstring sDir = csDir;
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError;
	int rc = G_FAIL;

	vecFiles.clear();

	StringUtil::EnsurePathEnding(sDir);
	StringUtil::EnsurePathEnding(sDirSearchExpression);
	sDirSearchExpression += TEXT("*");

	hFind = FindFirstFile(sDirSearchExpression.c_str(), &ffd);
	if(hFind==INVALID_HANDLE_VALUE)
	{
		rc = G_PATH_NOT_FOUND;
		return rc;
	}

	do 
	{
		if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;

		if(bAbsolutePaths)
			vecFiles.push_back(sDir + ffd.cFileName);
		else
			vecFiles.push_back(ffd.cFileName);

	} while (FindNextFile(hFind, &ffd)!=0);

	dwError = GetLastError();
	if (dwError==ERROR_NO_MORE_FILES)
		rc = G_OK;

	FindClose(hFind);

	return rc;
}
int FileUtil::GetCurrentDir(tstring &tsCurrentDir)
{
	TCHAR szModulePath[MAX_PATH];
	int rc = G_FAIL;

	if (!GetModuleFileName(GetModuleHandle(NULL), szModulePath, sizeof(szModulePath)))
		return rc;

	tsCurrentDir = szModulePath;
	tsCurrentDir = tsCurrentDir.substr(0, tsCurrentDir.rfind(TEXT("\\")));
	tsCurrentDir += TEXT("\\");

	rc = G_OK;

	return rc;
}
int FileUtil::GetTempDir(tstring &tsTempPath)
{
	int rc = G_FAIL;
	int tmpPathSize;
	TCHAR *tmpDir;
	tsTempPath = TEXT("");

	if ( (tmpPathSize = GetTempPath(0, NULL)) ) 
	{
		if ( (tmpDir = new TCHAR[tmpPathSize]) ) 
		{
			if (GetTempPath(tmpPathSize, tmpDir))
			{
				tsTempPath = tmpDir;
				rc = G_OK;
			}
			delete [] tmpDir;
		}
	}

	return rc;
}
int FileUtil::GetWindowsTempDir(tstring &tsTempPath)
{
	int rc = G_FAIL;
	TCHAR tcsBuf[32767] = { 0 };
	DWORD dwExpandedSize = ExpandEnvironmentStrings(TEXT("%WINDIR%"), tcsBuf, _countof(tcsBuf));
	if(dwExpandedSize==0)
		return rc;
	
	tsTempPath = tcsBuf;
	StringUtil::EnsurePathEnding(tsTempPath);
	tsTempPath += TEXT("TEMP\\");
	rc = G_OK;

	return rc;
}
bool FileUtil::GetVersionOfFile(const TCHAR *csFileName, TCHAR *csVerBuf, size_t uiVerBufLength,
							 TCHAR *csLangBuf, size_t uiLangBufLength)
{
	DWORD dwScratch;
	DWORD * pdwLangChar;
	DWORD dwInfSize ;
	UINT uSize;
	BYTE * pbyInfBuff;
	TCHAR szVersion [32];
	TCHAR szResource [80];
	TCHAR * csVersion = szVersion;
	bool rc = false;

	dwInfSize = GetFileVersionInfoSize(csFileName, &dwScratch);
	if (dwInfSize <= 0)
		return rc;

	pbyInfBuff = new BYTE [dwInfSize];
	memset(pbyInfBuff, 0, dwInfSize);
	if (!pbyInfBuff)
		return rc;

	if (GetFileVersionInfo (csFileName, 0, dwInfSize, pbyInfBuff))
	{
		if (VerQueryValue (pbyInfBuff, TEXT("\\VarFileInfo\\Translation"), (void**)(&pdwLangChar), &uSize))
		{
			StringCbPrintf(szResource, _countof(szResource) * sizeof(TCHAR), 
				TEXT("\\StringFileInfo\\%04X%04X\\FileVersion"), LOWORD (*pdwLangChar), HIWORD (*pdwLangChar));

			if (VerQueryValue(pbyInfBuff, szResource, (void**)(&csVersion), &uSize))
			{
				StringCbCopy(csVerBuf, uiVerBufLength * sizeof(TCHAR), csVersion);
				rc = true;
			}
		}
	}

	delete [] pbyInfBuff;

	return rc;
}
bool FileUtil::IsFolderPath(const tstring & objPath)
{
	HANDLE hFile = NULL;
	WIN32_FIND_DATA findData;
	bool bResult = false;

	if(( hFile = FindFirstFile( objPath.c_str(), &findData )) != INVALID_HANDLE_VALUE )
	{
		FindClose( hFile );
		if( findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			bResult = true;
		else
			bResult = false;
	}

	return bResult;
}
bool FileUtil::ListFilesInDirectory( tstring &tsDirectory, vector<tstring> & vecFiles , bool bEnforceDirectoryCheck /* = true */ )
{
	// check if we should enforce this or not
	if( bEnforceDirectoryCheck )
	{
		// check if the directory exists and it is a directory
		if( !FileUtil::IsFolderPath(tsDirectory) )
			return false;
	}

	WIN32_FIND_DATA		wfdFindFile;			// find file data
	HANDLE				hDirectoryHandle;		// handle to directory
	tstring				tsPrepDirectory;		// prepare directory name with "*"

	// prep directory to retrieve its content
	tsPrepDirectory = tsDirectory;
	StringUtil::EnsurePathEnding( tsDirectory );
	tsPrepDirectory = tsDirectory + TEXT("*");

	// retrieve handle to directory and data
	hDirectoryHandle = FindFirstFile( tsPrepDirectory.c_str(), &wfdFindFile );
	if( hDirectoryHandle != INVALID_HANDLE_VALUE )
	{
		// while we have more files
		BOOL bHasNext = TRUE;
		while( bHasNext )
		{
			// extract the file name
			tstring tsFileName = wfdFindFile.cFileName;

			// add filters to ignore these special case hidden directories
			if( tsFileName != TEXT(".") && tsFileName != TEXT("..") )
			{
				// retrieve the full path of the current file
				tstring tcsFilePath = tsDirectory + tsFileName;

				// check if it is a directory
				if( wfdFindFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					// recurse child directories
					FileUtil::ListFilesInDirectory(tcsFilePath, vecFiles);
				}
				else 
				{
					// add the file to the return vector
					vecFiles.push_back(tcsFilePath);
				}
			}

			// go to the next file
			bHasNext = FindNextFile( hDirectoryHandle, &wfdFindFile );
		}
		// close handle to directory
		FindClose( hDirectoryHandle );

		// successful
		return true;
	}

	return false;
}
int FileUtil::RemoveFile( const TCHAR *tcsFileName )
{
	if (DeleteFile(tcsFileName)==0)
	{
		switch(GetLastError())
		{
			//Doesn't exist
		case ERROR_FILE_NOT_FOUND:
			return G_FILE_NOT_FOUND;
		case ERROR_ACCESS_DENIED:
			return G_ACCESS_DENIED;
		default:
			return G_FAIL;
		}
	}

	return G_OK;
}

#define BUF_SIZE 255

DWORD WINAPI LoggingLevelThreadFunction( LPVOID lpParam );

#ifdef _USE_WAUF_PROXY_EVENT_LOG_MESSAGES
bool FileUtil::SetupLoggingLevelMonitoring(TCHAR * tsRegistryName, TCHAR * tsDWORDName)
{
	bool bResult = false;

#ifndef _DEBUG
	try
	{
#endif
		// create a thread to keep track of the logging state for this registry key
		psLLTData pData;
		DWORD   dwThreadId;
		HANDLE  hThread; 

		// Create MAX_THREADS worker threads.

		{
			// Allocate memory for thread data.

			pData = (psLLTData) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					sizeof(sLLTData));

			if( pData != NULL )
			{
				// Generate unique data for each thread to work with.

				size_t nLen=_tcslen(tsRegistryName)+1;
				pData->tsRegistryName = (TCHAR *) malloc(nLen);
				_tcscpy_s(pData->tsRegistryName, nLen, tsRegistryName);
				nLen=_tcslen(tsDWORDName)+1;
				pData->tsDWORDName = (TCHAR *) malloc(nLen);
				_tcscpy_s(pData->tsDWORDName, nLen, tsDWORDName);
				pData->bThreadRunning = false;
				pData->dwLastError = 0;
				pData->dwLoggingLevel = 0xFFFFFFFF; // indicates  unset

				// Create the thread to begin execution on its own.

				hThread = CreateThread( 
					NULL,                   // default security attributes
					0,                      // use default stack size  
					LoggingLevelThreadFunction,       // thread function name
					pData,          // argument to thread function 
					CREATE_SUSPENDED,                      // do not want it to start until we are finished setting up
					&dwThreadId);   // returns the thread identifier 

				pData->dwThreadId = dwThreadId;
				pData->hThread = hThread;
				pData->bThreadExited = false;

				// Check the return value for success.
				// If CreateThread fails, terminate execution. 
				// This will automatically clean up threads and memory. 
				if (hThread == NULL) 
				{
					free(pData);
					pData = NULL;
				}
				else
				{
					bResult = true;
					pData->hThreadTerminateSignal = CreateEvent(NULL, FALSE, FALSE, NULL); 
					pmd = pData;
					DWORD dwResult = ResumeThread(hThread);
					// wait for thread to startup
					int nCount = 0;
					while (!pData->bThreadRunning)
					{
						if (pData->bThreadExited)
						{
							bResult = false;
							break;
						}
						else
						{
							Sleep(100);
							nCount++;
						}
#ifndef _DEBUG
						if (nCount>100)
						{
							// well that did not work
							BreakdownLoggingLevelMonitoring();
							bResult = false;
							break;
						}
#endif
					}
				}
			} // End of main thread creation loop.
		}
		pmd = pData;
#ifndef _DEBUG
		}
		catch(...)
		{
		}
#endif
		return bResult;
		// Wait until all threads have terminated.
	}

bool FileUtil::BreakdownLoggingLevelMonitoring()
{
	bool bResult = true;
#ifndef _DEBUG
	try
	{
#endif
		SetEvent(pmd->hThreadTerminateSignal);
		// now wait a bit for the thread to terminate
		int count=0;
		while (pmd->bThreadRunning)
		{
			Sleep(100);
			count++;
			if (count > 100)
			{
				// must be stuck
				TerminateThread(
					pmd->hThread,
					  1);
				break;
			}
		}

		CloseHandle(pmd->hThread);
		CloseHandle(pmd->hThreadTerminateSignal);
		if(pmd != NULL)
		{
			free(pmd->tsRegistryName);
			free(pmd->tsDWORDName);
			HeapFree(GetProcessHeap(), 0, pmd);
			pmd = NULL;    // Ensure address is not reused.
		}
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
    return bResult;
}

DWORD WINAPI LoggingLevelThreadFunction( LPVOID lpParam ) 
{ 
#ifndef _DEBUG
	try
	{
#endif
		psLLTData pData;
		pData = (psLLTData)lpParam;

#ifdef _DEBUG  // this is the one for debugging service
	bool bStall=false;
	while (bStall)
	{
		Sleep(1000);
	}
#endif

		// ok - we are now setup and running as a thread 
		// we have to setup a semaphore - via the regutil function
		// and then wait until the logging level changes or until we are terminated
		// we store the current logging level in our internal data structure
		// where other loggin processes can see it
		RegUtil * ru = new RegUtil();
		HANDLE hHandles[2];
		hHandles[0] = pData->hThreadTerminateSignal;
		hHandles[1]=CreateEvent(NULL, FALSE, FALSE, NULL); 

		HKEY hKey = ru->startMonitoring(pData->tsRegistryName, pData->tsDWORDName, hHandles[1]);
		bool bDone = false;
		if (hHandles[1] == NULL)
		{
			// failed
			bDone=true;
		}
		else
		{
			SetEvent(hHandles[1]); // we create it signaled to get the registry value on startup

		}


		// now wait until someone signals a change or we are terminated
		while (!bDone)
		{
			DWORD dwResult = WaitForMultipleObjects(2, hHandles, FALSE, INFINITE);
			// now determine why wait ended
			switch (dwResult)
			{
				case WAIT_FAILED:
					{
						// well - that is the end of this 
						pData->dwLastError = GetLastError();
						bDone = true;
					}
					break;
				case WAIT_OBJECT_0:
					{
						// thread signaled - we should end
						bDone = true;
					}
					break;
				case WAIT_OBJECT_0+1:
					{
						// semaphore signaled - we must have had a registry key change of some kind
						{
							DWORD dwOld = pData->dwLoggingLevel;
							// get the current value
							DWORD dwSize = sizeof(DWORD);
							LONG lResult = RegGetValue(HKEY_LOCAL_MACHINE, pData->tsRegistryName, pData->tsDWORDName,
								RRF_RT_REG_DWORD, NULL, &pData->dwLoggingLevel, &dwSize);
							if (lResult != ERROR_SUCCESS)
							{
								pData->dwLoggingLevel = 0xFFFFFFFF; // indicate unknown value
								pData->dwLastError = GetLastError();
							}
							ru->stopMonitoring(hKey);
							// decide if we need to log this
							if (dwOld!=pData->dwLoggingLevel)
							{
								if (dwOld != 0xFFFFFFFF && (pData->dwLoggingLevel == 0x00000000 || pData->dwLoggingLevel == 0xFFFFFFFF))
								{
									// debugging was on and is now off
									FileUtil * fu = new FileUtil();
									fu->LogToWindowsEventUsingLoggingLevelsCategoryEventIDAndFormatting(NULL, pData->tsDWORDName, 0, 
										EVENTLOG_INFORMATION_TYPE, DEBUGGING_STATE_MESSAGES, DEBUGGING_STATE_CHANGE,
										_T("Debug logging is INACTIVE for %s\n"), pData->tsDWORDName);
									delete fu;
								}
								else
								{
									if (pData->dwLoggingLevel != 0x00000000 && pData->dwLoggingLevel != 0xFFFFFFFF)
									{
										// we have a debugging logging level activated - non optionally log this info
										FileUtil * fu = new FileUtil();
										fu->LogToWindowsEventUsingLoggingLevelsCategoryEventIDAndFormatting(NULL, pData->tsDWORDName, 0, 
											EVENTLOG_INFORMATION_TYPE, DEBUGGING_STATE_MESSAGES, DEBUGGING_STATE_CHANGE, 
											_T("Debug logging is ACTIVE and level is set to 0x%08X for %s\n"), 
											pData->dwLoggingLevel, pData->tsDWORDName);
										delete fu;
									}
								}
							}
							hKey = ru->startMonitoring(pData->tsRegistryName, pData->tsDWORDName, hHandles[1]);
							pData->bThreadRunning = true;
						}
					}
					break;
				case WAIT_ABANDONED_0:
					{
						// thread signaled - we should end
						bDone = true;
					}
					break;
				case WAIT_ABANDONED_0+1:
					{
						// sempahore signaled - we should end
						bDone = true;
					}
					break;
			}
		}

		ru->stopMonitoring(hKey);
		delete ru;
		pData->bThreadRunning = false;
		pData->bThreadExited = true;
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
    return 0; 
} 
#endif
void FileUtil::LogToWindowsEventUsingLoggingLevelsCategoryEventIDAndFormatting(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, WORD wCategory, DWORD dwEventID, TCHAR *csMsg, ... )
{
#ifndef _DEBUG
	try
	{
#endif
		TCHAR csTemp[1023] = {0};
		va_list vaList;

		va_start(vaList, csMsg);
		StringCbVPrintf(csTemp, _countof(csTemp), csMsg, vaList);
		va_end(vaList);

		LPCTSTR lpc = (LPCTSTR) csTemp;

		LogToWindowsEventUsingLoggingLevels(tsRegistryName, tsDWORDName, dwMask, wType, &lpc, 1, wCategory, dwEventID);
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif

}

void FileUtil::LogToWindowsEventUsingLoggingLevelsWithFormatting(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, TCHAR *csMsg, ... )
{
#ifndef _DEBUG
	try
	{
#endif
		TCHAR csTemp[1023] = {0};
		va_list vaList;

		va_start(vaList, csMsg);
		StringCbVPrintf(csTemp, _countof(csTemp), csMsg, vaList);
		va_end(vaList);

		LPCTSTR lpc = (LPCTSTR) csTemp;

		LogToWindowsEventUsingLoggingLevels(tsRegistryName, tsDWORDName, dwMask, wType, &lpc);
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
}
	
void FileUtil::LogToWindowsEventUsingLoggingLevels(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, const char *lpStrings)
{
#ifndef _DEBUG
	try
	{
#endif
		LPCTSTR lpc = (LPCTSTR) lpStrings;
		LogToWindowsEventUsingLoggingLevels(tsRegistryName, tsDWORDName, dwMask, wType, &lpc);
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
}

void FileUtil::LogToWindowsEventUsingLoggingLevels(TCHAR * tsRegistryName, TCHAR * tsDWORDName, DWORD dwMask, WORD wType, LPCTSTR *lpStrings, WORD wNumStrings, WORD wCategory, DWORD dwEventID, PSID lpUserSid, LPVOID lpRawData, DWORD dwDataSize)
{
#ifndef _DEBUG
	try
	{
#endif
#ifdef _USE_WAUF_PROXY_EVENT_LOG_MESSAGES
		// we want to use the logging levels value in the registry to determine what we should log and when we should just return
		if (tsRegistryName!=NULL && this !=NULL && pmd == NULL)
		{
			bool bResult=SetupLoggingLevelMonitoring(tsRegistryName, tsDWORDName);
		}
#endif
		if (lpUserSid == NULL && this !=NULL && pTokenUser != NULL)
		{
			lpUserSid = pTokenUser->User.Sid;
		}

		if (this == NULL || tsRegistryName==NULL || dwMask==0 || (pmd!=NULL && pmd->dwLoggingLevel != 0xFFFFFFFF && (pmd->dwLoggingLevel&dwMask)!=0))
		{
			// Get a handle to use with ReportEvent().
			HANDLE hEventSource = NULL;
			hEventSource = RegisterEventSource(NULL, tsDWORDName);
			if (hEventSource != NULL)
			{
				// Write to event log.
				ReportEvent(
					hEventSource,						//event source
					wType,								//event type
					wCategory,							//category - none
					dwEventID,							//event id
					lpUserSid,							//used Sid
					wNumStrings,						//num strings
					dwDataSize,							//data size
					lpStrings,							//strings
					lpRawData);							//raw data
				DeregisterEventSource(hEventSource);
			}
		}
#ifndef _DEBUG
	}
	catch(...)
	{
	}
#endif
}

void FileUtil::LogToWindowsEvent(WORD wType, const wchar_t *wcsMsg, ... )
{
	wchar_t wcsTemp[1023] = {0};
	HANDLE hEventSource;
	LPWSTR lpwszStrings[1];
	wstring wsMsgToLog;
	va_list vaList;

	/*if(wsMsgToLog.length() > _countof(wcsTemp))
	{
		wsMsgToLog = wsMsgToLog.substr(0, _countof(wcsTemp) - 10);
		wsMsgToLog += L"...";
	}*/

	va_start(vaList, wcsMsg);
	StringCbVPrintfW(wcsTemp, _countof(wcsTemp), wcsMsg, vaList);
	va_end(vaList);
	
	lpwszStrings[0] = wcsTemp;

	// Get a handle to use with ReportEvent().
	hEventSource = RegisterEventSource(NULL, TEXT("UFAgentMon"));
	if (hEventSource != NULL)
	{
		// Write to event log.
		ReportEvent(
			hEventSource,						//event source
			wType,								//event type
			STATUS,								//category - none
			GENERAL,							//event id
			NULL,								//used Sid
			1,									//num strings
			0,									//data size
			(LPCTSTR*) &lpwszStrings[0],		//strings
			NULL);								//raw data
		DeregisterEventSource(hEventSource);
	}

	return;
}
