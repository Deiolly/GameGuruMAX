//
// DarkEXE
//

// Internal Includes
#define _CRT_SECURE_NO_DEPRECATE

// Pragmas
#pragma warning(disable : 4099)
#pragma warning(disable : 4996)

// Includes
#include "FileReader.h"
#include "windows.h"
#include "direct.h"
#include "time.h"
#include "io.h"
#include "DarkEXE.h"
#include "resource.h"
#include "globstruct.h"
#include "EXEBlock.h"
#include "macros.h"
#include "CFileC.h"
#include "CObjectsC.h"
#include "CGfxC.h"
#include "CSystemC.h"

// Globals required
extern struct _finddata_t			filedata;
extern long							hInternalFile;
extern int							FileReturnValue;
time_t								TodaysDay = 0;
LPSTR								gRefCommandLineString=NULL;
char								gUnpackDirectory[_MAX_PATH];
char								gpDBPDataName[_MAX_PATH];
DWORD								gEncryptionKey;
CEXEBlock							CEXE;
GlobStruct							g_Glob;
GlobStruct*							g_pGlob = &g_Glob;
int									g_iCountConversions = 0;

// Support Functions

void DeleteContentsOfDBPDATA ( bool bOnlyIfOlderThan2DAYS )
{
	// Delete all files in current folder
	FFindFirstFile();
	int iAttempts=20;
	while(FGetFileReturnValue()!=-1L && iAttempts>0)
	{
		// only if older than 2 days
		bool bGo = false;
		if ( bOnlyIfOlderThan2DAYS==false ) bGo = true;
		if ( bOnlyIfOlderThan2DAYS==true )
		{
			time_t ThisFileDays = filedata.time_write / 60 / 60 / 24;
			time_t Difference = TodaysDay - ThisFileDays; 
			if ( Difference >= 2 )
			{
				// this file is at least 2 days old
				bGo = true;
			}
		}

		// go
		if ( bGo )
		{
			if(FGetActualTypeValue(filedata.attrib)==1)
			{
				if(stricmp(filedata.name, ".")!=NULL
				&& stricmp(filedata.name, "..")!=NULL)
				{
					char file[_MAX_PATH];
					strcpy(file, filedata.name);
					char old[_MAX_PATH];
					getcwd(old, _MAX_PATH);
					BOOL bResult = RemoveDirectory(file);
					if(bResult==FALSE)
					{
						_chdir(file);
						FFindCloseFile();
						DeleteContentsOfDBPDATA ( bOnlyIfOlderThan2DAYS );
						_chdir(old);
						BOOL bResult = RemoveDirectory(file);
						FFindFirstFile();
					}
					iAttempts--;
				}
			}
			else
			{
				DeleteFile(filedata.name);
			}
		}
		FFindNextFile();
	}
	FFindCloseFile();
}

void MakeOrEnterUniqueDBPDATA(void)
{
	// Default DBPDATA string
	strcpy(gpDBPDataName,"dbpdata");

	// Prepare DBPDATA string builder
	DWORD dwBuildID=2;
	char pBuildStart[_MAX_PATH];
	strcpy(pBuildStart, gpDBPDataName);

	// Make it as unique
	mkdir(gpDBPDataName);

	// get todays day when made directory
	FFindFirstFile();
	TodaysDay = filedata.time_write / 60 / 60 / 24;
	FFindCloseFile();
}

