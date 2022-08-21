//----------------------------------------------------
//--- GAMEGURU - M-MapFile
//----------------------------------------------------

// Includes
#include "stdafx.h"
#include "gameguru.h"
#ifdef WICKEDENGINE
#include "GGTerrain\GGTerrainFile.h"
#include "GGTerrain\GGTerrain.h"
#include "GGTerrain\GGTrees.h"
#include "GGTerrain\GGGrass.h"
#endif

#include "GGThread.h"
using namespace GGThread;

class ExtractZipThread : public GGThread
{
protected:
	static char* pZipFileName;
	static const char* pExtractPath;
	static const char* const* pFileNames;
	static const unsigned long* pFileSizes;
	static volatile unsigned char* pFileDone;
	static volatile uint32_t iNextFile;
	static uint32_t iNumFiles;
	static threadLock lock;
	
	static ExtractZipThread* pThreads;
	static uint32_t iNumThreads;

	uint32_t iFileBlockID;

public:

	static void SetWork( char* zipFile, const char* extractPath, const char* const* files, const unsigned long* sizes, uint32_t numFiles )
	{
		pZipFileName = zipFile;
		pExtractPath = extractPath;
		iNextFile = 0;
		iNumFiles = numFiles;
		pFileNames = files;
		pFileSizes = sizes;

		if ( pFileDone ) delete [] pFileDone;
		pFileDone = new unsigned char[ numFiles ];
		for( int i = 0; i < numFiles; i++ ) pFileDone[ i ] = 0;
	}

	static bool AnyRunning()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) 
		{
			if ( pThreads[i].IsRunning() ) return true;
		}
		return false;
	}

	static void WaitForAll()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[i].Join();
	}

	static void StartThreads()
	{
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[i].Start();
	}

	static void SetThreads( uint32_t numThreads )
	{
		if ( numThreads > 250 ) numThreads = 250; // limited by file block IDs to 256 but play it safe
		if ( numThreads == iNumThreads ) return;
		if ( pThreads ) delete [] pThreads;
		
		pThreads = new ExtractZipThread[ numThreads ];
		iNumThreads = numThreads;
		for( uint32_t i = 0; i < iNumThreads; i++ ) pThreads[ i ].iFileBlockID = i+2; // start at index 2
	}

	static bool IsSetup() { return pThreads != 0; }

	static uint32_t GetProgress()
	{
		// no need for lock here since it is an atomic read
		return (iNextFile * 100) / iNumFiles;
	}

	uint32_t Run( ) 
	{
		OpenFileBlock ( pZipFileName, iFileBlockID, "mypassword" );

		while( 1 )
		{
			if ( bTerminate ) break;
			
			int localIndex = -1;
			while( !lock.Acquire() );

			unsigned long maxSize = 0;
			for( int i = 0; i < iNumFiles; i++ )
			{
				if ( pFileDone[i] == 0 && pFileSizes[i] >= maxSize )
				{
					maxSize = pFileSizes[ i ];
					localIndex = i;
				}
			}

			if ( localIndex >= 0 ) pFileDone[localIndex] = 1;

			lock.Release();

			if ( localIndex < 0 ) break;

			const char* filename = pFileNames[ localIndex ];
			ExtractFileFromBlock( iFileBlockID, filename, pExtractPath );
		}

		CloseFileBlock( iFileBlockID );

		return 0;
	}

	ExtractZipThread() : GGThread() 
	{
		
	}

	~ExtractZipThread() 
	{
		if ( pThreads ) delete [] pThreads;
	}
};

char* ExtractZipThread::pZipFileName = 0;
const char* ExtractZipThread::pExtractPath = 0;
const char* const* ExtractZipThread::pFileNames = 0;
const unsigned long* ExtractZipThread::pFileSizes = 0;
volatile unsigned char* ExtractZipThread::pFileDone = 0;
volatile uint32_t ExtractZipThread::iNextFile = 0;
uint32_t ExtractZipThread::iNumFiles = 0;
threadLock ExtractZipThread::lock;
ExtractZipThread* ExtractZipThread::pThreads = 0;
uint32_t ExtractZipThread::iNumThreads = 0;

#ifdef ENABLEIMGUI
	#include "..\Imgui\imgui_gg_extras.h"
	std::vector<cstr> g_sDefaultAssetFiles;
#endif

#ifdef STORYBOARD
extern StoryboardStruct Storyboard;
extern int g_Storyboard_First_Level_Node;
extern int g_Storyboard_Current_Level;
extern char g_Storyboard_First_fpm[256];
extern char g_Storyboard_Current_fpm[256];
extern char g_Storyboard_Current_lua[256];
#endif

// 
//  MAP FILE FORMAT
// 

// MapFile Globals
int g_mapfile_iStage = 0;
int g_mapfile_iNumberOfLevels = 0;
int g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
float g_mapfile_fProgress = 0.0f;
float g_mapfile_fProgressSpan = 0.0f;
std::vector<cstr> g_mapfile_fppFoldersToRemoveList;
std::vector<cstr> g_mapfile_fppFilesToRemoveList;
cstr g_mapfile_mapbankpath;
cstr g_mapfile_levelpathfolder;
bool g_bAllowBackwardCompatibleConversion = false;
bool g_bNeedToConvertClassicPositionsToMAX = false;

#ifdef ENABLEIMGUI
bool restore_old_map = false;
#endif

void mapfile_saveproject_fpm ( void )
{
	LPSTR pOldDir = GetDir();

	//  use default or special worklevel stored
	if (  t.goverridefpmdestination_s != "" ) 
	{
		t.ttempprojfilename_s=t.goverridefpmdestination_s;
	}
	else
	{
		t.ttempprojfilename_s=g.projectfilename_s;
	}
	#ifdef WICKEDENGINE
	//PE: Default folder could be d:\max\ , and write folder could be f:\docwrite\
	//PE: So then moving zip, it takes from d:\... and move to f:\... , d:\\ dont exists so .fpm file is deleted.
	//PE: To solve always use full path to .fpm
	char destination[MAX_PATH];
	strcpy(destination, t.ttempprojfilename_s.Get());
	GG_GetRealPath(destination, 1);
	t.ttempprojfilename_s = destination;
	#endif
	//75593 : Saving FPM g.level file: D:\github\GameGuruRepo\GameGuru\Files\map bank\my-test-map.fpm S:0MB   V: (579,0) 
	if (g.editorsavebak == 1) 
	{
		//PE: Make a backup before overwriting a fpm level.
		char backupname[1024];
		strcpy(backupname, t.ttempprojfilename_s.Get());
		backupname[strlen(backupname) - 1] = 'k';
		backupname[strlen(backupname) - 2] = 'a';
		backupname[strlen(backupname) - 3] = 'b';
		DeleteAFile(backupname);
		CopyAFile(t.ttempprojfilename_s.Get(), backupname);
	}

	//  log prompts
	timestampactivity(0, cstr(cstr("Saving FPM level file: ")+t.ttempprojfilename_s).Get() );

	#ifdef WICKEDENGINE
	//PE: Save heightmap.
	uint32_t iHeightmapSize = 0, iHeightmapWidth = 0, iHeightmapHeight = 0;
	iHeightmapSize = GGTerrain::GGTerrain_GetHeightmapDataSize(iHeightmapWidth, iHeightmapHeight);
	t.visuals.iHeightmapWidth = t.gamevisuals.iHeightmapWidth = iHeightmapWidth;
	t.visuals.iHeightmapHeight = t.gamevisuals.iHeightmapHeight = iHeightmapHeight;
	#endif

	//  Switch visuals to gamevisuals as this is what we want to save
	t.editorvisuals=t.visuals ; t.visuals=t.gamevisuals  ; visuals_save ( );

	//  Copy visuals.ini into levelfile folder
	t.tincludevisualsfile=0;
	#ifdef WICKEDENGINE
	char pRealVisFile[MAX_PATH];
	strcpy(pRealVisFile, g.fpscrootdir_s.Get());
	strcat(pRealVisFile, "\\visuals.ini");
	GG_GetRealPath(pRealVisFile, 1);
	if (FileExist(pRealVisFile) == 1)
	{
		t.tvisfile_s = g.mysystem.levelBankTestMap_s + "visuals.ini";
		if (FileExist(t.tvisfile_s.Get()) == 1)  DeleteAFile(t.tvisfile_s.Get());
		CopyAFile(pRealVisFile, t.tvisfile_s.Get());
		t.tincludevisualsfile = 1;
	}
	#else
	if (  FileExist( cstr(g.fpscrootdir_s+"\\visuals.ini").Get() ) == 1 ) 
	{
		t.tvisfile_s=g.mysystem.levelBankTestMap_s+"visuals.ini"; //"levelbank\\testmap\\visuals.ini";
		if (  FileExist(t.tvisfile_s.Get()) == 1  )  DeleteAFile (  t.tvisfile_s.Get() );
		CopyAFile (  cstr(g.fpscrootdir_s+"\\visuals.ini").Get(),t.tvisfile_s.Get() );
		t.tincludevisualsfile=1;
	}
	#endif

	//  And switch back for benefit to editor visuals
	t.visuals=t.editorvisuals; // messes up when click test game again, old: gosub _visuals_save

	//  Delete any old file
	if (  FileExist(t.ttempprojfilename_s.Get()) == 1  )  DeleteAFile (  t.ttempprojfilename_s.Get() );

	//  Copy CFG to testgame area for saving with other files
	t.tttfile_s="cfg.cfg";
	cstr cfgfile_s = g.mysystem.editorsGridedit_s + t.tttfile_s;
	cstr cfginlevelbank_s = g.mysystem.levelBankTestMap_s + t.tttfile_s;
	if (  FileExist( cfgfile_s.Get() ) == 1 )
	{
		if ( FileExist( cfginlevelbank_s.Get() ) == 1 ) DeleteAFile ( cfginlevelbank_s.Get() );
#ifdef WICKEDENGINE
		//PE: We need full path from write folder in wicked.
		char destination[MAX_PATH];
		strcpy(destination, cfgfile_s.Get());
		GG_GetRealPath(destination, 1);
		cfgfile_s = destination;
#endif
		CopyAFile ( cfgfile_s.Get(), cfginlevelbank_s.Get() );
	}
	#ifdef WICKEDENGINE
	//PE: For some reason cfg.cfg is missing from the fpm ?
	if (FileExist(cfginlevelbank_s.Get()) == 0)
	{
		//PE: Create a new directly inside testmap.
		editor_savecfg(cfginlevelbank_s.Get());
		timestampactivity(0, cstr(cstr("Creating cfg.cfg file: ") + cfginlevelbank_s).Get());
	}
	#endif

	#ifdef WICKEDENGINE
	extern std::vector<sRubberBandType> vEntityLockedList;
	cfginlevelbank_s = g.mysystem.levelBankTestMap_s + "locked.cfg";
	//PE: Save a copy of locked objects.
	if (FileExist(cfginlevelbank_s.Get()) == 1)  DeleteAFile(cfginlevelbank_s.Get());
	OpenToWrite(1, cfginlevelbank_s.Get());
	WriteLong(1, vEntityLockedList.size());
	for (int i = 0; i < vEntityLockedList.size(); i++)
	{
		WriteLong(1, vEntityLockedList[i].e);
	}
	CloseFile(1);
	#endif

	// Create a FPM (zipfile)
	CreateFileBlock (  1, t.ttempprojfilename_s.Get() );
	SetFileBlockKey (  1, "mypassword" );
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );
	AddFileToBlock (  1, "header.dat" );
	AddFileToBlock (  1, "playerconfig.dat" );
	#ifdef WICKEDENGINE
	AddFileToBlock(1, "locked.cfg");
	#endif
	AddFileToBlock (  1, "cfg.cfg" );
	// entity and waypoint files
	AddFileToBlock (  1, "map.ele" );
	AddFileToBlock (  1, "map.ent" );
	AddFileToBlock (  1, "map.way" );
	// darkai obstacle data (container zero)
	AddFileToBlock (  1, "map.obs" );
	// terrain files
	#ifdef WICKEDENGINE
	// save all node folders (containing terrain geometry and virtual textures)
	ChecklistForFiles (  );
	std::vector<std::string> terrainNodeFolders;
	terrainNodeFolders.clear();
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.tfile_s = ChecklistString(t.c);
		if ( t.tfile_s != "." && t.tfile_s != ".." && t.tfile_s != "ttsfiles" ) 
		{
			if (ChecklistValueA(t.c) == 1)
			{
				if (cstr(Lower(Left(t.tfile_s.Get(), 2))) == "tt")
				{
					// found terrain node folder
					terrainNodeFolders.push_back(t.tfile_s.Get());
				}
			}
		}
	}
	for (int tt = 0; tt < terrainNodeFolders.size(); tt++)
	{
		LPSTR pTerrainNodeFolder = (LPSTR)terrainNodeFolders[tt].c_str();
		SetDir(pTerrainNodeFolder);
		ChecklistForFiles();
		SetDir("..");
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			if (t.tfile_s != "." && t.tfile_s != "..")
			{
				cstr pFullRelPathToTerrainFile = cstr(pTerrainNodeFolder) + "\\" + t.tfile_s.Get();
				AddFileToBlock ( 1, pFullRelPathToTerrainFile.Get() );
			}
		}
	}
	// and associated nav mesh
	//AddFileToBlock (1, "rawlevelgeometry.obj"); not needed, calculated in realtime!
	#else
	AddFileToBlock (  1, "m.dat" );
	#ifdef VRTECH
	AddFileToBlock (  1, "vegmask.png");// dds" );
	#else
	AddFileToBlock (  1, "vegmask.dds" );
	#endif
	AddFileToBlock (  1, "vegmaskgrass.dat" );
	if ( FileExist ( "superpalette.ter" ) == 1 ) 
		AddFileToBlock ( 1, "superpalette.ter" );
	#endif

	#ifdef WICKEDENGINE
	#define MAXGROUPSLISTS 100 // duplicated in GridEdit.cpp
	for (int gi = 0; gi < MAXGROUPSLISTS; gi++)
	{
		char pGroupImgFilename[MAX_PATH];
		sprintf(pGroupImgFilename, "groupimg%d.png", gi);
		if (FileExist(pGroupImgFilename) == 1)
		{
			AddFileToBlock (1, pGroupImgFilename);
		}
	}
	#endif

	#ifdef WICKEDENGINE
	// new multi-grass system stores grass choices in testmap folder
	AddFileToBlock ( 1, "grass_coloronly.dds" );
	//AddFileToBlock ( 1, "grass_normal.dds" );
	//AddFileToBlock ( 1, "grass_surface.dds" );
	#endif

	#ifdef WICKEDENGINE
	// new terrain system saves its data settings
	cstr TerrainDataFile_s = g.mysystem.levelBankTestMap_s + "ggterrain.dat";
	GGTerrainFile_SaveTerrainData(TerrainDataFile_s.Get(), g.gdefaultwaterheight);
	AddFileToBlock (1, "ggterrain.dat");
	uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
	char *data = new char[terrain_sculpt_size];
	if (data)
	{
		cstr sculpt_data_name = cstr((int)terrain_sculpt_size) + cstr(".dat");
		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + sculpt_data_name;
		GGTerrain::GGTerrain_GetSculptData((uint8_t*)data);
		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());
		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, data, terrain_sculpt_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, sculpt_data_name.Get());
		}
		delete(data);
	}

	//PE: Save Paint Data.
	uint32_t terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
	data = new char[terrain_paint_size];
	if (data)
	{
		cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");

		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + paint_data_name;

		GGTerrain::GGTerrain_GetPaintData((uint8_t*)data);

		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());

		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, data, terrain_paint_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, paint_data_name.Get());
		}
		delete(data);
	}

	//PE: Save Tree Data.
	uint32_t terrain_tree_size = GGTrees::GGTrees_GetDataSize() * 4;
	data = new char[terrain_tree_size];
	if (data)
	{
		cstr tree_data_name = cstr((int)terrain_tree_size) + cstr(".tre");

		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + tree_data_name;

		GGTrees::GGTrees_GetData((float*)data);

		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());

		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, data, terrain_tree_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, tree_data_name.Get());
		}
		delete(data);
	}

	//PE: Save grass Data.
	uint32_t terrain_grass_size = GGGrass::GGGrass_GetDataSize();
	data = new char[terrain_grass_size];
	if (data)
	{
		cstr grass_data_name = cstr((int)terrain_grass_size) + cstr(".gra");

		TerrainDataFile_s = g.mysystem.levelBankTestMap_s + grass_data_name;

		GGGrass::GGGrass_GetData((uint8_t*)data);

		if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());

		HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hwritefile != INVALID_HANDLE_VALUE)
		{
			DWORD byteswritten;
			WriteFile(hwritefile, data, terrain_grass_size, &byteswritten, NULL);
			CloseHandle(hwritefile);
			AddFileToBlock(1, grass_data_name.Get());
		}
		delete(data);
	}

	//PE: Save heightmap data if any.
	if (iHeightmapSize > 0 && iHeightmapWidth > 0 && iHeightmapHeight > 0)
	{
		data = new char[iHeightmapSize];
		if (data)
		{
			cstr heightmap_data_name = "heightmapdata.raw";

			TerrainDataFile_s = g.mysystem.levelBankTestMap_s + heightmap_data_name;

			GGTerrain::GGTerrain_GetHeightmapData((uint16_t*)data);

			if (FileExist(TerrainDataFile_s.Get()) == 1) DeleteAFile(TerrainDataFile_s.Get());

			HANDLE hwritefile = GG_CreateFile(TerrainDataFile_s.Get(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hwritefile != INVALID_HANDLE_VALUE)
			{
				DWORD byteswritten;
				WriteFile(hwritefile, data, iHeightmapSize, &byteswritten, NULL);
				CloseHandle(hwritefile);
				AddFileToBlock(1, heightmap_data_name.Get());
			}
			delete(data);
		}

	}

	#else
	#ifdef VRTECH
	// add custom content
	LPSTR pOldDir2 = GetDir();
	SetDir(pOldDir); //PE: We need to be here for ScanLevelForCustomContent to work.
	cstr sStoreProjAsItGetsChanged = g.projectfilename_s;
	ScanLevelForCustomContent ( t.ttempprojfilename_s.Get() );
	g.projectfilename_s = sStoreProjAsItGetsChanged;
	SetDir(pOldDir2);
	// putting back optional custom terrain texture
	if ( FileExist ( "Texture_D.dds" ) == 1 ) 
		AddFileToBlock ( 1, "Texture_D.dds" );
	if ( FileExist ( "Texture_D.jpg" ) == 1 ) 
		AddFileToBlock ( 1, "Texture_D.jpg" );
	#else
	if ( FileExist ( "Texture_D.dds" ) == 1 ) 
		AddFileToBlock ( 1, "Texture_D.dds" );
	#endif
	#ifdef VRTECH
	// Don't include large files until find a nice to way reduce them considerably (or find a faster way to transfer multiplayer FPM)
	#else
	AddFileToBlock (  1, "watermask.dds" );
	if ( FileExist ( "globalenvmap.dds" ) == 1 ) 
		AddFileToBlock ( 1, "globalenvmap.dds" );
	if ( FileExist ( "Texture_N.dds" ) == 1 ) 
		AddFileToBlock ( 1, "Texture_N.dds" );
	if ( FileExist ( "Texture_N.jpg" ) == 1 ) 
		AddFileToBlock ( 1, "Texture_N.jpg" );
	#endif
	#endif

	// lightmap files
	if ( PathExist("lightmaps") == 1 ) 
	{
		AddFileToBlock (  1, "lightmaps\\objectlist.dat" );
		AddFileToBlock (  1, "lightmaps\\objectnummax.dat" );
		t.tnummaxfile_s=t.lightmapper.lmpath_s+"objectnummax.dat";
		if (  FileExist(t.tnummaxfile_s.Get()) == 1 ) 
		{
			OpenToRead (  1,t.tnummaxfile_s.Get() );
			t.temaxinfolder = ReadLong ( 1 );
			CloseFile (  1 );
		}
		else
		{
			t.temaxinfolder=4999;
		}
		for ( t.e = 0 ; t.e<=  (t.temaxinfolder*2)+100; t.e++ )
		{
			t.tname_s=t.lightmapper.lmpath_s+Str(t.e)+".dds";
			if (  FileExist(t.tname_s.Get()) == 1 ) 
			{
				AddFileToBlock (  1, cstr(cstr("lightmaps\\")+Str(t.e)+".dds").Get() );
			}
		}
		t.tfurthestobjnumber=g.lightmappedobjectoffset;
		SetDir (  "lightmaps" );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  cstr(Lower(Right(t.tfile_s.Get(),4))) == ".dbo" ) 
				{
					t.tfile_s=Right(t.tfile_s.Get(),Len(t.tfile_s.Get())-Len("object"));
					t.tfile_s=Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-4);
					t.tfilevalue = ValF(t.tfile_s.Get()) ; if (  t.tfilevalue>t.tfurthestobjnumber  )  t.tfurthestobjnumber = t.tfilevalue;
				}
			}
		}
		SetDir (  ".." );
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj <= t.tfurthestobjnumber; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "lightmaps\\object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  AddFileToBlock (  1, cstr(cstr("lightmaps\\object")+Str(t.tobj)+".dbo").Get() );
		}
	}

	#ifdef VRTECH
	// ttsfiles files
	if ( PathExist("ttsfiles") == 1 ) 
	{
		SetDir ( "ttsfiles" );
		ChecklistForFiles (  );
		std::vector<cstr> ttsfileslist;
		ttsfileslist.clear();
		for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
		{
			t.tfile_s = ChecklistString(t.c);
			if ( t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if ( cstr(Lower(Right(t.tfile_s.Get(),4))) == ".txt" || cstr(Lower(Right(t.tfile_s.Get(),4))) == ".wav" || cstr(Lower(Right(t.tfile_s.Get(),4))) == ".lip") 
				{
					ttsfileslist.push_back(t.tfile_s);
				}
			}
		}
		SetDir ( ".." );
		for ( t.c = 0; t.c < ttsfileslist.size(); t.c++ )
		{
			AddFileToBlock ( 1, cstr(cstr("ttsfiles\\")+ttsfileslist[t.c]).Get() );
		}
	}
	#endif

	//  visual settings
	if (  t.tincludevisualsfile == 1  )  AddFileToBlock (  1, "visuals.ini" );

	//  conkit data
	if (  FileExist("conkit.dat")  )  AddFileToBlock (  1,"conkit.dat" );

	//  ebe files
	ChecklistForFiles (  );
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.tfile_s = ChecklistString(t.c);
		if ( t.tfile_s != "." && t.tfile_s != ".." ) 
		{
			cstr strEnt = cstr(Lower(Right(t.tfile_s.Get(),4)));
			if ( strcmp ( strEnt.Get(), ".ebe" ) == NULL )
			{
				AddFileToBlock ( 1, t.tfile_s.Get() );
				cstr tNameOnly = Left(t.tfile_s.Get(),strlen(t.tfile_s.Get())-4);
				cstr tThisFile = tNameOnly + cstr(".fpe");
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				#ifdef WICKEDENGINE
				// wicked saves DBOs
				tThisFile = tNameOnly + cstr(".dbo");
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				// wicked saves EBE texture DDSs (further below)
				#else
				#ifdef VRTECH
				tThisFile = tNameOnly + cstr(".x");
				#else
				tThisFile = tNameOnly + cstr(".dbo");
				#endif
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				tThisFile = tNameOnly + cstr(".bmp");
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				#ifdef VRTECH
				tThisFile = tNameOnly + cstr("_D.jpg");
				#else
				tThisFile = tNameOnly + cstr("_D.dds");
				#endif
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				#ifdef VRTECH
				 // Don't include large files until find a nice to way reduce them considerably (or find a faster way to transfer multiplayer FPM)
				#else
				 tThisFile = tNameOnly + cstr("_N.dds");
				 if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				 tThisFile = tNameOnly + cstr("_S.dds");
				 if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				#endif
				#endif
			}
			#ifdef WICKEDENGINE
			// see above
			#else
			strEnt = cstr(Lower(Right(t.tfile_s.Get(),6)));
			#ifdef VRTECH
			if ( strcmp ( strEnt.Get(), "_d.jpg" ) == NULL )
			#else
			if ( strcmp ( strEnt.Get(), "_d.dds" ) == NULL || strcmp ( strEnt.Get(), "_n.dds" ) == NULL || strcmp ( strEnt.Get(), "_s.dds" ) == NULL )
			#endif
			{
				AddFileToBlock ( 1, t.tfile_s.Get() );
			}
			#ifdef VRTECH
			 // Don't include large files until find a nice to way reduce them considerably (or find a faster way to transfer multiplayer FPM)
			#else
			 strEnt = cstr(Lower(Right(t.tfile_s.Get(),6)));
			 if ( strcmp ( strEnt.Get(), "_n.dds" ) == NULL || strcmp ( strEnt.Get(), "_s.dds" ) == NULL )
			 {
				AddFileToBlock ( 1, t.tfile_s.Get() );
			 }
			#endif
			#endif
		}
	}

	#ifdef WICKEDENGINE
	// cannot just discover EBE textures, so go through all entity parents for all EBEs and save their textures
	for ( int iEntID = 1; iEntID <= g.entidmaster; iEntID++ )
	{
		if (strlen(t.entitybank_s[iEntID].Get()) > 0)
		{
			if (t.entityprofile[iEntID].ebe.dwRLESize > 0)
			{	
				char pJustNameOfTex[MAX_PATH];
				strcpy(pJustNameOfTex, t.entityprofile[iEntID].texd_s.Get());
				pJustNameOfTex[strlen(pJustNameOfTex) - strlen("_color.dds")] = 0;
				cstr tThisFile = cstr(pJustNameOfTex) + "_color.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				tThisFile = cstr(pJustNameOfTex) + "_normal.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
				tThisFile = cstr(pJustNameOfTex) + "_surface.dds";
				if ( FileExist(tThisFile.Get()) ) AddFileToBlock ( 1, tThisFile.Get() );
			}
		}
	}
	#endif

	/* g_bTerrainGeneratorChooseRealTerrain no longer used
	#ifdef WICKEDENGINE
	LPSTR pTerrainPreference = "TerrainPreference.tmp";
	if (FileExist(pTerrainPreference) == 1) DeleteFileA(pTerrainPreference);
	extern bool g_bTerrainGeneratorChooseRealTerrain;
	OpenToWrite(3, pTerrainPreference);
	if (g_bTerrainGeneratorChooseRealTerrain == true)
	{
		WriteString(3, "terrainisgrass");
	}
	else
	{
		WriteString(3, "terrainisgrid");
	}
	CloseFile(3);
	AddFileToBlock (1, pTerrainPreference);
	#endif
	*/

	SetDir ( pOldDir );
	SaveFileBlock ( 1 );

	#ifdef WICKEDENGINE
	// collect ALL entity profile files
	g.filecollectionmax = 0;
	Undim (t.filecollection_s);
	Dim (t.filecollection_s, 500);
	void addthisentityprofilesfilestocollection (void);
	for (int entid = 1; entid <= g.entidmaster; entid++)
	{
		if (strlen(t.entitybank_s[entid].Get()) > 0)
		{
			t.e = 0; t.entid = entid;
			addthisentityprofilesfilestocollection ();
		}
	}
	// create an itinery LST file (so auto updater can find and download dependent files)
	cstr LSTFile_s = cstr(Left(g.projectfilename_s.Get(), Len(g.projectfilename_s.Get()) - 4)) + ".lst";
	if (FileExist(LSTFile_s.Get()) == 1) DeleteAFile (LSTFile_s.Get());
	std::vector <cstr> lstlist_s;
	Dim (lstlist_s, g.filecollectionmax);
	int iListIndex = 0;
	for (int i = 0; i < g.filecollectionmax; i++)
	{
		LPSTR pThisFile = t.filecollection_s[i].Get();
		int iThisSize = strlen (pThisFile);
		if (iThisSize  > 0)
		{
			// must have a file specified
			if (pThisFile[iThisSize - 1] == '\\' || pThisFile[iThisSize - 1] == '/')
			{
				// ignore folders
			}
			else
			{
				if (FileExist(pThisFile) == 1)
				{
					lstlist_s[iListIndex++] = pThisFile;
				}
			}
		}
	}
	SaveArray (LSTFile_s.Get(), lstlist_s);
	UnDim (lstlist_s);
	#endif

	//  does crazy cool stuff
	t.tsteamsavefilename_s = t.ttempprojfilename_s;
	#ifdef VRTECH
	//mp_save_workshop_files_needed ( ); // no longer needed, not using workshop
	#else
	mp_save_workshop_files_needed ( ); // no longer needed, not using workshop
	#endif

	//  log prompts
	timestampactivity(0,"Saving FPM level file complete");
}

void mapfile_emptyebesfromtestmapfolder(bool bIgnoreValidTextureFiles)
{
	ChecklistForFiles();
	for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
	{
		t.tfile_s = ChecklistString(t.c);
		if (t.tfile_s != "." && t.tfile_s != "..")
		{
			// only if not a CUSTOM content piece
#ifdef VRTECH
			if (strnicmp(t.tfile_s.Get(), "CUSTOM_", 7) != NULL)
#else
			if (1)
#endif
			{
				cstr strEnt = cstr(Lower(Right(t.tfile_s.Get(), 4)));
				if (stricmp(strEnt.Get(), ".ebe") == NULL || stricmp(strEnt.Get(), ".fpe") == NULL)
				{
					DeleteAFile(t.tfile_s.Get());
					cstr tNameOnly = Left(t.tfile_s.Get(), strlen(t.tfile_s.Get()) - 4);
					cstr tThisFile = tNameOnly + cstr(".fpe");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr(".dbo");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
#ifdef VRTECH
					tThisFile = tNameOnly + cstr(".x");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
#endif
					tThisFile = tNameOnly + cstr(".bmp");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr("_D.dds");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr("_N.dds");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr("_S.dds");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
#ifdef VRTECH
					tThisFile = tNameOnly + cstr("_D.jpg");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr("_N.jpg");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
					tThisFile = tNameOnly + cstr("_S.jpg");
					if (FileExist(tThisFile.Get()) == 1) DeleteAFile(tThisFile.Get());
#endif
				}
				strEnt = cstr(Lower(Right(t.tfile_s.Get(), 6)));
				if (bIgnoreValidTextureFiles == false)
				{
#ifdef VRTECH
					if (strcmp(strEnt.Get(), "_d.dds") == NULL || strcmp(strEnt.Get(), "_n.dds") == NULL || strcmp(strEnt.Get(), "_s.dds") == NULL
						|| strcmp(strEnt.Get(), "_d.jpg") == NULL || strcmp(strEnt.Get(), "_n.jpg") == NULL || strcmp(strEnt.Get(), "_s.jpg") == NULL)
					{
						DeleteAFile(t.tfile_s.Get());
					}
#else
					if (strcmp(strEnt.Get(), "_d.dds") == NULL || strcmp(strEnt.Get(), "_n.dds") == NULL || strcmp(strEnt.Get(), "_s.dds") == NULL)
					{
						if (stricmp(t.tfile_s.Get(), "Texture_D.dds") != NULL && stricmp(t.tfile_s.Get(), "Texture_N.dds") != NULL)
						{
							DeleteAFile(t.tfile_s.Get());
						}
					}
#endif
				}
				}
		}
	}
}

void mapfile_emptyterrainfilesfromtestmapfolder ( void )
{
	// also makes sense to empty terrain files too as we are clearing this folder for new activity
	#ifdef WICKEDENGINE
	// move to terrain node folder location
	LPSTR pOldDir = GetDir();
	char pRealWritableArea[MAX_PATH];
	strcpy(pRealWritableArea, pOldDir);
	strcat(pRealWritableArea, "\\levelbank\\testmap\\");
	GG_GetRealPath(pRealWritableArea, 1);
	SetDir(pRealWritableArea);
	// all node folders (containing terrain geometry and virtual textures)
	ChecklistForFiles (  );
	std::vector<std::string> terrainNodeFolders;
	terrainNodeFolders.clear();
	for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
	{
		t.tfile_s = ChecklistString(t.c);
		if ( t.tfile_s != "." && t.tfile_s != ".." && t.tfile_s != "ttsfiles" ) 
		{
			if (ChecklistValueA(t.c) == 1)
			{
				if (cstr(Lower(Left(t.tfile_s.Get(), 2))) == "tt")
				{
					// found terrain node folder
					terrainNodeFolders.push_back(t.tfile_s.Get());
				}
			}
		}
	}
	for (int tt = 0; tt < terrainNodeFolders.size(); tt++)
	{
		LPSTR pTerrainNodeFolder = (LPSTR)terrainNodeFolders[tt].c_str();
		SetDir(pTerrainNodeFolder);
		ChecklistForFiles();
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			if (t.tfile_s != "." && t.tfile_s != "..")
			{
				DeleteAFile ( t.tfile_s.Get() );
			}
		}
		SetDir("..");
		RemoveDirectoryA(pTerrainNodeFolder);
	}
	SetDir(pOldDir);
	#endif
}

