#pragma once

#include <Windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>

class StringUtil
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
	typedef std::wstringstream tstringstream;
#else
	typedef std::string tstring;
	typedef std::stringstream tstringstream;
#endif

	static int StrToInt(tstring str);
#ifndef UNICODE
	static int StrToIntW(std::wstring str);
#endif
	static std::wstring	StrToWstr(const std::string &orig);
	static std::string	WStrToStr(const std::wstring &orig, UINT codePage = CP_ACP);
	static void		EnsurePathEnding(tstring &path);
	static void		EnsurePathEndingW(std::wstring & path);
	static int GetEnvironmentVariable(const TCHAR *csVarName, tstring &sValue);
	static std::wstring ParseSubstringFromString(const std::wstring &in, const std::wstring &seek, const std::wstring &start, const std::wstring &end);
	static std::vector<tstring> StringToVector(const tstring & inputString, const TCHAR * separator);
	static tstring Replace(tstring tsInput, tstring tsReplace, tstring tsNew);
	static void HexStrToByteArray(tstring tsHexString, unsigned char *baResult, size_t uiArraySize);
	static tstring IntToStr(int intToConvert);
	static std::wstring Trim(std::wstring wstr);

private:
	StringUtil(void);
	~StringUtil(void);

	static bool util_strsafe_copy_tcs(char ** pcsDest, size_t uiDestLength, const TCHAR *tcsSrc);
};