void scanallfolder ( std::string subThisFolder_s, std::string subFolder_s )
{
	// into folder
	if ( subThisFolder_s.length() > 0 ) SetDir ( (LPSTR)subThisFolder_s.c_str() );

	// first scan all files and folders - store folders locally
	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	std::string* pFolders = new std::string[iFoldersCount+1];
	for ( int c = 1; c <= iFoldersCount; c++ )
	{
		pFolders[c] = "";
		LPSTR pFileFolderName = ChecklistString(c);
		if ( strcmp ( pFileFolderName, "." ) != NULL && strcmp ( pFileFolderName, ".." ) !=NULL )
		{
			if ( ChecklistValueA(c) == 1 )
			{
				pFolders[c] = pFileFolderName;
			}
			else
			{
				// look for an X file with no DBO counterpart
				if (strnicmp(pFileFolderName + strlen(pFileFolderName) - 2, ".x", 2) == NULL)
				{
					// is there a DBO already?
					char pNewDBOFilename[MAX_PATH];
					strcpy(pNewDBOFilename, pFileFolderName);
					pNewDBOFilename[strlen(pNewDBOFilename) - 2] = 0;
					strcat(pNewDBOFilename, ".dbo");
					if (FileExist(pNewDBOFilename) == 0)
					{
						// no, so we create it
						if (ObjectExist(1) == 1) DeleteObject(1);
						LoadObject(pFileFolderName, 1);
						if (ObjectExist(1) == 1)
						{
							// DBO created from original Classic load/save system (ready for next generation usage as a DBO)
							SaveObject(pNewDBOFilename, 1);
							g_iCountConversions++;
						}
					}
				}
			}
		}
	}

	// now use local folder list and investigate each one
	for ( int f = 1; f <= iFoldersCount; f++ )
	{
		std::string pFolderName = pFolders[f];
		if ( pFolderName.length() > 0 )
		{
			std::string relativeFolderPath_s = pFolderName;
			if ( subFolder_s.length() > 0 ) relativeFolderPath_s = subFolder_s + "\\" + pFolderName;
			scanallfolder ( pFolderName, relativeFolderPath_s );
		}
	}

	// back out of folder
	if ( subThisFolder_s.length() > 0 ) SetDir ( ".." );

	// finally free resources
	delete[] pFolders;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// command line
	gRefCommandLineString=lpCmdLine;

	// Prepare Virtual Directory from Data Appended to EXE File
	char ActualEXEFilename[_MAX_PATH];
	GetModuleFileName(hInstance, ActualEXEFilename, _MAX_PATH);

	// Get current working directory (for temp folder compare)
	char CurrentDirectory[_MAX_PATH];
	getcwd(CurrentDirectory, _MAX_PATH);

	// Find temporary directory (C:\WINDOWS\Temp)
	char WindowsTempDirectory[_MAX_PATH];
	GetTempPath(_MAX_PATH, WindowsTempDirectory);
	if(stricmp(WindowsTempDirectory, CurrentDirectory)!=NULL)
	{
		// XP Temp Folder
		_chdir(WindowsTempDirectory);
		MakeOrEnterUniqueDBPDATA();
		strcat(WindowsTempDirectory, "\\");
		strcat(WindowsTempDirectory, gpDBPDataName);
		strcpy(gUnpackDirectory, WindowsTempDirectory);
		_chdir(gpDBPDataName);
		DeleteContentsOfDBPDATA(false);
		_chdir(CurrentDirectory);
	}
	else
	{
		// Pre-XP Temp Folder
		GetWindowsDirectory(WindowsTempDirectory, _MAX_PATH);
		_chdir(WindowsTempDirectory);
		mkdir("temp");
		_chdir("temp");
		MakeOrEnterUniqueDBPDATA();
		strcat(WindowsTempDirectory, "\\temp\\");
		strcat(WindowsTempDirectory, gpDBPDataName);
		strcpy(gUnpackDirectory, WindowsTempDirectory);
		_chdir(gpDBPDataName);
		DeleteContentsOfDBPDATA(false);
		_chdir(CurrentDirectory);
	}

	// Send critical start info to CEXE
	CEXE.StartInfo(gUnpackDirectory, gEncryptionKey);

	// Place absolute EXE filename in CEXE structure
	if ( CEXE.m_pAbsoluteAppFile ) strcpy ( CEXE.m_pAbsoluteAppFile, ActualEXEFilename );

	// In case of error
	LPSTR pErrorString = NULL;

	// Instead of EXE Loda, have to enter values manually
	CEXE.m_pInitialAppName = new char[32];
	CEXE.m_dwInitialDisplayMode = 0;
	CEXE.m_dwInitialDisplayWidth = 799;
	CEXE.m_dwInitialDisplayHeight = 599;
	CEXE.m_dwInitialDisplayDepth = 32;
	strcpy(CEXE.m_pInitialAppName, "Guru-MapEditor");

	// Manually create runtime error string array
	CEXE.m_pRuntimeErrorStringsArray = new LPSTR[9999];
	for ( int n=0; n<9999; n++ )
	{
		CEXE.m_pRuntimeErrorStringsArray[n] = new char[256];
		memset ( CEXE.m_pRuntimeErrorStringsArray[n], 0, sizeof(256) );
	}

	// Global Shared Data
	ZeroMemory(&g_Glob, sizeof(GlobStruct));
	g_Glob.bWindowsMouseVisible		= true;
	g_Glob.dwForeColor				= -1; // (white)
	g_Glob.dwBackColor				= 0;
	g_Glob.bEscapeKeyEnabled		= true;

	// initialise a window
	bool bResult = true;;
	bResult = CEXE.Init(hInstance, bResult, gRefCommandLineString);

	// create a D3D area
	SetDisplayMode ( GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 32, 1 );

	// all or specific one
	if (gRefCommandLineString && strlen(gRefCommandLineString) > 0)
	{
		// silently convert one if not already a DBO
		if (strnicmp(gRefCommandLineString + strlen(gRefCommandLineString) - 2, ".x", 2) == NULL)
		{
			if (FileExist(gRefCommandLineString) == 1)
			{
				char pDBOVersion[MAX_PATH];
				strcpy(pDBOVersion, gRefCommandLineString);
				pDBOVersion[strlen(pDBOVersion) - 2] = 0;
				strcat(pDBOVersion, ".dbo");
				if (FileExist(pDBOVersion) == 0)
				{
					LoadObject(gRefCommandLineString, 1);
					if (ObjectExist(1) == 1)
					{
						// DBO created from original Classic load/save system (ready for next generation usage as a DBO)
						SaveObject(pDBOVersion, 1);
					}
				}
			}
		}
	}
	else
	{
		// scan all files in Files folder, create any missing DBOs
		scanallfolder ("Files", "");

		// report if any conversions occurred
		if (g_iCountConversions > 0)
		{
			char pHelpfulPrompt[MAX_PATH];
			if (g_iCountConversions == 1)
				sprintf(pHelpfulPrompt, "Successfully converted 1 model from Classic to MAX", g_iCountConversions);
			else
				sprintf(pHelpfulPrompt, "Successfully converted %d models from Classic to MAX", g_iCountConversions);
			MessageBox(NULL, pHelpfulPrompt, "Guru Converter", MB_ICONINFORMATION | MB_OK);
		}
	}

	// Free Display First
	CEXE.FreeUptoDisplay();

	// use current date to remove
	_chdir(CurrentDirectory);

	// Delete temporary unpack directory
	rmdir(gUnpackDirectory);

	// Complete
	return 0;
}