#ifdef WICKEDENGINE
void lm_emptylightmapandttsfilesfolder_wicked( void )
{
	//PE: lightmaps
	LPSTR pOldDir = GetDir();
	char pRealWritableArea[MAX_PATH];
	strcpy(pRealWritableArea, pOldDir);
	if( pestrcasestr(pOldDir,"\\testmap"))
		strcat(pRealWritableArea, "\\lightmaps");
	else
		strcat(pRealWritableArea, "\\levelbank\\testmap\\lightmaps");
	GG_GetRealPath(pRealWritableArea, 1);
	if (PathExist(pRealWritableArea))
	{
		SetDir(pRealWritableArea);
		ChecklistForFiles();
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			if (t.tfile_s != "." && t.tfile_s != "..")
			{
				if (FileExist(t.tfile_s.Get()) == 1)  DeleteAFile(t.tfile_s.Get());
			}
		}
		SetDir(pOldDir);
		RemoveDirectoryA(pRealWritableArea);
	}
	//PE: ttsfiles
	strcpy(pRealWritableArea, pOldDir);
	if (pestrcasestr(pOldDir, "\\testmap"))
		strcat(pRealWritableArea, "\\ttsfiles");
	else
		strcat(pRealWritableArea, "\\levelbank\\testmap\\ttsfiles");
	GG_GetRealPath(pRealWritableArea, 1);
	if (PathExist(pRealWritableArea))
	{
		SetDir(pRealWritableArea);
		ChecklistForFiles();
		for (t.c = 1; t.c <= ChecklistQuantity(); t.c++)
		{
			t.tfile_s = ChecklistString(t.c);
			if (t.tfile_s != "." && t.tfile_s != "..")
			{
				if (FileExist(t.tfile_s.Get()) == 1)  DeleteAFile(t.tfile_s.Get());
			}
		}
		SetDir(pOldDir);
		RemoveDirectoryA(pRealWritableArea);
	}
}
#endif

void mapfile_loadproject_fpm ( void )
{

	//PE: Deselect any objects, if we load a level with less entityties then t.widget.pickedEntityIndex it can crash.
	t.widget.pickedEntityIndex = 0;
	t.gridentity = 0;

	// can do extra steps when load in FPM
	bool bThisIsTheNewTerrainSystem = false;

	//  Ensure FPM exists
	t.trerfeshvisualsassets=0;
	timestampactivity(0, cstr(cstr("_mapfile_loadproject_fpm: ")+g.projectfilename_s+" "+GetDir()).Get() );
	if ( FileExist(g.projectfilename_s.Get()) == 1 ) 
	{
		//  Empty the lightmap folder
		timestampactivity(0,"LOADMAP: lm_emptylightmapandttsfilesfolder");
		lm_emptylightmapandttsfilesfolder ( );

		#ifdef WICKEDENGINE
		lm_emptylightmapandttsfilesfolder_wicked();
		#endif

		// empty any terrain node files
		mapfile_emptyterrainfilesfromtestmapfolder();

		//  Store and switch folders
		t.tdirst_s=GetDir() ; SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );

		// Delete key testmap file (if any)
		if ( FileExist("header.dat") == 1 ) DeleteAFile ( "header.dat" );
		if ( FileExist("playerconfig.dat") == 1 ) DeleteAFile ( "playerconfig.dat" );
		if ( FileExist("watermask.dds") == 1 ) DeleteAFile ( "watermask.dds" );
		if ( FileExist("watermask.png") == 1 ) DeleteAFile ( "watermask.png" );
		if ( FileExist("vegmask.png") == 1) DeleteAFile( "vegmask.png"); //PE: If we switch from a new fpm to a old only with .dds, old need to be removed.
		if ( FileExist("visuals.ini") == 1 ) DeleteAFile ( "visuals.ini" );
		if ( FileExist("conkit.dat") == 1 ) DeleteAFile ( "conkit.dat" );
		if ( FileExist("map.obs") == 1 ) DeleteAFile ( "map.obs" );
		if ( FileExist("locked.cfg") == 1) DeleteAFile("locked.cfg");

		#ifdef WICKEDENGINE
		if (FileExist("cfg.cfg") == 1) DeleteAFile("cfg.cfg");

		// new terrain system loads its data settings in this file (see below)
		if (FileExist("ggterrain.dat") == 1) DeleteAFile("ggterrain.dat");
		uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
		cstr sculpt_data_name = cstr( (int) terrain_sculpt_size) + cstr(".dat");
		if (FileExist(sculpt_data_name.Get()) == 1) DeleteAFile(sculpt_data_name.Get());

		uint32_t terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
		cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");
		if (FileExist(paint_data_name.Get()) == 1) DeleteAFile(paint_data_name.Get());

		uint32_t tree_data_size = GGTrees::GGTrees_GetDataSize() * 4;
		cstr tree_data_name = cstr((int)tree_data_size) + cstr(".tre");
		if (FileExist(tree_data_name.Get()) == 1) DeleteAFile(tree_data_name.Get());

		cstr old_tree_data_name = "4800000.tre"; //PE: Tree format 1.0. Make sure it deleted.
		if (FileExist(old_tree_data_name.Get()) == 1) DeleteAFile(old_tree_data_name.Get());


		uint32_t grass_data_size = GGGrass::GGGrass_GetDataSize();
		cstr grass_data_name = cstr((int)grass_data_size) + cstr(".gra");
		if (FileExist(grass_data_name.Get()) == 1) DeleteAFile(grass_data_name.Get());

		if (FileExist("heightmapdata.raw") == 1) DeleteAFile("heightmapdata.raw");

		#else
		//  Delete terrain texture files (if any)
		if ( FileExist("Texture_D.dds") == 1 ) DeleteAFile ( "Texture_D.dds" );
		if ( FileExist("Texture_N.dds") == 1 ) DeleteAFile ( "Texture_N.dds" );
		#ifdef VRTECH
		if ( FileExist("Texture_D.jpg") == 1 ) DeleteAFile ( "Texture_D.jpg" );
		if ( FileExist("Texture_N.jpg") == 1 ) DeleteAFile ( "Texture_N.jpg" );
		#endif
		#endif

		//  Delete env map for PBR (if any)
		if ( FileExist("globalenvmap.dds") == 1 ) DeleteAFile ( "globalenvmap.dds" );

		#ifdef WICKEDENGINE
		LPSTR pTerrainPreference = "TerrainPreference.tmp";
		if (FileExist(pTerrainPreference) == 1) DeleteFileA(pTerrainPreference);
		#endif

		// empty any ebe files
		mapfile_emptyebesfromtestmapfolder(false);

		//  Restore folder to Files (for extraction)
		SetDir ( t.tdirst_s.Get() );

		//  Read FPM into testmap area
		timestampactivity(0,"LOADMAP: read FPM block");
		t.tpath_s=g.mysystem.levelBankTestMap_s.Get(); //"levelbank\\testmap\\";

		if ( !ExtractZipThread::IsSetup() )
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo( &sysinfo );
			uint32_t numThreads = sysinfo.dwNumberOfProcessors;
			if ( numThreads > 3 ) numThreads--;
			ExtractZipThread::SetThreads( numThreads );
		}

		OpenFileBlock (  g.projectfilename_s.Get(),1,"mypassword" );
		uint32_t numFiles = GetFileBlockNumFiles( 1 );
		const char* const* pAllFiles = GetFileBlockAllFileNames( 1 );
		const unsigned long* pAllSizes = GetFileBlockAllFileSizes( 1 );
		ExtractZipThread::SetWork( g.projectfilename_s.Get(), t.tpath_s.Get(), pAllFiles, pAllSizes, numFiles );
		ExtractZipThread::StartThreads();
		ExtractZipThread::WaitForAll();
		CloseFileBlock( 1 ); // must remain open until all work completed for pAllFiles to remain valid

		SetDir ( g.mysystem.levelBankTestMapAbs_s.Get() );

		/* g_bTerrainGeneratorChooseRealTerrain no longer used
		#ifdef WICKEDENGINE
		extern bool g_bTerrainGeneratorChooseRealTerrain;
		if (FileExist(pTerrainPreference) == 1)
		{
			OpenToRead(3, pTerrainPreference);
			LPSTR pTerrainPref = ReadString(3);
			if (strcmp (pTerrainPref, "terrainisgrass") == NULL)
				g_bTerrainGeneratorChooseRealTerrain = true;
			else
				g_bTerrainGeneratorChooseRealTerrain = false;
			CloseFile(3);
		}
		else
		{
			// if no file exists, assume 'real terrain'
			g_bTerrainGeneratorChooseRealTerrain = true;
		}
		#endif
		*/

		//  If file still not present, extraction failed
		if (  FileExist("header.dat") == 0 ) 
		{
			//  inform user the FPM could not be loaded (corrupt file)
			t.tloadsuccessfully=0;
		}

		// If file still not present, extraction failed
		SetDir ( g.mysystem.levelBankTestMapAbs_s.Get() );		
		#ifdef VRTECH
		if ( g.memskipwatermask == 0 && (FileExist("watermask.dds") == 0 || FileExist("watermask.png") == 0) )
		#else
		if ( g.memskipwatermask == 0  && FileExist("watermask.dds") == 0 )
		#endif
		{
			//  Only Reloaded Formats have this texture file, so fail load if not there (Classic FPM)
			t.tloadsuccessfully=2;
		}

		#ifdef WICKEDENGINE
		// new terrain system loads its data settings
		//PE: We are already inside "levelbank\\testmap\\" ?
		//cstr TerrainDataFile_s = g.mysystem.levelBankTestMap_s + "ggterrain.dat";
		//PE: In editor g.mysystem.levelBankTestMap_s have "c:" and works, in standalone we dont have that so:
		cstr TerrainDataFile_s = "ggterrain.dat";
		g_bNeedToConvertClassicPositionsToMAX = false;
		if (FileExist(TerrainDataFile_s.Get()) == 0) g_bNeedToConvertClassicPositionsToMAX = true;
		GGTerrainFile_LoadTerrainData(TerrainDataFile_s.Get(),false);
		extern bool bTreeGlobalInit;
		bTreeGlobalInit = false;

		//PE: Restore Sculpt Data.
		terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();
		char *data = new char[terrain_sculpt_size];
		if (data)
		{
			cstr sculpt_data_name = cstr((int)terrain_sculpt_size) + cstr(".dat");
			if (FileExist(sculpt_data_name.Get()) == 1)
			{
				char FilenameString[_MAX_PATH];
				strcpy(FilenameString, sculpt_data_name.Get());
				if (DB_FileExist(FilenameString))
				{
					// Open file to be read
					HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hreadfile != INVALID_HANDLE_VALUE)
					{
						// Read file into memory
						DWORD bytesread;
						ReadFile(hreadfile, data, terrain_sculpt_size, &bytesread, NULL);
						CloseHandle(hreadfile);
						GGTerrain::GGTerrain_SetSculptData(terrain_sculpt_size, (uint8_t*) data);
					}
				}
			}
			delete(data);
		}

		void check_new_terrain_parameters(void);
		check_new_terrain_parameters();

		void reset_terrain_paint_date(void);
		reset_terrain_paint_date(); //PE: Reset old paint textures, after this. try loading from saved fpm.

		//PE: Restore Paint Texture Data.
		terrain_paint_size = GGTerrain::GGTerrain_GetPaintDataSize();
		data = new char[terrain_paint_size];
		if (data)
		{
			cstr paint_data_name = cstr((int)terrain_paint_size) + cstr(".ptd");
			if (FileExist(paint_data_name.Get()) == 1)
			{
				char FilenameString[_MAX_PATH];
				strcpy(FilenameString, paint_data_name.Get());
				if (DB_FileExist(FilenameString))
				{
					// Open file to be read
					HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hreadfile != INVALID_HANDLE_VALUE)
					{
						// Read file into memory
						DWORD bytesread;
						ReadFile(hreadfile, data, terrain_paint_size, &bytesread, NULL);
						CloseHandle(hreadfile);
						GGTerrain::GGTerrain_SetPaintData(terrain_paint_size, (uint8_t*)data);
						//PE: We already made a delay invalidate region so all fine...
					}
				}
			}
			delete(data);
		}

		//PE: Default hide all tree's.
		GGTrees::GGTrees_HideAll();

		//PE: Restore Tree Data.
		tree_data_size = GGTrees::GGTrees_GetDataSize() * 4;
		data = new char[tree_data_size];
		if (data)
		{
			cstr tree_data_name = cstr((int)tree_data_size) + cstr(".tre");
			cstr old_tree_data_name = "4800000.tre"; //PE: Tree format 1.0
			bool bConvertOldFormat = false;
			if (FileExist(old_tree_data_name.Get()) == 1)
			{
				bConvertOldFormat = true;
			}
			if (FileExist(tree_data_name.Get()) == 1 || bConvertOldFormat)
			{
				char FilenameString[_MAX_PATH];
				strcpy(FilenameString, tree_data_name.Get());
				if (bConvertOldFormat)
				{
					strcpy(FilenameString, old_tree_data_name.Get());
					uint32_t* dataInt = (uint32_t*)data;
					dataInt[0] = 2; //PE: New version
					data += 4;
					tree_data_size -= 4;
				}
				if (DB_FileExist(FilenameString))
				{
					// Open file to be read
					HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hreadfile != INVALID_HANDLE_VALUE)
					{
						// Read file into memory
						DWORD bytesread;
						ReadFile(hreadfile, data, tree_data_size, &bytesread, NULL);
						CloseHandle(hreadfile);
						if (!bConvertOldFormat)
						{
							//PE: Skip conversion from 2.0 to 3.0 as the 2.0 using 3.0 data have been out for some time now. and people would already have saved 3.0 trees with 2.0 version.
							//PE: Mainly so people will not have to correct the trees again. No way to see if 3.0 data has been saved as 2.0 now.
							uint32_t* dataInt = (uint32_t*)data;
							if (dataInt[0] == 2) dataInt[0] = 3;
						}
						if (bConvertOldFormat)
						{
							int size = GGTrees::GGTrees_GetDataSize();
							size--; //PE: Remove version.
							uint32_t* dataInt = (uint32_t*)data;
							int scale = 85; //PE: 85=1.0 hlsl: GetTreeScale( uint data ) { return ((data >> 16) & 0xFF) / 170.0 + 0.5; }

							for (int i = 0; i < size; i += 3 )
							{
								//PE: Convert old tree data to new 2.0 format.
								uint32_t data = dataInt[2 + i];
								uint32_t type = (data & 0xF);
								uint32_t visible = (data & 0x100);
								uint32_t id = (data >> 10);
								uint32_t scaleindex = (data >> 4) & 0x7; //hlsl: uint index = (IN.data >> 4) & 0x7;
								float randScale = (scaleindex / 7.0 ) * 0.25 + 0.75; //hlsl. old scale.
								int newscale = (int)((float) (57 + (scaleindex*4.0)) * randScale);
								type &= 0x1F;
								uint32_t varIndex = id & 0x7;
								data = (type << 11) | (varIndex << 8);
								if (visible) data |= 0x1;
								else data &= ~0x1;
								dataInt[2 + i] = data;
								dataInt[2 + i] = (dataInt[2 + i] & 0xFF00FFFF) | (newscale << 16);
							}
							data -= 4;
						}
						GGTrees::GGTrees_SetData((float*)data);
					}
					else if (bConvertOldFormat)
					{
						data -= 4;
					}
				}
				else if (bConvertOldFormat)
				{
					data -= 4;
				}
			}
			delete(data);
		}


		//PE: Restore grass Data.
		grass_data_size = GGGrass::GGGrass_GetDataSize();
		data = new char[grass_data_size];
		if (data)
		{
			cstr grass_data_name = cstr((int)grass_data_size) + cstr(".gra");
			if (FileExist(grass_data_name.Get()) == 1)
			{
				char FilenameString[_MAX_PATH];
				strcpy(FilenameString, grass_data_name.Get());
				if (DB_FileExist(FilenameString))
				{
					// Open file to be read
					HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hreadfile != INVALID_HANDLE_VALUE)
					{
						// Read file into memory
						DWORD bytesread;
						ReadFile(hreadfile, data, grass_data_size, &bytesread, NULL);
						CloseHandle(hreadfile);
						GGGrass::GGGrass_SetData(grass_data_size,(uint8_t*)data);
					}
				}
			}
			delete(data);
		}

		#endif

		//  load in visuals from loaded file
		timestampactivity(0,"LOADMAP: load in visuals");
		if (  FileExist("visuals.ini") == 1 ) 
		{
			t.tstorefpscrootdir_s=g.fpscrootdir_s;
			g.fpscrootdir_s="" ; visuals_load ( );
			g.fpscrootdir_s=t.tstorefpscrootdir_s;
			t.trerfeshvisualsassets=1;

			//  Ensure visuals settings are copied to gamevisuals (the true destination)
			t.gamevisuals=t.visuals;
			t.editorvisuals=t.visuals;

			//  And ensure editor visuals mimic required settings from loaded data
			visuals_editordefaults ( );
			t.visuals=t.editorvisuals;
			t.visuals.skyindex=t.gamevisuals.skyindex;
			t.visuals.sky_s=t.gamevisuals.sky_s;
			t.visuals.terrainindex=t.gamevisuals.terrainindex;
			t.visuals.terrain_s=t.gamevisuals.terrain_s;
			t.visuals.vegetationindex=t.gamevisuals.vegetationindex;
			t.visuals.vegetation_s=t.gamevisuals.vegetation_s;

			//  Re-acquire indices now the lists have changed
			//  takes visuals.sky$ visuals.terrain$ visuals.vegetation$
			visuals_updateskyterrainvegindex ( );
			t.gamevisuals.skyindex=t.visuals.skyindex;
			t.gamevisuals.terrainindex=t.visuals.terrainindex;
			t.gamevisuals.vegetationindex=t.visuals.vegetationindex;
		}

#ifdef WICKEDENGINE

		uint32_t iHeightmapSize = 0, iHeightmapWidth = 0, iHeightmapHeight = 0;
		iHeightmapWidth = t.gamevisuals.iHeightmapWidth;
		iHeightmapHeight = t.gamevisuals.iHeightmapHeight;
		iHeightmapSize = iHeightmapWidth * iHeightmapHeight * sizeof(uint16_t);

		//PE: Load heightmap data if any.
		if (iHeightmapSize > 0 && iHeightmapWidth > 0 && iHeightmapHeight > 0)
		{
			data = new char[iHeightmapSize];
			if (data)
			{
				cstr heightmap_data_name = "heightmapdata.raw";
				if (FileExist(heightmap_data_name.Get()) == 1)
				{
					char FilenameString[_MAX_PATH];
					strcpy(FilenameString, heightmap_data_name.Get());
					if (DB_FileExist(FilenameString))
					{
						// Open file to be read
						HANDLE hreadfile = GG_CreateFile(FilenameString, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if (hreadfile != INVALID_HANDLE_VALUE)
						{
							// Read file into memory
							DWORD bytesread;
							ReadFile(hreadfile, data, iHeightmapSize, &bytesread, NULL);
							CloseHandle(hreadfile);
							GGTerrain::GGTerrain_SetHeightmapData((uint16_t*) data, iHeightmapWidth, iHeightmapHeight);
						}
					}
				}
				delete(data);
			}
		}

#endif

		//  if MAP OBS exists, we are not generating OBS data this time
		t.aisystem.generateobs=1;
		t.tobsfile_s="map.obs";
		if (  FileExist(t.tobsfile_s.Get()) == 1 ) 
		{
			t.aisystem.generateobs=0;
		}

		//  if CFG file present, copy to editor folder for later use (stores FPG for us)
		if (  t.game.runasmultiplayer == 0 ) 
		{
			//  single player/editor only - not needed when loading multiplayer map
			if (  t.tloadsuccessfully == 1 ) 
			{
				t.tttfile_s="cfg.cfg";
				if (  FileExist(t.tttfile_s.Get()) == 1 ) 
				{
					cstr cfgfile_s = g.mysystem.editorsGrideditAbs_s + t.tttfile_s;
					if ( FileExist( cfgfile_s.Get() ) == 1  )  DeleteAFile ( cfgfile_s.Get() );
					CopyAFile ( t.tttfile_s.Get(), cfgfile_s.Get() );
				}
				#ifdef WICKEDENGINE
				//PE: Load locked objects.
				extern std::vector<sRubberBandType> vEntityLockedList;
				vEntityLockedList.clear();
				if (FileExist("locked.cfg") == 1)
				{
					OpenToRead(1, "locked.cfg");
					int iAll = ReadLong(1);
					for (int i = 0; i < iAll; i++)
					{
						int e = ReadLong(1);
						sRubberBandType vEntityLockedItem;
						vEntityLockedItem.e = e;
						vEntityLockedList.push_back(vEntityLockedItem);
					}
					CloseFile(1);
				}
				#endif

			}
		}

		// Retore and switch folders
		SetDir ( t.tdirst_s.Get() );

		//  if visuals file present, apply it
		timestampactivity(0,"LOADMAP: apply visuals");
		if (  t.trerfeshvisualsassets == 1 ) 
		{
			//  if loading from game (level load), ensure it's the game visuals we use
			if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1  )  t.visuals = t.gamevisuals;
			//  and refresh assets based on restore
			t.visuals.refreshshaders=1;
			t.visuals.refreshterraintexture=1;
			t.visuals.refreshvegtexture=1;

			#ifdef WICKEDENGINE
			//PE: Remember sun angle.
			float oSx = t.visuals.SunAngleX;
			float oSy = t.visuals.SunAngleY;
			float oSz = t.visuals.SunAngleZ;

			//PE: Special wicked setup so retain visuals.
			t.visuals.refreshskysettings = 0;
			g.skyindex = t.visuals.skyindex; if (g.skyindex > g.skymax)  g.skyindex = g.skymax;
			t.visuals.sky_s = t.skybank_s[g.skyindex];
			t.terrainskyspecinitmode = 0;
			sky_skyspec_init( false );
			t.sky.currenthour_f = 8.0;
			t.sky.daynightprogress = 0;
			t.sky.changingsky = 1;

			// if change sky, regenerate env map
			cubemap_generateglobalenvmap();

			visuals_loop();

			#else
			visuals_loop ( );
			visuals_shaderlevels_update ( );
			#endif

			#ifdef WICKEDENGINE
			//PE: In wicked we want to restore the sun angle from the map and not use skyspec.ini settings. (only when loading a old level).
			if (t.visuals.skyindex == 0 || t.visuals.bDisableSkybox)
			{
				//PE: Only if we re not using a simple skybox.
				t.visuals.SunAngleX = oSx;
				t.visuals.SunAngleY = oSy;
				t.visuals.SunAngleZ = oSz;
			}
			extern void Wicked_Update_Visuals(void *voidvisual);
			Wicked_Update_Visuals((void*) &t.visuals );
			#endif

		}
	}
	else
	{
		t.tloadsuccessfully=0;
	}

	#ifdef WICKEDENGINE
	// when finished loading an OLD LEVEL FROM CLASSIC, need to copy and convert assets to make it a MAX level
	if ( t.tloadsuccessfully != 0 && g_bAllowBackwardCompatibleConversion == true)
	{
		// this ensures when level files acted on, they can be used in MAX
		mapfile_convertCLASSICtoMAX(g.projectfilename_s.Get());
	}
	#endif
}

void mapfile_newmap ( void )
{
	//  Defaults
	t.layermax=20 ; t.maxx=500 ; t.maxy=500;
	t.olaylistmax=100;

	// when new map called, empty all terrain files 
	mapfile_emptyterrainfilesfromtestmapfolder();
}

void mapfile_loadmap ( void )
{
	// Load header data (need main mapdata for visdata)
	t.filename_s=t.levelmapptah_s+"header.dat";
	if (  FileExist(t.filename_s.Get()) == 1 ) 
	{
		// Header - version number
		OpenToRead (  1,t.filename_s.Get() );
		t.versionmajor = ReadLong ( 1 );
		t.versionminor = ReadLong ( 1 );
		CloseFile (  1 );
	}

	// 080917 - if old header, delete map.obj as it contains corrupt waypoint data
	if ( t.versionmajor < 1 )
	{
		LPSTR pOldDir = GetDir();
		//LPSTR pObstacleWaypointData = "levelbank\\testmap\\map.obs";
		//if ( FileExist ( pObstacleWaypointData ) == 1 ) DeleteFile ( pObstacleWaypointData );
		cstr obstacleWaypointData_s = g.mysystem.levelBankTestMap_s+"map.obs";
		if ( FileExist ( obstacleWaypointData_s.Get() ) == 1 ) DeleteFileA ( obstacleWaypointData_s.Get() );
	}
}

void mapfile_savemap ( void )
{
	// Store old folder
	t.old_s=GetDir();

	// Enter folder
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); //"levelbank\\testmap\\" );

	// Clear old files out (TEMP)
	if (  FileExist("header.dat") == 1  )  DeleteAFile (  "header.dat" );

	// Create header file
	OpenToWrite (  1,"header.dat" );

	// Version 0.0 = Reloaded
	// Version 1.0 = GameGuru DX11 (new obstacle data save fixes)
	t.versionmajor = 1; WriteLong ( 1, t.versionmajor );
	t.versionminor = 0; WriteLong ( 1, t.versionminor );

	// end of header
	CloseFile (  1 );

	// Restore
	SetDir (  t.old_s.Get() );
}

void mapfile_loadplayerconfig ( void )
{
	//  Load player settings
	t.filename_s=t.levelmapptah_s+"playerconfig.dat";
	if (  FileExist(t.filename_s.Get()) == 1 ) 
	{
		//  Reloaded Header
		OpenToRead (  1,t.filename_s.Get() );

		//  verion header
		t.tmp_s = ReadString ( 1 );
		t.tversion = ReadLong ( 1 );

		//  player settings
		if (  t.tversion >= 201006 ) 
		{
			t.a_f = ReadFloat ( 1 ); t.playercontrol.jumpmax_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.gravity_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.fallspeed_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.climbangle_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.footfallpace_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.wobblespeed_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.wobbleheight_f=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.accel_f=t.a_f;
		}

		//  extra player settings
		if (  t.tversion >= 20100651 ) 
		{
			t.a_f = ReadFloat ( 1 ); t.playercontrol.regenrate=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.regenspeed=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.regendelay=t.a_f;
		}

		//  extra player settings for third person (V1.01.002)
		if (  t.tversion >= 20100652 ) 
		{
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.enabled=t.a;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.startmarkere=t.a;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.charactere=t.a;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.thirdperson.cameradistance=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.thirdperson.cameraheight=t.a_f;
			t.a_f = ReadFloat ( 1 ); t.playercontrol.thirdperson.cameraspeed=t.a_f;
		}
		if (  t.tversion >= 20100653 ) 
		{
			t.a_f = ReadFloat ( 1 ); t.playercontrol.thirdperson.camerafocus=t.a_f;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.cameralocked=t.a;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.camerashoulder=t.a;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.camerafollow=t.a;
			t.a = ReadLong ( 1 ); t.playercontrol.thirdperson.camerareticle=t.a;
		}

		CloseFile (  1 );
	}
}

void mapfile_saveplayerconfig ( void )
{
	//  Store old folder
	t.old_s=GetDir();

	//  Enter folder
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\" );

	//  Version for player config has minor value (between betas)
	t.gtweakversion=20100653;

	//  WriteLong (  out )
	if (  FileExist("playerconfig.dat") == 1  )  DeleteAFile (  "playerconfig.dat" );
	OpenToWrite (  1,"playerconfig.dat" );

	//  verion header
	WriteString (  1,"version" );
	WriteLong (  1,t.gtweakversion );

	//  player settings
	WriteFloat (  1,t.playercontrol.jumpmax_f );
	WriteFloat (  1,t.playercontrol.gravity_f );
	WriteFloat (  1,t.playercontrol.fallspeed_f );
	WriteFloat (  1,t.playercontrol.climbangle_f );
	WriteFloat (  1,t.playercontrol.footfallpace_f );
	WriteFloat (  1,t.playercontrol.wobblespeed_f );
	WriteFloat (  1,t.playercontrol.wobbleheight_f );
	WriteFloat (  1,t.playercontrol.accel_f );

	//  extra settings from V1.0065 (20100651)
	WriteFloat (  1,t.playercontrol.regenrate );
	WriteFloat (  1,t.playercontrol.regenspeed );
	WriteFloat (  1,t.playercontrol.regendelay );

	//  extra settings from V1.01.030 (20100652)
	WriteLong (  1,t.playercontrol.thirdperson.enabled );
	WriteLong (  1,t.playercontrol.thirdperson.startmarkere );
	WriteLong (  1,t.playercontrol.thirdperson.charactere );
	WriteFloat (  1,t.playercontrol.thirdperson.cameradistance );
	WriteFloat (  1,t.playercontrol.thirdperson.cameraheight );
	WriteFloat (  1,t.playercontrol.thirdperson.cameraspeed );

	//  20100653 additions
	WriteFloat (  1,t.playercontrol.thirdperson.camerafocus );
	WriteLong (  1,t.playercontrol.thirdperson.cameralocked );
	WriteLong (  1,t.playercontrol.thirdperson.camerashoulder );
	WriteLong (  1,t.playercontrol.thirdperson.camerafollow );
	WriteLong (  1,t.playercontrol.thirdperson.camerareticle );

	CloseFile (  1 );

	//  Restore
	SetDir (  t.old_s.Get() );
}

