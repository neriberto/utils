#include "VariantUtil.h"

using namespace std;
VariantUtil::VariantUtil(void)
{
}

VariantUtil::~VariantUtil(void)
{
}

//
// VARIANT related
//

bool VariantUtil::SetVariantValue( VARIANT * variant, const wchar_t * cwstr )
{
	if (variant == NULL)
		return false;

	CComBSTR tempBstr(cwstr);

	if (variant->vt == VT_BSTR || variant->vt == VT_EMPTY) 
	{	
		variant->vt = VT_BSTR;
		if(FAILED(tempBstr.CopyTo(&(variant->bstrVal)))){
			return false;
		};
	} 
	else if (variant->vt == (VT_BSTR | VT_BYREF) ) 
	{
		if(FAILED(tempBstr.CopyTo(variant->pbstrVal))){
			return false;
		};
	} 
	else 
	{
		variant->vt = VT_BSTR;
		if(FAILED(tempBstr.CopyTo(&(variant->bstrVal)))){
			return false;
		};
	}

	return true;
}


//
// Used in : DBUtils
//
bool VariantUtil::SetVariantValue( VARIANT * variant, const wstring &wstr )
{
	if (variant == NULL)
		return false;

	CComBSTR tempBstr(wstr.c_str());

	if (variant->vt == VT_BSTR || variant->vt == VT_EMPTY) {	//if type is not set use bstr as well
		variant->vt = VT_BSTR;
		if(FAILED(tempBstr.CopyTo(&(variant->bstrVal)))){
			return false;
		};
	} else if (variant->vt == (VT_BSTR | VT_BYREF) ) {
		if(FAILED(tempBstr.CopyTo(variant->pbstrVal))){
			return false;
		};
	} else {
		variant->vt = VT_BSTR;
		if(FAILED(tempBstr.CopyTo(&(variant->bstrVal)))){
			return false;
		};
	}

	return true;
}

bool VariantUtil::SetVariantValue( VARIANT *variant, const DATE & date )
{
	if (variant == NULL)
		return false;

	if (variant->vt == VT_DATE || variant->vt == VT_EMPTY) {	//if type is not set use date as well
		variant->vt = VT_DATE;
		variant->date = date;
	} else if (variant->vt == (VT_DATE | VT_BYREF) ) {
		*(variant->pdate) = date;
	} else {
		return false;
	}

	return true;
}

bool VariantUtil::SetVariantValueUI( VARIANT *variant, const ULONG ulVal)
{
	if (variant == NULL)
		return false;

	if (variant->vt == VT_UI4 || variant->vt == VT_EMPTY) {	//if type is not set use date as well
		variant->vt = VT_UI4;
		variant->ulVal = ulVal;
	} else if (variant->vt == (VT_UI4 | VT_BYREF) ) {
		*(variant->pulVal) = ulVal;
	} else {
		return false;
	}

	return true;
}

bool VariantUtil::GetVariantValue( const VARIANT &variant, wstring &str )
{
	str = L"";

	if (variant.vt == VT_BSTR) {
		if(variant.bstrVal == NULL) return false;

		str = variant.bstrVal;
	} else if (variant.vt == (VT_BSTR | VT_BYREF) ) {
		if(variant.pbstrVal == NULL) return false;

		str = *(variant.pbstrVal);
	} else {
		return false;
	}

	return true;
}

bool VariantUtil::GetVariantValue( VARIANT * variant, wstring &str )
{
	str = L"";

	if (variant->vt == VT_BSTR) {
		if(variant->bstrVal == NULL) return false;

		str = variant->bstrVal;
	} else if (variant->vt == (VT_BSTR | VT_BYREF) ) {
		if(variant->pbstrVal == NULL) return false;

		str = *(variant->pbstrVal);
	} else {
		return false;
	}

	return true;
}

// bool VariantUtil::GetVariantValue( const VARIANT &variant, _bstr_t &val )
// {
// 	val = "";
// 	if (variant.vt == VT_BSTR) {
// 		if(variant.bstrVal == NULL) return false;
// 
// 		val = variant.bstrVal;
// 	} else if (variant.vt == (VT_BSTR|VT_BYREF)) {
// 		if(variant.pbstrVal == NULL) return false;
// 
// 		val = *variant.pbstrVal;
// 	} else {
// 		//unknown parameter type
// 		return false;
// 	}
// 	return true;
// }

bool VariantUtil::GetVariantValueUI( const VARIANT &variant, int &uiVal )
{
	if (variant.vt == VT_UI4 ) {	//if type is not set use date as well
		uiVal = variant.uiVal;
	} else if (variant.vt == (VT_UI4 | VT_BYREF) ) {
		uiVal = *(variant.puiVal);
	} else {
		return false;
	}

	return true;
}

bool VariantUtil::SafeArrayToVArray(VARIANT * safeArray, VARIANT * vArray, const size_t maxVArraySize, size_t & vArraySize)
{
	long aIdx[1];
	SAFEARRAY FAR* psa = NULL;
	LONG lLBound, lUBound;
	if(safeArray->vt != (VT_ARRAY | VT_VARIANT))
		return false;

	psa = V_ARRAY(safeArray);
	if(!psa)
		return false;

	if(SafeArrayGetDim(psa) != 1){
		return false;
	}

	if(FAILED(SafeArrayGetLBound(psa, 1, &lLBound))){
		return false;
	}

	if(FAILED(SafeArrayGetUBound(psa, 1, &lUBound))){
		return false;
	}

	size_t ii = 0;
	vArraySize = 0;
	for(long pos = lLBound; pos <= lUBound && ii < maxVArraySize; ++pos, ++ii)
	{
		aIdx[0] = pos;
		if(FAILED(SafeArrayGetElement(psa, aIdx, &vArray[ii]))){
			return false;
		}
	}
	vArraySize = ii;

	return true;
}

void VariantUtil::CreateSafeArrayFromVec(vector<wstring> & vecSrc, VARIANT * pvSafeArray)
{
	SAFEARRAY *pSA;
	SAFEARRAYBOUND aDim[1];
	aDim[0].lLbound= 0;
	aDim[0].cElements = (ULONG)vecSrc.size();
	pSA= SafeArrayCreate(VT_VARIANT,1,aDim);
	if (!pSA){
		return;
	}

	long aLong[1], pos = 0;
	for(size_t ii = 0; ii < vecSrc.size(); ii++, pos++)
	{
		VARIANT vTmp;
		VariantInit(&vTmp);
		vTmp.vt= VT_BSTR;
		SetVariantValue(&vTmp, vecSrc[ii].c_str());
		aLong[0]= pos;
		if (FAILED(SafeArrayPutElement(pSA, aLong, &vTmp))) 
		{
			VariantClear(&vTmp);
			break;
		}
		VariantClear(&vTmp);
	}

	VariantInit(pvSafeArray);
	pvSafeArray->vt = VT_ARRAY | VT_VARIANT;
	pvSafeArray->parray = pSA;
}

int VariantUtil::GetSizeOfSafeArray(VARIANT * safeArray)
{
	SAFEARRAY FAR* psa = NULL;
	LONG lLBound, lUBound;
	if(safeArray->vt != (VT_ARRAY | VT_VARIANT))
		return 0;

	psa = V_ARRAY(safeArray);
	if(!psa)
		return 0;

	if(SafeArrayGetDim(psa) != 1){
		return 0;
	}

	if(FAILED(SafeArrayGetLBound(psa, 1, &lLBound))){
		return 0;
	}

	if(FAILED(SafeArrayGetUBound(psa, 1, &lUBound))){
		return 0;
	}
	
	return lUBound - lLBound + 1;
}