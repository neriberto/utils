#pragma once

#define REGKEY_OSC					TEXT("SOFTWARE\\OPSWAT\\GEARS")
#define REGKEY_PARAM_INSTALLDIR		TEXT("installdir")

//////////////////////////////////////////////////////////////////////////
// Global Names
//////////////////////////////////////////////////////////////////////////
#define PRODUCT_NAME				"SecureSurf"

#ifdef WA_APPRIVER
#define EXE_NAME_GUI				TEXT("SecureSurf.exe")
#define EXE_NAME_GUI_W				L"SecureSurf.exe"
#define EXE_NAME_GUI_CAPS			"SECURESURF.EXE"
#define EXE_NAME_GUI_PROGRESS		TEXT("SecureSurfProg.exe")
#else
#define EXE_NAME_GUI				TEXT("OPSWAT GEARS.exe")
#define EXE_NAME_GUI_W				L"OPSWAT GEARS.exe" 
#define EXE_NAME_GUI_CAPS			"OPSWAT GEARS.EXE"
#define EXE_NAME_GUI_PROGRESS		TEXT("OPSWAT GEARS Prog.exe")
#endif
#define EXE_NAME_WAGEARS			TEXT("wagears.exe")

//////////////////////////////////////////////////////////////////////////
// Registry Keys
//////////////////////////////////////////////////////////////////////////
#ifdef WA_APPRIVER
#define REGKEY_PARAM_OSC				"SecureSurf"
#else
#define REGKEY_PARAM_OSC				"OPSWAT GEARS"
#endif