#ifdef VRTECH
void addthisentityprofilesfilestocollection ( void )
{
	// takes t.entid 
	// entity profile file
	t.tentityname1_s = cstr("entitybank\\") + t.entitybank_s[t.entid];
	t.tentityname2_s = cstr(Left(t.tentityname1_s.Get(), Len(t.tentityname1_s.Get()) - 4)) + ".bin";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tentityname2_s).Get()) == 1)
	{
		t.tentityname_s = t.tentityname2_s;
	}
	else
	{
		t.tentityname_s = t.tentityname1_s;
	}
	addtocollection(t.tentityname_s.Get());

	//  entity files in folder
	t.tentityfolder_s = t.tentityname_s;
	for (t.n = Len(t.tentityname_s.Get()); t.n >= 1; t.n += -1)
	{
		if (cstr(Mid(t.tentityname_s.Get(), t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(), t.n)) == "/")
		{
			t.tentityfolder_s = Left(t.tentityfolder_s.Get(), t.n);
			break;
		}
	}

	//  model files (main model, final appended model and all other append
	int iModelAppendFileCount = t.entityprofile[t.entid].appendanimmax;
	if (Len (t.entityappendanim[t.entid][0].filename.Get()) > 0) iModelAppendFileCount = 0;
	for (int iModels = -1; iModels <= iModelAppendFileCount; iModels++)
	{
		LPSTR pModelFile = "";
		if (iModels == -1)
		{
			pModelFile = t.entityprofile[t.entid].model_s.Get();
		}
		else
		{
			pModelFile = t.entityappendanim[t.entid][iModels].filename.Get();
		}
		t.tlocaltofpe = 1;
		for (t.n = 1; t.n <= Len(pModelFile); t.n++)
		{
			if (cstr(Mid(pModelFile, t.n)) == "\\" || cstr(Mid(pModelFile, t.n)) == "/")
			{
				t.tlocaltofpe = 0; break;
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile1_s = t.tentityfolder_s + pModelFile;
		}
		else
		{
			t.tfile1_s = pModelFile;
		}

		#ifdef WICKEDENGINE
		//Wicked prefer DBO over X.
		t.tfile2_s = cstr(Left(t.tfile1_s.Get(), Len(t.tfile1_s.Get()) - 2)) + ".dbo";
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile2_s).Get()) == 1)
		{
			t.tfile_s = t.tfile2_s;
		}
		else
		{
			t.tfile_s = t.tfile1_s;
		}
		#else
		// never prefer DBO over X, always transport X (for Player)
		//t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
		//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
		//{
		//	t.tfile_s=t.tfile2_s;
		//}
		//else
		//{
		t.tfile_s = t.tfile1_s;
		//}
		#endif
		t.tmodelfile_s = t.tfile_s;
		addtocollection(t.tmodelfile_s.Get());

		#ifdef WICKEDENGINE
		//PE: CCP have missing textures the body part, i seen entrys like 'baseColorMap0    = tempfinalalbedo0.dds' , so always copy over main texture.
		if (t.entityprofile[t.entityelement[t.e].bankindex].ischaracter)
		{
			cstr ccpname = t.tmodelfile_s;
			std::string sParseName = t.tmodelfile_s.Get();
			std::string sParseNext = sParseName;
			replaceAll(sParseName, ".dbo", "0.dds"); //CCP missing main texture (body).
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "1.dds"); //PE: Legs with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
			sParseName = sParseNext;
			replaceAll(sParseName, ".dbo", "2.dds"); //PE: Feets with custom skin also need to be added.
			addtocollection((char *)sParseName.c_str());
		}
		#endif

		// if entity did not specify texture it is multi-texture, so interogate model file
		// do it for every model
		findalltexturesinmodelfile(t.tmodelfile_s.Get(), t.tentityfolder_s.Get(), t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
	}

	// Export entity FPE BMP file if flagged
	if (g.gexportassets == 1)
	{
		t.tfile3_s = cstr(Left(t.tentityname_s.Get(), Len(t.tentityname_s.Get()) - 4)) + ".bmp";
		if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
		{
			addtocollection(t.tfile3_s.Get());
		}
	}

	// entity characterpose file (if any)
	t.tfile3_s = cstr(Left(t.tfile1_s.Get(), Len(t.tfile1_s.Get()) - 2)) + ".dat";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
	{
		addtocollection(t.tfile3_s.Get());
	}

	#ifdef WICKEDENGINE
	// bullet physics hull decomp file (if any)
	t.tfile3_s = cstr(Left(t.tentityname_s.Get(), Len(t.tentityname_s.Get()) - 4)) + ".bullet";
	if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
	{
		addtocollection(t.tfile3_s.Get());
	}
	#endif

	//  texture files
	int iStartingType = 0;
	if (t.e == 0) iStartingType = 1; // parent profile only
	for (int iBothTypes = iStartingType; iBothTypes < 2; iBothTypes++)
	{
		// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
		cstr pTextureFile = "", pAltTextureFile = "";
		if (iBothTypes == 0) { pTextureFile = t.entityelement[t.e].eleprof.texd_s; pAltTextureFile = t.entityelement[t.e].eleprof.texaltd_s; }
		if (iBothTypes == 1) { pTextureFile = t.entityprofile[t.entid].texd_s; pAltTextureFile = t.entityprofile[t.entid].texaltd_s; }

		t.tlocaltofpe = 1;
		for (t.n = 1; t.n <= Len(pTextureFile.Get()); t.n++)
		{
			if (cstr(Mid(pTextureFile.Get(), t.n)) == "\\" || cstr(Mid(pTextureFile.Get(), t.n)) == "/")
			{
				t.tlocaltofpe = 0; break;
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile_s = t.tentityfolder_s + pTextureFile;
		}
		else
		{
			t.tfile_s = pTextureFile;
		}
		addtocollection(t.tfile_s.Get());

		// always allow a DDS texture of same name to be copied over (for test game compatibility)
		for (int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++)
		{
			if (iTwoExtensions == 0) t.tfileext_s = Right (t.tfile_s.Get(), 3);
			if (iTwoExtensions == 1) t.tfileext_s = "dds";
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 6)), 2)) == "_d")
			{
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_n." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_s." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_i." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_o." + t.tfileext_s; addtocollection(t.tfile_s.Get());
				t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_cube." + t.tfileext_s; addtocollection(t.tfile_s.Get());
			}
			int iNewPBRTextureMode = 0;
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 10)), 6)) == "_color") iNewPBRTextureMode = 6 + 4;
			if (cstr(Left(Lower(Right(t.tfile_s.Get(), 11)), 7)) == "_albedo") iNewPBRTextureMode = 7 + 4;
			if (iNewPBRTextureMode > 0)
			{
				cstr pToAdd;
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_color." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_albedo." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_normal." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_specular." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_metalness." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_gloss." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_mask." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_ao." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_height." + t.tfileext_s; addtocollection(pToAdd.Get());
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_detail." + t.tfileext_s; addtocollection(pToAdd.Get());
				#ifdef WICKEDENGINE
				pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_surface." + t.tfileext_s; addtocollection(pToAdd.Get());
				#endif
			}
		}
		if (t.tlocaltofpe == 1)
		{
			t.tfile_s = t.tentityfolder_s + pAltTextureFile;
		}
		else
		{
			t.tfile_s = pAltTextureFile;
		}
		addtocollection(t.tfile_s.Get());
	}

	// also include textures specified by textureref entries
	// and assemblyccp from new character creator plus
	cstr tFPEFilePath = g.fpscrootdir_s + "\\Files\\";
	tFPEFilePath += t.tentityname1_s;
	FILE* tFPEFile = GG_fopen (tFPEFilePath.Get(), "r");
	if (tFPEFile)
	{
		char tTempLine[2048];
		while (!feof(tFPEFile))
		{
			fgets (tTempLine, 2047, tFPEFile);
			#ifdef WICKEDENGINE
			//PE: We need to add all custom mesh textures.
			if (strstr(tTempLine, "baseColorMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "normalMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			if (strstr(tTempLine, "surfaceMap"))
			{
				char* pToFilename = strstr(tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					//PE: CCP is special as we dont have to add entitybank...
					if (pestrcasestr(pToFilename, "charactercreatorplus\\parts\\"))
					{
						addtocollection(pToFilename);
					}
					else
					{
						cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
						addtocollection(tTextureFile.Get());
					}
				}
			}
			#endif
			if (strstr (tTempLine, "textureref"))
			{
				char* pToFilename = strstr (tTempLine, "=");
				if (pToFilename)
				{
					while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
					if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
					cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
					addtocollection (tTextureFile.Get());
				}
			}
			if (strstr (tTempLine, "ccpassembly"))
			{
				// LB: Copied this so I can work out textures from this field, but may be able to adapt 
				// this "CreateVectorListFromCPPAssembly" call to here as well.
				char* pAssemblyString = strstr (tTempLine, "=");
				if (pAssemblyString)
				{
					// get past equals and any spaces
					while (*pAssemblyString == '=' || *pAssemblyString == 32) pAssemblyString++;

					// now we have the assembly string; adult male hair 01,adult male head 01,adult male body 03,adult male legs 04e,adult male feet 04
					// delimited by a comma, and indicates which parts we used (to specify the textures to copy over)
					cstr assemblyString_s = FirstToken(pAssemblyString, ",");
					while (assemblyString_s.Len() > 0)
					{
						// work out texture files from this reference, i.e adult male hair 01
						char pAssemblyReference[1024];
						strcpy(pAssemblyReference, assemblyString_s.Get());
						if (pAssemblyReference[strlen(pAssemblyReference) - 1] == '\n') pAssemblyReference[strlen(pAssemblyReference) - 1] = 0;
						strlwr(pAssemblyReference);
						#ifdef PRODUCTV3
						int iBaseCount = 4;
						#else
						int iBaseCount = 4; //(3) PE: Added zombie female
						#endif
						for (int iBaseIndex = 0; iBaseIndex < iBaseCount; iBaseIndex++)
						{
							LPSTR pBaseName = "";
							#ifdef PRODUCTV3
							if (iBaseIndex == 0) pBaseName = "adult male";
							if (iBaseIndex == 1) pBaseName = "adult female";
							if (iBaseIndex == 2) pBaseName = "child male";
							if (iBaseIndex == 3) pBaseName = "child female";
							#else
							if (iBaseIndex == 0) pBaseName = "adult male";
							if (iBaseIndex == 1) pBaseName = "adult female";
							if (iBaseIndex == 2) pBaseName = "zombie male";
							if (iBaseIndex == 3) pBaseName = "zombie female";
							#endif
							if (strstr(pAssemblyReference, pBaseName) != NULL)
							{
								// found category
								cstr pPartFolder = "";
								if (iBaseIndex == 0) pPartFolder = "charactercreatorplus\\parts\\adult male\\";
								if (iBaseIndex == 1) pPartFolder = "charactercreatorplus\\parts\\adult female\\";
								#ifdef PRODUCTV3
								if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\child male\\";
								if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\child female\\";
								#else
								if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\zombie male\\";
								if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\zombie female\\";
								#endif

								// add final texture files
								cstr pTmpFile = pPartFolder + pAssemblyReference;
								char pRemoveTag[MAX_PATH];
								strcpy(pRemoveTag, pTmpFile.Get());
								for (int nnn = 0; nnn < strlen(pRemoveTag); nnn++)
								{
									if (pRemoveTag[nnn] == '[')
									{
										if (pRemoveTag[nnn - 1] == ' ') nnn--;
										pRemoveTag[nnn] = 0;
										break;
									}
								}

								// need to strip out the tag [xxx] part to find texture proper
								pTmpFile = pRemoveTag;
								addtocollection (cstr(pTmpFile + "_ao.dds").Get());
								addtocollection (cstr(pTmpFile + "_color.dds").Get());
								addtocollection (cstr(pTmpFile + "_gloss.dds").Get());
								addtocollection (cstr(pTmpFile + "_mask.dds").Get());
								addtocollection (cstr(pTmpFile + "_metalness.dds").Get());
								#ifdef WICKEDENGINE
								addtocollection(cstr(pTmpFile + "_surface.dds").Get());
								#endif
								addtocollection (cstr(pTmpFile + "_normal.dds").Get());
							}
						}
						assemblyString_s = NextToken(",");
					}
				}
			}
		}
		fclose (tFPEFile);
	}
}

void mapfile_collectfoldersandfiles ( cstr levelpathfolder )
{
	LPSTR pOldDir = GetDir();

	// Collect ALL files in string array list
	Undim ( t.filecollection_s );
	g.filecollectionmax = 0;
	Dim ( t.filecollection_s, 500 );

	//  Stage 1 - specify all common files
	//addtocollection("editors\\gfx\\guru-forexe.ico");
	addtocollection( cstr(cstr("languagebank\\")+g.language_s+"\\textfiles\\guru-wordcount.ini").Get() );
	addtocollection(cstr(cstr("languagebank\\")+g.language_s+"\\textfiles\\guru-words.txt").Get() );
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\inittext.ssp").Get());
	addtocollection("audiobank\\misc\\silence.wav");
	addtocollection("audiobank\\misc\\explode.wav");
	addtocollection("audiobank\\misc\\ammo.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_01.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_02.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_03.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_04.wav");
	addtocollection("editors\\gfx\\14.png");
	addtocollection("editors\\gfx\\14-white.png");
	addtocollection("editors\\gfx\\14-red.png");
	addtocollection("editors\\gfx\\14-green.png");
	addtocollection("editors\\gfx\\dummy.png");
	addtocollection("editors\\keymap\\default.ini");
	addtocollection("editors\\keymap\\weaponslots.dat");
	addtocollection("scriptbank\\gameloop.lua");
	addtocollection("scriptbank\\gameplayercontrol.lua");
	addtocollection("scriptbank\\global.lua");
	addtocollection("scriptbank\\music.lua");
	addfoldertocollection(cstr(cstr("languagebank\\")+g.language_s+"\\artwork\\watermark").Get() );
	addfoldertocollection("scriptbank\\people\\ai");
	addtocollection("scriptbank\\people\\patrol.byc");
	addtocollection("scriptbank\\people\\patrol.lua");
	addfoldertocollection("scriptbank\\ai");
	addfoldertocollection("scriptbank\\images");
	addfoldertocollection("audiobank\\materials");

#ifdef WICKEDENGINE
	addtocollection("scriptbank\\perlin_noise.lua");
#endif

#ifdef WICKEDENGINE
	//PE: Missing foot step material sounds
	addfoldertocollection("audiobank\\materials\\dirt");
	addfoldertocollection("audiobank\\materials\\grass");
	addfoldertocollection("audiobank\\materials\\grass\\extras");
	addfoldertocollection("audiobank\\materials\\gravel");
	addfoldertocollection("audiobank\\materials\\metal");
	addfoldertocollection("audiobank\\materials\\puddle");
	addfoldertocollection("audiobank\\materials\\sand");
	addfoldertocollection("audiobank\\materials\\snow");
	addfoldertocollection("audiobank\\materials\\tarmac");
	addfoldertocollection("audiobank\\materials\\underwater");
	addfoldertocollection("audiobank\\materials\\wood");
#endif

	addfoldertocollection("audiobank\\music\\theescape");
	addfoldertocollection("audiobank\\voices\\player");
	addfoldertocollection("audiobank\\voices\\characters");
	addfoldertocollection("audiobank\\character\\soldier\\onAggro");
	addfoldertocollection("audiobank\\character\\soldier\\onAlert");
	addfoldertocollection("audiobank\\character\\soldier\\onDeath");
	addfoldertocollection("audiobank\\character\\soldier\\onHurt");
	addfoldertocollection("audiobank\\character\\soldier\\onHurtPlayer");
	addfoldertocollection("audiobank\\character\\soldier\\onIdle");
	addfoldertocollection("audiobank\\character\\soldier\\onInteract");
	addfoldertocollection("databank");
	addfoldertocollection("savegames");
	addfoldertocollection("titlesbank\\default\\");
	addtocollection("titlesbank\\cursorcontrol.lua");
	addtocollection("titlesbank\\resolutions.lua");
	addtocollection("titlesbank\\fillgameslots.lua");
	addtocollection("titlesbank\\gamedata.lua");
	addtocollection("titlesbank\\restoregame.lua");
	addfoldertocollection("effectbank\\reloaded");
	addfoldertocollection("effectbank\\reloaded\\media");
	addfoldertocollection("effectbank\\reloaded\\media\\materials");
	addfoldertocollection("effectbank\\explosion");
	addfoldertocollection("effectbank\\particles");
	#ifdef WICKEDENGINE
	addfoldertocollection("effectbank\\particles\\weather");
	addfoldertocollection("lensflares");
	#endif
	addfoldertocollection("fontbank");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\ammohealth");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\sliders");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\panels");
	addfoldertocollection("gamecore\\decals\\splat");
	addfoldertocollection("gamecore\\decals\\bloodsplat");
	addfoldertocollection("gamecore\\decals\\impact");
	addfoldertocollection("gamecore\\decals\\gunsmoke");
	addfoldertocollection("gamecore\\decals\\smoke1");
	addfoldertocollection("gamecore\\decals\\muzzleflash4");
	addfoldertocollection("gamecore\\decals\\splash_droplets");
	addfoldertocollection("gamecore\\decals\\splash_foam");
	addfoldertocollection("gamecore\\decals\\splash_large");
	addfoldertocollection("gamecore\\decals\\splash_misty");
	addfoldertocollection("gamecore\\decals\\splash_ripple");
	addfoldertocollection("gamecore\\decals\\splash_small");
	addfoldertocollection("gamecore\\projectiletypes");
	addfoldertocollection("gamecore\\vrcontroller");
	addfoldertocollection("gamecore\\vrcontroller\\oculus");
	addfoldertocollection("gamecore\\projectiletypes\\fantasy\\fireball");
	addfoldertocollection("gamecore\\projectiletypes\\fantasy\\magicbolt");
	addfoldertocollection("gamecore\\projectiletypes\\modern\\handgrenade");
	addfoldertocollection("gamecore\\projectiletypes\\modern\\rpggrenade");
	#ifdef WICKEDENGINE
	addfoldertocollection("gamecore\\bulletholes");


	addfoldertocollection("editors\\lut");
	addfoldertocollection("terrainbank\\temp\\");
	addtocollection("terrainbank\\desert\\default.obj"); // for temp flat terrain!

	addfoldertocollection("treebank"); // for temp flat terrain!
	addfoldertocollection("treebank\\billboards"); // for temp flat terrain!
	addfoldertocollection("treebank\\textures"); // for temp flat terrain!
	
	//PE: Storyboard - Need standalone / lua menu's working.
	addfoldertocollection("terraintextures");
	for (int i = 0; i < 42; i++)
	{
		char addfolder[MAX_PATH];
		sprintf(addfolder, "terraintextures\\mat%d", i);
		addfoldertocollection(addfolder);
	}
	addfoldertocollection("grassbank");
	#ifdef WICKEDENGINE
	#else
	addfoldertocollection("databank\\extendedblood");
	#endif
	addfoldertocollection("particlesbank");
	addfoldertocollection("particlesbank\\user");

	addtocollection("effectbank\\common\\noise64.png");
	addtocollection("effectbank\\common\\dist2.png");
	addfoldertocollection("effectbank\\common"); //Just in case we get more.

	addtocollection("pinetree_high_color_1024.dds");
	addtocollection("pinetree.dds");
	addtocollection("noise.dds");

	#ifdef STORYBOARD
	char fileName[MAX_PATH] = "\0";
	void FindFirstSplash(char *splash_name);
	FindFirstSplash(fileName);
	if(strlen(fileName) > 0)
		addtocollection(fileName);
	#endif

	addtocollection("skybank\\clear\\"); //for fallback.
	
	#endif
	// add any material decals that are active
	for ( t.m = 0; t.m <= g.gmaterialmax; t.m++ )
	{
		if ( t.material[t.m].usedinlevel == 1 )
		{
			cstr decalFolder_s = cstr("gamecore\\decals\\")+t.material[t.m].decal_s;
			addfoldertocollection(decalFolder_s.Get());
		}
	}

	addfoldertocollection("gamecore\\muzzleflash");
	addfoldertocollection("gamecore\\projectiletypes");
	//addfoldertocollection("gamecore\\brass"); // now done below when detect guns used in level
	addfoldertocollection("gamecore\\hudlayers");

	//  Stage 1B - Style dependent files
	titles_getstyle ( );
	addtocollection("titlesbank\\style.txt");
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1280x720").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1280x800").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1366x768").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1440x900").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1600x900").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1680x1050").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1920x1080").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1920x1200").Get() );

	// include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	// (eventually to be controlled from a nice UI)
	std::vector<cstr> fppFoldersToRemoveList;
	std::vector<cstr> fppFilesToRemoveList;
	fppFoldersToRemoveList.clear();
	fppFilesToRemoveList.clear();

	// Stage 2 - collect all files (from all levels)
	t.levelindex=0;
	Dim ( t.levellist_s, 100  );
	#ifdef WICKEDENGINE
	addtocollection(t.visuals.sAmbientMusicTrack.Get());
	addtocollection(t.visuals.sCombatMusicTrack.Get());
	#endif
	#ifdef STORYBOARD
	//Add all storyboard files to scan list.
	if (g.bUseStoryBoardSetup)
	{
		addfoldertocollection("editors\\templates\\fonts");
		addtocollection("editors\\uiv3\\Roboto-Medium.ttf");
		addtocollection("editors\\uiv3\\standalone_ea.png");
		addtocollection("editors\\uiv3\\standalone_ea-ea.png");

		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
		int a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
		while (a == 1)
		{
			++t.levelmax;
			t.levellist_s[t.levelmax] = g_Storyboard_Current_fpm;
			addtocollection(g_Storyboard_Current_fpm);
			a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
		}

		//Restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);


		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			//Add project.
			char project[MAX_PATH];
			strcpy(project, "projectbank\\");
			strcat(project, Storyboard.gamename);
			addfoldertocollection(project);
			//strcat(project, "\\project.dat");
			//addtocollection("editors\\uiv3\\loadingsplash.jpg");

			//Add all media used by storyboard.
			for (int nodeid = 0; nodeid < STORYBOARD_MAXNODES; nodeid++)
			{
				if (Storyboard.Nodes[nodeid].used)
				{
					if (strlen(Storyboard.Nodes[nodeid].screen_music) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_music);
					}
					if (strlen(Storyboard.Nodes[nodeid].screen_backdrop) > 0)
					{
						addtocollection(Storyboard.Nodes[nodeid].screen_backdrop);
					}

					for (int i = 0; i < STORYBOARD_MAXWIDGETS; i++)
					{
						if (Storyboard.Nodes[nodeid].widget_used[i] == 1)
						{
							if (strlen(Storyboard.Nodes[nodeid].widget_normal_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_normal_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_highlight_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_selected_thumb[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_selected_thumb[i]);
							}
							if (strlen(Storyboard.Nodes[nodeid].widget_click_sound[i]) > 0)
							{
								addtocollection(Storyboard.Nodes[nodeid].widget_click_sound[i]);
							}
						}
					}
				}
			}

		}
	}
#endif


	t.tlevelstoprocess = 1;
	t.tlevelfile_s="";
	g.projectfilename_s=t.tmasterlevelfile_s;
	while ( t.tlevelstoprocess == 1 ) 
	{
	// load in level FPM
	if ( Len(t.tlevelfile_s.Get())>1 ) 
	{
		g.projectfilename_s=t.tlevelfile_s;
		mapfile_loadproject_fpm ( );
		game_loadinentitiesdatainlevel ( );
	}

	// 061018 - check if an FPP file exists for this level file
	cstr pFPPFile = cstr(Left(g.projectfilename_s.Get(),strlen(g.projectfilename_s.Get())-4)) + ".fpp";
	if ( FileExist( pFPPFile.Get() ) == 1 ) 
	{
		// used to specify additional files required for standalone executable
		// handy as a workaround until reported issue resolved
		int iFPPStandaloneExtraFilesMode = 0;
		Dim ( t.data_s, 999 );
		LoadArray ( pFPPFile.Get(), t.data_s );
		for ( t.l = 0 ; t.l <= 999; t.l++ )
		{
			t.line_s = t.data_s[t.l];
			LPSTR pLine = t.line_s.Get();
			if ( Len(pLine) > 0 ) 
			{
				if ( strnicmp ( pLine, "[standalone add files]", 22 ) == NULL )
				{
					// denotes our standalone extra files
					iFPPStandaloneExtraFilesMode = 1;
				}
				else
				{
					if ( strnicmp ( pLine, "[standalone delete files]", 25 ) == NULL )
					{
						// denotes our standalone remove files
						iFPPStandaloneExtraFilesMode = 2;
					}
					else
					{
						// this prevents newer FPP files from getting confused with this original simple method
						if ( iFPPStandaloneExtraFilesMode == 1 )
						{
							// add
							if ( pLine[strlen(pLine)-1] == '\\' )
							{
								// include whole folder
								addfoldertocollection(pLine);
							}
							else
							{
								// include specific file
								addtocollection(pLine);
							}
						}
						if ( iFPPStandaloneExtraFilesMode == 2 )
						{
							// remove
							if ( pLine[strlen(pLine)-1] == '\\' )
							{
								// remove whole folder
								fppFoldersToRemoveList.push_back(cstr(pLine));
							}
							else
							{
								// remove specific file
								fppFilesToRemoveList.push_back(cstr(pLine));
							}
						}
					}
				}
			}
		}
		UnDim(t.data_s);
	}	

	//  chosen sky, terrain and veg
	addfoldertocollection(cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );
	addfoldertocollection(cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
	#ifdef WICKEDENGINE
	addtocollection("vegbank\\AllGrass.png");
	addtocollection("vegbank\\veg.dbo");
	addtocollection("vegbank\\clump.dbo");
	#else
	addtocollection("skybank\\cloudportal.dds");
	addfoldertocollection(cstr(cstr("vegbank\\")+g.vegstyle_s).Get() );
	#endif

	// pre-add the skins folder - can optimize later to find only skins we used (118MB)
	addfoldertocollection("charactercreatorplus\\skins" );

	// choose all entities and associated files
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			// Check for custom images loaded in lua script
			if ( t.entityelement[t.e].eleprof.aimain_s != "" )
			{
				cstr tLuaScript = g.fpscrootdir_s+"\\Files\\scriptbank\\";
				tLuaScript += t.entityelement[t.e].eleprof.aimain_s;
				FILE* tLuaScriptFile = GG_fopen ( tLuaScript.Get() , "r" );
				if ( tLuaScriptFile )
				{
					#ifdef WICKEDENGINE
					bool bExtractedImageFilesThisScript = false;
					#endif

					char tTempLine[2048];
					while ( !feof(tLuaScriptFile) )
					{
						fgets ( tTempLine , 2047 , tLuaScriptFile );
						if ( strstr ( tTempLine , "LoadImages" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr tFolderToAdd = cstr( cstr("scriptbank\\images\\") + cstr(pImageFolder) );
									addfoldertocollection ( tFolderToAdd.Get() );
								}
							}
						}

						// Handle new load image and sound commands, they can be in nested folders
						if ( strstr ( tTempLine , "LoadImage " ) 
						||	 strstr ( tTempLine , "LoadImage(" )
						||   strstr ( tTempLine, "LoadGlobalSound ")
						||	 strstr ( tTempLine, "LoadGlobalSound(" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr pFile = cstr(pImageFolder);
									addtocollection ( pFile.Get() );
								}
							}

							#ifdef WICKEDENGINE
							if (!bExtractedImageFilesThisScript)
							{
								bExtractedImageFilesThisScript = true;

								extern void InitParseLuaScript(entityeleproftype *tmpeleprof);
								extern void ParseLuaScript(entityeleproftype *tmpeleprof, char * script);

								// PropertiesVariables are not filled for t.entityelement[t.e], so create a temp variable to fill the values just in case we change anything we shouldn't.
								entityeleproftype tempeleprof = t.entityelement[t.e].eleprof;

								// Parse the lua script so we can check if it contains any string variables (for filenames).
								InitParseLuaScript(&tempeleprof);
								cstr script_name = "scriptbank\\";
								script_name += tempeleprof.aimain_s;
								ParseLuaScript(&tempeleprof, script_name.Get());

								// This script loads an image, now retrieve the filename and add it to the collection.
								for (int i = 0; i < MAXPROPERTIESVARIABLES; i++)
								{
									// Check the lua variable is a string.
									if (tempeleprof.PropertiesVariable.VariableType[i] == 2)
									{
										// Check if the string contains an image file.
										char* pLuaString = &tempeleprof.PropertiesVariable.VariableValue[i][0];
										if (strstr(pLuaString, ".png") || strstr(pLuaString, ".jpg") || strstr(pLuaString, ".dds"))
										{
											// This string is the name of an image file, add it to the collection.
											addtocollection(pLuaString);
										}
									}
								}
							}
							#endif
						}
						if (strstr(tTempLine, "SetSkyTo(" )) {
							char* pSkyFolder = strstr(tTempLine, "\"");
							if (pSkyFolder)
							{
								pSkyFolder++;
								char* pSkyFolderEnd = strstr(pSkyFolder, "\"");
								if (pSkyFolderEnd)
								{
									*pSkyFolderEnd = '\0';
									cstr tFolderToAdd = cstr(cstr("skybank\\") + cstr(pSkyFolder));
									addfoldertocollection(tFolderToAdd.Get());
								}
							}
						}
					}
					fclose ( tLuaScriptFile );
				}
			}

			// gives t.entid and adds ALL entity profile related files to the collection
			addthisentityprofilesfilestocollection();
			/*
			//  entity profile file
			t.tentityname1_s=cstr("entitybank\\")+t.entitybank_s[t.entid];
			t.tentityname2_s=cstr(Left(t.tentityname1_s.Get(),Len(t.tentityname1_s.Get())-4))+".bin";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tentityname2_s).Get() ) == 1 ) 
			{
				t.tentityname_s=t.tentityname2_s;
			}
			else
			{
				t.tentityname_s=t.tentityname1_s;
			}
			addtocollection(t.tentityname_s.Get());

			//  entity files in folder
			t.tentityfolder_s=t.tentityname_s;
			for ( t.n = Len(t.tentityname_s.Get()) ; t.n >= 1 ; t.n+= -1 )
			{
				if (  cstr(Mid(t.tentityname_s.Get(),t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(),t.n)) == "/" ) 
				{
					t.tentityfolder_s=Left(t.tentityfolder_s.Get(),t.n);
					break;
				}
			}

			//  model files (main model, final appended model and all other append
			int iModelAppendFileCount = t.entityprofile[t.entid].appendanimmax;
			if ( Len ( t.entityappendanim[t.entid][0].filename.Get() ) > 0 ) iModelAppendFileCount = 0;
			for ( int iModels = -1; iModels <= iModelAppendFileCount; iModels++ )
			{
				LPSTR pModelFile = "";
				if ( iModels == -1 ) 
				{
					pModelFile = t.entityprofile[t.entid].model_s.Get();
				}
				else
				{
					pModelFile = t.entityappendanim[t.entid][iModels].filename.Get();
				}
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n <= Len(pModelFile); t.n++ )
				{
					if (  cstr(Mid(pModelFile,t.n)) == "\\" || cstr(Mid(pModelFile,t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile1_s=t.tentityfolder_s+pModelFile;
				}
				else
				{
					t.tfile1_s=pModelFile;
				}
				#ifdef WICKEDENGINE
				//Wicked prefer DBO over X.
				t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				{
					t.tfile_s=t.tfile2_s;
				}
				else
				{
					t.tfile_s = t.tfile1_s;
				}
				#else
				// never prefer DBO over X, always transport X (for Player)
				//t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				//{
				//	t.tfile_s=t.tfile2_s;
				//}
				//else
				//{
				t.tfile_s=t.tfile1_s;
				//}
				#endif
				t.tmodelfile_s=t.tfile_s;
				addtocollection(t.tmodelfile_s.Get());

				#ifdef WICKEDENGINE
				//PE: CCP have missing textures the body part, i seen entrys like 'baseColorMap0    = tempfinalalbedo0.dds' , so always copy over main texture.
				if (t.entityprofile[t.entityelement[t.e].bankindex].ischaracter)
				{
					cstr ccpname = t.tmodelfile_s;
					std::string sParseName = t.tmodelfile_s.Get();
					replaceAll(sParseName, ".dbo", "0.dds"); //CCP missing main texture (body).
					addtocollection((char *) sParseName.c_str());
				}
				#endif
				// if entity did not specify texture it is multi-texture, so interogate model file
				// do it for every model
				findalltexturesinmodelfile(t.tmodelfile_s.Get(), t.tentityfolder_s.Get(), t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
			}

			// Export entity FPE BMP file if flagged
			if ( g.gexportassets == 1 ) 
			{
				t.tfile3_s=cstr(Left(t.tentityname_s.Get(),Len(t.tentityname_s.Get())-4))+".bmp";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
				{
					addtocollection(t.tfile3_s.Get());
				}
			}

			// entity characterpose file (if any)
			t.tfile3_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dat";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
			{
				addtocollection(t.tfile3_s.Get());
			}

			#ifdef WICKEDENGINE
			// bullet physics hull decomp file (if any)
			t.tfile3_s = cstr(Left(t.tentityname_s.Get(), Len(t.tentityname_s.Get()) - 4)) + ".bullet";
			if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + t.tfile3_s).Get()) == 1)
			{
				addtocollection(t.tfile3_s.Get());
			}
			#endif

			//  texture files
			for ( int iBothTypes = 0; iBothTypes < 2; iBothTypes++ )
			{
				// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
				cstr pTextureFile = "", pAltTextureFile = "";
				if ( iBothTypes == 0 ) { pTextureFile = t.entityelement[t.e].eleprof.texd_s; pAltTextureFile = t.entityelement[t.e].eleprof.texaltd_s; } 
				if ( iBothTypes == 1 ) { pTextureFile = t.entityprofile[t.entid].texd_s; pAltTextureFile = t.entityprofile[t.entid].texaltd_s; } 

				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(pTextureFile.Get()); t.n++ )
				{
					if (  cstr(Mid(pTextureFile.Get(),t.n)) == "\\" || cstr(Mid(pTextureFile.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pTextureFile;
				}
				else
				{
					t.tfile_s=pTextureFile;
				}
				addtocollection(t.tfile_s.Get());

				// always allow a DDS texture of same name to be copied over (for test game compatibility)
				for ( int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++ )
				{
					if ( iTwoExtensions == 0 ) t.tfileext_s = Right ( t.tfile_s.Get(), 3);
					if ( iTwoExtensions == 1 ) t.tfileext_s = "dds";
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),6)),2)) == "_d" ) 
					{
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_n."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_s."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_i."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_o."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_cube."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					}
					int iNewPBRTextureMode = 0;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),10)),6)) == "_color" ) iNewPBRTextureMode = 6+4;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),11)),7)) == "_albedo" ) iNewPBRTextureMode = 7+4;
					if ( iNewPBRTextureMode > 0 ) 
					{
						cstr pToAdd;
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_color."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_albedo."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_normal."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_specular."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_metalness."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_gloss."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_mask."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_ao."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_height."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_detail."+t.tfileext_s; addtocollection(pToAdd.Get());
						#ifdef WICKEDENGINE
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_surface." + t.tfileext_s; addtocollection(pToAdd.Get());
						#endif
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pAltTextureFile;
				}
				else
				{
					t.tfile_s=pAltTextureFile;
				}
				addtocollection(t.tfile_s.Get());
			}

			// also include textures specified by textureref entries
			// and assemblyccp from new character creator plus
			cstr tFPEFilePath = g.fpscrootdir_s+"\\Files\\";
			tFPEFilePath += t.tentityname1_s;
			FILE* tFPEFile = GG_fopen ( tFPEFilePath.Get() , "r" );
			if ( tFPEFile )
			{
				char tTempLine[2048];
				while ( !feof(tFPEFile) )
				{
					fgets ( tTempLine , 2047 , tFPEFile );
					#ifdef WICKEDENGINE
					//PE: We need to add all custom mesh textures.
					if (strstr(tTempLine, "baseColorMap"))
					{
						char* pToFilename = strstr(tTempLine, "=");
						if (pToFilename)
						{
							while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
							//PE: Special case if ccp, use path directly.
							if (pestrcasestr(pToFilename, "charactercreatorplus"))
								tTextureFile = pToFilename;
							addtocollection(tTextureFile.Get());
						}
					}
					if (strstr(tTempLine, "normalMap"))
					{
						char* pToFilename = strstr(tTempLine, "=");
						if (pToFilename)
						{
							while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
							//PE: Special case if ccp, use path directly.
							if (pestrcasestr(pToFilename, "charactercreatorplus"))
								tTextureFile = pToFilename;
							addtocollection(tTextureFile.Get());
						}
					}
					if (strstr(tTempLine, "surfaceMap"))
					{
						char* pToFilename = strstr(tTempLine, "=");
						if (pToFilename)
						{
							while (*pToFilename == '=' || *pToFilename == 32) pToFilename++;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 13) pToFilename[strlen(pToFilename) - 1] = 0;
							if (pToFilename[strlen(pToFilename) - 1] == 10) pToFilename[strlen(pToFilename) - 1] = 0;
							cstr tTextureFile = cstr(t.tentityfolder_s + cstr(pToFilename));
							//PE: Special case if ccp, use path directly.
							if (pestrcasestr(pToFilename, "charactercreatorplus"))
								tTextureFile = pToFilename;
							addtocollection(tTextureFile.Get());
						}
					}
					#endif
					if ( strstr ( tTempLine , "textureref" ) )
					{
						char* pToFilename = strstr ( tTempLine , "=" );
						if ( pToFilename )
						{
							while ( *pToFilename == '=' || *pToFilename == 32 ) pToFilename++;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							cstr tTextureFile = cstr( t.tentityfolder_s + cstr(pToFilename) );
							addtocollection ( tTextureFile.Get() );
						}
					}
					if ( strstr ( tTempLine , "ccpassembly" ) )
					{
						// LB: Copied this so I can work out textures from this field, but may be able to adapt 
						// this "CreateVectorListFromCPPAssembly" call to here as well.
						char* pAssemblyString = strstr ( tTempLine , "=" );
						if ( pAssemblyString )
						{
							// get past equals and any spaces
							while ( *pAssemblyString == '=' || *pAssemblyString == 32 ) pAssemblyString++;

							// now we have the assembly string; adult male hair 01,adult male head 01,adult male body 03,adult male legs 04e,adult male feet 04
							// delimited by a comma, and indicates which parts we used (to specify the textures to copy over)
							cstr assemblyString_s = FirstToken( pAssemblyString, ",");
							while (assemblyString_s.Len() > 0)
							{
								// work out texture files from this reference, i.e adult male hair 01
								char pAssemblyReference[1024];
								strcpy(pAssemblyReference, assemblyString_s.Get());
								if (pAssemblyReference[strlen(pAssemblyReference) - 1] == '\n') pAssemblyReference[strlen(pAssemblyReference) - 1] = 0;
								strlwr(pAssemblyReference);
								#ifdef PRODUCTV3
								 int iBaseCount = 4;
								#else
								 int iBaseCount = 4; //(3) PE: Added zombie female
								#endif
								for (int iBaseIndex = 0; iBaseIndex < iBaseCount; iBaseIndex++)
								{
									LPSTR pBaseName = "";
									#ifdef PRODUCTV3
									if (iBaseIndex == 0) pBaseName = "adult male";
									if (iBaseIndex == 1) pBaseName = "adult female";
									if (iBaseIndex == 2) pBaseName = "child male";
									if (iBaseIndex == 3) pBaseName = "child female";
									#else
									if (iBaseIndex == 0) pBaseName = "adult male";
									if (iBaseIndex == 1) pBaseName = "adult female";
									if (iBaseIndex == 2) pBaseName = "zombie male";
									if (iBaseIndex == 3) pBaseName = "zombie female";
									#endif
									if (strstr(pAssemblyReference, pBaseName) != NULL)
									{
										// found category
										cstr pPartFolder = "";
										if (iBaseIndex == 0) pPartFolder = "charactercreatorplus\\parts\\adult male\\";
										if (iBaseIndex == 1) pPartFolder = "charactercreatorplus\\parts\\adult female\\";
										#ifdef PRODUCTV3
										if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\child male\\";
										if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\child female\\";
										#else
										if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\zombie male\\";
										if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\zombie female\\";
										#endif

										// add final texture files
										cstr pTmpFile = pPartFolder + pAssemblyReference;
										char pRemoveTag[MAX_PATH];
										strcpy(pRemoveTag, pTmpFile.Get());
										for (int nnn = 0; nnn < strlen(pRemoveTag); nnn++)
										{
											if (pRemoveTag[nnn] == '[')
											{
												if (pRemoveTag[nnn - 1] == ' ') nnn--;
												pRemoveTag[nnn] = 0;
												break;
											}
										}

										// need to strip out the tag [xxx] part to find texture proper
										pTmpFile = pRemoveTag;
										addtocollection ( cstr(pTmpFile+"_ao.dds").Get() );
										addtocollection ( cstr(pTmpFile+"_color.dds").Get() );
										addtocollection ( cstr(pTmpFile+"_gloss.dds").Get() );
										addtocollection ( cstr(pTmpFile+"_mask.dds").Get() );
										addtocollection ( cstr(pTmpFile+"_metalness.dds").Get() );
										#ifdef WICKEDENGINE
										addtocollection(cstr(pTmpFile + "_surface.dds").Get());
										#endif
										addtocollection ( cstr(pTmpFile+"_normal.dds").Get() );
									}
								}
								assemblyString_s = NextToken( "," );
							}
						}
					}
				}
				fclose ( tFPEFile );
			}
			*/

			// shader file
			t.tfile_s=t.entityelement[t.e].eleprof.effect_s ; addtocollection(t.tfile_s.Get());
			if (cstr(Lower(Right(t.tfile_s.Get(), 3))) == ".fx") 
			{
				//Try to take the .blob.
				t.tfile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 3);
				t.tfile_s = t.tfile_s + ".blob";
				if (FileExist(t.tfile_s.Get()) == 1)
				{
					addtocollection(t.tfile_s.Get());
				}
			}

			//  script files
			t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s;
			addtocollection(t.tfile_s.Get());
			#ifdef WICKEDENGINE
			#ifdef STORYBOARD
			//Copy .byc
			std::string sLuaFile = t.tfile_s.Get();
			replaceAll(sLuaFile, ".lua", ".byc");
			addtocollection( (char *) sLuaFile.c_str());
			#endif
			#endif

			//  for the script associated, scan it and include any references to other scripts
			scanscriptfileandaddtocollection(t.tfile_s.Get());

			// sound files
			if (t.entityprofile[t.entid].ismarker == 1 && t.entityelement[t.e].eleprof.soundset_s.Len() > 0 ) 
			{
				t.tfile_s = t.entityelement[t.e].eleprof.soundset_s;
				addfoldertocollection( cstr(cstr("audiobank\\voices\\") + cstr(t.tfile_s.Get())).Get() );
			}
			t.tfile_s=t.entityelement[t.e].eleprof.soundset_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset1_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset2_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset3_s ; addtocollection(t.tfile_s.Get());
			///t.tfile_s=t.entityelement[t.e].eleprof.soundset4_s ; addtocollection(t.tfile_s.Get());
			#ifdef WICKEDENGINE
			t.tfile_s = t.entityelement[t.e].eleprof.soundset5_s; addtocollection(t.tfile_s.Get());
			t.tfile_s = t.entityelement[t.e].eleprof.soundset6_s; addtocollection(t.tfile_s.Get());
			t.tfile_s = t.entityelement[t.e].eleprof.overrideanimset_s; addtocollection(t.tfile_s.Get());	
			#endif
			// lipsync files associated with soundset references
			cstr tmpFile_s = t.entityelement[t.e].eleprof.soundset_s; tmpFile_s = cstr(Left( tmpFile_s.Get(), strlen(tmpFile_s.Get())-4)) + ".lip"; addtocollection(tmpFile_s.Get());
			tmpFile_s = t.entityelement[t.e].eleprof.soundset1_s; tmpFile_s = cstr(Left( tmpFile_s.Get(), strlen(tmpFile_s.Get())-4)) + ".lip"; addtocollection(tmpFile_s.Get());
			tmpFile_s = t.entityelement[t.e].eleprof.soundset2_s; tmpFile_s = cstr(Left( tmpFile_s.Get(), strlen(tmpFile_s.Get())-4)) + ".lip"; addtocollection(tmpFile_s.Get());
			tmpFile_s = t.entityelement[t.e].eleprof.soundset3_s; tmpFile_s = cstr(Left( tmpFile_s.Get(), strlen(tmpFile_s.Get())-4)) + ".lip"; addtocollection(tmpFile_s.Get());
			///tmpFile_s = t.entityelement[t.e].eleprof.soundset4_s; tmpFile_s = cstr(Left( tmpFile_s.Get(), strlen(tmpFile_s.Get())-4)) + ".lip"; addtocollection(tmpFile_s.Get());
			#ifdef WICKEDENGINE
			tmpFile_s = t.entityelement[t.e].eleprof.soundset5_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
			tmpFile_s = t.entityelement[t.e].eleprof.soundset6_s; tmpFile_s = cstr(Left(tmpFile_s.Get(), strlen(tmpFile_s.Get()) - 4)) + ".lip"; addtocollection(tmpFile_s.Get());
			#endif
			//  collectable guns
			cstr pGunPresent = "";
			if ( Len(t.entityprofile[t.entid].isweapon_s.Get()) > 1 ) pGunPresent = t.entityprofile[t.entid].isweapon_s;
			if ( t.entityprofile[t.entid].isammo == 0 )
			{
				// 270618 - only accept HASWEAPON if NOT ammo, so executables are not bloated with ammo that specifies another weapon type
				if ( Len(t.entityelement[t.e].eleprof.hasweapon_s.Get()) > 1 ) pGunPresent = t.entityelement[t.e].eleprof.hasweapon_s;
			}
			if ( Len(pGunPresent.Get()) > 1 )
			{
				t.tfile_s=cstr("gamecore\\guns\\")+pGunPresent; addfoldertocollection(t.tfile_s.Get());
				t.findgun_s = Lower( pGunPresent.Get() ) ; 
				gun_findweaponindexbyname ( );
				if ( t.foundgunid > 0 ) 
				{
					// ammo and brass
					for ( t.x = 0; t.x <= 1; t.x++ )
					{
						// ammo files
						t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
						if (  t.tpoolindex>0 ) 
						{
							t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s;
							if ( PathExist ( t.tfile_s.Get() ) ) addfoldertocollection(t.tfile_s.Get());
						}

						// brass files
						int iBrassIndex = g.firemodes[t.foundgunid][t.x].settings.brass;
						if ( iBrassIndex > 0 ) 
						{
							t.tfile_s = cstr(cstr("gamecore\\brass\\brass")+Str(iBrassIndex));
							if ( PathExist ( t.tfile_s.Get() ) )
								addfoldertocollection(t.tfile_s.Get());
						}
					}

					// and any projectile files associated with it
					cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
					if ( Len(pProjectilePresent.Get()) > 1 )
					{
						t.tfile_s=cstr("gamecore\\projectiletypes\\")+pProjectilePresent; 
						addfoldertocollection(t.tfile_s.Get());
					}
				}
			}
			//  zone marker can reference other levels to jump to
			if (  t.entityprofile[t.entid].ismarker == 3 ) 
			{
				t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
				if (  Len(t.tlevelfile_s.Get())>1 ) 
				{
					t.tlevelfile_s=cstr("mapbank\\")+levelpathfolder+t.tlevelfile_s+".fpm";
					if (  FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 ) 
					{
						++t.levelmax;
						t.levellist_s[t.levelmax]=t.tlevelfile_s;
						addtocollection(t.tlevelfile_s.Get());
					}
					else
					{
						//  nope, just a regular string entry in the marker field
						t.tlevelfile_s="";
					}
				}
			}
		}
	}

	//  decide if another level needs loading/processing
	if (  t.levelindex<t.levelmax ) 
	{
		t.tlevelfile_s = "";
		t.tlevelstoprocess = 0;
		while ( t.levelindex<t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
		{
			++t.levelindex;
			t.ttrylevelfile_s=t.levellist_s[t.levelindex];
			for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
			{
				if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
				{
					t.ttrylevelfile_s = "";
					break;
				}
			}
			if ( t.ttrylevelfile_s != "" ) 
			{
				t.tlevelfile_s = t.ttrylevelfile_s;
				t.tlevelstoprocess = 1;
			}
		}
	}
	else
	{
		t.tlevelstoprocess = 0;
	}

	//  for each level file
	}

	//  if multi-level, do NOT include the levelbank\testmap temp files
	t.tignorelevelbankfiles=0;
	if (  g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		timestampactivity(0,"Ignoring levelbank testmap folder for multilevel standalone");
		t.tignorelevelbankfiles=1;
	}
	else
	{
		//addfoldertocollection("levelbank\\testmap"); // 190417 - dont need contents, comes from FPM load!
		//addtocollection("levelbank\\testmap\\header.dat"); // 190719 - this does not exist at time of scan, and created as part of FPM anyhoo
	}

	// 010917 - go through and remove any X files that have DBO counterparts
	SetDir ( cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	t.filesmax = g.filecollectionmax;
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		if ( FileExist(t.src_s.Get()) == 1 ) 
		{
			char pSrcFile[1024];
			strcpy ( pSrcFile, t.filecollection_s[t.fileindex].Get() );
			if ( strnicmp ( pSrcFile + strlen(pSrcFile) - 4, ".dbo", 4 ) == NULL )
			{
				cstr dboequiv = cstr(Left(pSrcFile,strlen(pSrcFile)-4))+".x";
				if ( FileExist(dboequiv.Get()) == 1 ) 
				{
					// Found DBO, and an X file sitting alongside it, remove the X from consideration
					removefromcollection ( dboequiv.Get() );
				}
			}
		}
	}

	// also remove folders/files marked by FPP file
	if ( fppFoldersToRemoveList.size() > 0 || fppFilesToRemoveList.size() > 0 )
	{
		for ( int n = 0; n < fppFoldersToRemoveList.size(); n++ )
		{
			cstr pRemoveFolder = fppFoldersToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFolder.Get() );
		}
		for ( int n = 0; n < fppFilesToRemoveList.size(); n++ )
		{
			cstr pRemoveFile = fppFilesToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFile.Get() );
		}
	}
}

