/** \file CStorageUtils.h
\brief Defines utilities for handling storage. */
#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <tchar.h>

class CStorageUtils
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

	/// Drive Type Masks
#define DRIVE_TYPE__ANY				-1  
#define DRIVE_TYPE__TYPE__UNKNOWN	0x01
#define DRIVE_TYPE__NO_ROOT_DIR		0x02
#define DRIVE_TYPE__REMOVABLE		0x04
#define DRIVE_TYPE__FIXED			0x08
#define DRIVE_TYPE__REMOTE			0x10
#define DRIVE_TYPE__CDROM			0x20
#define DRIVE_TYPE__RAMDISK			0x40

	static bool GetAllFixedDrives( tstring & sDrives, // Out, receives all fixed drive letters
		TCHAR delimiter = TEXT(' ') // In, The delimiter that should be used in the returned string to separate between drives' letters
		);
	static bool GetAllDrives(int mask, std::vector<tstring> & drives);
	///Does the given drive match the given driveTypeMasks mask
	static bool IsSelectedDriveType(const tstring & drive, int mask);
};

