#pragma once

#include <Windows.h>
#include <string>
#include <map>

/* Description: A special class which monitors the license state of products that use the 'THORWA' licensing mechanism.
				Once monitoring is started, it will check the license state in specified intervals and check if the license state has changed.
				If the license state has changed from the last time it was run (or if it is the first time checking), it will invoke a
				user-specified callback that provides the new license state.
*/
class CLicenseEnforce
{
public:
#ifdef UNICODE
	typedef std::wstring tstring;
#else
	typedef std::string tstring;
#endif

	static CLicenseEnforce* Instance();
	static void DestroyInstance();

	enum LECode {
		LE_OK,
		LE_ERR_GENERIC,
		LE_ERR_LICENSEINVALID,				// License is invalid, License has been rejected
		LE_ERR_LICENSEUNKNOWN,				// License has never been checked, Unknown license state, It's not valid nor invalid
		LE_ERR_LICENSEOUTOFTOKENS,			// Implies license has been activated, but token state is unknown (possibly network issue, server is down)
		LE_ERR_LICENSETRIALMODE,					// License has not been entered, Trial mode
		LE_ERR_LICENSETRIALEXPIRED,			// Trial has expired; Only if a trial period is enforced
		LE_ERR_LICENSEEXPIRED,				// License has expired or has been revoked
		LE_ERR_BADCOMPONENT,
		LE_ERR_NETWORK,
		LE_ERR_INVALIDARGUMENT
	};
	enum ComponentType {
		Ese,
		Svd,
		UrlFiltering,
		Md4sa
	};
	typedef void(*OnLicenseStatusChange)(LECode leLicenseState);

	/************************************************************************/
	// Public Member Functions
	/************************************************************************/
	/* Description: Registers a user-defined callback which will be invoked when a license status change occurs. */
	LECode SetEventHandler(ComponentType componentType, OnLicenseStatusChange onLicenseStatusChange);

	/*	Description: Enforces a trial period. As long as a license is activated, the trial period is in effect. 
	*	Parameters:
	*				uiTrialPeriod [in] If greater than 0, it is the number of days a "trial period" is enforced. Once this period passes,
	*								the license state becomes LE_ERR_LICENSETRIALEXPIRED.
	* */
	LECode EnforceTrial(ComponentType ct, unsigned int uiTrialPeriod);

	/* Description: Registers the license for a particular component. To start monitoring for license status changes,
			call Start(). The license will be frequently checked to see if it has changed since the last time it was checked. */
	LECode ActivateLicense(ComponentType componentType, const TCHAR *wcsLicenseKey);

	/* Description: Gets the current license state. */
	LECode GetLicenseState(ComponentType componentType);

	/* Description: Gets information about the license and stores it in a string */
	LECode GetLicenseInfo(ComponentType componentType, tstring & wsLicenseInfo);

	/* Description: Call this function to start monitoring the license for the registered components.
	 *				The callback associated with the registered component will always be called on the first run. 
	 *				Do not call any Get* functions before calling this as it is required to initialize all the license information!
	 * */
	void Start();

	/* Description: Call this function to stop monitoring the license states. */	
	void Stop();

private:
	CLicenseEnforce();
	~CLicenseEnforce();

	static unsigned int thread_MonitorLicense(void* vpParams);

	bool init();
	void deinit();

	void doLicenseCheck(ComponentType ct, tstring wsComponentName);
	bool isTrialExpired(time_t ttTrialStart, time_t ttToday, unsigned int uiTrialPeriodInDays, unsigned int& uiDaysRemaining);
	void loadActivatedLicensesOnSystem();
	tstring getLicenseInfo(const TCHAR* wcsLicenseKey);
	bool isCallbackAvailable(ComponentType ct);
	bool isLicenseAvailable(ComponentType ct);
	bool isStatusAvailable(ComponentType ct);
	bool isLicenseInfoAvailable(ComponentType ct);
	std::wstring util_strsafe_convert_tcs(const TCHAR *tcsSrc);
	tstring util_strsafe_convert_mbs(const char *csSrc);

private:
	HANDLE m_hThread;
	HANDLE m_hMonitorEvent;

	BOOL m_bMonitoring;

	static CLicenseEnforce* m_Instance;

	CRITICAL_SECTION m_critsecLicense;

	struct ComponentInfo {
		tstring wsLicense;
		OnLicenseStatusChange onLicenseStatusChange;
		unsigned int uiTrialPeriod;
		tstring wsRegKey;
		tstring wsRegAttr;

		// The only way to set the license state / info is to call this setter function. This is to ensure that when one is modified,
		// the other is modified as well, and not only one of them, which could lead to the state conflicting with the info or vice versa.
		void setLicenseState(LECode leCode, tstring wsInfo) { leLicenseState = leCode; wsLicenseInfo = wsInfo; }
		LECode LicenseState() { return leLicenseState; }
		tstring LicenseInfo() { return wsLicenseInfo; }

	private:
		LECode leLicenseState;
		tstring wsLicenseInfo;
	};
	std::map<ComponentType, ComponentInfo> m_mapComponentInfo;
	// Maps thorwa token states to LicenseEnforce status codes (initialized in the constructor)
	std::map<int, LECode> m_mapTokenToLECode;
};
