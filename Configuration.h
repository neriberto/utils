#pragma once

#include <Windows.h>
#include <string>

using namespace std;

class Configuration
{
public:
	Configuration(void);
	~Configuration(void);

	/* Description: 
	*/
	static wstring			GetInstallDirWithPathEnding();
	static wstring			GetESEDirWithPathEnding();
	static BOOL				Is64BitOS();

private:

};
