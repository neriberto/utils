#include "StorageUtils.h"

#include <Windows.h>
#include <algorithm> // transform

bool CStorageUtils::GetAllFixedDrives(tstring & tsDrives,/* Out, receives all fixed drives letters */
									  TCHAR delimiter /* = TEXT(' ') In, The delimiter that should be used in the returned string to separate between drives' letters */)
{
	DWORD dwSize = 0;
	TCHAR *pDriveIter;
	TCHAR *lpBuffer = NULL;
	bool ret = false;

	tsDrives = TEXT("");

	// Get length that we need to allocate (not including the terminating null character)
	dwSize = GetLogicalDriveStrings(0, lpBuffer);

	if(dwSize <= 0)
		return ret;

	lpBuffer = new TCHAR[dwSize];
	if(lpBuffer)
	{
		tstring tsTemp;

		GetLogicalDriveStrings(dwSize, lpBuffer);

		tsTemp = pDriveIter = lpBuffer;
		while (!tsTemp.empty())
		{
			if(GetDriveType(pDriveIter)==DRIVE_FIXED)
			{
				tsDrives += pDriveIter;
				tsDrives += delimiter;
			}
			pDriveIter += tsTemp.length() + 1;
			tsTemp = pDriveIter;
		}

		// Remove trailing delimiter
		if( (tsDrives.size() > 0) && tsDrives[tsDrives.size()-1]==delimiter )
			tsDrives.erase(tsDrives.size() - 1);
		delete [] lpBuffer;

		// Capitalize
		transform(tsDrives.begin(), tsDrives.end(), tsDrives.begin(), toupper);

		ret = true;
	}

	return ret;
}

bool CStorageUtils::GetAllDrives(int mask, std::vector<tstring> & drives)
{
	bool ret = false;
	DWORD dwSize;
	TCHAR * lpBuffer = NULL;

	drives.clear();

	// Get length that we need to allocate (not including the terminating null character)
	dwSize = GetLogicalDriveStrings(0, lpBuffer);

	if(dwSize > 0)
	{
		lpBuffer = new TCHAR[dwSize];
		if(lpBuffer)
		{
			tstring tsDrive;
			TCHAR * pDriveIter;

			// A pointer to a buffer that receives a series of null-terminated strings, one for each valid drive 
			// in the system, plus with an additional null character. Each string is a device name.
			GetLogicalDriveStrings(dwSize, lpBuffer);

			tsDrive = pDriveIter = lpBuffer;
			while (!tsDrive.empty())
			{
				transform(tsDrive.begin(), tsDrive.end(), tsDrive.begin(), toupper);
				if(mask == DRIVE_TYPE__ANY || CStorageUtils::IsSelectedDriveType(tsDrive, mask))
					drives.push_back(tsDrive);

				pDriveIter += tsDrive.length() + 1;
				tsDrive = pDriveIter;
			}
			delete [] lpBuffer;
			ret = true;
		}
	}

	return ret;
}

bool CStorageUtils::IsSelectedDriveType(const tstring & drive, int mask)        
{
	int driveType;
	bool ret = false;

	driveType = GetDriveType(drive.c_str());

	switch (driveType)
	{
		case DRIVE_UNKNOWN : ret = ((mask & DRIVE_TYPE__TYPE__UNKNOWN) != 0); break;
		case DRIVE_NO_ROOT_DIR : ret = ((mask & DRIVE_TYPE__NO_ROOT_DIR) != 0); break;
		case DRIVE_REMOVABLE : ret = ((mask & DRIVE_TYPE__REMOVABLE) != 0); break; 
		case DRIVE_FIXED : ret = ((mask & DRIVE_TYPE__FIXED) != 0); break; 
		case DRIVE_REMOTE : ret = ((mask & DRIVE_TYPE__REMOTE) != 0); break; 
		case DRIVE_CDROM : ret = ((mask & DRIVE_TYPE__CDROM) != 0); break; 
		case DRIVE_RAMDISK : ret = ((mask & DRIVE_TYPE__RAMDISK) != 0); break; 
	}

	return ret;
}
