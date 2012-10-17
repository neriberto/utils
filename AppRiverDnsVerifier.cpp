
#include "AppRiverDnsVerifier.h"
#include "ClientCodes.h"
#include "SecureSurfDefines.h"
#include "Dns.h"

#include <tchar.h>
#include <strsafe.h> // MUST be included after tchar.h

/* Description: AppRiver has provided a special way for a client to check that they are indeed using AppRiver's
 *		DNS servers. This is done by doing a regular DNS Query to their DNS server and checking whether 
 *		it resolves to a specific IP address. 
 *		ie. securesurftest.com -> resolves to -> 67.192.226.156
 * */
int AppRiverDNSVerified(TCHAR *csPrimaryDns, size_t uiPrimaryDnsLength, bool& bPrimaryDnsVerified, 
						TCHAR *csSecondaryDns, size_t uiSecondaryDnsLength, bool& bSecondaryDnsVerified)
{
	int rc = G_FAIL;
	std::vector<Dns::tstring> vsDnsServers;
	int iDnsCounter = 0;

	bPrimaryDnsVerified = false;
	bSecondaryDnsVerified = false;

	// Get DNS server list
	if(G_FAILURE(Dns::GetSystemDnsServers(vsDnsServers)))
		return rc;

	for(int i=0; i<(int)vsDnsServers.size(); i++)
	{
		TCHAR tcsIpAddr[16] = { 0 };

		rc = Dns::ResolveUrl(vsDnsServers[i].c_str(), URL_APPRIVER_DNS_CHECK, tcsIpAddr);
		if( G_SUCCESS(rc) &&
			_tcscmp(tcsIpAddr, IP_APPRIVER_DNS_CHECK)==0)
		{
			// Found an AppRiver DNS server
			if(iDnsCounter==0)
			{
				StringCchCopy(csPrimaryDns, uiPrimaryDnsLength, vsDnsServers[i].c_str());
				bPrimaryDnsVerified = true;
			}
			else if(iDnsCounter==1)
			{
				StringCchCopy(csSecondaryDns, uiSecondaryDnsLength, vsDnsServers[i].c_str());
				bSecondaryDnsVerified = true;
			}

			// Success if at least one DNS server is AppRiver's
			rc = G_TRUE;
		}
		else if(G_SUCCESS(rc))
			rc = G_FALSE; // URL got resolved, but DNS server wasn't AppRiver's.

		iDnsCounter++;

		if(iDnsCounter>=2)
			break;
	}

	return rc;
}
