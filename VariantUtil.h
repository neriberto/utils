/*! \file VariantUtil.h
*
*/

#ifndef __VARIANT_UTIL__
#define __VARIANT_UTIL__

#include <atlbase.h>
#include <atlstr.h>
#include <atlcom.h>
#include <string>
#include <vector>

class VariantUtil
{
public:
	VariantUtil(void);
	~VariantUtil(void);

	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_BSTR will be used. 
	* \ingroup VariantUtil
	* @param [in, out] variant string to be copied to
	* @param [in] cwstr string to be copied from
	* @return false if variant is null or failed to copy the string
	*/ 
	static bool SetVariantValue( VARIANT * variant, const wchar_t * cwstr );
	
	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_BSTR will be used. 
	* \ingroup VariantUtil
	* @param [in, out] variant string to be copied to
	* @param [in] cwstr string to be copied from
	* @return false if variant is null or failed to copy
	*/ 
	static bool SetVariantValue( VARIANT * variant, const std::wstring &wstr );
	
	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_BSTR will be used. 
	* \ingroup VariantUtil
	* @param [in, out] variant Variant time to be copied to
	* @param [in] date double value 
	* @return false if variant is null or failed to copy
	*/ 
	static bool SetVariantValue( VARIANT *variant, const DATE & date );
	
	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_UI4 will be used. 
	* \ingroup VariantUtil
	* @param [in, out] variant string to be copied to
	* @param [in] ulVal string to be copied from
	* @return false if variant is null or failed to copy
	*/ 
	static bool SetVariantValueUI( VARIANT *variant, const ULONG ulVal);
	
	/**  
	* Read string from variant and copy to wstring
	* 
	* \ingroup VariantUtil
	* @param [in] variant string to be copied from
	* @param [in, out] str string to be copied to
	* @return false if failed to read
	*/ 
	static bool GetVariantValue( const VARIANT &variant, std::wstring &str );
	
	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_BSTR will be set. 
	* \ingroup VariantUtil
	* @param [in] variant string to be copied from
	* @param [in, out] cwstr string to be copied to
	* @return false if failed to read
	*/ 
	static bool GetVariantValue( VARIANT *variant, std::wstring &str );
	
	/**  
	* Set variant with given string
	* 
	* If variant type is not specified, VT_BSTR will be set. 
	* \ingroup VariantUtil
	* @param [in] variant value to be copied from
	* @param [in, out] cwstr int value to be copied to
	* @return false if variant is null or failed to copy the value
	*/ 
	static bool GetVariantValueUI( const VARIANT &variant, int &uiVal );
	
	/**  
	* Copy the elements of SafeArray to VARIANT array with given limit
	* 
	* \ingroup VariantUtil
	* @param [in] safeArray source safearray
	* @param [in, out] vArray target array of VRAIANT Type
	* @param [in] maxVArraySize the number of limit of elements to copied
	* @param [in, out] vArraySize the number of elements which is copied
	* @return false if ...
	*/ 
	static bool SafeArrayToVArray(VARIANT * safeArray, VARIANT * vArray, const size_t maxVArraySize, size_t & vArraySize);
	
	/**  
	* Return the size of 1 dimension SafeArray
	* 
	* \ingroup VariantUtil
	* @param [in] safeArray the safearray whose elements needs to be counted.
	* @return the size of the given safearray
	*/ 
	static int GetSizeOfSafeArray(VARIANT * safeArray);

	/**  
	* Create a safearray with copying the elements of given vector
	* 
	* \ingroup VariantUtil
	* @param [in] vecSrc vector which holds the list of strings
	* @param [in, out] pvSafeArray safearray where the strings will be copied to.
	*/ 
	static void CreateSafeArrayFromVec(std::vector<std::wstring> & vecSrc, VARIANT * pvSafeArray);
};

#endif //__VARIANT_UTIL__