void mapfile_savestandalone_start ( void )
{
	// first grab current folder for later restoring
	t.told_s=GetDir();

	// In Classic, I would run through the load process and collect files as they
	// where loaded in. In Reloaded, the currently loaded level data is scanned
	// to arrive at the required files for the Standalone EXE
	t.interactive.savestandaloneused=1;

	// this flag ensures the loadassets splash does not appear when making standalone
	t.levelsforstandalone = 1;

	// give prompts while standalone is saving
	//popup_text("Saving Standalone Game : Collecting Files");

	//  check for character creator usage
	///characterkit_checkForCharacters ( );

	// 040316 - v1.13b1 - find the nested folder structure of the level (could be in map bank\Easter\level1.fpm)
	t.told_s=GetDir();
	cstr mapbankpath;
	cstr levelpathfolder;
	if ( g.projectfilename_s.Get()[1] != ':' )
	{
		// relative project path
		g_mapfile_mapbankpath = cstr("mapbank\\");
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}
	else
	{
		// absolute project path
		//mapbankpath = t.told_s + cstr("\\mapbank\\");
		g_mapfile_mapbankpath = g.mysystem.mapbankAbs_s;
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}

	bool bGotNestedPath = false;
	for ( int n = Len(g_mapfile_levelpathfolder.Get()) ; n >= 1 ; n+= -1 )
	{
		if ( cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "\\" || cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "/" ) 
		{
			g_mapfile_levelpathfolder = Left ( g_mapfile_levelpathfolder.Get(), n );
			bGotNestedPath = true;
			break;
		}
	}
	if ( bGotNestedPath==false )
	{
		// 240316 - V1.131v1 - if NO nested folder, string must be empty!
		g_mapfile_levelpathfolder = "";
	}

	//  Name without EXE
	t.exename_s=g.projectfilename_s;
	if (  cstr(Lower(Right(t.exename_s.Get(),4))) == ".fpm" ) 
	{
		t.exename_s=Left(t.exename_s.Get(),Len(t.exename_s.Get())-4);
	}
	for ( t.n = Len(t.exename_s.Get()) ; t.n >= 1 ; t.n+= -1 )
	{
		if (  cstr(Mid(t.exename_s.Get(),t.n)) == "\\" || cstr(Mid(t.exename_s.Get(),t.n)) == "/" ) 
		{
			t.exename_s=Right(t.exename_s.Get(),Len(t.exename_s.Get())-t.n);
			break;
		}
	}
	//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "mylevel";

	//  the level to start off standalone export
	t.tmasterlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.exename_s+".fpm";

	#ifdef STORYBOARD
	if (g.bUseStoryBoardSetup)
	{
		//Use project name as exename
		if (strlen(Storyboard.gamename) > 0)
		{
			t.exename_s = Storyboard.gamename;
		}
	}
	#endif

	timestampactivity(0,cstr(cstr("Saving standalone from ")+t.tmasterlevelfile_s).Get() );

	//  Create MYDOCS folder if not exist
	if ( PathExist(g.myownrootdir_s.Get()) == 0 ) file_createmydocsfolder ( );

	// Create absolute My Games folder (if not exist)
	if ( PathExist ( g.exedir_s.Get() ) == 0 )
	{
		g.exedir_s="?";
		SetDir ( g.myownrootdir_s.Get() );
		t.mygamesfolder_s = "My Games";
		if ( PathExist(t.mygamesfolder_s.Get()) == 0 ) MakeDirectory ( t.mygamesfolder_s.Get() );
		if ( PathExist(t.mygamesfolder_s.Get()) == 1 ) 
		{
			SetDir ( t.mygamesfolder_s.Get() );
			g.exedir_s = GetDir();
		}
	}
	SetDir ( t.told_s.Get() );

	// Path to EXE (for dealing with relative EXE paths later)
	if ( g.exedir_s.Get()[1] == ':' )
	{
		t.exepath_s = g.exedir_s;
	}
	else
	{
		t.exepath_s = g.exedir_s;
	}
	if ( cstr(Right(t.exepath_s.Get(),1)) != "\\"  ) t.exepath_s = t.exepath_s+"\\";

	// Collect all files and folders and store in t.filecollection_s
	mapfile_collectfoldersandfiles ( levelpathfolder );

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	g_mapfile_fppFoldersToRemoveList.clear();
	g_mapfile_fppFilesToRemoveList.clear();

	// process in stages
	g_mapfile_iStage = 1;
	g_mapfile_fProgress = 0.0f;
}

void mapfile_savestandalone_stage2a ( void )
{
	// Stage 2 - count ALL levels referenced and make a list
	bool bWeUnloadedTheFirstLevel = false;
	t.levelindex=0;
	t.levelmax=0;
	Dim ( t.levellist_s, 100 );

	#ifdef STORYBOARD
	if (g.bUseStoryBoardSetup)
	{

		//Add all storyboard files to scan list.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

		int a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
		while (a == 1)
		{
			++t.levelmax;
			t.levellist_s[t.levelmax] = g_Storyboard_Current_fpm;
			addtocollection(g_Storyboard_Current_fpm);
			a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
		}
		//Restore to first level.
		FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
		g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
		strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

	}
	#endif


	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;
	g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
	while ( t.tlevelstoprocess == 1 ) 
	{
		if ( Len(t.tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			bWeUnloadedTheFirstLevel = true;
		}
		g_mapfile_iNumberOfEntitiesAcrossAllLevels += g.entityelementlist;
		for ( t.e = 1; t.e <= g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if ( t.entid>0 ) 
			{
				// zone marker can reference other levels to jump to
				if ( t.entityprofile[t.entid].ismarker == 3 ) 
				{
					t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
					if ( Len(t.tlevelfile_s.Get()) > 1 ) 
					{
						t.tlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.tlevelfile_s+".fpm";
						if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 ) 
						{
							++t.levelmax;
							t.levellist_s[t.levelmax]=t.tlevelfile_s;
						}
						else
							t.tlevelfile_s="";
					}
				}
			}
		}
		if ( t.levelindex<t.levelmax ) 
		{
			t.tlevelfile_s = "";
			t.tlevelstoprocess = 0;
			while ( t.levelindex<t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
			{
				++t.levelindex;
				t.ttrylevelfile_s=t.levellist_s[t.levelindex];
				for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
				{
					if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
					{
						t.ttrylevelfile_s = "";
						break;
					}
				}
				if ( t.ttrylevelfile_s != "" ) 
				{
					t.tlevelfile_s = t.ttrylevelfile_s;
					t.tlevelstoprocess = 1;
				}
			}
		}
		else
		{
			t.tlevelstoprocess = 0;
		}
	}
	g_mapfile_iNumberOfLevels = 1 + t.levelmax;

	// Stage 2 - collect all files (from all levels)
	t.levelindex=0;
	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;

	if (g.projectfilename_s != t.tmasterlevelfile_s)
		restore_old_map = true;
	g.projectfilename_s = t.tmasterlevelfile_s;
	if ( bWeUnloadedTheFirstLevel == true )
		t.tlevelfile_s = t.tmasterlevelfile_s;
}

int mapfile_savestandalone_stage2b ( void )
{
	int iMoveAlong = 0;
	if ( t.tlevelstoprocess == 1 ) 
	{
		// load in level FPM
		if ( Len(t.tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );

			//LB: Nah - objects not created to produce this file, so...generate it when loading each level in standalone (phew)
			// force generation of nav mesh file!
			//extern void game_createnavmeshfromlevel (bool bYup);
			//game_createnavmeshfromlevel(true);
		}

		// 061018 - check if an FPP file exists for this level file
		cstr pFPPFile = cstr(Left(g.projectfilename_s.Get(),strlen(g.projectfilename_s.Get())-4)) + ".fpp";
		if ( FileExist( pFPPFile.Get() ) == 1 ) 
		{
			// used to specify additional files required for standalone executable
			// handy as a workaround until reported issue resolved
			int iFPPStandaloneExtraFilesMode = 0;
			Dim ( t.data_s, 999 );
			LoadArray ( pFPPFile.Get(), t.data_s );
			for ( t.l = 0 ; t.l <= 999; t.l++ )
			{
				t.line_s = t.data_s[t.l];
				LPSTR pLine = t.line_s.Get();
				if ( Len(pLine) > 0 ) 
				{
					if ( strnicmp ( pLine, "[standalone add files]", 22 ) == NULL )
					{
						// denotes our standalone extra files
						iFPPStandaloneExtraFilesMode = 1;
					}
					else
					{
						if ( strnicmp ( pLine, "[standalone delete files]", 25 ) == NULL )
						{
							// denotes our standalone remove files
							iFPPStandaloneExtraFilesMode = 2;
						}
						else
						{
							// this prevents newer FPP files from getting confused with this original simple method
							if ( iFPPStandaloneExtraFilesMode == 1 )
							{
								// add
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// include whole folder
									addfoldertocollection(pLine);
								}
								else
								{
									// include specific file
									addtocollection(pLine);
								}
							}
							if ( iFPPStandaloneExtraFilesMode == 2 )
							{
								// remove
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// remove whole folder
									g_mapfile_fppFoldersToRemoveList.push_back(cstr(pLine));
								}
								else
								{
									// remove specific file
									g_mapfile_fppFilesToRemoveList.push_back(cstr(pLine));
								}
							}
						}
					}
				}
			}
			UnDim(t.data_s);
		}	

		//  chosen sky, terrain and veg
		addfoldertocollection(cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );
		addfoldertocollection(cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
		#ifdef WICKEDENGINE
		addtocollection("vegbank\\AllGrass.png");
		addtocollection("vegbank\\veg.dbo");
		addtocollection("vegbank\\clump.dbo");
		#else
		addtocollection("skybank\\cloudportal.dds");
		addfoldertocollection(cstr(cstr("vegbank\\")+g.vegstyle_s).Get() );
		#endif

		// start for loop
		t.e = 1;
		g_mapfile_fProgressSpan = g_mapfile_iNumberOfEntitiesAcrossAllLevels;
	}
	else
	{
		iMoveAlong = 1;
	}
	return iMoveAlong;
}

int mapfile_savestandalone_stage2c ( void )
{
	// choose all entities and associated files
	int iMoveAlong = 0;
	if ( t.e <= g.entityelementlist )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			// Check for custom images loaded in lua script
			if ( t.entityelement[t.e].eleprof.aimain_s != "" )
			{
				cstr tLuaScript = g.fpscrootdir_s+"\\Files\\scriptbank\\";
				tLuaScript += t.entityelement[t.e].eleprof.aimain_s;
				FILE* tLuaScriptFile = GG_fopen ( tLuaScript.Get() , "r" );
				if ( tLuaScriptFile )
				{
					char tTempLine[2048];
					while ( !feof(tLuaScriptFile) )
					{
						fgets ( tTempLine , 2047 , tLuaScriptFile );
						if ( strstr ( tTempLine , "LoadImages" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr tFolderToAdd = cstr( cstr("scriptbank\\images\\") + cstr(pImageFolder) );
									addfoldertocollection ( tFolderToAdd.Get() );
								}
							}
						}

						// Handle new load image and sound commands, they can be in nested folders
						if ( strstr ( tTempLine , "LoadImage " ) 
						||	 strstr ( tTempLine , "LoadImage(" )
						||	 strstr ( tTempLine , "LoadGlobalSound(" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr pFile = cstr(pImageFolder);
									addtocollection ( pFile.Get() );
								}
							}
						}
						if (strstr(tTempLine, "SetSkyTo(" )) {
							char* pSkyFolder = strstr(tTempLine, "\"");
							if (pSkyFolder)
							{
								pSkyFolder++;
								char* pSkyFolderEnd = strstr(pSkyFolder, "\"");
								if (pSkyFolderEnd)
								{
									*pSkyFolderEnd = '\0';
									cstr tFolderToAdd = cstr(cstr("skybank\\") + cstr(pSkyFolder));
									addfoldertocollection(tFolderToAdd.Get());
								}
							}
						}
					}
					fclose ( tLuaScriptFile );
				}
			}

			//  entity profile file
			t.tentityname1_s=cstr("entitybank\\")+t.entitybank_s[t.entid];
			t.tentityname2_s=cstr(Left(t.tentityname1_s.Get(),Len(t.tentityname1_s.Get())-4))+".bin";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tentityname2_s).Get() ) == 1 ) 
			{
				t.tentityname_s=t.tentityname2_s;
			}
			else
			{
				t.tentityname_s=t.tentityname1_s;
			}
			addtocollection(t.tentityname_s.Get());

			//  entity files in folder
			t.tentityfolder_s=t.tentityname_s;
			for ( t.n = Len(t.tentityname_s.Get()) ; t.n >= 1 ; t.n+= -1 )
			{
				if (  cstr(Mid(t.tentityname_s.Get(),t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(),t.n)) == "/" ) 
				{
					t.tentityfolder_s=Left(t.tentityfolder_s.Get(),t.n);
					break;
				}
			}

			//  model files (main model, final appended model and all other append
			int iModelAppendFileCount = t.entityprofile[t.entid].appendanimmax;
			if ( Len ( t.entityappendanim[t.entid][0].filename.Get() ) > 0 ) iModelAppendFileCount = 0;
			for ( int iModels = -1; iModels <= iModelAppendFileCount; iModels++ )
			{
				LPSTR pModelFile = "";
				if ( iModels == -1 ) 
				{
					pModelFile = t.entityprofile[t.entid].model_s.Get();
				}
				else
				{
					pModelFile = t.entityappendanim[t.entid][iModels].filename.Get();
				}
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n <= Len(pModelFile); t.n++ )
				{
					if (  cstr(Mid(pModelFile,t.n)) == "\\" || cstr(Mid(pModelFile,t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile1_s=t.tentityfolder_s+pModelFile;
				}
				else
				{
					t.tfile1_s=pModelFile;
				}
				t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				{
					t.tfile_s=t.tfile2_s;
				}
				else
				{
					t.tfile_s=t.tfile1_s;
				}
				t.tmodelfile_s=t.tfile_s;
				addtocollection(t.tmodelfile_s.Get());
				// if entity did not specify texture it is multi-texture, so interogate model file
				// do it for every model
				findalltexturesinmodelfile(t.tmodelfile_s.Get(), t.tentityfolder_s.Get(), t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
			}

			// Export entity FPE BMP file if flagged
			if ( g.gexportassets == 1 ) 
			{
				t.tfile3_s=cstr(Left(t.tentityname_s.Get(),Len(t.tentityname_s.Get())-4))+".bmp";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
				{
					addtocollection(t.tfile3_s.Get());
				}
			}

			// entity characterpose file (if any)
			t.tfile3_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dat";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
			{
				addtocollection(t.tfile3_s.Get());
			}

			//  texture files
			for ( int iBothTypes = 0; iBothTypes < 2; iBothTypes++ )
			{
				// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
				cstr pTextureFile = "", pAltTextureFile = "";
				if ( iBothTypes == 0 ) { pTextureFile = t.entityelement[t.e].eleprof.texd_s; pAltTextureFile = t.entityelement[t.e].eleprof.texaltd_s; } 
				if ( iBothTypes == 1 ) { pTextureFile = t.entityprofile[t.entid].texd_s; pAltTextureFile = t.entityprofile[t.entid].texaltd_s; } 

				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(pTextureFile.Get()); t.n++ )
				{
					if (  cstr(Mid(pTextureFile.Get(),t.n)) == "\\" || cstr(Mid(pTextureFile.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pTextureFile;
				}
				else
				{
					t.tfile_s=pTextureFile;
				}
				addtocollection(t.tfile_s.Get());

				// always allow a DDS texture of same name to be copied over (for test game compatibility)
				for ( int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++ )
				{
					if ( iTwoExtensions == 0 ) t.tfileext_s = Right ( t.tfile_s.Get(), 3);
					if ( iTwoExtensions == 1 ) t.tfileext_s = "dds";
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),6)),2)) == "_d" ) 
					{
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_n."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_s."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_i."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_o."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_cube."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					}
					int iNewPBRTextureMode = 0;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),10)),6)) == "_color" ) iNewPBRTextureMode = 6+4;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),11)),7)) == "_albedo" ) iNewPBRTextureMode = 7+4;
					if ( iNewPBRTextureMode > 0 ) 
					{
						cstr pToAdd;
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_color."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_albedo."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_normal."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_specular."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_metalness."+t.tfileext_s; addtocollection(pToAdd.Get());
						#ifdef WICKEDENGINE
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_surface." + t.tfileext_s; addtocollection(pToAdd.Get());
						#endif
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_gloss."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_mask."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_ao."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_height."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_detail."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_illumination." + t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_illum." + t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_emissive."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_cube." + t.tfileext_s; addtocollection(pToAdd.Get());
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pAltTextureFile;
				}
				else
				{
					t.tfile_s=pAltTextureFile;
				}
				addtocollection(t.tfile_s.Get());
			}

			// also include textures specified by textureref entries (from importer export)
			cstr tFPEFilePath = g.fpscrootdir_s+"\\Files\\";
			tFPEFilePath += t.tentityname1_s;
			FILE* tFPEFile = GG_fopen ( tFPEFilePath.Get() , "r" );
			if ( tFPEFile )
			{
				char tTempLine[2048];
				while ( !feof(tFPEFile) )
				{
					fgets ( tTempLine , 2047 , tFPEFile );
					if ( strstr ( tTempLine , "textureref" ) )
					{
						char* pToFilename = strstr ( tTempLine , "=" );
						if ( pToFilename )
						{
							while ( *pToFilename == '=' || *pToFilename == 32 ) pToFilename++;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							cstr tTextureFile = cstr( t.tentityfolder_s + cstr(pToFilename) );
							addtocollection ( tTextureFile.Get() );
						}
					}
				}
				fclose ( tFPEFile );
			}

			//#ifdef WICKEDENGINE
			//// Not all emissive files are picked up here, so grab them from the entity if they are there.
			//if (t.entityelement[t.e].eleprof.bCustomWickedMaterialActive)
			//{
			//	sObject* pObject = GetObjectData(t.entityelement[t.e].obj);
			//	if (pObject)
			//	{
			//		for (int i = 0; i < pObject->iMeshCount; i++)
			//		{
			//			sMesh* pMesh = pObject->ppMeshList[i];
			//			if (pMesh)
			//			{
			//				wiScene::MeshComponent* pMeshComponent = wiScene::GetScene().meshes.GetComponent(pMesh->wickedmeshindex);
			//				if (pMeshComponent)
			//				{
			//					uint64_t materialEntity = pMeshComponent->subsets[0].materialID;
			//					wiScene::MaterialComponent* pMeshMaterial = wiScene::GetScene().materials.GetComponent(materialEntity);
			//					if (pMeshMaterial)
			//					{
			//						if (pMeshMaterial->textures[3].name.length() > 0)
			//						{
			//							addtocollection((char*)pMeshMaterial->textures[3].name.c_str());
			//						}
			//					}
			//				}
			//			}
			//		}
			//	}
			//}
			//#endif

			//  shader file
			t.tfile_s=t.entityelement[t.e].eleprof.effect_s ; addtocollection(t.tfile_s.Get());
			//Try to take the .blob.
			if (cstr(Lower(Right(t.tfile_s.Get(), 3))) == ".fx") {
				t.tfile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 3);
				t.tfile_s = t.tfile_s + ".blob";
				if (FileExist(t.tfile_s.Get()) == 1)
				{
					addtocollection(t.tfile_s.Get());
				}
			}
			//  script files
			t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s ; addtocollection(t.tfile_s.Get());
			//  for the script associated, scan it and include any references to other scripts
			scanscriptfileandaddtocollection(t.tfile_s.Get());
			//  sound files
			//PE: Make sure voiceset from player start marker is added.
			if (t.entityprofile[t.entid].ismarker == 1 && t.entityelement[t.e].eleprof.soundset_s.Len() > 0) {
				t.tfile_s = t.entityelement[t.e].eleprof.soundset_s;
				addfoldertocollection(cstr(cstr("audiobank\\voices\\") + cstr(t.tfile_s.Get())).Get());
			}
			t.tfile_s=t.entityelement[t.e].eleprof.soundset_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset1_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset2_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset3_s ; addtocollection(t.tfile_s.Get());
			///t.tfile_s=t.entityelement[t.e].eleprof.soundset4_s ; addtocollection(t.tfile_s.Get());
			#ifdef WICKEDENGINE
			t.tfile_s = t.entityelement[t.e].eleprof.soundset5_s; addtocollection(t.tfile_s.Get());
			t.tfile_s = t.entityelement[t.e].eleprof.soundset6_s; addtocollection(t.tfile_s.Get());
			t.tfile_s = t.entityelement[t.e].eleprof.overrideanimset_s; addtocollection(t.tfile_s.Get());
			#endif
			//  collectable guns
			cstr pGunPresent = "";
			if ( Len(t.entityprofile[t.entid].isweapon_s.Get()) > 1 ) pGunPresent = t.entityprofile[t.entid].isweapon_s;
			if ( t.entityprofile[t.entid].isammo == 0 )
			{
				// 270618 - only accept HASWEAPON if NOT ammo, so executables are not bloated with ammo that specifies another weapon type
				if ( Len(t.entityelement[t.e].eleprof.hasweapon_s.Get()) > 1 ) pGunPresent = t.entityelement[t.e].eleprof.hasweapon_s;
			}
			if ( Len(pGunPresent.Get()) > 1 )
			{
				t.tfile_s=cstr("gamecore\\guns\\")+pGunPresent; addfoldertocollection(t.tfile_s.Get());
				t.findgun_s = Lower( pGunPresent.Get() ) ; 
				gun_findweaponindexbyname ( );
				if ( t.foundgunid > 0 ) 
				{
					// ammo and brass
					for ( t.x = 0; t.x <= 1; t.x++ )
					{
						// ammo files
						t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
						if (  t.tpoolindex>0 ) 
						{
							t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s;
							if ( PathExist ( t.tfile_s.Get() ) ) addfoldertocollection(t.tfile_s.Get());
						}

						// brass files
						int iBrassIndex = g.firemodes[t.foundgunid][t.x].settings.brass;
						if ( iBrassIndex > 0 ) 
						{
							t.tfile_s = cstr(cstr("gamecore\\brass\\brass")+Str(iBrassIndex));
							if ( PathExist ( t.tfile_s.Get() ) )
								addfoldertocollection(t.tfile_s.Get());
						}
					}

					// and any projectile files associated with it
					cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
					if ( Len(pProjectilePresent.Get()) > 1 )
					{
						t.tfile_s=cstr("gamecore\\projectiletypes\\")+pProjectilePresent; 
						addfoldertocollection(t.tfile_s.Get());
					}
				}
			}
			// zone marker can reference other levels to jump to
			if ( t.entityprofile[t.entid].ismarker == 3 ) 
			{
				t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
				if ( Len(t.tlevelfile_s.Get())>1 ) 
				{
					t.tlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.tlevelfile_s+".fpm";
					if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 ) 
					{
						//++t.levelmax; // created earlier now
						//t.levellist_s[t.levelmax]=t.tlevelfile_s;
						addtocollection(t.tlevelfile_s.Get());
					}
					else
					{
						// nope, just a regular string entry in the marker field
						t.tlevelfile_s="";
					}
				}
			}
		}
	}
	else
	{
		iMoveAlong = 1;
	}
	t.e++;
	return iMoveAlong;
}

