#pragma once

#include <Windows.h>

// The URL for which the DNS query is sent.
#define URL_APPRIVER_DNS_CHECK			TEXT("securesurftest.com")
#define IP_APPRIVER_DNS_CHECK			TEXT("67.192.226.156")

int AppRiverDNSVerified(TCHAR *csPrimaryDns, size_t uiPrimaryDnsLength, bool& bPrimaryDnsVerified, 
						TCHAR *csSecondaryDns, size_t uiSecondaryDnsLength, bool& bSecondaryDnsVerified);