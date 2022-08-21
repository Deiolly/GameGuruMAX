
////////////////////////////////////////////////////////////////////
// INFORMATION /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

/*
	CORE SET UP COMMANDS
*/

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

// SOUND DLL
// LPDIRECTSOUND8 GetSoundInterface ( void )

/*
*/

////////////////////////////////////////////////////////////////////
// DEFINES AND INCLUDES ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

//#define INITGUID

//#define DARKSDK	__declspec ( dllexport )
#define DARKSDK	
#define _CRT_SECURE_NO_WARNINGS
#define _USING_V110_SDK71_

#include "stdafx.h"

#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>
#include "core.h"
#include <stdio.h>

extern "C" FILE* GG_fopen( const char* filename, const char* mode );

//#include "eax.h"
//#include "eaxman.h"

#include ".\globstruct.h"
#include ".\..\..\Shared\Error\CError.h"

//#define SAFE_DELETE( p )		{ if ( p ) { delete ( p );       ( p ) = NULL; } }
//#define SAFE_RELEASE( p )		{ if ( p ) { ( p )->Release ( ); ( p ) = NULL; } }
//#define SAFE_DELETE_ARRAY( p )	{ if ( p ) { delete [ ] ( p );   ( p ) = NULL; } }

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// GLOBALS /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

extern GlobStruct*				g_pGlob;
//LPDIRECTSOUND8			g_pSound  = NULL;
char					g_szErrorList [ 256 ] [ 256 ];
bool					g_bErrorFile = false;

GetSoundPFN				g_pGetSound       = NULL;
GetSoundBufferPFN		g_pGetSoundBuffer = NULL;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
// FUNCTIONS ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

DARKSDK void ReceiveCoreDataPtr ( LPVOID pCore );
DARKSDK int  GetAssociatedDLLs  ( void );
DARKSDK void Destructor         ( void );

//extern void VoiceSetup	          ( void );
extern void HardDriveSetup        ( void );
		void LoadSystemDLL        ( void );
		void LoadSoundDLL         ( void );
		void SetupErrorCodes      ( void );
		void SetupFileBlocks      ( void );
		void DestroyFileBlocks    ( void );

void ReceiveCoreDataPtr ( LPVOID pCore )
{
	g_pGlob = ( GlobStruct* ) pCore;

	if ( !g_pGlob )
	{
		Error ( 1 );
		return;
	}

	SetupErrorCodes ( );
	LoadSoundDLL    ( );
}

int GetAssociatedDLLs ( void )
{
	return 2;
}

void Destructor ( void )
{
}

char* ENHANCEMENTSSetupString ( char* szInput )
{
	char* pReturn = NULL;
	DWORD dwSize  = strlen ( szInput );
	g_pGlob->CreateDeleteString ( ( char** ) &pReturn, dwSize + 1 );
	if ( !pReturn )
	{
		Error ( 2 );
		return NULL;
	}
	memcpy ( pReturn, szInput, dwSize );
	pReturn [ dwSize ] = 0;
	return pReturn;
}

void LoadSoundDLL ( void )
{
}

void SetupErrorCodes ( void )
{
	// set up error codes for when things fail

	char  szPath [ 255 ] = "";
	char  szDir  [ 255 ] = "";
	HKEY  hKey;
	DWORD dwBufLen;

	// open the registry key
	RegOpenKeyEx    ( HKEY_LOCAL_MACHINE, "SOFTWARE\\Dark Basic\\Dark Basic Pro", 0, KEY_QUERY_VALUE, &hKey );
	RegQueryValueEx ( hKey, "install-path", NULL, NULL, ( LPBYTE ) szPath, &dwBufLen );
	RegCloseKey     ( hKey );

	// append our path
	strcat_s ( szPath, 255, "\\compiler\\plugins-licensed\\" );

	GetCurrentDirectory ( 255, szDir );
	SetCurrentDirectory ( szPath );

	// clear out the error table
	memset ( g_szErrorList, 0, sizeof ( g_szErrorList ) );

	// attempt to open the "loc" file
	FILE* fp = GG_fopen ( "Enhancements.loc", "rt" );

	// if the file is valid we should be running from DB Pro
	if ( fp )
	{
		// read in each line of the file
		char szTemp [ 256 ];
		int  iIndex = 0;
		
		while ( fgets ( szTemp, sizeof ( szTemp ), fp ) )
		{
			char* token = strtok ( szTemp, "\n" );

			while ( token )
			{
				strcpy ( g_szErrorList [ iIndex++ ], token );
				token = strtok ( 0, "\n" );
			}
		}

		// close the file
		fclose ( fp );

		// save state as we have the error file loaded
		g_bErrorFile = true;
	}

	// restory directory
	SetCurrentDirectory ( szDir );
}

void Error ( int iID )
{
	/*
	if ( g_bErrorFile )
	{
		MessageBox ( NULL, g_szErrorList [ iID ], g_szErrorList [ 0 ], MB_ICONERROR | MB_OK );
	}
	else
	{
		char szNum [ 3 ];

		//itoa ( iID, szNum, 10 );
		_itoa ( iID, szNum, 10 );

		MessageBox ( NULL, szNum, "Enhancement Runtime Error", MB_ICONERROR | MB_OK );
	}
	*/
	char szNum[512];
	sprintf ( szNum, "Enhancement Runtime Error %d", iID );
	RunTimeError ( 0, szNum );
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