void mapfile_savestandalone_stage2d ( void )
{
	// decide if another level needs loading/processing
	if ( t.levelindex<t.levelmax ) 
	{
		t.tlevelfile_s = "";
		t.tlevelstoprocess = 0;
		while ( t.levelindex<t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
		{
			++t.levelindex;
			t.ttrylevelfile_s=t.levellist_s[t.levelindex];
			for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
			{
				if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
				{
					t.ttrylevelfile_s = "";
					break;
				}
			}
			if ( t.ttrylevelfile_s != "" ) 
			{
				t.tlevelfile_s = t.ttrylevelfile_s;
				t.tlevelstoprocess = 1;
			}
		}
	}
	else
	{
		t.tlevelstoprocess = 0;
	}
}

void mapfile_savestandalone_stage2e ( void )
{
	//  if multi-level, do NOT include the levelbank\testmap temp files
	t.tignorelevelbankfiles=0;
	if (  g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		timestampactivity(0,"Ignoring levelbank testmap folder for multilevel standalone");
		t.tignorelevelbankfiles=1;
	}
	else
	{
		//addfoldertocollection("levelbank\\testmap"); // 190417 - dont need contents, comes from FPM load!
		addtocollection("levelbank\\testmap\\header.dat");
	}
}

void mapfile_savestandalone_stage3 ( void )
{
	// prompt
	//popup_text_change("Saving Standalone Game : Creating Paths");

	//  Create game folder
	SetDir (  t.exepath_s.Get() );
	MakeDirectory (  t.exename_s.Get() );
	SetDir (  t.exename_s.Get() );
	MakeDirectory (  "Files" );
	SetDir (  "Files" );

	//  Ensure gamesaves files are removed (if any)
	if (  PathExist("gamesaves") == 1 ) 
	{
		SetDir (  "gamesaves" );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  Len(t.tfile_s.Get())>2 ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  ".." );
	}

	//  Ensure file path exists (by creating folders)
	createallfoldersincollection();

	// If not copying levelbank files, must still create the folder
	if ( t.tignorelevelbankfiles == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  cstr(t.exepath_s+t.exename_s+"\\Files").Get() );
		if (  PathExist("levelbank") == 0  )  MakeDirectory (  "levelbank" );
		SetDir (  "levelbank" );
		if (  PathExist("testmap") == 0  )  MakeDirectory (  "testmap" );
		SetDir (  "testmap" );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		if (  PathExist("ttsfiles") == 0  )  MakeDirectory (  "ttsfiles" );
		SetDir (  t.olddir_s.Get() );
	}

	// If existing standalone there, ensure lightmaps are removed (as they will be unintentionally encrypted)
	t.destpath_s=t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\lightmaps";
	if (  PathExist(t.destpath_s.Get()) == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  t.destpath_s.Get() );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  t.olddir_s.Get() );
	}

	// If existing standalone there, ensure ttsfiles are removed
	t.destpath_s=t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\ttsfiles";
	if ( PathExist(t.destpath_s.Get()) == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir ( t.destpath_s.Get() );
		ChecklistForFiles ( );
		for ( t.c = 1 ; t.c <= ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if ( t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if ( FileExist(t.tfile_s.Get()) == 1  ) DeleteAFile ( t.tfile_s.Get() );
			}
		}
		SetDir (  t.olddir_s.Get() );
	}

	// 010917 - go through and remove any X files that have DBO counterparts
	SetDir ( cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		if ( FileExist(t.src_s.Get()) == 1 ) 
		{
			char pSrcFile[1024];
			strcpy ( pSrcFile, t.filecollection_s[t.fileindex].Get() );
			if ( strnicmp ( pSrcFile + strlen(pSrcFile) - 4, ".dbo", 4 ) == NULL )
			{
				cstr dboequiv = cstr(Left(pSrcFile,strlen(pSrcFile)-4))+".x";
				if ( FileExist(dboequiv.Get()) == 1 ) 
				{
					// Found DBO, and an X file sitting alongside it, remove the X from consideration
					removefromcollection ( dboequiv.Get() );
				}
			}
		}
	}

	// also remove folders/files marked by FPP file
	if ( g_mapfile_fppFoldersToRemoveList.size() > 0 || g_mapfile_fppFilesToRemoveList.size() > 0 )
	{
		for ( int n = 0; n < g_mapfile_fppFoldersToRemoveList.size(); n++ )
		{
			cstr pRemoveFolder = g_mapfile_fppFoldersToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFolder.Get() );
		}
		for ( int n = 0; n < g_mapfile_fppFilesToRemoveList.size(); n++ )
		{
			cstr pRemoveFile = g_mapfile_fppFilesToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFile.Get() );
		}
	}
}

void mapfile_savestandalone_stage4 ( void )
{
	// restore dir before proceeding
	SetDir(t.told_s.Get());

	//  CopyAFile (  collection to exe folder )
	t.filesmax = g.filecollectionmax;
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		char pRealSrc[MAX_PATH];
		strcpy(pRealSrc, t.src_s.Get());
		GG_GetRealPath(pRealSrc, 0);
		if ( FileExist(pRealSrc) == 1 ) 
		{
			t.dest_s = t.exepath_s+t.exename_s+"\\Files\\"+t.src_s;
			if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
			CopyAFile ( pRealSrc, t.dest_s.Get() );
		}
	}

	// switch to original root to copy exe files and dependencies
	SetDir ( g.originalrootdir_s.Get() );

	//  Copy game engine and rename it
	t.dest_s=t.exepath_s+t.exename_s+"\\"+t.exename_s+".exe";
	if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
	#ifdef ENABLEIMGUI
	if (FileExist("VR Quest App.exe") == 1)
		CopyAFile("VR Quest App.exe", t.dest_s.Get());
	else if (FileExist("GameGuruMAX.exe") == 1)
		CopyAFile("GameGuruMAX.exe", t.dest_s.Get());
	else
		CopyAFile ( "Guru-MapEditor.exe", t.dest_s.Get() );
	#else
	CopyAFile ( "Guru-MapEditor.exe", t.dest_s.Get() );
	#endif

	// Copy critical DLLs
	#ifdef WICKEDENGINE

	// AssImp DLL tied to executable, may also need it for non-DBO model loading?!
	char pCritDLLFilename[MAX_PATH];
	strcpy(pCritDLLFilename, "assimp.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
	CopyAFile ( pCritDLLFilename, t.dest_s.Get() );

	// Steam DLL now required for authentication step (not needed for standalone game running)
	strcpy(pCritDLLFilename, "steam_api64.dll");
	t.dest_s = t.exepath_s + t.exename_s + "\\" + pCritDLLFilename;
	if (FileExist(t.dest_s.Get()) == 1) DeleteAFile (t.dest_s.Get());
	CopyAFile (pCritDLLFilename, t.dest_s.Get());

	// for wicked, create fonts and shaders folder
	cstr destExeRoot_s = t.exepath_s + t.exename_s;
	SetDir ( destExeRoot_s.Get() );
	if (PathExist("fonts") == 0) MakeDirectory("fonts");
	SetDir ( destExeRoot_s.Get() );
	if (PathExist("shaders") == 0) MakeDirectory("shaders");

	// for wicked, copy fonts folder
	SetDir ( g.originalrootdir_s.Get() );
	SetDir("fonts");
	ChecklistForFiles();
	for ( int c = 1; c <= ChecklistQuantity(); c++ )
	{
		LPSTR pFontFile = ChecklistString(c);
		if (stricmp(pFontFile, ".") != NULL && stricmp(pFontFile, "..") != NULL)
		{
			t.dest_s = t.exepath_s + t.exename_s + "\\fonts\\" + pFontFile;
			if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
			CopyAFile(pFontFile, t.dest_s.Get());
		}
	}
	// for wicked, copy shaders folder
	SetDir ( g.originalrootdir_s.Get() );
	SetDir("shaders");
	ChecklistForFiles();
	for ( int c = 1; c <= ChecklistQuantity(); c++ )
	{
		LPSTR pShaderFile = ChecklistString(c);
		if (stricmp(pShaderFile, ".") != NULL && stricmp(pShaderFile, "..") != NULL)
		{
			t.dest_s = t.exepath_s + t.exename_s + "\\shaders\\" + pShaderFile;
			if (FileExist(t.dest_s.Get()) == 1) DeleteAFile(t.dest_s.Get());
			CopyAFile(pShaderFile, t.dest_s.Get());
		}
	}

	// restore to original folder 
	SetDir ( g.originalrootdir_s.Get() );

	#else
	#ifdef PHOTONMP
	 int iSkipSteamFilesIfPhoton = 4; // do not need GGWMR any more!
	#else
	 int iSkipSteamFilesIfPhoton = 7;
	#endif
	for ( int iCritDLLs = 1; iCritDLLs <= iSkipSteamFilesIfPhoton; iCritDLLs++ )
	{
		LPSTR pCritDLLFilename = "";
		switch ( iCritDLLs )
		{
			case 1 : pCritDLLFilename = "avcodec-57.dll"; break;
			case 2 : pCritDLLFilename = "avformat-57.dll"; break;
			case 3 : pCritDLLFilename = "avutil-55.dll"; break;
			case 4 : pCritDLLFilename = "swresample-2.dll"; break;
			//case 5 : pCritDLLFilename = "GGWMR.dll"; break;
			case 5 : pCritDLLFilename = "steam_api.dll"; break;
			case 6 : pCritDLLFilename = "sdkencryptedappticket.dll"; break;
		}
		t.dest_s=t.exepath_s+t.exename_s+"\\"+pCritDLLFilename;
		if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
		CopyAFile ( pCritDLLFilename, t.dest_s.Get() );
	}
	#endif

	// Copy steam files (see above)
	#ifdef PHOTONMP
	 // No Steam in Photon build
	#else
	 t.dest_s=t.exepath_s+t.exename_s+"\\steam_appid.txt";
	 if ( FileExist(t.dest_s.Get()) == 1  ) DeleteAFile (  t.dest_s.Get() );
	 if ( FileExist("steam_appid.txt") == 1  ) CopyAFile ( "steam_appid.txt",t.dest_s.Get() );
	#endif

	// copy visuals settings file
	t.visuals=t.gamevisuals ; visuals_save ( );
	
	// if visuals exists, switch to root folder and save to executable folder
	if ( FileExist("visuals.ini") == 1 ) 
	{
		SetDir (  g.fpscrootdir_s.Get() ); // odd this, but already set dir to root further up!
		char pSrcVisFile[MAX_PATH];
		strcpy(pSrcVisFile, "visuals.ini");
		t.dest_s=t.exepath_s+t.exename_s+"\\visuals.ini";
		#ifdef WICKEDENGINE
		char pRealVisFile[MAX_PATH];
		strcpy(pRealVisFile, t.dest_s.Get());
		GG_GetRealPath(pRealVisFile, 1);
		t.dest_s = pRealVisFile;
		char pRealSrcVisFile[MAX_PATH];
		strcpy(pRealSrcVisFile, pSrcVisFile);
		GG_GetRealPath(pRealSrcVisFile, 1);
		strcpy(pSrcVisFile, pRealSrcVisFile);
		#endif
		if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
		CopyAFile ( pSrcVisFile, t.dest_s.Get() );
	}
	t.visuals=t.editorvisuals  ; visuals_save ( );

	//  Create a setup.ini file here reflecting game
	Dim (  t.setuparr_s,999  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\setup.ini" ; t.i=0;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMERUN]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vsync="+Str(g.gvsync) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "superflatterrain="+Str(t.terrain.superflat) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "smoothcamerakeys="+Str(g.globals.smoothcamerakeys) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionmode="+Str(g.globals.occlusionmode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionsize="+Str(g.globals.occlusionsize) ; ++t.i;
	if ( g.vrqcontrolmode != 0 )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning=1" ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning="+Str(g.globals.hidelowfpswarning) ; ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hardwareinfomode="+Str(g.ghardwareinfomode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "fullscreen="+Str(g.gfullscreen) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aspectratio="+Str(g.gaspectratio) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "dividetexturesize="+Str( g.gdividetexturesize ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "producelogfiles=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "adapterordinal="+Str( g.gadapterordinal ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "adapterd3d11only="+Str( g.gadapterd3d11only ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidedistantshadows=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "invmouse="+Str( g.gminvert ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disablerightmousehold="+Str( g.gdisablerightmousehold ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "profileinstandalone="+Str( 0 ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "allowfragmentation="+Str( t.game.allowfragmentation ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "pbroverride="+Str( g.gpbroverride ) ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipibr=" + Str(g.memskipibr); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "underwatermode=" + Str(g.underwatermode); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "usegrassbelowwater=" + Str(g.usegrassbelowwater); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipwatermask=" + Str(g.memskipwatermask); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "lowestnearcamera=" + Str(g.lowestnearcamera); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "standalonefreememorybetweenlevels=" + Str(g.standalonefreememorybetweenlevels); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videoprecacheframes=" + Str(g.videoprecacheframes); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisabletreeobstacles=" + Str(g.aidisabletreeobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisableobstacles=" + Str(g.aidisableobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "skipunusedtextures=" + Str(g.skipunusedtextures); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videodelayedload=" + Str(g.videodelayedload); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxtotalmeshlights=" + Str(g.maxtotalmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxpixelmeshlights=" + Str(g.maxpixelmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainoldlight=" + Str(g.terrainoldlight); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainusevertexlights=" + Str(g.terrainusevertexlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxterrainlights=" + Str(g.maxterrainlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainlightfadedistance=" + Str(g.terrainlightfadedistance); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "showstaticlightinrealtime=" + Str(g.showstaticlightinrealtime); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "drawcalloptimizer=" + Str(g.globals.drawcalloptimizer); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "forcenowaterreflection=" + Str(g.globals.forcenowaterreflection); ++t.i;
	
	
	if ( t.DisableDynamicRes == false )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 0 ) ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 1 ) ; ++t.i;
	}

	
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowdistance=" + Str(g.globals.realshadowdistancehigh); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowresolution="+Str(g.globals.realshadowresolution) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascadecount="+Str(g.globals.realshadowcascadecount) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "flashlightshadows=" + Str(g.globals.flashlightshadows); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "speedshadows=" + Str(g.globals.speedshadows); ++t.i;
	
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade0="+Str(g.globals.realshadowcascade[0]) ; ++t.i;
	if (g.globals.realshadowsize[0] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize0=" + Str(g.globals.realshadowsize[0]); ++t.i;	
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade1="+Str(g.globals.realshadowcascade[1]) ; ++t.i;
	if (g.globals.realshadowsize[1] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize1=" + Str(g.globals.realshadowsize[1]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade2="+Str(g.globals.realshadowcascade[2]) ; ++t.i;
	if (g.globals.realshadowsize[2] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize2=" + Str(g.globals.realshadowsize[2]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade3="+Str(g.globals.realshadowcascade[3]) ; ++t.i;
	if (g.globals.realshadowsize[3] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize3=" + Str(g.globals.realshadowsize[3]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade4="+Str(g.globals.realshadowcascade[4]) ; ++t.i;
	if (g.globals.realshadowsize[4] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize4=" + Str(g.globals.realshadowsize[4]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade5="+Str(g.globals.realshadowcascade[5]) ; ++t.i;
	if (g.globals.realshadowsize[5] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize5=" + Str(g.globals.realshadowsize[5]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade6="+Str(g.globals.realshadowcascade[6]) ; ++t.i;
	if (g.globals.realshadowsize[6] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize6=" + Str(g.globals.realshadowsize[6]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade7="+Str(g.globals.realshadowcascade[7]) ; ++t.i;
	if (g.globals.realshadowsize[7] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize7=" + Str(g.globals.realshadowsize[7]); ++t.i;
	}

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "enableplrspeedmods=" + Str(g.globals.enableplrspeedmods); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disableweaponjams=" + Str(g.globals.disableweaponjams); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMEMENUOPTIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowterrain="+g.graphicslowterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowentity="+g.graphicslowentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowgrass="+g.graphicslowgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumterrain="+g.graphicsmediumterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumentity="+g.graphicsmediumentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumgrass="+g.graphicsmediumgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighterrain="+g.graphicshighterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighentity="+g.graphicshighentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighgrass="+g.graphicshighgrass_s; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CUSTOMIZATIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "switchtoalt="+Str(g.ggunaltswapkey1) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "melee key="+Str(g.ggunmeleekey) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "zoomholdbreath="+Str(g.gzoomholdbreath) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyUP="+Str(t.listkey[1]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyDOWN="+Str(t.listkey[2]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyLEFT="+Str(t.listkey[3]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRIGHT="+Str(t.listkey[4]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyJUMP="+Str(t.listkey[5]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyCROUCH="+Str(t.listkey[6]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyENTER="+Str(t.listkey[7]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRELOAD="+Str(t.listkey[8]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKLEFT="+Str(t.listkey[9]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKRIGHT="+Str(t.listkey[10]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRUN="+Str(t.listkey[11]) ; ++t.i;

	// vr extras
	if ( g.vrqcontrolmode != 0 || g.gxbox != 0 )
	{
		// CONTROLLER
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CONTROLLER]" ; ++t.i;
		if ( g.vrqcontrolmode != 0 )
		{
			if ( g.vrqorggcontrolmode == 2 )
			{
				// No controller by default in EDU mode
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=0"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
			else
			{
				//PE: Could not get standalone working , until i see xbox=1 , should it not be based on original setup.ini g.gxbox ?
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=";+Str(g.gxbox); ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
		}
		else
		{
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox="+Str(g.gxbox); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype="+Str(g.gxboxcontrollertype); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert="+Str(g.gxboxinvert) ; ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag="+Str(g.gxboxmag) ; ++t.i;
		}
	}
	if ( g.vrqcontrolmode != 0 )
	{
		// VR
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
		#ifdef WICKEDENGINE
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[VR]" ; ++t.i;
		extern bool g_bStandaloneVRMode;
		if (g_bStandaloneVRMode == true)
		{
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=3"; ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodemag=100"; ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vroffsetangx="+Str(g.gvroffsetangx); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrwmroffsetangx="+Str(g.gvrwmroffsetangx); ++t.i;
		}
		else
		{
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=0"; ++t.i;
		}
		#else
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[VR]" ; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=3"; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodemag=100"; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vroffsetangx="+Str(g.gvroffsetangx); ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrwmroffsetangx="+Str(g.gvrwmroffsetangx); ++t.i;
		#endif
	}

	if (  FileExist(t.setupfile_s.Get()) == 1  )  DeleteAFile (  t.setupfile_s.Get() );
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	//  Also save out the localisation ptr file
	Dim (  t.setuparr_s,2  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\userdetails.ini";
	t.setuparr_s[0]="[LOCALIZATION]";
	t.setuparr_s[1]=cstr("language=")+g.language_s;
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	//  Itinery of all files in standalone
	t.titineryfile_s=t.exepath_s+t.exename_s+"\\contents.txt";
	SaveArray (  t.titineryfile_s.Get(),t.filecollection_s );

	//  cleanup file array
	UnDim (  t.filecollection_s );
}

int mapfile_savestandalone_continue ( void )
{
	int iSuccess = 0;
	//#ifdef WICKEDENGINE
	//static bool bOnceOnly = false;
	//#endif
	switch ( g_mapfile_iStage )
	{
		case 1 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 5.0f )
						g_mapfile_iStage = 20;
					break;

		case 20 :	mapfile_savestandalone_stage2a();
					g_mapfile_fProgress = 6.0f; 
					g_mapfile_iStage = 21;
					break;

		case 21 :	if ( mapfile_savestandalone_stage2b() == 0 )
					{
						g_mapfile_iStage = 22;
						//#ifdef WICKEDENGINE
						//bOnceOnly = false;
						//#endif
					}
					else
					{
						g_mapfile_iStage = 29;
					}
					break;

		case 22: 
		{
			//if (!bOnceOnly)
			//{
			//	bOnceOnly = true;
			//	t.e = 1;
			//}
			if (mapfile_savestandalone_stage2c() == 0)
			{
				g_mapfile_fProgress += (80.0f / g_mapfile_fProgressSpan);
			}
			else
			{
				g_mapfile_iStage = 23;
			}
			break;
		}

		case 23 :	mapfile_savestandalone_stage2d();
					g_mapfile_iStage = 21; 
					break;

		case 29 :	mapfile_savestandalone_stage2e(); 
					g_mapfile_iStage = 30; 
					break;

		case 30 :	mapfile_savestandalone_stage3(); 
					g_mapfile_fProgress = 90.0f; 
					g_mapfile_iStage = 40; 
					break;

		case 40 :	mapfile_savestandalone_stage4(); 
					g_mapfile_fProgress = 95.0f; 
					g_mapfile_iStage = 50; 
					break;

		case 50 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 100.0f )
						g_mapfile_iStage = 60;
					break;

		case 60 :	g_mapfile_fProgress = 100.0f; 
					g_mapfile_iStage = 99; 
					iSuccess = 1;
					break;
	}
	return iSuccess;
}

float mapfile_savestandalone_getprogress ( void )
{
	return g_mapfile_fProgress;
}

void mapfile_savestandalone_finish ( void )
{
	#ifdef WICKEDENGINE
	// disable encryption for now, wicked cannot load encrypted images!
	#else
	// encrypt media
	t.dest_s=t.exepath_s+t.exename_s;
	if ( g.gexportassets == 0 ) 
	{
		if ( PathExist( cstr(t.dest_s + "\\Files").Get() ) ) 
		{
			//  NOTE; Need to exclude lightmaps from encryptor  set encrypt ignore list "lightmaps"
			EncryptAllFiles ( cstr(t.dest_s + "\\Files").Get() );
		}
	}
	#endif

	//  if not tignorelevelbankfiles, copy unencrypted files
	if (  t.tignorelevelbankfiles == 0 ) 
	{
		//  now copy the files we do not want to encrypt
		g.filecollectionmax = 0;
		Dim (  t.filecollection_s,500  );

		//  lightmap DBOs
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		t.tfurthestobjnumber=g.lightmappedobjectoffset;
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffset+99999 ; t.tobj+= 100 )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  t.tfurthestobjnumber = t.tobj+100;
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj <= t.tfurthestobjnumber; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffsetfinish; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		t.nmax=500;
		for ( t.n = 0 ; t.n<=  5000 ; t.n+= 100 )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  t.nmax = t.n+100;
		}
		for ( t.n = 0 ; t.n<=  t.nmax; t.n++ )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  addtocollection(t.tfile_s.Get());
		}
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectlist.dat" ; addtocollection(t.tfile_s.Get());
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectnummax.dat" ; addtocollection(t.tfile_s.Get());

		//  Copy the 'unencrypted files' collection to exe folder
		timestampactivity(0, cstr(cstr("filecollectionmax=")+Str(g.filecollectionmax)).Get() );
		SetDir ( cstr(t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap").Get() );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		for ( t.fileindex = 1 ; t.fileindex<=  g.filecollectionmax; t.fileindex++ )
		{
			t.src_s=t.filecollection_s[t.fileindex];
			if (  FileExist(t.src_s.Get()) == 1 ) 
			{
				t.dest_s=t.exepath_s+t.exename_s+"\\Files\\"+t.src_s;
				if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
				CopyAFile (  t.src_s.Get(),t.dest_s.Get() );
			}
		}
	}

	// restore directory, restore original level and close up
	mapfile_savestandalone_restoreandclose();
}

void mapfile_savestandalone_restoreandclose ( void )
{
	// Restore directory
	SetDir ( t.told_s.Get() );

	// restore original level FPM files
	timestampactivity(0, cstr(cstr("check '")+g.projectfilename_s+"' vs '"+t.tmasterlevelfile_s+"'").Get() );
	if ( g.projectfilename_s != t.tmasterlevelfile_s || restore_old_map )
	{
		restore_old_map = false;
		if ( Len(t.tmasterlevelfile_s.Get()) > 1 )
		{
			g.projectfilename_s=t.tmasterlevelfile_s;
			// need to load EVERYTHING back in
			gridedit_load_map ( );
		}
	}

	// finish popup system
	//popup_text_close();

	// no longer making standalone
	t.levelsforstandalone = 0;
}
#else
void mapfile_savestandalone_start ( void )
{
	// first grab current folder for later restoring
	t.told_s=GetDir();

	// In Classic, I would run through the load process and collect files as they
	// where loaded in. In Reloaded, the currently loaded level data is scanned
	// to arrive at the required files for the Standalone EXE
	t.interactive.savestandaloneused=1;

	// this flag ensures the loadassets splash does not appear when making standalone
	t.levelsforstandalone = 1;

	// give prompts while standalone is saving
	//popup_text("Saving Standalone Game : Collecting Files");

	//  check for character creator usage
	characterkit_checkForCharacters ( );

	// 040316 - v1.13b1 - find the nested folder structure of the level (could be in map bank\Easter\level1.fpm)
	if ( g.projectfilename_s.Get()[1] != ':' )
	{
		// relative project path
		g_mapfile_mapbankpath = cstr("mapbank\\");
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}
	else
	{
		// absolute project path
		//mapbankpath = t.told_s + cstr("\\mapbank\\");
		g_mapfile_mapbankpath = g.mysystem.mapbankAbs_s;
		g_mapfile_levelpathfolder = Right ( g.projectfilename_s.Get(), strlen(g.projectfilename_s.Get()) - strlen(g_mapfile_mapbankpath.Get()) );
	}

	bool bGotNestedPath = false;
	for ( int n = Len(g_mapfile_levelpathfolder.Get()) ; n >= 1 ; n+= -1 )
	{
		if ( cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "\\" || cstr(Mid(g_mapfile_levelpathfolder.Get(),n)) == "/" ) 
		{
			g_mapfile_levelpathfolder = Left ( g_mapfile_levelpathfolder.Get(), n );
			bGotNestedPath = true;
			break;
		}
	}
	if ( bGotNestedPath==false )
	{
		// 240316 - V1.131v1 - if NO nested folder, string must be empty!
		g_mapfile_levelpathfolder = "";
	}

	//  Name without EXE
	t.exename_s=g.projectfilename_s;
	if (  cstr(Lower(Right(t.exename_s.Get(),4))) == ".fpm" ) 
	{
		t.exename_s=Left(t.exename_s.Get(),Len(t.exename_s.Get())-4);
	}
	for ( t.n = Len(t.exename_s.Get()) ; t.n >= 1 ; t.n+= -1 )
	{
		if (  cstr(Mid(t.exename_s.Get(),t.n)) == "\\" || cstr(Mid(t.exename_s.Get(),t.n)) == "/" ) 
		{
			t.exename_s=Right(t.exename_s.Get(),Len(t.exename_s.Get())-t.n);
			break;
		}
	}
	//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "mylevel";

	//  the level to start off standalone export
	t.tmasterlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.exename_s+".fpm";
	timestampactivity(0,cstr(cstr("Saving standalone from ")+t.tmasterlevelfile_s).Get() );

	//  Create MYDOCS folder if not exist
	if ( PathExist(g.myownrootdir_s.Get()) == 0 ) file_createmydocsfolder ( );

	// Create absolute My Games folder (if not exist)
	if ( PathExist ( g.exedir_s.Get() ) == 0 )
	{
		g.exedir_s="?";
		SetDir ( g.myownrootdir_s.Get() );
		t.mygamesfolder_s = "My Games";
		if ( PathExist(t.mygamesfolder_s.Get()) == 0 ) MakeDirectory ( t.mygamesfolder_s.Get() );
		if ( PathExist(t.mygamesfolder_s.Get()) == 1 ) 
		{
			SetDir ( t.mygamesfolder_s.Get() );
			g.exedir_s = GetDir();
		}
	}
	SetDir ( t.told_s.Get() );

	// Path to EXE (for dealing with relative EXE paths later)
	if ( g.exedir_s.Get()[1] == ':' )
	{
		t.exepath_s = g.exedir_s;
	}
	else
	{
		t.exepath_s = g.exedir_s;
	}
	if ( cstr(Right(t.exepath_s.Get(),1)) != "\\"  ) t.exepath_s = t.exepath_s+"\\";

	//  Collect ALL files in string array list
	Undim ( t.filecollection_s );
	g.filecollectionmax = 0;
	Dim ( t.filecollection_s, 500 );

	//  Stage 1 - specify all common files
	addtocollection("editors\\gfx\\guru-forexe.ico");
	addtocollection( cstr(cstr("languagebank\\")+g.language_s+"\\textfiles\\guru-wordcount.ini").Get() );
	addtocollection(cstr(cstr("languagebank\\")+g.language_s+"\\textfiles\\guru-words.txt").Get() );
	addtocollection(cstr(cstr("languagebank\\") + g.language_s + "\\inittext.ssp").Get());
	addtocollection("audiobank\\misc\\silence.wav");
	addtocollection("audiobank\\misc\\explode.wav");
	addtocollection("audiobank\\misc\\ammo.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_01.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_02.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_03.wav");
	addtocollection("audiobank\\misc\\Bullet_FlyBy_04.wav");
	addtocollection("scriptbank\\gameloop.lua");
	addtocollection("scriptbank\\gameplayercontrol.lua");
	addtocollection("scriptbank\\global.lua");
	addtocollection("scriptbank\\music.lua");
	addfoldertocollection(cstr(cstr("languagebank\\")+g.language_s+"\\artwork\\watermark").Get() );
	addfoldertocollection("scriptbank\\ai");
	addfoldertocollection("scriptbank\\images");
	addfoldertocollection("audiobank\\materials");
	addfoldertocollection("audiobank\\music\\theescape");
	addfoldertocollection("audiobank\\voices\\player");
	addfoldertocollection("audiobank\\voices\\characters");
	addfoldertocollection("audiobank\\character\\soldier\\onAggro");
	addfoldertocollection("audiobank\\character\\soldier\\onAlert");
	addfoldertocollection("audiobank\\character\\soldier\\onDeath");
	addfoldertocollection("audiobank\\character\\soldier\\onHurt");
	addfoldertocollection("audiobank\\character\\soldier\\onHurtPlayer");
	addfoldertocollection("audiobank\\character\\soldier\\onIdle");
	addfoldertocollection("audiobank\\character\\soldier\\onInteract");
	addfoldertocollection("databank");
	addfoldertocollection("savegames");
	addfoldertocollection("titlesbank\\default\\");
	addtocollection("titlesbank\\cursorcontrol.lua");
	addtocollection("titlesbank\\resolutions.lua");
	addtocollection("titlesbank\\fillgameslots.lua");
	addtocollection("titlesbank\\gamedata.lua");
	addtocollection("titlesbank\\restoregame.lua");
	addfoldertocollection("effectbank\\reloaded");
	addfoldertocollection("effectbank\\reloaded\\media");
	addfoldertocollection("effectbank\\reloaded\\media\\materials");
	addfoldertocollection("effectbank\\explosion");
	addfoldertocollection("effectbank\\particles");
	addfoldertocollection("fontbank");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\ammohealth");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\sliders");
	addfoldertocollection("languagebank\\neutral\\gamecore\\huds\\panels");
	addfoldertocollection("gamecore\\decals\\bloodsplat");
	addfoldertocollection("gamecore\\decals\\impact");
	addfoldertocollection("gamecore\\decals\\gunsmoke");
	addfoldertocollection("gamecore\\decals\\smoke1");
	addfoldertocollection("gamecore\\decals\\muzzleflash4");
	addfoldertocollection("gamecore\\decals\\splash_droplets");
	addfoldertocollection("gamecore\\decals\\splash_foam");
	addfoldertocollection("gamecore\\decals\\splash_large");
	addfoldertocollection("gamecore\\decals\\splash_misty");
	addfoldertocollection("gamecore\\decals\\splash_ripple");
	addfoldertocollection("gamecore\\decals\\splash_small");
	addfoldertocollection("gamecore\\projectiletypes");
	addfoldertocollection("gamecore\\projectiletypes\\fantasy\\fireball");
	addfoldertocollection("gamecore\\projectiletypes\\fantasy\\magicbolt");
	addfoldertocollection("gamecore\\projectiletypes\\modern\\handgrenade");
	addfoldertocollection("gamecore\\projectiletypes\\modern\\rpggrenade");

	// add any material decals that are active
	for ( t.m = 0; t.m <= g.gmaterialmax; t.m++ )
	{
		if ( t.material[t.m].usedinlevel == 1 )
		{
			cstr decalFolder_s = cstr("gamecore\\decals\\")+t.material[t.m].decal_s;
			addfoldertocollection(decalFolder_s.Get());
		}
	}

	addfoldertocollection("gamecore\\muzzleflash");
	addfoldertocollection("gamecore\\projectiletypes");
	//addfoldertocollection("gamecore\\brass"); // now done below when detect guns used in level
	addfoldertocollection("gamecore\\hudlayers");

	//  Stage 1B - Style dependent files
	titles_getstyle ( );
	addtocollection("titlesbank\\style.txt");
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1280x720").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1280x800").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1366x768").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1440x900").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1600x900").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1680x1050").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1920x1080").Get() );
	addfoldertocollection(cstr(cstr("titlesbank\\")+t.ttheme_s+"\\1920x1200").Get() );

	//  include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	//  add character creator folder, if needed
	if (  t.characterkitcontrol.gameHasCharacterCreatorIn  ==  1 ) 
	{
		addfoldertocollection("characterkit\\bodyandhead");
		addfoldertocollection("characterkit\\FacialHair");
		addfoldertocollection("characterkit\\Headattachments");
		addfoldertocollection("characterkit\\Headattachments");
		if (  PathExist("entitybank\\user\\charactercreator")  ==  1 ) 
		{
			addfoldertocollection("entitybank\\user\\charactercreator");
		}
	}

	// Pre-Stage 2 - clear a list which will collect all folders/files to REMOVE from the final standalone file transfer
	// list, courtesy of the special FPP file which controls the final files to be used for standalone creation
	g_mapfile_fppFoldersToRemoveList.clear();
	g_mapfile_fppFilesToRemoveList.clear();

	// process in stages
	g_mapfile_iStage = 1;
	g_mapfile_fProgress = 0.0f;
}

void mapfile_savestandalone_stage2a ( void )
{
	// Stage 2 - count ALL levels referenced and make a list
	bool bWeUnloadedTheFirstLevel = false;
	t.levelindex=0;
	t.levelmax=0;
	Dim ( t.levellist_s, 100 );
	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;
	g_mapfile_iNumberOfEntitiesAcrossAllLevels = 0;
	while ( t.tlevelstoprocess == 1 ) 
	{
		if ( Len(t.tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
			bWeUnloadedTheFirstLevel = true;
		}
		g_mapfile_iNumberOfEntitiesAcrossAllLevels += g.entityelementlist;
		for ( t.e = 1; t.e <= g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if ( t.entid>0 ) 
			{
				// zone marker can reference other levels to jump to
				if ( t.entityprofile[t.entid].ismarker == 3 ) 
				{
					t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
					if ( Len(t.tlevelfile_s.Get()) > 1 ) 
					{
						t.tlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.tlevelfile_s+".fpm";
						if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 ) 
						{
							++t.levelmax;
							t.levellist_s[t.levelmax]=t.tlevelfile_s;
						}
						else
							t.tlevelfile_s="";
					}
				}
			}
		}
		if ( t.levelindex<t.levelmax ) 
		{
			t.tlevelfile_s = "";
			t.tlevelstoprocess = 0;
			while ( t.levelindex<t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
			{
				++t.levelindex;
				t.ttrylevelfile_s=t.levellist_s[t.levelindex];
				for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
				{
					if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
					{
						t.ttrylevelfile_s = "";
						break;
					}
				}
				if ( t.ttrylevelfile_s != "" ) 
				{
					t.tlevelfile_s = t.ttrylevelfile_s;
					t.tlevelstoprocess = 1;
				}
			}
		}
		else
		{
			t.tlevelstoprocess = 0;
		}
	}
	g_mapfile_iNumberOfLevels = 1 + t.levelmax;

	// Stage 2 - collect all files (from all levels)
	t.levelindex=0;
	t.tlevelfile_s="";
	t.tlevelstoprocess = 1;
	g.projectfilename_s = t.tmasterlevelfile_s;
	if ( bWeUnloadedTheFirstLevel == true )
		t.tlevelfile_s = t.tmasterlevelfile_s;
}

int mapfile_savestandalone_stage2b ( void )
{
	int iMoveAlong = 0;
	if ( t.tlevelstoprocess == 1 ) 
	{
		// load in level FPM
		if ( Len(t.tlevelfile_s.Get())>1 ) 
		{
			g.projectfilename_s=t.tlevelfile_s;
			mapfile_loadproject_fpm ( );
			game_loadinentitiesdatainlevel ( );
		}

		// 061018 - check if an FPP file exists for this level file
		cstr pFPPFile = cstr(Left(g.projectfilename_s.Get(),strlen(g.projectfilename_s.Get())-4)) + ".fpp";
		if ( FileExist( pFPPFile.Get() ) == 1 ) 
		{
			// used to specify additional files required for standalone executable
			// handy as a workaround until reported issue resolved
			int iFPPStandaloneExtraFilesMode = 0;
			Dim ( t.data_s, 999 );
			LoadArray ( pFPPFile.Get(), t.data_s );
			for ( t.l = 0 ; t.l <= 999; t.l++ )
			{
				t.line_s = t.data_s[t.l];
				LPSTR pLine = t.line_s.Get();
				if ( Len(pLine) > 0 ) 
				{
					if ( strnicmp ( pLine, "[standalone add files]", 22 ) == NULL )
					{
						// denotes our standalone extra files
						iFPPStandaloneExtraFilesMode = 1;
					}
					else
					{
						if ( strnicmp ( pLine, "[standalone delete files]", 25 ) == NULL )
						{
							// denotes our standalone remove files
							iFPPStandaloneExtraFilesMode = 2;
						}
						else
						{
							// this prevents newer FPP files from getting confused with this original simple method
							if ( iFPPStandaloneExtraFilesMode == 1 )
							{
								// add
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// include whole folder
									addfoldertocollection(pLine);
								}
								else
								{
									// include specific file
									addtocollection(pLine);
								}
							}
							if ( iFPPStandaloneExtraFilesMode == 2 )
							{
								// remove
								if ( pLine[strlen(pLine)-1] == '\\' )
								{
									// remove whole folder
									g_mapfile_fppFoldersToRemoveList.push_back(cstr(pLine));
								}
								else
								{
									// remove specific file
									g_mapfile_fppFilesToRemoveList.push_back(cstr(pLine));
								}
							}
						}
					}
				}
			}
			UnDim(t.data_s);
		}	

		//  chosen sky, terrain and veg
		addtocollection("skybank\\cloudportal.dds");
		addfoldertocollection(cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );
		//addfoldertocollection("skybank\\night"); // no need for NIGHT until we have day/night cycles
		addfoldertocollection(cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
		addfoldertocollection(cstr(cstr("vegbank\\")+g.vegstyle_s).Get() );

		// start for loop
		t.e = 1;
		g_mapfile_fProgressSpan = g_mapfile_iNumberOfEntitiesAcrossAllLevels;
	}
	else
	{
		iMoveAlong = 1;
	}
	return iMoveAlong;
}

int mapfile_savestandalone_stage2c ( void )
{
	// choose all entities and associated files
	int iMoveAlong = 0;
	if ( t.e <= g.entityelementlist )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			// Check for custom images loaded in lua script
			if ( t.entityelement[t.e].eleprof.aimain_s != "" )
			{
				cstr tLuaScript = g.fpscrootdir_s+"\\Files\\scriptbank\\";
				tLuaScript += t.entityelement[t.e].eleprof.aimain_s;
				FILE* tLuaScriptFile = GG_fopen ( tLuaScript.Get() , "r" );
				if ( tLuaScriptFile )
				{
					char tTempLine[2048];
					while ( !feof(tLuaScriptFile) )
					{
						fgets ( tTempLine , 2047 , tLuaScriptFile );
						if ( strstr ( tTempLine , "LoadImages" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr tFolderToAdd = cstr( cstr("scriptbank\\images\\") + cstr(pImageFolder) );
									addfoldertocollection ( tFolderToAdd.Get() );
								}
							}
						}

						// Handle new load image and sound commands, they can be in nested folders
						if ( strstr ( tTempLine , "LoadImage " ) 
						||	 strstr ( tTempLine , "LoadImage(" )
						||   strstr ( tTempLine, "LoadGlobalSound ")
						||	 strstr ( tTempLine, "LoadGlobalSound(" ) )
						{
							char* pImageFolder = strstr ( tTempLine , "\"" );
							if ( pImageFolder )
							{
								pImageFolder++;
								char* pImageFolderEnd = strstr ( pImageFolder , "\"" );
								if ( pImageFolderEnd )
								{
									*pImageFolderEnd = '\0';
									cstr pFile = cstr(pImageFolder);
									addtocollection ( pFile.Get() );
								}
							}
						}
						if (strstr(tTempLine, "SetSkyTo(" )) {
							char* pSkyFolder = strstr(tTempLine, "\"");
							if (pSkyFolder)
							{
								pSkyFolder++;
								char* pSkyFolderEnd = strstr(pSkyFolder, "\"");
								if (pSkyFolderEnd)
								{
									*pSkyFolderEnd = '\0';
									cstr tFolderToAdd = cstr(cstr("skybank\\") + cstr(pSkyFolder));
									addfoldertocollection(tFolderToAdd.Get());
								}
							}
						}
					}
					fclose ( tLuaScriptFile );
				}
			}

			//  entity profile file
			t.tentityname1_s=cstr("entitybank\\")+t.entitybank_s[t.entid];
			t.tentityname2_s=cstr(Left(t.tentityname1_s.Get(),Len(t.tentityname1_s.Get())-4))+".bin";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tentityname2_s).Get() ) == 1 ) 
			{
				t.tentityname_s=t.tentityname2_s;
			}
			else
			{
				t.tentityname_s=t.tentityname1_s;
			}
			addtocollection(t.tentityname_s.Get());

			//  entity files in folder
			t.tentityfolder_s=t.tentityname_s;
			for ( t.n = Len(t.tentityname_s.Get()) ; t.n >= 1 ; t.n+= -1 )
			{
				if (  cstr(Mid(t.tentityname_s.Get(),t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(),t.n)) == "/" ) 
				{
					t.tentityfolder_s=Left(t.tentityfolder_s.Get(),t.n);
					break;
				}
			}

			//  model files (main model, final appended model and all other append
			int iModelAppendFileCount = t.entityprofile[t.entid].appendanimmax;
			if ( Len ( t.entityappendanim[t.entid][0].filename.Get() ) > 0 ) iModelAppendFileCount = 0;
			for ( int iModels = -1; iModels <= iModelAppendFileCount; iModels++ )
			{
				LPSTR pModelFile = "";
				if ( iModels == -1 ) 
				{
					pModelFile = t.entityprofile[t.entid].model_s.Get();
				}
				else
				{
					pModelFile = t.entityappendanim[t.entid][iModels].filename.Get();
				}
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n <= Len(pModelFile); t.n++ )
				{
					if (  cstr(Mid(pModelFile,t.n)) == "\\" || cstr(Mid(pModelFile,t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile1_s=t.tentityfolder_s+pModelFile;
				}
				else
				{
					t.tfile1_s=pModelFile;
				}
				t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				{
					t.tfile_s=t.tfile2_s;
				}
				else
				{
					t.tfile_s=t.tfile1_s;
				}
				t.tmodelfile_s=t.tfile_s;
				addtocollection(t.tmodelfile_s.Get());
				// if entity did not specify texture it is multi-texture, so interogate model file
				// do it for every model
				findalltexturesinmodelfile(t.tmodelfile_s.Get(), t.tentityfolder_s.Get(), t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
			}

			// Export entity FPE BMP file if flagged
			if ( g.gexportassets == 1 ) 
			{
				t.tfile3_s=cstr(Left(t.tentityname_s.Get(),Len(t.tentityname_s.Get())-4))+".bmp";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
				{
					addtocollection(t.tfile3_s.Get());
				}
			}

			// entity characterpose file (if any)
			t.tfile3_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dat";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get() ) == 1 ) 
			{
				addtocollection(t.tfile3_s.Get());
			}

			//  texture files
			for ( int iBothTypes = 0; iBothTypes < 2; iBothTypes++ )
			{
				// can be from ELEPROF of entityelement (older maps point to old texture names) or parent ELEPROF original
				cstr pTextureFile = "", pAltTextureFile = "";
				if ( iBothTypes == 0 ) { pTextureFile = t.entityelement[t.e].eleprof.texd_s; pAltTextureFile = t.entityelement[t.e].eleprof.texaltd_s; } 
				if ( iBothTypes == 1 ) { pTextureFile = t.entityprofile[t.entid].texd_s; pAltTextureFile = t.entityprofile[t.entid].texaltd_s; } 

				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(pTextureFile.Get()); t.n++ )
				{
					if (  cstr(Mid(pTextureFile.Get(),t.n)) == "\\" || cstr(Mid(pTextureFile.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pTextureFile;
				}
				else
				{
					t.tfile_s=pTextureFile;
				}
				addtocollection(t.tfile_s.Get());

				if (cstr(Lower(Right(t.tfile_s.Get(), 3))) == "tga") {
					//PE: Always add the .dds type when tga.
					t.tfileext_s = "dds";
					if (cstr(Left(Lower(Right(t.tfile_s.Get(), 6)), 2)) == "_d")
					{
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_d." + t.tfileext_s; addtocollection(t.tfile_s.Get());
					}
					if (cstr(Left(Lower(Right(t.tfile_s.Get(), 6)), 2)) == "d2")
					{
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "d2." + t.tfileext_s; addtocollection(t.tfile_s.Get());
					}
				}
				// always allow a DDS texture of same name to be copied over (for test game compatibility)
				for ( int iTwoExtensions = 0; iTwoExtensions <= 1; iTwoExtensions++ )
				{
					if ( iTwoExtensions == 0 ) t.tfileext_s = Right ( t.tfile_s.Get(), 3);
					if ( iTwoExtensions == 1 ) t.tfileext_s = "dds";
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),6)),2)) == "_d" ) 
					{
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_n."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_s."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_i."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_o."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
						t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_cube."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					}
					//PE: Support old _d2
					if (cstr(Left(Lower(Right(t.tfile_s.Get(), 6)), 2)) == "d2")
					{
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_n." + t.tfileext_s; addtocollection(t.tfile_s.Get());
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_s." + t.tfileext_s; addtocollection(t.tfile_s.Get());
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_i." + t.tfileext_s; addtocollection(t.tfile_s.Get());
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_o." + t.tfileext_s; addtocollection(t.tfile_s.Get());
						t.tfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 6)) + "_cube." + t.tfileext_s; addtocollection(t.tfile_s.Get());
					}
					int iNewPBRTextureMode = 0;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),10)),6)) == "_color" ) iNewPBRTextureMode = 6+4;
					if ( cstr(Left(Lower(Right(t.tfile_s.Get(),11)),7)) == "_albedo" ) iNewPBRTextureMode = 7+4;
					if ( iNewPBRTextureMode > 0 ) 
					{
						cstr pToAdd;
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_color."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_albedo."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_normal."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_specular."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_metalness."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_gloss."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_ao."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_height."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_detail."+t.tfileext_s; addtocollection(pToAdd.Get());
						//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/315
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_illumination."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-iNewPBRTextureMode))+"_emissive."+t.tfileext_s; addtocollection(pToAdd.Get());
						pToAdd = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - iNewPBRTextureMode)) + "_cube." + t.tfileext_s; addtocollection(pToAdd.Get());

					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+pAltTextureFile;
				}
				else
				{
					t.tfile_s=pAltTextureFile;
				}
				addtocollection(t.tfile_s.Get());
			}

			// also include textures specified by textureref entries (from importer export)
			cstr tFPEFilePath = g.fpscrootdir_s+"\\Files\\";
			tFPEFilePath += t.tentityname1_s;
			FILE* tFPEFile = GG_fopen ( tFPEFilePath.Get() , "r" );
			if ( tFPEFile )
			{
				char tTempLine[2048];
				while ( !feof(tFPEFile) )
				{
					fgets ( tTempLine , 2047 , tFPEFile );
					if ( strstr ( tTempLine , "textureref" ) )
					{
						char* pToFilename = strstr ( tTempLine , "=" );
						if ( pToFilename )
						{
							while ( *pToFilename == '=' || *pToFilename == 32 ) pToFilename++;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 13 ) pToFilename[strlen(pToFilename)-1] = 0;
							if ( pToFilename[strlen(pToFilename)-1] == 10 ) pToFilename[strlen(pToFilename)-1] = 0;
							cstr tTextureFile = cstr( t.tentityfolder_s + cstr(pToFilename) );
							addtocollection ( tTextureFile.Get() );
						}
					}
				}
				fclose ( tFPEFile );
			}

			//  shader file
			t.tfile_s=t.entityelement[t.e].eleprof.effect_s ; addtocollection(t.tfile_s.Get());
			//Try to take the .blob.
			if (cstr(Lower(Right(t.tfile_s.Get(), 3))) == ".fx") {
				t.tfile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 3);
				t.tfile_s = t.tfile_s + ".blob";
				if (FileExist(t.tfile_s.Get()) == 1)
				{
					addtocollection(t.tfile_s.Get());
				}
			}
			//  script files
			t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s ; addtocollection(t.tfile_s.Get());
			//  for the script associated, scan it and include any references to other scripts
			scanscriptfileandaddtocollection(t.tfile_s.Get());
			//  sound files
			//PE: Make sure voiceset from player start marker is added.
			if (t.entityprofile[t.entid].ismarker == 1 && t.entityelement[t.e].eleprof.soundset_s.Len() > 0 ) {
				t.tfile_s = t.entityelement[t.e].eleprof.soundset_s;
				addfoldertocollection( cstr(cstr("audiobank\\voices\\") + cstr(t.tfile_s.Get())).Get() );
			}
			t.tfile_s=t.entityelement[t.e].eleprof.soundset_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset1_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset2_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset3_s ; addtocollection(t.tfile_s.Get());
			t.tfile_s=t.entityelement[t.e].eleprof.soundset4_s ; addtocollection(t.tfile_s.Get());
			//  collectable guns
			cstr pGunPresent = "";
			if ( Len(t.entityprofile[t.entid].isweapon_s.Get()) > 1 ) pGunPresent = t.entityprofile[t.entid].isweapon_s;
			if ( t.entityprofile[t.entid].isammo == 0 )
			{
				// 270618 - only accept HASWEAPON if NOT ammo, so executables are not bloated with ammo that specifies another weapon type
				if ( Len(t.entityelement[t.e].eleprof.hasweapon_s.Get()) > 1 ) pGunPresent = t.entityelement[t.e].eleprof.hasweapon_s;
			}
			if ( Len(pGunPresent.Get()) > 1 )
			{
				t.tfile_s=cstr("gamecore\\guns\\")+pGunPresent; addfoldertocollection(t.tfile_s.Get());
				t.findgun_s = Lower( pGunPresent.Get() ) ; 
				gun_findweaponindexbyname ( );
				if ( t.foundgunid > 0 ) 
				{
					// ammo and brass
					for ( t.x = 0; t.x <= 1; t.x++ )
					{
						// ammo files
						t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
						if (  t.tpoolindex>0 ) 
						{
							t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s;
							if ( PathExist ( t.tfile_s.Get() ) ) addfoldertocollection(t.tfile_s.Get());
						}

						// brass files
						int iBrassIndex = g.firemodes[t.foundgunid][t.x].settings.brass;
						if ( iBrassIndex > 0 ) 
						{
							t.tfile_s = cstr(cstr("gamecore\\brass\\brass")+Str(iBrassIndex));
							if ( PathExist ( t.tfile_s.Get() ) )
								addfoldertocollection(t.tfile_s.Get());
						}
					}

					// and any projectile files associated with it
					cstr pProjectilePresent = t.gun[t.foundgunid].projectile_s;
					if ( Len(pProjectilePresent.Get()) > 1 )
					{
						t.tfile_s=cstr("gamecore\\projectiletypes\\")+pProjectilePresent; 
						addfoldertocollection(t.tfile_s.Get());
					}
				}
			}
			// zone marker can reference other levels to jump to
			if ( t.entityprofile[t.entid].ismarker == 3 ) 
			{
				t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
				if ( Len(t.tlevelfile_s.Get())>1 ) 
				{
					t.tlevelfile_s=cstr("mapbank\\")+g_mapfile_levelpathfolder+t.tlevelfile_s+".fpm";
					if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tlevelfile_s).Get()) == 1 ) 
					{
						//++t.levelmax; // created earlier now
						//t.levellist_s[t.levelmax]=t.tlevelfile_s;
						addtocollection(t.tlevelfile_s.Get());
					}
					else
					{
						// nope, just a regular string entry in the marker field
						t.tlevelfile_s="";
					}
				}
			}
		}
	}
	else
	{
		iMoveAlong = 1;
	}
	t.e++;
	return iMoveAlong;
}

