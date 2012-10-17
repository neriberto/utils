#ifndef WADEFS_H
#define WADEFS_H

#include <string>

namespace WAUtils
{

#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

}

#endif // WADEFS_H