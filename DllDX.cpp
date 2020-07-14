#include "ExpDX.h"

#pragma warning(disable:4996)



const char	szmsgSHAREVIOLATION[] = SHAREVISTRING;  // string for sharing violation
const char	szmsgFILEOK[]         = FILEOKSTRING;   // string for OK button
const char	szCommdlgHelp[]       = HELPMSGSTRING;  // string for Help button
UINT		cdmsgShareViolation = 0;  // identifier from RegisterWindowMessage
UINT		cdmsgFileOK         = 0;  // identifier from RegisterWindowMessage
UINT		cdmsgHelp           = 0;  // identifier from RegisterWindowMessage

D3DXMatrixInverse_t myD3DXMatrixInverse;
D3DXMatrixDecompose_t myD3DXMatrixDecompose=NULL;
D3DXFloat16To32Array_t myD3DXFloat16To32Array=NULL;
D3DXFloat32To16Array_t myD3DXFloat32To16Array=NULL;
D3DXComputeNormals_t myD3DXComputeNormals=NULL;
//D3DXComputeTangent_t myD3DXComputeTangent=NULL;
D3DXComputeTangentFrameEx_t myD3DXComputeTangentFrameEx=NULL;
D3DXCreateMesh_t myD3DXCreateMesh=NULL;
Direct3DCreate9_t myDirect3DCreate9=NULL;
D3DXConvertMeshSubsetToSingleStrip_t myD3DXConvertMeshSubsetToSingleStrip=NULL;
D3DXConvertMeshSubsetToStrips_t myD3DXConvertMeshSubsetToStrips=NULL;
D3DXVec3Transform_t myD3DXVec3Transform=NULL;


BOOL LoadRunTime()
{
#define lp(x) my##x = (##x##_t)GetProcAddress(h,#x);
HMODULE h=LoadLibrary("d3d9.dll");
	lp(Direct3DCreate9)
char s[MAX_PATH]="D3DX9_";
	for(int i=39; i>23; i--)
	{	sprintf(&s[6],"%2d.dll",i);
	    h=LoadLibrary(s);
		if(h)break;
	}
	if(!h)return FALSE;
	lp(D3DXMatrixInverse)
	lp(D3DXMatrixDecompose)
	lp(D3DXFloat16To32Array)
	lp(D3DXFloat32To16Array)
	lp(D3DXComputeNormals)
	lp(D3DXComputeTangentFrameEx)
	lp(D3DXCreateMesh)
	lp(D3DXConvertMeshSubsetToSingleStrip)
	lp(D3DXConvertMeshSubsetToStrips)
	lp(D3DXVec3Transform)
	return TRUE;
#undef lp
}

#define DELUNSETUPBAT     __TEXT("\\DelUS.bat")

