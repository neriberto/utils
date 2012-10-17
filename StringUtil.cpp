#include "StringUtil.h"
#include "ClientCodes.h"

// Standard includes
#include <strsafe.h>


using namespace std;

StringUtil::StringUtil(void)
{
}

StringUtil::~StringUtil(void)
{
}

/* Description: String-safe method for copying a TCHAR buffer to a char buffer.
* */
bool StringUtil::util_strsafe_copy_tcs(char ** pcsDest, size_t uiDestLength, const TCHAR *tcsSrc)
{
#ifdef UNICODE
	// The function returns 0 if it does not succeed.
	if(WideCharToMultiByte(CP_ACP, 0, tcsSrc, 
		-1, // ...this parameter can be set to -1 if the string is null-terminated.
		*pcsDest, (int)uiDestLength * sizeof(char), 0, 0)==0)
		return false;
#else
	if(FAILED(StringCbCopy(*pcsDest, uiDestLength * sizeof(char), (char*)tcsSrc)))
		return false;
#endif

	return true;
}
int StringUtil::StrToInt(tstring str)
{
#ifdef UNICODE
	return _wtol(str.c_str());
#else
	return atoi(str.c_str());
#endif
}
#ifndef UNICODE
int StringUtil::StrToIntW(wstring str)
{
	return _wtol(str.c_str());
}
#endif
wstring StringUtil::StrToWstr(const string &orig) 
{
	wstring ret;
	// Convert to a wchar_t*
	// You must first convert to a char * for this to work.
	size_t origsize = strlen(orig.c_str()) + 1;
	size_t convertedChars = 0;
	wchar_t * converted = new wchar_t[origsize];
	if(!converted)
		return L"";

	mbstowcs_s(&convertedChars, converted, origsize, orig.c_str(), _TRUNCATE);
	ret = (wstring)converted;
	delete [] converted ;

	return ret;
}

string StringUtil::WStrToStr(const wstring &orig, unsigned int codePage) {
	string ret;
	size_t origsize = orig.length() + 1;
	char* converted  = new char[origsize];
	if(!converted)
		return "";
	
	memset(converted, 0, origsize);
	WideCharToMultiByte(codePage,0,orig.c_str(),-1,converted ,(int)origsize,0,0);
	ret = (string)converted;
	delete [] converted;
	return ret;
}

int StringUtil::GetEnvironmentVariable(const TCHAR *csVarName, StringUtil::tstring &sValue)
{
	LPTSTR csValue = NULL;
	DWORD dwSize = 512;
	DWORD dwRet;
	int rc = G_FAIL;

	sValue = TEXT("");

	csValue = (LPTSTR)malloc(dwSize * sizeof(TCHAR));
	dwRet = ::GetEnvironmentVariable(csVarName, csValue, dwSize);

	if(dwRet==0)
	{
		if(GetLastError()==ERROR_ENVVAR_NOT_FOUND)
		{
			rc = G_INVALID_PARAM;			
		}
		goto end;
	}
	else if(dwSize < dwRet)
	{
		csValue = (LPTSTR)realloc(csValue, dwRet * sizeof(TCHAR));
		dwRet = ::GetEnvironmentVariable(csVarName, csValue, dwRet);
		sValue = csValue;		
	}
	else
		sValue = csValue;

	rc = G_OK;

end:
	if(csValue!=NULL)
		free(csValue);

	return rc;
}
void StringUtil::EnsurePathEnding(tstring & path)
{
	if (path.length() > 0 && path[path.length()-1]!=TEXT('\\'))
		path += TEXT('\\');
}
void StringUtil::EnsurePathEndingW(wstring & path)
{
	if (path.length() > 0 && path[path.length()-1]!=(L'\\'))
		path += (L'\\');
}
wstring StringUtil::ParseSubstringFromString(const wstring &in, const wstring &seek, const wstring &start, const wstring &end)
{
	wstring::size_type posStart = 0;
	wstring::size_type posEnd = wstring::npos;
	if( !seek.empty() )
	{
		posStart = in.find(seek);
		if( posStart == wstring::npos )
		{
			return L"";
		}
		posStart += seek.size();
	}
	if( !start.empty() )
	{
		posStart = in.find(start, posStart);
		if( posStart == wstring::npos )
		{
			return L"";
		}
		posStart = posStart + start.size();
	}
	if( !end.empty() )
	{
		posEnd = in.find(end, posStart);
	}
	//Out of bounds issues arise if we are not careful with the npos case
	if( posEnd == wstring::npos )
	{
		return in.substr(posStart);
	}
	else
	{
		return in.substr(posStart, posEnd - posStart);
	}
}
vector<StringUtil::tstring> StringUtil::StringToVector(const StringUtil::tstring & str, const TCHAR * separator)
{
	tstring::size_type index1 = 0,index2 = 0;
	vector<tstring> ret;

	if( !(tstring(separator)).empty() ) 
	{
		index1 = str.find_first_not_of(separator, 0);
		while (index1 != -1) 
		{
			index2 = str.find_first_of(separator, index1);
			if (index2 == -1) 
			{
				index2 = str.length();
			}
			ret.push_back( str.substr(index1, index2 - index1));
			index1 = str.find_first_not_of(separator, index2);
		}
	}
	return ret;
}

StringUtil::tstring StringUtil::Replace(tstring tsInput, tstring tsReplace, tstring tsReplaceWith)
{
	size_t pos;
	tstring tsNew = tsInput;

	// Avoid infinite loop
	if(tsReplace==tsReplaceWith)
		return tsNew;

	while( (pos=tsNew.find(tsReplace.c_str()))!=tstring::npos )
		tsNew.replace(pos, tsReplace.length(), tsReplaceWith.c_str());

	return tsNew;
}

void StringUtil::HexStrToByteArray(tstring tsHexString, unsigned char *baResult, size_t uiArraySize)
{
	if(tsHexString.empty())
		return; 

	char *csTmp = new char[tsHexString.length()+1];
	string sTmp;

	// Convert safe string to ascii string.
	util_strsafe_copy_tcs(&csTmp, tsHexString.length()+1, tsHexString.c_str());
	sTmp = csTmp;
	delete [] csTmp;
	
	// Make sure the hex string length is double the size of the byte array; Two hex letters for each byte "0A" -> 10
	unsigned int x;
	if(sTmp.length()==uiArraySize * 2)
	{
		// Convert GUID to 16-byte array
		for(size_t i=0; i<uiArraySize; i++)
		{
			stringstream ss;
			ss << hex << sTmp.substr(i*2, 2).c_str();
			ss >> x;
			baResult[i] = x;
		}
	}		
}
StringUtil::tstring StringUtil::IntToStr(int intToConvert)
{
	tstring sRet;
	tstringstream ss;
	ss << intToConvert;
	sRet = ss.str();

	return sRet;
}
wstring StringUtil::Trim(wstring wstr)
{
	if(wstr.length() == 0)
		return wstr;
	size_t start, end;
	start = wstr.find_first_not_of(L" \"\t\r\n");
	end = wstr.find_last_not_of(L" \"\t\r\n");
	if(start != wstr.npos && end != wstr.npos)
		return wstr.substr(start, end-start+1); 
	else
		return L"";
}