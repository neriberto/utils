#pragma once

#include <Windows.h>
#include <string>

using namespace std;

class CEseUtil
{
public:
	CEseUtil(void);
	~CEseUtil(void);

	/* Description: 
	*/
	static wstring			GetInstallDirWithPathEnding();
	static BOOL				Is64BitOS();

private:

};