void mapfile_savestandalone_stage2d ( void )
{
	// decide if another level needs loading/processing
	if ( t.levelindex<t.levelmax ) 
	{
		t.tlevelfile_s = "";
		t.tlevelstoprocess = 0;
		while ( t.levelindex<t.levelmax && strcmp ( t.tlevelfile_s.Get(), "" )==NULL ) 
		{
			++t.levelindex;
			t.ttrylevelfile_s=t.levellist_s[t.levelindex];
			for ( t.n = 1; t.n <= t.levelindex-1; t.n++ )
			{
				if ( t.ttrylevelfile_s == t.levellist_s[t.n] ) 
				{
					t.ttrylevelfile_s = "";
					break;
				}
			}
			if ( t.ttrylevelfile_s != "" ) 
			{
				t.tlevelfile_s = t.ttrylevelfile_s;
				t.tlevelstoprocess = 1;
			}
		}
	}
	else
	{
		t.tlevelstoprocess = 0;
	}
}

void mapfile_savestandalone_stage2e ( void )
{
	//  if multi-level, do NOT include the levelbank\testmap temp files
	t.tignorelevelbankfiles=0;
	if (  g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		timestampactivity(0,"Ignoring levelbank testmap folder for multilevel standalone");
		t.tignorelevelbankfiles=1;
	}
	else
	{
		//addfoldertocollection("levelbank\\testmap"); // 190417 - dont need contents, comes from FPM load!
		addtocollection("levelbank\\testmap\\header.dat");
	}
}

void mapfile_savestandalone_stage3 ( void )
{
	// prompt
	//popup_text_change("Saving Standalone Game : Creating Paths");

	//  Create game folder
	SetDir (  t.exepath_s.Get() );
	MakeDirectory (  t.exename_s.Get() );
	SetDir (  t.exename_s.Get() );
	MakeDirectory (  "Files" );
	SetDir (  "Files" );

	//  Ensure gamesaves files are removed (if any)
	if (  PathExist("gamesaves") == 1 ) 
	{
		SetDir (  "gamesaves" );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  Len(t.tfile_s.Get())>2 ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  ".." );
	}

	//  Ensure file path exists (by creating folders)
	createallfoldersincollection();

	//  If not copying levelbank files, must still create the folder
	if (  t.tignorelevelbankfiles == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  cstr(t.exepath_s+t.exename_s+"\\Files").Get() );
		if (  PathExist("levelbank") == 0  )  MakeDirectory (  "levelbank" );
		SetDir (  "levelbank" );
		if (  PathExist("testmap") == 0  )  MakeDirectory (  "testmap" );
		SetDir (  "testmap" );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		SetDir (  t.olddir_s.Get() );
	}

	//  If existing standalone there, ensure lightmaps are removed (as they will be unintentionally encrypted)
	t.destpath_s=t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap\\lightmaps";
	if (  PathExist(t.destpath_s.Get()) == 1 ) 
	{
		t.olddir_s=GetDir();
		SetDir (  t.destpath_s.Get() );
		ChecklistForFiles (  );
		for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  FileExist(t.tfile_s.Get()) == 1  )  DeleteAFile (  t.tfile_s.Get() );
			}
		}
		SetDir (  t.olddir_s.Get() );
	}

	// 010917 - go through and remove any X files that have DBO counterparts
	SetDir ( cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		if ( FileExist(t.src_s.Get()) == 1 ) 
		{
			char pSrcFile[1024];
			strcpy ( pSrcFile, t.filecollection_s[t.fileindex].Get() );
			if ( strnicmp ( pSrcFile + strlen(pSrcFile) - 4, ".dbo", 4 ) == NULL )
			{
				cstr dboequiv = cstr(Left(pSrcFile,strlen(pSrcFile)-4))+".x";
				if ( FileExist(dboequiv.Get()) == 1 ) 
				{
					// Found DBO, and an X file sitting alongside it, remove the X from consideration
					removefromcollection ( dboequiv.Get() );
				}
			}
		}
	}

	// also remove folders/files marked by FPP file
	if ( g_mapfile_fppFoldersToRemoveList.size() > 0 || g_mapfile_fppFilesToRemoveList.size() > 0 )
	{
		for ( int n = 0; n < g_mapfile_fppFoldersToRemoveList.size(); n++ )
		{
			cstr pRemoveFolder = g_mapfile_fppFoldersToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFolder.Get() );
		}
		for ( int n = 0; n < g_mapfile_fppFilesToRemoveList.size(); n++ )
		{
			cstr pRemoveFile = g_mapfile_fppFilesToRemoveList[n];
			removeanymatchingfromcollection ( pRemoveFile.Get() );
		}
	}
}