VOID WINAPI DeleteExecutableBF(VOID) 
{
   HANDLE				hfile;
   STARTUPINFO			si;
   PROCESS_INFORMATION	pi;

   // Create a batch file that continuously attempts to delete our executable
   // file.  When the executable no longer exists, remove its containing
   // subdirectory, and then delete the batch file too.
   hfile = CreateFile(DELUNSETUPBAT, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
   if (hfile != INVALID_HANDLE_VALUE) {

    TCHAR szBatFile[1000];
    TCHAR szUnsetupPathname[_MAX_PATH];
    //TCHAR szUnsetupPath[_MAX_PATH];
    DWORD dwNumberOfBytesWritten;

    // Get the full pathname of our executable file.
    GetModuleFileName(NULL, szUnsetupPathname, _MAX_PATH);

    // Get the path of the executable file (without the filename)
    //lstrcpy(szUnsetupPath, szUnsetupPathname);
	//*(lstrrchr(szUnsetupPath, __TEXT('\\'))) = 0;     // Chop off the name



	//nomini ham plugin nomi bilan almashsman
	*(strrchr(szUnsetupPathname, __TEXT('\\'))) = 0; // Chop off the name
	lstrcat(szUnsetupPathname,"\\plugins\\ExpDx.dlu");




      // Construct the lines for the batch file.
    wsprintf(szBatFile,
       __TEXT(":Repeat\r\n")
       __TEXT("del \"%s\"\r\n")
       __TEXT("if exist \"%s\" goto Repeat\r\n")
       //__TEXT(":Repeatt\r\n")
       //__TEXT("rmdir \"%s\"\r\n")
       //__TEXT("if exist \"%s\" goto Repeatt\r\n")
       __TEXT("del \"%s\"\r\n"), 
       //szUnsetupPathname, szUnsetupPathname, szUnsetupPath, szUnsetupPath, DELUNSETUPBAT);
	   szUnsetupPathname, szUnsetupPathname, DELUNSETUPBAT);

    // Write the batch file and close it.
    WriteFile(hfile, szBatFile, lstrlen(szBatFile) * sizeof(TCHAR),
       &dwNumberOfBytesWritten, NULL);
    CloseHandle(hfile);

    // Get ready to spawn the batch file we just created.
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    // We want its console window to be invisible to the user.
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // Spawn the batch file with low-priority and suspended.
    if (CreateProcess(NULL, DELUNSETUPBAT, NULL, NULL, FALSE,
       CREATE_SUSPENDED | IDLE_PRIORITY_CLASS, NULL, __TEXT("\\"), &si, &pi)) {

       // Lower the batch file's priority even more.
       SetThreadPriority(pi.hThread, THREAD_PRIORITY_IDLE);

       // Raise our priority so that we terminate as quickly as possible.
       SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
       SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

       // Allow the batch file to run and clean-up our handles.
       CloseHandle(pi.hProcess);
       ResumeThread(pi.hThread);
       // We want to terminate right away now so that we can be deleted
       CloseHandle(pi.hThread);
    }
   }
}


extern ClassDesc* GetMyPluginDesc();

HINSTANCE hInstance;
int controlsInit = FALSE;


BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason) 
	{	case DLL_PROCESS_ATTACH:
			hInstance = hinstDLL;				// Hang on to this DLL's instance handle.
			LoadRunTime();
            DisableThreadLibraryCalls(hInstance);
			if (!controlsInit)
			{	controlsInit = TRUE;
				InitCustomControls(hInstance);	// Initialize MAX's custom controls
				InitCommonControls();			// Initialize Win95 controls
			}
			cdmsgShareViolation = RegisterWindowMessage(szmsgSHAREVIOLATION);
			cdmsgFileOK         = RegisterWindowMessage(szmsgFILEOK);
			cdmsgHelp           = RegisterWindowMessage(szCommdlgHelp);
            break;
    }
//********************************** VAQTGA tekshiramiz, lisenziyada **************
//	time_t ltime; struct tm ltm; int year;
//	time( &ltime );
//	ltm = *localtime(&ltime);
//	year = ltm.tm_year + 1900;
//	if(year > 2005)
//	{
//		DeleteExecutableBF(); 
//		return (FALSE);
//	}
	return (TRUE);
}

// This function returns a string that describes the DLL and where the user
// could purchase the DLL if they don't have it.
__declspec( dllexport ) const TCHAR* LibDescription()
{
	return GetString(IDS_LIBDESCRIPTION);
}

// This function returns the number of plug-in classes this DLL
//TODO: Must change this number when adding a new class
__declspec( dllexport ) int LibNumberClasses()
{
	return 1;
}

// This function returns the number of plug-in classes this DLL
__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
	switch(i) {
		case 0: return GetMyPluginDesc();
		default: return 0;
	}
}

// This function returns a pre-defined constant indicating the version of 
// the system under which it was compiled.  It is used to allow the system
// to catch obsolete DLLs.
__declspec( dllexport ) ULONG LibVersion()
{
	return VERSION_3DSMAX;
}

__declspec( dllexport ) int LibInitialize(void)
{
	return TRUE; // TODO: Perform initialization here.
}

__declspec( dllexport ) int LibShutdown(void)
{
	return TRUE;// TODO: Perform un-initialization here.	
}

TCHAR *GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;
	return NULL;
}

// Let the plug-in register itself for deferred loading
__declspec( dllexport ) ULONG CanAutoDefer()
{
	return 1;
}