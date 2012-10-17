#pragma once

#include "WADefs.h"

#include <Windows.h>
#include <tchar.h>
#include <string>

namespace WAUtils {

class CRegistryHash
{
public:
    CRegistryHash(tstring sRegPath, HKEY hk = HKEY_LOCAL_MACHINE);
    ~CRegistryHash(void);

private:
    // Hold the path of the registry which is can be accessed through hash interface
    tstring sRegPath;
	HKEY hkRootKey;
    // Check if the set registry entry exists
    void InitRegKey(void);

public:
    // get the specified value out of the registry
    void get(tstring name, BYTE *& buff_out, size_t & buff_allocated, BOOL bNameEncrypted = FALSE);
	BOOL erase(tstring name, BOOL bNameEncrypted = FALSE);
	tstring get(tstring name, BOOL bNameEncrypted = FALSE);

    // set the specified name value pair into the registry
    void set(tstring name, const TCHAR *svalue, const size_t buffsize, BOOL bEncryptName = FALSE);
	void set(tstring name, const TCHAR *svalue, BOOL bEncryptName = FALSE);
};

} //namespace WAUtils