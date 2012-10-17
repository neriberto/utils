#pragma once

#include <string>
#include <sstream>

class CWinServiceUtil
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
	typedef std::wstringstream tstringstream;
#else
	typedef std::string tstring;
	typedef std::stringstream tstringstream;
#endif

	static int Stop(tstring tsServiceName);
	static int Start(tstring tsServiceName);

private:
	CWinServiceUtil(void);
	~CWinServiceUtil(void);
};