void mapfile_savestandalone_stage4 ( void )
{
	// prompt
	//popup_text_change("Saving Standalone Game : Copying Files");

	//  CopyAFile (  collection to exe folder )
	for ( t.fileindex = 1 ; t.fileindex<=  t.filesmax; t.fileindex++ )
	{
		t.src_s=t.filecollection_s[t.fileindex];
		if (  FileExist(t.src_s.Get()) == 1 ) 
		{
			t.dest_s=t.exepath_s+t.exename_s+"\\Files\\"+t.src_s;
			if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
			CopyAFile (  t.src_s.Get(),t.dest_s.Get() );
		}
	}

	// switch to original root to copy exe files and dependencies
	SetDir ( g.originalrootdir_s.Get() );

	//  Copy game engine and rename it
	t.dest_s=t.exepath_s+t.exename_s+"\\"+t.exename_s+".exe";
	if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
	CopyAFile ( "Guru-MapEditor.exe", t.dest_s.Get() );

	// Copy critical DLLs
	for ( int iCritDLLs = 1; iCritDLLs <= 6; iCritDLLs++ )
	{
		LPSTR pCritDLLFilename = "";
		switch ( iCritDLLs )
		{
			case 1 : pCritDLLFilename = "steam_api.dll"; break;
			case 2 : pCritDLLFilename = "sdkencryptedappticket.dll"; break;
			case 3 : pCritDLLFilename = "avcodec-57.dll"; break;
			case 4 : pCritDLLFilename = "avformat-57.dll"; break;
			case 5 : pCritDLLFilename = "avutil-55.dll"; break;
			case 6 : pCritDLLFilename = "swresample-2.dll"; break;
		}
		t.dest_s=t.exepath_s+t.exename_s+"\\"+pCritDLLFilename;
		if ( FileExist(t.dest_s.Get()) == 1 ) DeleteAFile ( t.dest_s.Get() );
		CopyAFile ( pCritDLLFilename, t.dest_s.Get() );
	}

	// Copy steam files (see above)
	t.dest_s=t.exepath_s+t.exename_s+"\\steam_appid.txt";
	if ( FileExist(t.dest_s.Get()) == 1  ) DeleteAFile (  t.dest_s.Get() );
	if ( FileExist("steam_appid.txt") == 1  ) CopyAFile ( "steam_appid.txt",t.dest_s.Get() );

	//  Copy visuals settings file
	t.visuals=t.gamevisuals ; visuals_save ( );
	if (  FileExist("visuals.ini") == 1 ) 
	{
		SetDir (  g.fpscrootdir_s.Get() );
		t.dest_s=t.exepath_s+t.exename_s+"\\visuals.ini";
		if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
		CopyAFile (  "visuals.ini",t.dest_s.Get() );
	}
	t.visuals=t.editorvisuals  ; visuals_save ( );

	//  Create a setup.ini file here reflecting game
	Dim (  t.setuparr_s,999  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\setup.ini" ; t.i=0;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMERUN]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vsync="+Str(g.gvsync) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "superflatterrain="+Str(t.terrain.superflat) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "smoothcamerakeys="+Str(g.globals.smoothcamerakeys) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionmode="+Str(g.globals.occlusionmode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "occlusionsize="+Str(g.globals.occlusionsize) ; ++t.i;
	if ( g.vrqcontrolmode != 0 )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning=1" ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidelowfpswarning="+Str(g.globals.hidelowfpswarning) ; ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hardwareinfomode="+Str(g.ghardwareinfomode) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "fullscreen="+Str(g.gfullscreen) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aspectratio="+Str(g.gaspectratio) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "dividetexturesize="+Str( g.gdividetexturesize ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "producelogfiles=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "hidedistantshadows=0"; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainshadows="+Str ( g.globals.terrainshadows); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "invmouse="+Str( g.gminvert ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disablerightmousehold="+Str( g.gdisablerightmousehold ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "profileinstandalone="+Str( 0 ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "allowfragmentation="+Str( t.game.allowfragmentation ) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "pbroverride="+Str( g.gpbroverride ) ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipibr=" + Str(g.memskipibr); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "underwatermode=" + Str(g.underwatermode); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "usegrassbelowwater=" + Str(g.usegrassbelowwater); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "memskipwatermask=" + Str(g.memskipwatermask); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "lowestnearcamera=" + Str(g.lowestnearcamera); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "standalonefreememorybetweenlevels=" + Str(g.standalonefreememorybetweenlevels); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videoprecacheframes=" + Str(g.videoprecacheframes); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisabletreeobstacles=" + Str(g.aidisabletreeobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "aidisableobstacles=" + Str(g.aidisableobstacles); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "skipunusedtextures=" + Str(g.skipunusedtextures); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "videodelayedload=" + Str(g.videodelayedload); ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxtotalmeshlights=" + Str(g.maxtotalmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxpixelmeshlights=" + Str(g.maxpixelmeshlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainoldlight=" + Str(g.terrainoldlight); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainusevertexlights=" + Str(g.terrainusevertexlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "maxterrainlights=" + Str(g.maxterrainlights); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "terrainlightfadedistance=" + Str(g.terrainlightfadedistance); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "showstaticlightinrealtime=" + Str(g.showstaticlightinrealtime); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "drawcalloptimizer=" + Str(g.globals.drawcalloptimizer); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "forcenowaterreflection=" + Str(g.globals.forcenowaterreflection); ++t.i;
	
	
	if ( t.DisableDynamicRes == false )
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 0 ) ; ++t.i;
	}
	else
	{
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disabledynamicres="+Str( 1 ) ; ++t.i;
	}

	
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowdistance=" + Str(g.globals.realshadowdistancehigh); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowresolution="+Str(g.globals.realshadowresolution) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascadecount="+Str(g.globals.realshadowcascadecount) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "flashlightshadows=" + Str(g.globals.flashlightshadows); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "speedshadows=" + Str(g.globals.speedshadows); ++t.i;
	
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade0="+Str(g.globals.realshadowcascade[0]) ; ++t.i;
	if (g.globals.realshadowsize[0] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize0=" + Str(g.globals.realshadowsize[0]); ++t.i;	
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade1="+Str(g.globals.realshadowcascade[1]) ; ++t.i;
	if (g.globals.realshadowsize[1] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize1=" + Str(g.globals.realshadowsize[1]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade2="+Str(g.globals.realshadowcascade[2]) ; ++t.i;
	if (g.globals.realshadowsize[2] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize2=" + Str(g.globals.realshadowsize[2]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade3="+Str(g.globals.realshadowcascade[3]) ; ++t.i;
	if (g.globals.realshadowsize[3] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize3=" + Str(g.globals.realshadowsize[3]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade4="+Str(g.globals.realshadowcascade[4]) ; ++t.i;
	if (g.globals.realshadowsize[4] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize4=" + Str(g.globals.realshadowsize[4]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade5="+Str(g.globals.realshadowcascade[5]) ; ++t.i;
	if (g.globals.realshadowsize[5] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize5=" + Str(g.globals.realshadowsize[5]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade6="+Str(g.globals.realshadowcascade[6]) ; ++t.i;
	if (g.globals.realshadowsize[6] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize6=" + Str(g.globals.realshadowsize[6]); ++t.i;
	}
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowcascade7="+Str(g.globals.realshadowcascade[7]) ; ++t.i;
	if (g.globals.realshadowsize[7] > 0) {
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "realshadowsize7=" + Str(g.globals.realshadowsize[7]); ++t.i;
	}

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "enableplrspeedmods=" + Str(g.globals.enableplrspeedmods); ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "disableweaponjams=" + Str(g.globals.disableweaponjams); ++t.i;


	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[GAMEMENUOPTIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowterrain="+g.graphicslowterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowentity="+g.graphicslowentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicslowgrass="+g.graphicslowgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumterrain="+g.graphicsmediumterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumentity="+g.graphicsmediumentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicsmediumgrass="+g.graphicsmediumgrass_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighterrain="+g.graphicshighterrain_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighentity="+g.graphicshighentity_s; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "graphicshighgrass="+g.graphicshighgrass_s; ++t.i;

	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CUSTOMIZATIONS]" ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "switchtoalt="+Str(g.ggunaltswapkey1) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "melee key="+Str(g.ggunmeleekey) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "zoomholdbreath="+Str(g.gzoomholdbreath) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyUP="+Str(t.listkey[1]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyDOWN="+Str(t.listkey[2]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyLEFT="+Str(t.listkey[3]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRIGHT="+Str(t.listkey[4]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyJUMP="+Str(t.listkey[5]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyCROUCH="+Str(t.listkey[6]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyENTER="+Str(t.listkey[7]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRELOAD="+Str(t.listkey[8]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKLEFT="+Str(t.listkey[9]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyPEEKRIGHT="+Str(t.listkey[10]) ; ++t.i;
	t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "keyRUN="+Str(t.listkey[11]) ; ++t.i;

	// vr extras
	if ( g.vrqcontrolmode != 0 || g.gxbox != 0 )
	{
		// CONTROLLER
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
		t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[CONTROLLER]" ; ++t.i;
		if ( g.vrqcontrolmode != 0 )
		{
			if ( g.vrqorggcontrolmode == 2 )
			{
				// No controller by default in EDU mode
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=0"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
			else
			{
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox=1"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype=2"; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert=0" ; ++t.i;
				t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag=100" ; ++t.i;
			}
		}
		else
		{
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xbox="+Str(g.gxbox); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxcontrollertype="+Str(g.gxboxcontrollertype); ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxinvert="+Str(g.gxboxinvert) ; ++t.i;
			t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "xboxmag="+Str(g.gxboxmag) ; ++t.i;
		}
	}
	if ( g.vrqcontrolmode != 0 )
	{
		if ( g.vrqorggcontrolmode == 2 )
		{
			// NO VR IN EDU
		}
		else
		{
			// VR
			//t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "" ; ++t.i;
			//t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "[VR]" ; ++t.i;
			//t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmode=5"; ++t.i;
			//t.setuparr_s[t.i] = ""; t.setuparr_s[t.i] = t.setuparr_s[t.i] + "vrmodemag=100"; ++t.i;
		}
	}

	if (  FileExist(t.setupfile_s.Get()) == 1  )  DeleteAFile (  t.setupfile_s.Get() );
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	//  Also save out the localisation ptr file
	Dim (  t.setuparr_s,2  );
	t.setupfile_s=t.exepath_s+t.exename_s+"\\userdetails.ini";
	t.setuparr_s[0]="[LOCALIZATION]";
	t.setuparr_s[1]=cstr("language=")+g.language_s;
	SaveArray (  t.setupfile_s.Get(),t.setuparr_s );
	UnDim (  t.setuparr_s );

	//  Itinery of all files in standalone
	t.titineryfile_s=t.exepath_s+t.exename_s+"\\contents.txt";
	SaveArray (  t.titineryfile_s.Get(),t.filecollection_s );

	//  cleanup file array
	UnDim (  t.filecollection_s );
}

int mapfile_savestandalone_continue ( void )
{
	int iSuccess = 0;
	switch ( g_mapfile_iStage )
	{
		case 1 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 5.0f )
						g_mapfile_iStage = 20;
					break;

		case 20 :	mapfile_savestandalone_stage2a();
					g_mapfile_fProgress = 6.0f; 
					g_mapfile_iStage = 21;
					break;

		case 21 :	if ( mapfile_savestandalone_stage2b() == 0 )
					{
						g_mapfile_iStage = 22;
					}
					else
					{
						g_mapfile_iStage = 29;
					}
					break;

		case 22 :	if ( mapfile_savestandalone_stage2c() == 0 )
					{
						g_mapfile_fProgress += (80.0f/g_mapfile_fProgressSpan);
					}
					else
					{
						g_mapfile_iStage = 23; 
					}
					break;

		case 23 :	mapfile_savestandalone_stage2d();
					g_mapfile_iStage = 21; 
					break;

		case 29 :	mapfile_savestandalone_stage2e(); 
					g_mapfile_iStage = 30; 
					break;

		case 30 :	mapfile_savestandalone_stage3(); 
					g_mapfile_fProgress = 90.0f; 
					g_mapfile_iStage = 40; 
					break;

		case 40 :	mapfile_savestandalone_stage4(); 
					g_mapfile_fProgress = 95.0f; 
					g_mapfile_iStage = 50; 
					break;

		case 50 :	g_mapfile_fProgress+=0.1f; 
					if ( g_mapfile_fProgress >= 100.0f )
						g_mapfile_iStage = 60;
					break;

		case 60 :	g_mapfile_fProgress = 100.0f; 
					g_mapfile_iStage = 99; 
					iSuccess = 1;
					break;
	}
	return iSuccess;
}

float mapfile_savestandalone_getprogress ( void )
{
	return g_mapfile_fProgress;
}

void mapfile_savestandalone_finish ( void )
{
	// encrypt media
	t.dest_s=t.exepath_s+t.exename_s;
	if ( g.gexportassets == 0 ) 
	{
		if ( PathExist( cstr(t.dest_s + "\\Files").Get() ) ) 
		{
			//  NOTE; Need to exclude lightmaps from encryptor  set encrypt ignore list "lightmaps"
			EncryptAllFiles ( cstr(t.dest_s + "\\Files").Get() );
		}
	}

	//  if not tignorelevelbankfiles, copy unencrypted files
	if (  t.tignorelevelbankfiles == 0 ) 
	{
		//  now copy the files we do not want to encrypt
		g.filecollectionmax = 0;
		Dim (  t.filecollection_s,500  );

		//  lightmap DBOs
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		t.tfurthestobjnumber=g.lightmappedobjectoffset;
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffset+99999 ; t.tobj+= 100 )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  t.tfurthestobjnumber = t.tobj+100;
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj <= t.tfurthestobjnumber; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		for ( t.tobj = g.lightmappedobjectoffset; t.tobj<= g.lightmappedobjectoffsetfinish; t.tobj++ )
		{
			t.tname_s = ""; t.tname_s = t.tname_s + "levelbank\\testmap\\lightmaps\\"+"object"+Str(t.tobj)+".dbo";
			if (  FileExist(t.tname_s.Get()) == 1  )  addtocollection(t.tname_s.Get());
		}
		t.nmax=500;
		for ( t.n = 0 ; t.n<=  5000 ; t.n+= 100 )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  t.nmax = t.n+100;
		}
		for ( t.n = 0 ; t.n<=  t.nmax; t.n++ )
		{
			t.tfile_s=cstr("levelbank\\testmap\\lightmaps\\")+Str(t.n)+".dds";
			if (  FileExist(t.tfile_s.Get()) == 1  )  addtocollection(t.tfile_s.Get());
		}
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectlist.dat" ; addtocollection(t.tfile_s.Get());
		t.tfile_s="levelbank\\testmap\\lightmaps\\objectnummax.dat" ; addtocollection(t.tfile_s.Get());

		//  Copy the 'unencrypted files' collection to exe folder
		timestampactivity(0, cstr(cstr("filecollectionmax=")+Str(g.filecollectionmax)).Get() );
		SetDir ( cstr(t.exepath_s+t.exename_s+"\\Files\\levelbank\\testmap").Get() );
		if (  PathExist("lightmaps") == 0  )  MakeDirectory (  "lightmaps" );
		SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
		for ( t.fileindex = 1 ; t.fileindex<=  g.filecollectionmax; t.fileindex++ )
		{
			t.src_s=t.filecollection_s[t.fileindex];
			if (  FileExist(t.src_s.Get()) == 1 ) 
			{
				t.dest_s=t.exepath_s+t.exename_s+"\\Files\\"+t.src_s;
				if (  FileExist(t.dest_s.Get()) == 1  )  DeleteAFile (  t.dest_s.Get() );
				CopyAFile (  t.src_s.Get(),t.dest_s.Get() );
			}
		}
	}

	// restore directory, restore original level and close up
	mapfile_savestandalone_restoreandclose();
}

void mapfile_savestandalone_restoreandclose ( void )
{
	// Restore directory
	SetDir ( t.told_s.Get() );

	// restore original level FPM files
	timestampactivity(0, cstr(cstr("check '")+g.projectfilename_s+"' vs '"+t.tmasterlevelfile_s+"'").Get() );
	if ( g.projectfilename_s != t.tmasterlevelfile_s ) 
	{
		if ( Len(t.tmasterlevelfile_s.Get()) > 1 ) 
		{
			g.projectfilename_s=t.tmasterlevelfile_s;
			// need to load EVERYTHING back in
			gridedit_load_map ( );
		}
	}

	// finish popup system
	//popup_text_close();

	// no longer making standalone
	t.levelsforstandalone = 0;
}
#endif

void scanscriptfileandaddtocollection ( char* tfile_s )
{
	cstr tscriptname_s =  "";
	cstr tlinethis_s =  "";
	int lookforlen = 0;
	cstr lookfor_s =  "";
	int lookforlen2 = 0;
	cstr lookfor2_s = "";
	cstr lookfor3_s = "";
	cstr tline_s =  "";
	int l = 0;
	int c = 0;
	int tt = 0;
	std::vector <cstr> scriptpage_s; //Allow us to run recursively
	Dim (  scriptpage_s,10000  );
	if (  FileExist(tfile_s) == 1 ) 
	{
		LoadArray (  tfile_s,scriptpage_s );

		lookfor_s=Lower("Include(") ; lookforlen=Len(lookfor_s.Get());
		lookfor2_s = Lower("require \""); lookforlen2 = Len(lookfor2_s.Get());
		lookfor3_s = Lower("Include (");

		for ( l = 0 ; l < scriptpage_s.size() ; l++ )
		{
			tline_s=Lower(scriptpage_s[l].Get());

			for (c = 0; c <= Len(tline_s.Get()) - lookforlen2 - 1; c++)
			{
				tlinethis_s = Right(tline_s.Get(), Len(tline_s.Get()) - c);

				// ignore commented out lines
				if (cstr(Left(tlinethis_s.Get(), 2)) == "--") break;

				if (cstr(Left(tlinethis_s.Get(), lookforlen2)) == lookfor2_s.Get() || cstr(Left(tlinethis_s.Get(), lookforlen2)) == lookfor3_s.Get())
				{
					//  found script has included ANOTHER script
					// skip spaces and quotes 
					int i = lookforlen2 + 1;

					while (i < Len(tlinethis_s.Get()) &&
						(cstr(Mid(tlinethis_s.Get(), i)) == " " ||
							cstr(Mid(tlinethis_s.Get(), i)) == "\""))
					{
						i++;
					};

					// if couldn't find the script name skip this line
					if (i == Len(tlinethis_s.Get())) break;

					tscriptname_s = Right(tline_s.Get(), Len(tline_s.Get()) - c - i + 1);

					for (int il = Len(tscriptname_s.Get()); il > 0; il--) {
						if (cstr(Mid(tscriptname_s.Get(), il)) == "\"") {
							tscriptname_s = Left(tscriptname_s.Get(), il-1);
							break;
						}
					}

					std::string script_name = tscriptname_s.Get();
					replaceAll(script_name, "\\\\", "\\");
					replaceAll(script_name, "scriptbank\\", "");
					tscriptname_s = script_name.c_str();

					if( !pestrcasestr(tscriptname_s.Get(),".lua"))
						tscriptname_s += ".lua";

					for (tt = Len(tscriptname_s.Get()); tt >= 4; tt += -1)
					{
						if (cstr(Mid(tscriptname_s.Get(), tt - 0)) == "a" && cstr(Mid(tscriptname_s.Get(), tt - 1)) == "u" && cstr(Mid(tscriptname_s.Get(), tt - 2)) == "l" && cstr(Mid(tscriptname_s.Get(), tt - 3)) == ".")
						{
							break;
						}
					}
					tscriptname_s = Left(tscriptname_s.Get(), tt);

					if (addtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get()) == true) {
						//Newly added , also scan this entry.
						scanscriptfileandaddtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get());
					}
				}
			}

			for ( c = 0 ; c<=  Len(tline_s.Get())-lookforlen-1; c++ )
			{
				tlinethis_s=Right(tline_s.Get(),Len(tline_s.Get())-c);

				// ignore commented out lines
				if ( cstr( Left( tlinethis_s.Get(), 2 )) == "--" ) break;

				if (  cstr( Left( tlinethis_s.Get(), lookforlen )) == lookfor_s.Get() )
				{
					//  found script has included ANOTHER script
					// skip spaces and quotes 
					int i = lookforlen + 1;

					while ( i < Len( tlinethis_s.Get() ) &&
						   ( cstr( Mid( tlinethis_s.Get(), i )) == " " ||
						     cstr( Mid( tlinethis_s.Get(), i )) == "\"" ) ) 
					{
						i++;
					};
			
					// if couldn't find the script name skip this line
					if (i == Len(tlinethis_s.Get())) break;

					tscriptname_s=Right(tline_s.Get(),Len(tline_s.Get())-c-i+1);
					for (tt = Len(tscriptname_s.Get()); tt >= 4; tt += -1)
					{
						if (cstr(Mid(tscriptname_s.Get(), tt - 0)) == "a" && cstr(Mid(tscriptname_s.Get(), tt - 1)) == "u" && cstr(Mid(tscriptname_s.Get(), tt - 2)) == "l" && cstr(Mid(tscriptname_s.Get(), tt - 3)) == ".")
						{
							break;
						}
					}
					tscriptname_s = Left(tscriptname_s.Get(), tt);

					if (addtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get()) == true) {
						//Newly added , also scan this entry.
						scanscriptfileandaddtocollection(cstr(cstr("scriptbank\\") + tscriptname_s).Get());
					}
				}
			}
		}
	}
	UnDim (  scriptpage_s );
}

bool addtocollection ( char* file_s )
{
	int tarrsize = 0;
	int tfound = 0;
	int f = 0;
	file_s=Lower(file_s);
	//  Ensure this entry is not already present
	tfound=0;
	for ( f = 1 ; f<=  g.filecollectionmax; f++ )
	{
		if (  t.filecollection_s[f] == cstr(file_s)  )  tfound = 1;
	}
	if (  tfound == 0 ) 
	{
		//  Expand file collection array if nearly full
		++g.filecollectionmax;
		tarrsize=ArrayCount(t.filecollection_s);
		if (  g.filecollectionmax>tarrsize-10 ) 
		{
			Dim (  t.filecollection_s,tarrsize+50  );
		}
		t.filecollection_s[g.filecollectionmax]=file_s;
		return true;
	}
	return false;
}

void removefromcollection ( char* file_s )
{
	int tfound = 0;
	file_s=Lower(file_s);
	for ( int f = 1 ; f <= g.filecollectionmax; f++ )
		if ( t.filecollection_s[f] == cstr(file_s)  )  
			tfound = f;
	if ( tfound > 0 ) 
	{
		// remove from consideration
		t.filecollection_s[tfound] = "";
	}
}

void removeanymatchingfromcollection ( char* folderorfile_s )
{
	int tfound = 0;
	folderorfile_s = Lower(folderorfile_s);
	for ( int f = 1; f <= g.filecollectionmax; f++ )
	{
		if ( strnicmp ( t.filecollection_s[f].Get(), folderorfile_s, strlen(folderorfile_s) ) == NULL )
		{
			// remove from consideration
			t.filecollection_s[f] = "";
		}
	}
}

void addfoldertocollection ( char* path_s )
{
	cstr tfile_s =  "";
	cstr told_s =  "";
	cstr usePath = path_s;
	int c = 0;
	told_s = GetDir();
#ifdef WICKEDENGINE
	//PE: In wicked also check if folder is inside docwrite.
	if (!PathExist(usePath.Get()))
	{
		extern char szWriteDir[MAX_PATH];
		cstr testPath = cstr(szWriteDir) + "Files\\" + usePath;
		if (PathExist(testPath.Get()))
		{
			usePath = testPath;
		}
	}
#endif
	if ( PathExist (usePath.Get()) )
	{
		SetDir (usePath.Get());
		ChecklistForFiles (  );
		for ( c = 1 ; c<=  ChecklistQuantity(); c++ )
		{
			if (  ChecklistValueA(c) == 0 ) 
			{
				tfile_s=ChecklistString(c);
				if (  tfile_s != "." && tfile_s != ".." ) 
				{
					//PE: Still adding using the relative path path_s
					addtocollection( cstr(cstr(path_s)+"\\"+tfile_s).Get() );
				}
			}
		}
		SetDir (  told_s.Get() );
	}
	else
	{
		timestampactivity(0, cstr(cstr("Tried adding path that does not exist: ")+path_s).Get() );
	}
}

void addallinfoldertocollection ( cstr subThisFolder_s, cstr subFolder_s )
{
	// into folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( subThisFolder_s.Get() );

	// first scan and record all files and folders - store folders locally
	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	cstr* pFolders = new cstr[iFoldersCount+1];
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
				cstr relativeFilePath_s = subFolder_s + "\\" + pFileFolderName;
				addtocollection ( relativeFilePath_s.Get() );
			}
		}
	}

	// now use local folder list and investigate each one
	for ( int f = 1; f <= iFoldersCount; f++ )
	{
		cstr pFolderName = pFolders[f];
		if ( pFolderName.Len() > 0 )
		{
			cstr relativeFolderPath_s = pFolderName;
			if ( subFolder_s.Len() > 0 ) relativeFolderPath_s = subFolder_s + "\\" + pFolderName;
			addallinfoldertocollection ( pFolderName, relativeFolderPath_s );
		}
	}

	// back out of folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( ".." );

	// finally free resources
	delete[] pFolders;
}

void createallfoldersincollection ( void )
{
	LPSTR pOldDir = GetDir();
	t.strwork = ""; t.strwork = t.strwork + "Create full path structure ("+Str(t.filesmax)+") for standalone executable";
	timestampactivity(0, t.strwork.Get() );
	t.filesmax = g.filecollectionmax;
	for ( t.fileindex = 1 ; t.fileindex <= t.filesmax; t.fileindex++ )
	{
		t.olddir_s=GetDir();
		t.src_s=t.filecollection_s[t.fileindex];
		t.srcstring_s = t.src_s;
		while (Len(t.srcstring_s.Get()) > 0)
		{
			for (t.c = 1; t.c <= Len(t.srcstring_s.Get()); t.c++)
			{
				if (cstr(Mid(t.srcstring_s.Get(), t.c)) == "\\" || cstr(Mid(t.srcstring_s.Get(), t.c)) == "/")
				{
					t.chunk_s = Left(t.srcstring_s.Get(), t.c - 1);
					if (Len(t.chunk_s.Get()) > 0)
					{
						if (PathExist(t.chunk_s.Get()) == 0)  MakeDirectory(t.chunk_s.Get());
						if (PathExist(t.chunk_s.Get()) == 0)
						{
							timestampactivity(0, cstr(cstr("Path:") + t.src_s).Get());
							timestampactivity(0, cstr(cstr("Unable to create folder:'") + t.chunk_s + "' [error code " + Mid(t.srcstring_s.Get(), t.c) + ":" + Str(t.c) + ":" + Str(Len(t.srcstring_s.Get())) + "]").Get());
						}
						if (PathExist(t.chunk_s.Get()) == 1)
						{
							// sometimes an absolute path can be inserted into path sequence (i.e lee\fred\d;\blob\doug)
							SetDir(t.chunk_s.Get());
						}
					}
					t.srcstring_s = Right(t.srcstring_s.Get(), Len(t.srcstring_s.Get()) - t.c);
					t.c = 1; // start from beginning as string has been cropped
					break;
				}
			}
			if (t.c > Len(t.srcstring_s.Get())) break;
		}

		SetDir ( t.olddir_s.Get() );
	}
	SetDir ( pOldDir );
}

void findalltexturesinmodelfile ( char* inputfile_s, char* folder_s, char* texpath_s )
{
	cstr returntexfile_s =  "";
	int tfoundpiccy = 0;
	cstr texfile_s =  "";
	int filesize = 0;
	int mbi = 0;
	int a = 0;
	int b = 0;
	int c = 0;
	int d = 0;

	// do two passes, second one if there is an accompanying DBO (which has unscrambled data to find texture names)
	for ( int iPass = 0; iPass < 2; iPass++ )
	{
		// filename to attempt a scan of
		cstr filestr = inputfile_s;
		if (iPass == 1)
		{
			LPSTR pFilename = filestr.Get();
			if (stricmp (pFilename + strlen(pFilename) - 2, ".x") == NULL)
			{
				filestr = cstr(Left(pFilename, strlen(pFilename) - 2)) + ".dbo";
			}
		}
		char* file_s = filestr.Get();

		// To determine if a model file requires texture files, we scan the file for a
		// match to the Text ( .TGA or .JPG (and use texfile$) )
		returntexfile_s="";
		if (FileExist(file_s) == 1)
		{
			filesize = FileSize(file_s);
			mbi = 255;
			OpenToRead (11, file_s);
			if (FileOpen(11) == 1)
			{
				MakeMemblockFromFile(mbi, 11);
				CloseFile(11);
				for (b = 0; b <= filesize - 4; b++)
				{
					if (ReadMemblockByte(mbi, b + 0) == Asc("."))
					{
						tfoundpiccy = 0;
						if (ReadMemblockByte(mbi, b + 1) == Asc("T") || ReadMemblockByte(mbi, b + 1) == Asc("t"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("G") || ReadMemblockByte(mbi, b + 2) == Asc("g"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("A") || ReadMemblockByte(mbi, b + 3) == Asc("a"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("J") || ReadMemblockByte(mbi, b + 1) == Asc("j"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("P") || ReadMemblockByte(mbi, b + 2) == Asc("p"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("G") || ReadMemblockByte(mbi, b + 3) == Asc("g"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("D") || ReadMemblockByte(mbi, b + 1) == Asc("d"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("D") || ReadMemblockByte(mbi, b + 2) == Asc("d"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("S") || ReadMemblockByte(mbi, b + 3) == Asc("s"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("B") || ReadMemblockByte(mbi, b + 1) == Asc("b"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("M") || ReadMemblockByte(mbi, b + 2) == Asc("m"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("P") || ReadMemblockByte(mbi, b + 3) == Asc("p"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (ReadMemblockByte(mbi, b + 1) == Asc("P") || ReadMemblockByte(mbi, b + 1) == Asc("p"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("N") || ReadMemblockByte(mbi, b + 2) == Asc("n"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("G") || ReadMemblockByte(mbi, b + 3) == Asc("g"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						//PE: mainly from the building pack they are recorded as psd.
						if (ReadMemblockByte(mbi, b + 1) == Asc("P") || ReadMemblockByte(mbi, b + 1) == Asc("p"))
						{
							if (ReadMemblockByte(mbi, b + 2) == Asc("S") || ReadMemblockByte(mbi, b + 2) == Asc("s"))
							{
								if (ReadMemblockByte(mbi, b + 3) == Asc("D") || ReadMemblockByte(mbi, b + 3) == Asc("d"))
								{
									tfoundpiccy = 1;
								}
							}
						}
						if (tfoundpiccy == 1)
						{
							//  track back
							for (c = b; c >= b - 255; c += -1)
							{
								if (ReadMemblockByte(mbi, c) >= Asc(" ") && ReadMemblockByte(mbi, c) <= Asc("z") && ReadMemblockByte(mbi, c) != 34)
								{
									//  part of filename
								}
								else
								{
									//  no more filename
									break;
								}
							}
							texfile_s = "";
							for (d = c + 1; d <= b + 3; d++)
							{
								texfile_s = texfile_s + Chr(ReadMemblockByte(mbi, d));
							}
							texfile_s = Lower(texfile_s.Get());

							//LB: Some DBO texture filenames can be very corrupted, such as below. Scan for and correct!
							//    C:\Users\leeba\Documents\GameGuruApps\GameGuruMAX\Files\entitybank\Urban Assets\entitybank\C:/Program Files (x86)/The Game Creators/FPS Creator Reloaded/Files/entitybank/Valuable Assets/Scenery/Buildings/Derelict/building10_D_surface.dds
							//
							LPSTR pTextFilePtr = texfile_s.Get();
							if (strlen(pTextFilePtr) > 4)
							{
								// scan for telltail abs path
								bool bUsesAbsPath = false;
								for (int nn = 0; nn < strlen(pTextFilePtr); nn++)
								{
									if (pTextFilePtr[nn] == ':')
									{
										// this texture filename uses an absolutely path
										bUsesAbsPath = true;
										break;
									}
								}
								if (bUsesAbsPath == true )
								{
									// truncate to JUST the texture filename, remove all path (depend on folder_s)!
									for (int nnn = strlen(pTextFilePtr)-1; nnn > 0; nnn--)
									{
										if (pTextFilePtr[nnn] == '\\' || pTextFilePtr[nnn] == '/')
										{
											texfile_s = pTextFilePtr + nnn + 1;
											break;
										}
									}
								}
							}

							if (strnicmp(texfile_s.Get(), "effectbank\\", 11) == NULL)
							{
								addtocollection(texfile_s.Get());
							}
							else
							{
								// detect PBR texture set
								bool bDetectedPBRTextureSetName = false;
								cstr texfilenoext_s = cstr(Left(texfile_s.Get(), Len(texfile_s.Get()) - 4));
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 6, "_color", 6) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 6); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 7, "_normal", 7) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 7); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 10, "_metalness", 10) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 10); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 10, "_roughness", 10) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 10); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 6, "_gloss", 6) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 6); bDetectedPBRTextureSetName = true; }
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 3, "_ao", 3) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 3); bDetectedPBRTextureSetName = true; }
								#ifdef WICKEDENGINE
								if (strnicmp(texfilenoext_s.Get() + strlen(texfilenoext_s.Get()) - 3, "_surface", 3) == NULL) { texfilenoext_s = Left(texfilenoext_s.Get(), strlen(texfilenoext_s.Get()) - 3); bDetectedPBRTextureSetName = true; }
								#endif
								if (bDetectedPBRTextureSetName == true)
								{
									//PE: Need to check filename only and current object folder.
									bool tex_found = false;
									int pos = 0;
									for (pos = texfilenoext_s.Len(); pos > 0; pos--) 
									{
										if (cstr(Mid(texfilenoext_s.Get(), pos)) == "\\" || cstr(Mid(texfilenoext_s.Get(), pos)) == "/")
											break;
									}
									if (pos > 0) 
									{
										cstr directfile = Right(texfilenoext_s.Get(), texfilenoext_s.Len() - pos);
										cstr tmp = cstr(cstr(folder_s) + directfile + "_color.dds").Get();
										if (FileExist(tmp.Get())) 
										{
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_normal.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_metalness.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_gloss.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_ao.dds").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_illumination.dds").Get();
											addtocollection(tmp.Get());
											#ifdef WICKEDENGINE
											tmp = cstr(cstr(folder_s) + directfile + "_surface.dds").Get();
											addtocollection(tmp.Get());
											#endif
											tex_found = true;
										}
										tmp = cstr(cstr(folder_s) + directfile + "_color.png").Get();
										if (FileExist(tmp.Get())) 
										{
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_normal.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_metalness.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_gloss.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_ao.png").Get();
											addtocollection(tmp.Get());
											tmp = cstr(cstr(folder_s) + directfile + "_illumination.png").Get();
											addtocollection(tmp.Get());
											#ifdef WICKEDENGINE
											//PE: surface still .dds
											tmp = cstr(cstr(folder_s) + directfile + "_surface.dds").Get();
											addtocollection(tmp.Get());
											#endif
											tex_found = true;
										}
									}

									//PE: We get some strange folder created in the standalone from here.
									// add other PBR textures just in case not detected in model data
									if (!tex_found)
									{
										cstr texfileColor_s = texfilenoext_s + "_color.dds";
										//Only if the src is exists.
										if (FileExist(cstr(cstr(folder_s) + texpath_s + texfileColor_s).Get()) || FileExist(cstr(cstr(folder_s) + texfileColor_s).Get())) 
										{
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileColor_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileColor_s).Get());
											cstr texfileNormal_s = texfilenoext_s + "_normal.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileNormal_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileNormal_s).Get());
											cstr texfileMetalness_s = texfilenoext_s + "_metalness.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileMetalness_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileMetalness_s).Get());
											cstr texfileGloss_s = texfilenoext_s + "_gloss.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileGloss_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileGloss_s).Get());
											cstr texfileAO_s = texfilenoext_s + "_ao.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileAO_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileAO_s).Get());
											cstr texfileIllumination_s = texfilenoext_s + "_illumination.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfileIllumination_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfileIllumination_s).Get());
											#ifdef WICKEDENGINE
											cstr texfilesurface_s = texfilenoext_s + "_surface.dds";
											addtocollection(cstr(cstr(folder_s) + texpath_s + texfilesurface_s).Get());
											addtocollection(cstr(cstr(folder_s) + texfilesurface_s).Get());
											#endif
										}
									}
								}

								if (FileExist(cstr(cstr(folder_s) + texpath_s + texfile_s).Get()))
									addtocollection(cstr(cstr(folder_s) + texpath_s + texfile_s).Get());
								if (FileExist(cstr(cstr(folder_s) + texfile_s).Get()))
									addtocollection(cstr(cstr(folder_s) + texfile_s).Get());

								if (cstr(Right(texfile_s.Get(), 4)) != ".dds")
								{
									//  also convert to DDS and add those too
									if (FileExist(cstr(cstr(folder_s) + texfile_s + ".png").Get()))
										addtocollection(cstr(cstr(folder_s) + texfile_s + ".png").Get());
									texfile_s = cstr(Left(texfile_s.Get(), Len(texfile_s.Get()) - 4)) + ".dds";
									if (FileExist(cstr(cstr(folder_s) + texpath_s + texfile_s).Get()))
										addtocollection(cstr(cstr(folder_s) + texpath_s + texfile_s).Get());
									if (FileExist(cstr(cstr(folder_s) + texfile_s).Get()))
										addtocollection(cstr(cstr(folder_s) + texfile_s).Get());
								}
							}
							b += 4;
						}
					}
				}
				DeleteMemblock(mbi);
			}
		}
	}
}

#ifdef VRTECH
//
// Scan default installation, keep core copy of default files for reference (so know custom content when we see it)
//

void CreateItineraryFile ( void )
{
	g_sDefaultAssetFiles.clear();
	#ifdef WICKEDENGINE
	// no itintery file for now
	#else
	// check if we have an itinerary file
	LPSTR pOldDir = GetDir();
	SetDir ( g.fpscrootdir_s.Get() );
	cstr pItineraryFile = "assetitinerary.dat";
	if (g.globals.generateassetitinerary > 0 && FileExist ( pItineraryFile.Get() ) == 1) DeleteFileA(pItineraryFile.Get());
	if ( FileExist ( pItineraryFile.Get() ) == 0 )
	{
		// Create itinerary file
		scanallfolder ( "Files", "" );
		SetDir ( pOldDir );

		// Create binay block and dump string data to that (faster)
		DWORD dwStringBlockSize = 0;
		dwStringBlockSize += 4;
		for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
		{
			DWORD dwStringSize = strlen(g_sDefaultAssetFiles[n].Get());
			dwStringBlockSize += dwStringSize;
			dwStringBlockSize++;
		}
		MakeMemblock ( 1, dwStringBlockSize );
		int pos = 0;
		WriteMemblockDWord ( 1, pos, dwStringBlockSize ); pos += 4;
		for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
		{
			LPSTR pString = g_sDefaultAssetFiles[n].Get();
			for ( int c = 0; c < strlen(pString); c++ )
				WriteMemblockByte ( 1, pos++, (unsigned char) pString[c] ); //PE: chars like '�' will set -10 and generate errors.
			WriteMemblockByte ( 1, pos++, 0 );
		}
		SetDir ( g.fpscrootdir_s.Get() );
		OpenToWrite ( 1, pItineraryFile.Get() );
		MakeFileFromMemblock ( 1, 1 );
		CloseFile ( 1 );
		SetDir ( pOldDir );
		DeleteMemblock ( 1 );
	}
	else
	{
		// Load itinerary file (fast)
		OpenToRead ( 1, pItineraryFile.Get() );
		MakeMemblockFromFile ( 1, 1 );
		int pos = 0;
		char pString[2050];
		LPSTR pStringPtr = pString;
		DWORD dwMemblockSize = ReadMemblockDWord ( 1, pos ); pos += 4;
		while ( pos < dwMemblockSize )
		{
			*pStringPtr = ReadMemblockByte ( 1, pos ); pos++;
			if ( *pStringPtr == 0 )
			{
				g_sDefaultAssetFiles.push_back ( pString );
				memset ( pString, 0, sizeof(pString) );
				pStringPtr = pString;
			}
			else
			{
				pStringPtr++;
			}
		}
		CloseFile ( 1 );
		DeleteMemblock ( 1 );	
	}
	SetDir ( pOldDir );
	#endif
}

void scanallfolder ( cstr subThisFolder_s, cstr subFolder_s )
{
	// into folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( subThisFolder_s.Get() );

	// first scan all files and folders - store folders locally
	ChecklistForFiles();
	int iFoldersCount = ChecklistQuantity();
	cstr* pFolders = new cstr[iFoldersCount+1];
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
				// found file reference
				cstr relativeFilePath_s = subFolder_s + "\\" + pFileFolderName;

				// clean up string
				LPSTR pOldStr = relativeFilePath_s.Get();
				LPSTR pCleanStr = new char[strlen(pOldStr)+1];
				int nn = 0;
				for ( int n = 0; n < strlen(pOldStr); n++ )
				{
					if ( pOldStr[n] == '\\' && pOldStr[n+1] == '\\' ) n++; // skip duplicate backslashes
					pCleanStr[nn++] = pOldStr[n];
				}
				pCleanStr[nn] = 0; 

				// add to master asset list of known stock assets
				g_sDefaultAssetFiles.push_back ( pCleanStr );
			}
		}
	}

	// now use local folder list and investigate each one
	for ( int f = 1; f <= iFoldersCount; f++ )
	{
		cstr pFolderName = pFolders[f];
		if ( pFolderName.Len() > 0 )
		{
			cstr relativeFolderPath_s = pFolderName;
			if ( subFolder_s.Len() > 0 ) relativeFolderPath_s = subFolder_s + "\\" + pFolderName;
			scanallfolder ( pFolderName, relativeFolderPath_s );
		}
	}

	// back out of folder
	if ( subThisFolder_s.Len() > 0 ) SetDir ( ".." );

	// finally free resources
	delete[] pFolders;
}

bool IsFileAStockAsset ( LPSTR pCheckThisFile )  
{
	// check if this file exists in stock assets
	char pFileToCheck[2048];
	strcpy ( pFileToCheck, pCheckThisFile );
	for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
	{
		LPSTR pCompare = g_sDefaultAssetFiles[n].Get();
		if ( stricmp ( pFileToCheck, pCompare ) == NULL )
			return true;
	}
	// before we return false, check special case for DBO files that have X files
	bool bSecondaryCheck = false;
	if ( strnicmp ( pCheckThisFile + strlen(pCheckThisFile) - 4, ".dbo", 4 ) == NULL )
	{
		pFileToCheck[strlen(pFileToCheck)-4] = 0;
		strcat ( pFileToCheck, ".x" );
		bSecondaryCheck = true;
	}
	if ( bSecondaryCheck == true )
	{
		for ( int n = 0; n < g_sDefaultAssetFiles.size(); n++ )
		{
			LPSTR pCompare = g_sDefaultAssetFiles[n].Get();
			if ( stricmp ( pFileToCheck, pCompare ) == NULL )
				return true;
		}
	}
	return false;
}

void ScanLevelForCustomContent ( LPSTR pFPMBeingSaved )
{
	// just before save an FPM, if any custom content used, add it to the FPM 
	// in a special Files folder to be referenced by forced redirection 
	// so it prefers FPM included files over stock assets on the local machine 
	// (which may have been changed, unknown to the user sharing their FPM)
	t.tmasterlevelfile_s=cstr("mapbank\\")+pFPMBeingSaved;
	timestampactivity(0,cstr(cstr("Scanning level for custom content:")+t.tmasterlevelfile_s).Get() );
	mapfile_collectfoldersandfiles ( "" );

	// Go through all files required by level, and create custom files for those that are not in default asset itinerary
	for ( t.fileindex = 1; t.fileindex <= g.filecollectionmax; t.fileindex++ )
	{
		SetDir ( cstr(g.fpscrootdir_s + "\\Files\\").Get() );
		t.src_s = t.filecollection_s[t.fileindex];
		// From now on, DBOs on their own are not allowed, only transport X files
		//if ( FileExist(t.src_s.Get()) == 0 )
		//{
			// special case of X files not existing, but DBOs do (if saving a previously loaded custom content FPM)
			//if ( strlen(t.src_s.Get()) > 2 )
			//{
			//	char pReplaceXWithDBO[2048];
			//	strcpy ( pReplaceXWithDBO, t.src_s.Get() );
			//	pReplaceXWithDBO[strlen(pReplaceXWithDBO)-2] = 0;
			//	strcat ( pReplaceXWithDBO, ".dbo" );
			//	t.src_s = pReplaceXWithDBO;
			//}
		//}
		if ( FileExist(t.src_s.Get()) == 1 ) 
		{
			// clean up string
			LPSTR pOldStr = t.src_s.Get();
			LPSTR pCleanStr = new char[strlen(t.src_s.Get())+1];
			int nn = 0;
			for ( int n = 0; n < strlen(pOldStr); n++ )
			{
				if ( pOldStr[n] == '\\' && pOldStr[n+1] == '\\' ) n++; // skip duplicate backslashes
				pCleanStr[nn++] = pOldStr[n];
			}
			pCleanStr[nn] = 0; 

			// is this a default file
			if ( IsFileAStockAsset ( pCleanStr ) == false )
			{
				// if file inside the ttsfiles folder (which is accounted for separately)?
				int iFindLastFolder = 0;
				bool bWeAreInTTSFilesFolder = false;
				for ( int n = strlen(pCleanStr); n > 0; n-- )
				{
					if (pCleanStr[n] == '\\' || pOldStr[n + 1] == '/')
					{
						if (iFindLastFolder == 1)
						{
							LPSTR pFindTTSFilesFolder = strstr(pCleanStr + n, "ttsfiles");
							if ( pFindTTSFilesFolder != 0 )
								bWeAreInTTSFilesFolder = true;
							break;
						}
						if (iFindLastFolder == 0) iFindLastFolder = 1;
					}
				}
				if ( bWeAreInTTSFilesFolder == false )
				{
					// No - copy file and add to file block of FPM being saved (current calling function)
					LPSTR pOneFiledStr = new char[10 + strlen(t.src_s.Get()) + 1];
					strcpy(pOneFiledStr, "CUSTOM_");
					int nnn = 7;
					for (int n = 0; n < strlen(pOldStr); n++)
					{
						if (pOldStr[n] == '\\' && pOldStr[n + 1] == '\\') n++; // skip duplicate backslashes
						if (pOldStr[n] == '\\')
							pOneFiledStr[nnn++] = '_';
						else
							pOneFiledStr[nnn++] = pOldStr[n];
					}
					pOneFiledStr[nnn] = 0;

					// copy file over and add to file block
					cstr pCustomRefFileSource = g.fpscrootdir_s + "\\Files\\" + pCleanStr;//"\\files\\audiobank\\misc\\item.wav";
					cstr sFileRefOneFileDest = pOneFiledStr;//"CUSTOM_Files_audiobank_misc_item.wav";

					// check if the DBO is not necessary (i.e. a character creator part)
					bool bAllowCustomFileToBeAdded = true;
					if (g.globals.generateassetitinerary == 2) bAllowCustomFileToBeAdded = false;
					/* though we do need DBO when saving an FPM and sharing it between VRQ users
					if ( strnicmp ( pCustomRefFileSource.Get()+strlen(pCustomRefFileSource.Get())-4,".dbo",4) == NULL )
					{
						// for DBO files, we either delete them (making the compat. with Player and keeping file sizes down)
						char pFPEAlongside[MAX_PATH];
						strcpy(pFPEAlongside, pCustomRefFileSource.Get());
						pFPEAlongside[strlen(pFPEAlongside) - 4] = 0;
						strcat(pFPEAlongside, ".fpe");
						if (FileExist(pFPEAlongside) == 1)
						{
							// inspect FPE, if it contains 'ccpassembly', then its a character creator DBO 
							// and we can skip including this file into the final FPM
							FILE* tFPEFile = GG_fopen ( pFPEAlongside, "r" );
							if (tFPEFile)
							{
								char tTempLine[2048];
								while (!feof(tFPEFile))
								{
									fgets(tTempLine, 2047, tFPEFile);
									if (strstr(tTempLine, "ccpassembly"))
									{
										bAllowCustomFileToBeAdded = false;
										break;
									}
								}
								fclose(tFPEFile);
							}
						}
					}
					*/
					if ( bAllowCustomFileToBeAdded == true )
					{
						if (FileExist(sFileRefOneFileDest.Get())) DeleteFileA(sFileRefOneFileDest.Get());
						SetDir(g.mysystem.levelBankTestMap_s.Get());
						CopyAFile(pCustomRefFileSource.Get(), sFileRefOneFileDest.Get());
						AddFileToBlock(1, sFileRefOneFileDest.Get());
					}
				}
			}
		}
	}

	// when done, need to be in levelbank folder for rest of FPM saving
	SetDir ( cstr(g.fpscrootdir_s + "\\Files\\").Get() );
	SetDir ( g.mysystem.levelBankTestMap_s.Get() );
}
#endif

#ifdef WICKEDENGINE
void mapfile_convertCLASSICtoMAX(LPSTR pFPMLoaded)
{
	//  Store and switch folders
	LPSTR pOldDir = GetDir();

	// check if in mapbank of GAMEGURU CLASSIC
	bool bAllowOneWayConversion = false;
	char pReconstructGameGuruRootFiles[MAX_PATH];
	strcpy(pReconstructGameGuruRootFiles, "");
	char pReconstructGameGuruFolder[MAX_PATH];
	strcpy(pReconstructGameGuruFolder, "");
	char pReconstructGameGuruEXE[MAX_PATH];
	strcpy(pReconstructGameGuruEXE, pFPMLoaded);
	LPSTR pFindClassicFolder = strstr(pReconstructGameGuruEXE, "Game Guru\\Files\\mapbank\\");
	if (pFindClassicFolder != NULL)
	{
		pFindClassicFolder[0] = 0;
		strcpy(pReconstructGameGuruRootFiles, pReconstructGameGuruEXE);
		strcat(pReconstructGameGuruRootFiles, "Game Guru\\Files\\");
		strcpy(pReconstructGameGuruFolder, pReconstructGameGuruEXE);
		strcat(pReconstructGameGuruFolder, "Game Guru\\Files\\entitybank\\");
		strcat(pReconstructGameGuruEXE, "Game Guru\\GameGuru.exe");
		if ( FileExist(pReconstructGameGuruEXE)==1 )
		{
			bAllowOneWayConversion = true;
		}
	}
	if (bAllowOneWayConversion == false)
		return;

	// go into levelbank\testmap
	SetDir ( g.mysystem.levelBankTestMap_s.Get() ); // "levelbank\\testmap\\"

	// debug log for conversion
	timestampactivity(0, "mapfile_convertCLASSICtoMAX:" );

	// original terrain heights are 1024x1024 (stretched over 51200x51200 units = spacing of 50.0f (vs new terrain spacing of 10.0f))
	int iSrcHeightWidth = 1024;
	int iSrcHeightHeight = 1024;

#ifdef FULLTERRAINEDITING
	// load in old terrain heights
	LPSTR pTerrainHeightFile = "m.dat";
	if (FileExist(pTerrainHeightFile) == 1)
	{
		//PE: WIP - Dont match 100% yet.
		//PE: Should be: height - 600 * 2.0 / 10000.0. ?
		//PE: Got it, looks like it fit now, need'ed ggterrain_global_params.height = 5000.0 :)
		if (1)
		{
			uint32_t size1 = 4096 * 4096 * sizeof(uint8_t);
			uint32_t size2 = 4096 * 4096 * sizeof(float);
			uint32_t terrain_sculpt_size = GGTerrain::GGTerrain_GetSculptDataSize();

			char *data = new char[terrain_sculpt_size];
			if (data)
			{
				GGTerrain::GGTerrain_GetSculptData((uint8_t*)data);
				float* pHeightMapEdit = (float*)(data + size1);
				uint8_t* pHeightMapEditType = (uint8_t*)data;

				if (FileOpen(1) == 1) CloseFile(1);
				OpenToRead(1, pTerrainHeightFile);
				if (MemblockExist(1) == 1) DeleteMemblock(1);
				if (FileOpen(1) == 1)
				{
					ReadMemblock(1, 1);
					int mi = 0;
					for (int z = 0; z <= 1023; z++)
					{
						for (int x = 0; x <= 1023; x++)
						{
							float scale = 2.0;
							int offsetz = 1024; //1536
							int offsetx = 1024; //1536
							float fHeight = (ReadMemblockFloat(1, mi) - 600.0) * 2.0; //Test 5000.0 look OK.
							float fHeight1, fHeight2, fHeight3;
							fHeight1 = fHeight2 = fHeight3 = fHeight;
							if (z < 1023 && x < 1023)
							{
								fHeight1 = (ReadMemblockFloat(1, mi + 4) - 600.0) * 2.0;
								fHeight2 = (ReadMemblockFloat(1, mi + 4 + (1024 * 4)) - 600.0) * 2.0;
								fHeight3 = (ReadMemblockFloat(1, mi + (1024 * 4)) - 600.0) * 2.0;
							}

							if (x > 0 && x < 1023 && z > 0 && z < 1023)
							{
								uint32_t newindex = (4096 - 1 - (offsetz + (z * scale + 0))) * 4096 + (offsetx + (x * scale + 0));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = (fHeight) / 10000.0;

								newindex = (4096 - 1 - (offsetz + (z * scale + 0))) * 4096 + (offsetx + (x * scale + 1));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight1) * 0.5) / 10000.0;

								newindex = (4096 - 1 - (offsetz + (z * scale + 1))) * 4096 + (offsetx + (x * scale + 1));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight2) * 0.5) / 10000.0;

								newindex = (4096 - 1 - (offsetz + (z * scale + 1))) * 4096 + (offsetx + (x * scale + 0));
								pHeightMapEditType[newindex] = 1; //Replace.
								pHeightMapEdit[newindex] = ((fHeight + fHeight3) * 0.5) / 10000.0;

							}
							else
							{
								for (int zz = 0; zz <= 1; zz++)
								{
									for (int xx = 0; xx <= 1; xx++)
									{
										uint32_t newindex = (4096 - 1 - (offsetz + (z * scale + zz))) * 4096 + (offsetx + (x * scale + xx));
										pHeightMapEditType[newindex] = 0;
									}
								}
							}
							mi += 4;
						}
					}
					DeleteMemblock(1);
					CloseFile(1);

					void procedural_set_heightmap_level(void);
					procedural_set_heightmap_level();
					GGTerrain::GGTerrain_SetSculptData(terrain_sculpt_size, (uint8_t*)data);
					void check_new_terrain_parameters(void);
					check_new_terrain_parameters();
				}
				delete(data);

			}
		}
	}

#else

	float* fSrcHeightData = new float[iSrcHeightWidth*iSrcHeightHeight];

	// load in old terrain heights
	LPSTR pTerrainHeightFile = "m.dat";
	if ( FileExist(pTerrainHeightFile) == 1 ) 
	{
		OpenToRead ( 1, pTerrainHeightFile );
		if (MemblockExist(1) == 1) DeleteMemblock(1);
		ReadMemblock ( 1, 1 );
		int mi = 0;
		for ( int z = 0; z <= 1023; z++ )
		{
			for ( int x = 0; x <= 1023; x++ )
			{
				float fHeight = ReadMemblockFloat ( 1, mi );
				if (x > 0 && x < 1023 && z > 0 && z < 1023)
					fSrcHeightData[x+(z*iSrcHeightWidth)] = fHeight;
				else
					fSrcHeightData[x+(z*iSrcHeightWidth)] = 0.0f;
				mi += 4;
			}
		}
		DeleteMemblock (  1 );
		CloseFile (  1 );
	}

	// old terrain and new terrain are at different scales, so sample from an offset and scale factor
	// that lines up center of old terrain with center of terrain node zero (TTR0XR0)
	float fActualCoverage = (1024.0f / 51200.0f) * 10000.0f;
	float fSampleStartCorner = 512.0f - (fActualCoverage/2.0f);
	int iNearestSrcXZ = (int)fSampleStartCorner;
	
	// leelee, hack!
	iNearestSrcXZ += 3;

	float fScaleToSrc = 10000.0f / 51200.0f;

	// save out new terrain node height files
	LPSTR pTerrainNodeFolder = "TTR0XR0";
	if (PathExist(pTerrainNodeFolder) == 0) MakeDirectory(pTerrainNodeFolder);
	SetDir(pTerrainNodeFolder);
	unsigned int iLOD = 0;
	for (unsigned int iIndexX = 0; iIndexX < 10; iIndexX++)
	{
		for (unsigned int iIndexZ = 0; iIndexZ < 10; iIndexZ++)
		{
			std::string sBaseFile = "terrain_data_" + std::to_string(iIndexX) + "_" + std::to_string(iIndexZ) + "_level_" + std::to_string(iLOD);
			std::string sHeightFile = sBaseFile + ".dat";
			FILE* fp = NULL;
			fopen_s(&fp, sHeightFile.c_str(), "wb");
			if (fp)
			{
				// dimenions of height field
				unsigned int iFileWidth = 101;
				unsigned int iFileHeight = 101;
				fwrite(&iFileWidth, sizeof(unsigned int), 1, fp);
				fwrite(&iFileHeight, sizeof(unsigned int), 1, fp);

				// write out as geometry
				float fTerrainNewGap = 10.0f;
				int iX = 0;
				int iZ = 0;
				float fX = 0.0f;
				float fZ = 0.0f;
				for (int i = 0; i < iFileWidth * iFileHeight; i++)
				{
					// get source coordinate to old terrain data using the scaled coordinate 
					float fWorldX = (iIndexX * 100 * fTerrainNewGap) + fX;
					float fWorldZ = (iIndexZ * 100 * fTerrainNewGap) + fZ;
					fWorldX *= fScaleToSrc;
					fWorldZ *= fScaleToSrc;
					float fScaledOffsetX = fWorldX / fTerrainNewGap;
					float fScaledOffsetZ = fWorldZ / fTerrainNewGap;
					float fSrcX = (float)iNearestSrcXZ + fScaledOffsetX;
					float fSrcZ = (float)iNearestSrcXZ + fScaledOffsetZ;

					// calculate surface height between four points
					// takes surface position height between four points
					float fHeight = 0.0f;
					int iSrcX = (int)(fSrcX);
					int iSrcZ = (int)(fSrcZ);
					int iSrcX2 = iSrcX+1;
					int iSrcZ2 = iSrcZ+1;
					float fMidX = (fSrcX - (float)iSrcX);
					float fMidZ = (fSrcZ - (float)iSrcZ);
					float fHeightX1 = fSrcHeightData[iSrcX + (iSrcZ*iSrcHeightWidth)];
					float fHeightX2 = fSrcHeightData[iSrcX2 + (iSrcZ*iSrcHeightWidth)];
					float fHeightA =  fHeightX1 + (fHeightX2-fHeightX1)*fMidX;
					fHeightX1 = fSrcHeightData[iSrcX + (iSrcZ2*iSrcHeightWidth)];
					fHeightX2 = fSrcHeightData[iSrcX2 + (iSrcZ2*iSrcHeightWidth)];
					float fHeightB =  fHeightX1 + (fHeightX2-fHeightX1)*fMidX;
					fHeight = fHeightA + (fHeightB - fHeightA)*fMidZ;

					// new terrain nodes raise objects by 600, so deduct this affect
					if ( fHeight > 0.0f ) fHeight -= 600.0f;

					// calculate vert
					XMFLOAT3 pos;
					pos.x = fX;
					pos.y = fHeight;
					pos.z = fZ;
					fX += fTerrainNewGap; iX++;
					if (iX == iFileWidth)
					{
						fX = 0.0f;
						iX = 0;
						fZ += fTerrainNewGap;
						iZ++;
					}

					// identity normal
					XMFLOAT3 normal = XMFLOAT3(0, 1, 0);

					// position
					fwrite(&pos.x, sizeof(float), 1, fp);
					fwrite(&pos.y, sizeof(float), 1, fp);
					fwrite(&pos.z, sizeof(float), 1, fp);

					// normals
					fwrite(&normal.x, sizeof(float), 1, fp);
					fwrite(&normal.y, sizeof(float), 1, fp);
					fwrite(&normal.z, sizeof(float), 1, fp);
				}

				// file complete
				fclose(fp);
			}
		}
	}
#endif

	// and finally leave node zero folder
	SetDir("..");

	// load in old terrain textures and vegmask
	// TODO

	// create new terrain node virtual texture map (highest LOD)
	// TODO

	// save grass map into terrain node files
	int iGrassMemblockThreshhold = 74; // Old classic grass not rendered below this value
	t.tfileveggrass_s = "vegmaskgrass.dat";
	grass_loadgrass();
	t.tfileveggrass_s = "TTR0XR0\\vegmaskgrass.dat";
	if ( MemblockExist(t.terrain.grassmemblock) == 0 ) 
	{
		MakeMemblock ( t.terrain.grassmemblock, 4+4+4+((MAXTEXTURESIZE*MAXTEXTURESIZE)*4) );
	}
	WriteMemblockDWord ( t.terrain.grassmemblock, 0, MAXTEXTURESIZE );
	WriteMemblockDWord ( t.terrain.grassmemblock, 4, MAXTEXTURESIZE );
	WriteMemblockDWord ( t.terrain.grassmemblock, 8, 32 );
	t.tPindex = 4+4+4;
	for ( t.tP = 0 ; t.tP <= MAXTEXTURESIZE*MAXTEXTURESIZE - 1; t.tP++ )
	{
		if (ReadMemblockByte(t.terrain.grassmemblock, t.tPindex + 2) >= iGrassMemblockThreshhold )
		{
			WriteMemblockByte(t.terrain.grassmemblock, t.tPindex + 1, 31); // green - full sized grass
			WriteMemblockByte(t.terrain.grassmemblock, t.tPindex + 2, 1); // red - only one grass type in classic
		}
		else
		{
			WriteMemblockByte(t.terrain.grassmemblock, t.tPindex + 1, 0);
			WriteMemblockByte(t.terrain.grassmemblock, t.tPindex + 2, 0);
		}
		WriteMemblockByte ( t.terrain.grassmemblock, t.tPindex+0, 0 );
		WriteMemblockByte ( t.terrain.grassmemblock, t.tPindex+3, 0 );
		t.tPindex += 4;
	}
	grass_savegrass();

	// and delete old grass file from testmap
	t.tfileveggrass_s = "vegmaskgrass.dat";
	if (FileExist(t.tfileveggrass_s.Get()) == 1) DeleteFileA(t.tfileveggrass_s.Get());

	// Restore folder
	SetDir(pOldDir);

	// load all entity parents so we can scan the associated files
	int entidmaster = 0;
	std::vector<std::string> pEntBank;
	pEntBank.clear();
	cstr filename_s = t.levelmapptah_s+"map.ent";
	if ( FileExist(filename_s.Get()) == 1 )
	{
		OpenToRead(1, cstr(t.levelmapptah_s + "map.ent").Get());
		entidmaster = ReadLong(1);
		if (entidmaster > 0)
		{
			for ( int entid = 1; entid <= entidmaster; entid++)
			{
				std::string pEntName = ReadString(1);
				pEntBank.push_back(pEntName);
			}
		}
		CloseFile(1);
	}
	// scan all entities and copy over assets not present
	for ( int entlistindex = 0; entlistindex < pEntBank.size(); entlistindex++)
	{
		// for each entity parent, copy all associated files over (fpe, bmp, model, textures)
		LPSTR pEntityName = (LPSTR)pEntBank[entlistindex].c_str();
		if (pEntityName && strlen(pEntityName) > 0)
		{
			// src points to classic folder, dest points to new location in writable area
			char pSrcFile[MAX_PATH];
			char pDestFile[MAX_PATH];

			// find just the entity folder
			char pEntityFolder[MAX_PATH];
			strcpy(pEntityFolder, pEntityName);
			for (int n = strlen(pEntityFolder); n > 0; n--)
			{
				if (pEntityFolder[n] == '\\' || pEntityFolder[n] == '/')
				{
					pEntityFolder[n+1] = 0;
					break;
				}
			}

			// skip if write into certain protected folders
			if (stricmp(pEntityFolder, "_markers\\") == NULL)
				continue;

			// copy FPE
			strcpy(pSrcFile, pReconstructGameGuruFolder);
			strcat(pSrcFile, pEntityName);
			strcpy(pDestFile, "entitybank\\");
			strcat(pDestFile, pEntityName);
			GG_GetRealPath(pDestFile, 1);
			CopyFileA(pSrcFile, pDestFile, TRUE);

			// load in FPE to obtain fields
			char pModelFile[MAX_PATH];
			char pTexturedFile[MAX_PATH];
			strcpy(pModelFile, "");
			strcpy(pTexturedFile, "");
			bool bCanBeIgnored = false;
			if (!pestrcasestr(pEntityName, "\\"))
			{
				//PE: If this is a EBE structure it will have no '\' so check if we can ignore it without generating errors.
				if(!FileExist(pSrcFile))
					bCanBeIgnored = true;
			}
			if (!bCanBeIgnored)
			{
				OpenToRead(1, pSrcFile);
				while (FileEnd(1) == 0)
				{
					LPSTR pLine = ReadString(1);
					LPSTR pToken = "model";
					LPSTR pModelFileSearch = strstr(pLine, pToken);
					if (pModelFileSearch != NULL)
					{
						LPSTR pModelFileEqual = strstr(pModelFileSearch, "=");
						if (pModelFileEqual != NULL)
						{
							pModelFileEqual++;
							while (*pModelFileEqual == 32 || *pModelFileEqual == 9) pModelFileEqual++;
							strcpy(pModelFile, pModelFileEqual);
						}
					}
					pToken = "textured";
					LPSTR pTexturedFileSearch = strstr(pLine, pToken);
					if (pTexturedFileSearch != NULL)
					{
						LPSTR pTexturedFileEqual = strstr(pTexturedFileSearch, "=");
						if (pTexturedFileEqual != NULL)
						{
							pTexturedFileEqual++;
							while (*pTexturedFileEqual == 32 || *pTexturedFileEqual == 9) pTexturedFileEqual++;
							strcpy(pTexturedFile, pTexturedFileEqual);
						}
					}
				}
				CloseFile(1);

				// copy BMP
				char pBMP[MAX_PATH];
				strcpy(pBMP, pEntityName);
				pBMP[strlen(pBMP) - 4] = 0;
				strcat(pBMP, ".bmp");
				strcpy(pSrcFile, pReconstructGameGuruFolder);
				strcat(pSrcFile, pBMP);
				strcpy(pDestFile, "entitybank\\");
				strcat(pDestFile, pBMP);
				GG_GetRealPath(pDestFile, 1);
				CopyFileA(pSrcFile, pDestFile, TRUE);
			}

			// model and textured can point to gamecore
			char pGameCoreAsset[MAX_PATH];
			LPSTR pGameCorePrefix = "gamecore\\";
			strcpy(pGameCoreAsset, "");

			// if model is from gamecore, copy those assets instead
			if (strlen(pModelFile) > 0)
			{
				if (strnicmp(pModelFile, pGameCorePrefix, strlen(pGameCorePrefix)) == NULL)
				{
					// copy gamecore asset 
					strcpy(pGameCoreAsset, pModelFile+strlen(pGameCorePrefix));
				}
				else
				{
					// model stored in FPE
					strcpy(pSrcFile, pReconstructGameGuruFolder);
					strcat(pSrcFile, pEntityFolder);
					strcat(pSrcFile, pModelFile);
					strcpy(pDestFile, "entitybank\\");
					strcat(pDestFile, pEntityFolder);
					strcat(pDestFile, pModelFile);
					GG_GetRealPath(pDestFile, 1);
					CopyFileA(pSrcFile, pDestFile, TRUE);
				}
			}

			// texture refs stored in FPE
			if (strlen(pTexturedFile) > 0)
			{
				// if model is from gamecore, copy those assets instead
				if (strnicmp(pTexturedFile, pGameCorePrefix, strlen(pGameCorePrefix))== NULL)
				{
					// copy gamecore asset 
					strcpy(pGameCoreAsset, pTexturedFile+strlen(pGameCorePrefix));
				}
				else
				{
					// single texture
					strcpy(pSrcFile, pReconstructGameGuruFolder);
					strcat(pSrcFile, pEntityFolder);
					strcat(pSrcFile, pTexturedFile);
					strcpy(pDestFile, "entitybank\\");
					strcat(pDestFile, pEntityFolder);
					strcat(pDestFile, pTexturedFile);
					GG_GetRealPath(pDestFile, 1);
					CopyFileA(pSrcFile, pDestFile, TRUE);

					// if a PBR texture name, expand to other known formats
					char pExt[MAX_PATH];
					char pBaseName[MAX_PATH];
					strcpy(pBaseName, pTexturedFile);
					if (strlen(pBaseName) > 4)
					{
						// retain extension 
						strcpy(pExt, pBaseName + strlen(pBaseName) - 4);

						// find base filename
						bool bFBF = false;
						pBaseName[strlen(pBaseName) - 4] = 0;
						LPSTR pFM = "_color"; if (strnicmp(pBaseName + strlen(pBaseName) - strlen(pFM), pFM, strlen(pFM)) == NULL) { pBaseName[strlen(pBaseName) - strlen(pFM)] = 0; bFBF = true; }
						pFM = "_D"; if (strnicmp(pBaseName + strlen(pBaseName) - strlen(pFM), pFM, strlen(pFM)) == NULL) { pBaseName[strlen(pBaseName) - strlen(pFM)] = 0; bFBF = true; }
						if (bFBF == true)
						{
							// go through all possible PBR extras
							for (int iPBR = 0; iPBR < 9; iPBR++)
							{
								LPSTR pType = "";
								if (iPBR == 0) pType = "_normal";
								if (iPBR == 1) pType = "_metalness";
								if (iPBR == 2) pType = "_gloss";
								if (iPBR == 3) pType = "_ao";
								if (iPBR == 4) pType = "_n";
								if (iPBR == 5) pType = "_s";
								if (iPBR == 6) pType = "_i";
								if (iPBR == 7) pType = "_d2";
								if (iPBR == 8) pType = "_illum";

								// attempt to copy this type
								strcpy(pSrcFile, pReconstructGameGuruFolder);
								strcat(pSrcFile, pEntityFolder);
								strcat(pSrcFile, pBaseName);
								strcat(pSrcFile, pType);
								strcat(pSrcFile, pExt);
								strcpy(pDestFile, "entitybank\\");
								strcat(pDestFile, pEntityFolder);
								strcat(pDestFile, pBaseName);
								strcat(pDestFile, pType);
								strcat(pDestFile, pExt);
								GG_GetRealPath(pDestFile, 1);
								CopyFileA(pSrcFile, pDestFile, TRUE);
							}
						}
						else
						{
							// just the texture name then
							strcpy(pSrcFile, pReconstructGameGuruFolder);
							strcat(pSrcFile, pEntityFolder);
							strcat(pSrcFile, pTexturedFile);
							strcpy(pDestFile, "entitybank\\");
							strcat(pDestFile, pEntityFolder);
							strcat(pDestFile, pTexturedFile);
							GG_GetRealPath(pDestFile, 1);
							CopyFileA(pSrcFile, pDestFile, TRUE);
						}
					}
				}
			}
			else
			{
				// multi-textured model - load in object and interogate
				// TODO
			}

			// if gamecore asset found, copy all of it
			if (strlen(pGameCoreAsset) > 0)
			{
				// find just the gamecore folder
				char pGameCoreFolder[MAX_PATH];
				strcpy(pGameCoreFolder, pGameCoreAsset);
				for (int n = strlen(pGameCoreFolder); n > 0; n--)
				{
					if (pGameCoreFolder[n] == '\\' || pGameCoreFolder[n] == '/')
					{
						pGameCoreFolder[n+1] = 0;
						break;
					}
				}

				// enter gamecore folder, and copy all files over
				char pSrcFolder[MAX_PATH];
				strcpy(pSrcFolder, pReconstructGameGuruRootFiles);
				strcat(pSrcFolder, "gamecore\\");
				strcat(pSrcFolder, pGameCoreFolder);
				SetDir(pSrcFolder);
				ChecklistForFiles();
				SetDir(pOldDir);
				for (int c = 1; c <= ChecklistQuantity(); c++)
				{
					LPSTR pFileName = ChecklistString(c);
					if (strcmp(pFileName, ".") != NULL && strcmp(pFileName, "..") != NULL)
					{
						strcpy(pSrcFile, pSrcFolder);
						strcat(pSrcFile, pFileName);
						strcpy(pDestFile, "gamecore\\");
						strcat(pDestFile, pGameCoreFolder);
						strcat(pDestFile, pFileName);
						GG_GetRealPath(pDestFile, 1);
						CopyFileA(pSrcFile, pDestFile, TRUE);
					}
				}
			}
		}
	}

	#ifndef FULLTERRAINEDITING
	// free usages
	if (fSrcHeightData)
	{
		delete fSrcHeightData;
		fSrcHeightData = NULL;
	}
	#endif

}
#endif
