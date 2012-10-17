#include "RegistryHash.h"
#include "StringUtil.h"
#include "Base64.h"
#include "blowfish.h"

// Standard includes
#include <Windows.h>
#include <strsafe.h>

namespace WAUtils {

CRegistryHash::CRegistryHash(tstring sRegPathIn, HKEY hk /* = HKEY_LOCAL_MACHINE */)
{
    this->sRegPath = sRegPathIn;
	this->hkRootKey = hk;

    //Create the reg key path if it doesn't exist
    InitRegKey();
}


CRegistryHash::~CRegistryHash(void)
{
}

//Return blowfish encrypted buffer of wstring
void getBlowfishEncryptedBuffer(const string& sinput, unsigned char *& buff_encrypted, size_t& buff_size)
{
	string stempCombined = sinput;

	//Return length of currently allocated storage
	buff_size = stempCombined.capacity();

	//make sure maxlen is a multiple of 8 for blowfish
	buff_size = buff_size + (8 - (buff_size % 8));

	//allocate buffer for blowfish
	buff_encrypted = new unsigned char[buff_size];

	//zero-out allocated memory
	memset(buff_encrypted, 0, buff_size);

	//Copy string to buffer
	StringCchCopyA((char *)buff_encrypted, strlen(stempCombined.c_str())+1, stempCombined.c_str());

	//Encrypt buffer
	CBlowFish oBlowFish((unsigned char *)"secret_key", 10);
	oBlowFish.Encrypt(buff_encrypted, buff_size);
}

tstring EncryptAndEncode(const tstring& tsOrig)
{
	//Blowfish Encrypt
	unsigned char *encrypted_buff = NULL;
	size_t        buff_size       = 0;
	string sOrig, sTemp;
	tstring tsResult;

#ifdef UNICODE
	sOrig = StringUtil::WStrToStr(tsOrig);
#else
	sOrig = tsOrig;
#endif

	getBlowfishEncryptedBuffer(sOrig, encrypted_buff, buff_size);

	//Base64 Encode
	sTemp = Base64::Base64Encode(encrypted_buff, (unsigned int)buff_size);

	// Get appropriate string
#ifdef UNICODE
	tsResult = StringUtil::StrToWstr(sTemp);
#else
	tsResult = sTemp;
#endif

	delete [] encrypted_buff;

	return tsResult;
}


tstring DecryptAndDecode(const tstring& orig)
{
	string sInput;

	// Get appropriate string
#ifdef UNICODE
	sInput = StringUtil::WStrToStr(orig);
#else
	sInput = orig;
#endif

	//Base64 Decode
	unsigned char *buff     = new unsigned char[sInput.length() + 1];
	size_t        buff_size = 0;

	buff_size = Base64::Base64Decode(sInput.c_str(), buff);

	//Blowfish Decrypt
	CBlowFish bfish((unsigned char *)"secret_key", 10);
	bfish.Decrypt(buff, buff_size);

	//char* to String to WString
	string  temp((char *)buff);
	tstring tsOutput;
	
	// Set appropriate string
#ifdef UNICODE
	tsOutput = StringUtil::StrToWstr(temp);
#else
	tsOutput = temp;
#endif

	delete [] buff;

	return tsOutput;
}

// Create the registry key if it does not exist
void CRegistryHash::InitRegKey(void)
{
    HKEY    hKey            = NULL;
    LPDWORD lpdwDisposition = NULL;
    LONG    ret             = NULL;

    ret = RegCreateKeyEx(
        this->hkRootKey,
        this->sRegPath.c_str(),
        NULL,     //Reserved
        NULL,     //class
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,     //handle cannot be inherited
        &hKey,
        lpdwDisposition
        );

	RegCloseKey(hKey);
}


// get the specified value out of the registry, return true if memory was allocated
void CRegistryHash::get(tstring name, BYTE *& buff_out, size_t& bytes_allocated, BOOL bNameEncrypted /* = FALSE */)
{
    bytes_allocated = 0;

    HKEY hKey = NULL;

    //Open key
    LONG ret = RegOpenKeyEx(this->hkRootKey,
                            this->sRegPath.c_str(),
                            NULL,
                            KEY_ALL_ACCESS,
                            &hKey);

    DWORD bytes_required = 0;
    DWORD data_type      = 0;

    if (ret == ERROR_SUCCESS)
    {
        //find out how many bytes need to be allocated
        ret = RegQueryValueEx(
            hKey,
            EncryptAndEncode(name).c_str(),
            NULL,
            &data_type,
            NULL,
            &bytes_required);

        if (ret == ERROR_SUCCESS)
        {
            //allocate memory, at this point we have to return true
            buff_out        = new BYTE[bytes_required];
            bytes_allocated = bytes_required;

            ret = RegQueryValueEx(
                hKey,
                EncryptAndEncode(name).c_str(),
                NULL,
                &data_type,
                buff_out,
                &bytes_required
                );
        }

		RegCloseKey(hKey);
    }
}


// set the specified name value pair into the registry
void CRegistryHash::set(tstring name, const TCHAR *svalue, const size_t numbytes, BOOL bEncryptName /* = FALSE */)
{
    HKEY hKey = NULL;

    LONG ret = RegOpenKeyEx(this->hkRootKey,
                            this->sRegPath.c_str(),
                            NULL,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (ret == ERROR_SUCCESS)
    {
		tstring tsNameTemp = (bEncryptName? EncryptAndEncode(name) : name);
        ret = RegSetValueEx(hKey, tsNameTemp.c_str(), NULL, REG_BINARY, (CONST BYTE*)svalue, (DWORD) numbytes);
        if (ret == ERROR_SUCCESS)
        {
			RegCloseKey(hKey);
            return;
        }
		RegCloseKey(hKey);
    }

    throw exception("Could not create registry entry");
}


// set the specified name value pair into the registry
void CRegistryHash::set(tstring name, const TCHAR *svalue, BOOL bEncryptName /* = FALSE */)
{
    HKEY hKey = NULL;

    LONG ret = RegOpenKeyEx(this->hkRootKey,
                            this->sRegPath.c_str(),
                            NULL,
                            KEY_ALL_ACCESS,
                            &hKey);

    if (ret == ERROR_SUCCESS)
    {
		tstring tsNameTemp = (bEncryptName? EncryptAndEncode(name) : name);
        tstring svaluetemp = EncryptAndEncode(svalue);
        ret = RegSetValueEx(hKey, tsNameTemp.c_str(), NULL, REG_SZ, (CONST BYTE *)svaluetemp.c_str(), (DWORD)(svaluetemp.length() + 1) * sizeof(TCHAR));
        if (ret == ERROR_SUCCESS)
        {
			RegCloseKey(hKey);
            return;
        }
		RegCloseKey(hKey);
    }
    throw exception("Could not create registry entry");
}


tstring CRegistryHash::get(tstring name, BOOL bNameEncrypted /* = FALSE */)
{
    tstring svalue = TEXT("");
    HKEY    hKey = NULL;

    //Open key
    LONG ret = RegOpenKeyEx(this->hkRootKey,
                            this->sRegPath.c_str(),
                            NULL,
                            KEY_ALL_ACCESS,
                            &hKey);

    DWORD bytes_required = 0;
    DWORD data_type      = 0;

    if (ret == ERROR_SUCCESS)
    {
        tstring tsNameTemp = (bNameEncrypted? EncryptAndEncode(name) : name);
        //find out how many bytes need to be allocated
        ret = RegQueryValueEx(
            hKey,
            tsNameTemp.c_str(),
            NULL,
            &data_type,
            NULL,
            &bytes_required);

        if (ret == ERROR_SUCCESS)
        {
            //allocate memory, at this point we have to return true
            unsigned char *buff_out = new unsigned char [bytes_required];

            ret = RegQueryValueEx(
                hKey,
                tsNameTemp.c_str(),
                NULL,
                &data_type,
                buff_out,
                &bytes_required
                );

            svalue = (TCHAR *)buff_out;
            svalue = DecryptAndDecode(svalue);

            delete [] buff_out;
        }

		RegCloseKey(hKey);
    }

    return svalue;
}

BOOL CRegistryHash::erase(tstring sName, 
						  BOOL bNameEncrypted // = FALSE
						  )
{
	HKEY    hKey = NULL;
	BOOL rc = FALSE;

	// Open key
	LONG ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		this->sRegPath.c_str(),
		NULL,
		KEY_ALL_ACCESS,
		&hKey);

	if (ret == ERROR_SUCCESS)
	{
		// Encrypt the field if necessary
		tstring sAttrName = (bNameEncrypted? EncryptAndEncode(sName) : sName);

		ret = RegDeleteValue(
			hKey,
			sAttrName.c_str()
			);

		if(ret==ERROR_SUCCESS)
			rc = TRUE;

		RegCloseKey(hKey);
	}

	return rc;
}
} //namespace WAUtils