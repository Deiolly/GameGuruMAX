//
// DBOFile Functions Header
//

#ifndef _DBOFILE_H_
#define _DBOFILE_H_

#include "DBOData.h"

#include "preprocessor-flags.h"
#ifdef WICKEDENGINE
#else
#include "..\global.h"
#endif

extern char g_WindowsTempDirectory[_MAX_PATH];

DARKSDK int LoadDBOEx ( LPSTR pFilename, sObject** ppObject );
DARKSDK int SaveDBOEx ( LPSTR pFilename, sObject* pObject );

#endif _DBOFILE_H_