
	
static inline bool G_SUCCESS(int code)
{
	return ((code >= 0) && (code <= 49));
}
static inline bool G_FAILURE(int code)
{
	return ((code >= 50) && (code <= 99));
}

// GENERIC codes
//! Success.
#define G_OK							0
//! True
#define G_TRUE							1
//! False
#define G_FALSE							2

//! Generic Error
#define G_FAIL							50
//! Uninitialized Error
#define G_NO_INIT						51
//! Native API error
#define G_NATIVE_API					52
//! Feature not supported
#define G_NOT_SUPPORTED					53
//! Feature not implemented
#define G_NOT_IMPLEMENTED				54
//! Invalid parameter
#define G_INVALID_PARAM					55
//! Network error -- could not find route to host
#define G_INET_NO_ROUTE					56
//! Initialization hasn't been performed yet
#define G_NOT_INITIALIZED				57
//! License is invalid
#define G_INVALID_LICENSE				58
//! File not found
#define G_FILE_NOT_FOUND				59
//! Path not found
#define G_PATH_NOT_FOUND				60
//! Access denied
#define G_ACCESS_DENIED					61

//! Up-to-date
#define G_UPDATE_UPTODATE				100
//! Update already in progress   
#define G_UPDATE_INPROGRESS				101

// LICENSE states
#define LICENSESTATE_VALID				1
#define LICENSESTATE_UNKNOWN			2

