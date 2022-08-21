//----------------------------------------------------
//--- GAMEGURU - M-Physics
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

#ifdef WICKEDENGINE
#include "GGTerrain\GGTerrain.h"
#include "GGTerrain\GGTrees.h"
using namespace GGTerrain;
using namespace GGTrees;
bool g_bSpecialPhysicsDebuggingMode = false;
GGVECTOR3 g_vPlayAreaMin = GGVECTOR3(0, 0, 0);
GGVECTOR3 g_vPlayAreaMax = GGVECTOR3(0, 0, 0);
//bool g_bSculptingRequiresNewPhysics = false;
bool g_bModifiedThisTerrainGrid[21][21];
int g_iLastProgressPercentage = -1;

//int g_iMaterialMapSize = 0;
//float g_fMaterialMapRealSize = 0;
//uint8_t* g_cMaterialMap = NULL;
bool g_bMapMatIDToMatIndexAvailable = false;
int g_iMapMatIDToMatIndex[32];

#endif

// 
//  Physics Subroutines and Functions
// 

// 
//  PHYSICS CODE
// 

void physics_inittweakables ( void )
{
	//  Editable in Player Start Marker
	t.playercontrol.regenrate=0;
	t.playercontrol.regenspeed=100;
	t.playercontrol.regendelay=3000;
	t.playercontrol.regentime=0;
	t.playercontrol.jumpmax_f=215.0;
	t.playercontrol.gravity_f=900.0;
	t.playercontrol.fallspeed_f=5000.0;
	t.playercontrol.climbangle_f = 66.0f; //LB: messed with ability to climbs stairs, and small pallets on floor... 50.0f; //LB: was 70.0;
	t.playercontrol.footfallpace_f=3.0;
	t.playercontrol.wobblespeed_f=460.0;
	t.playercontrol.wobbleheight_f=1.5;
	t.playercontrol.accel_f=25.0;

	//  third person defaults
	t.playercontrol.thirdperson.enabled=0;
	t.playercontrol.thirdperson.charactere=0;
	t.playercontrol.thirdperson.startmarkere=0;
	t.playercontrol.thirdperson.cameralocked=0;
	t.playercontrol.thirdperson.cameradistance=200.0;
	t.playercontrol.thirdperson.cameraheight=100.0;
	t.playercontrol.thirdperson.camerafocus=5.0;
	t.playercontrol.thirdperson.cameraspeed=50.0;
	t.playercontrol.thirdperson.camerashoulder=6.0;
	t.playercontrol.thirdperson.camerafollow=1;
	t.playercontrol.thirdperson.camerareticle=1;
}

void physics_init ( void )
{
	#ifdef WICKEDENGINE
	// create material ID to material sound mapping
	if (g_bMapMatIDToMatIndexAvailable == false)
	{
		g_bMapMatIDToMatIndexAvailable = true;
		LPSTR pMatConvertTableFile = "terraintextures\\matsounds.txt";
		if (FileExist(pMatConvertTableFile) == 1)
		{
			OpenToRead(1, pMatConvertTableFile);
			while (FileEnd(1) == 0)
			{
				LPSTR pLine = ReadString (1);
				if (pLine != NULL)
				{
					if (pLine[0] != ';' && strlen(pLine) < 30)
					{
						char pNums[MAX_PATH];
						memset(pNums, 0, sizeof(pNums));
						if (strlen(pLine) > 3)
						{
							strcpy_s (pNums, MAX_PATH, pLine + 3); // skip 'mat'
						}
						LPSTR pEqual = strstr(pNums, "=");
						if (pEqual)
						{
							char pMaterialIndex[32];
							strcpy(pMaterialIndex, pEqual + 1);
							*pEqual = 0;
							char pTerrainMatID[32];
							strcpy(pTerrainMatID, pNums);
							int iMaterialIndex = atoi(pMaterialIndex);
							int iMatID = atoi(pTerrainMatID);
							if (iMatID >= 1 && iMatID <= 32)
							{
								g_iMapMatIDToMatIndex[iMatID - 1] = iMaterialIndex;
							}
						}
					}
				}
			}
			CloseFile(1);
		}
	}
	#endif

	//  Player Control
	t.playercontrol.wobble_f=0.0;
	t.playercontrol.floory_f=0.0;
	t.playercontrol.topspeed_f=0.75;
	t.playercontrol.footfalltype=0;
	t.playercontrol.footfallcount=0;
	if (  t.playercontrol.regenrate>0 ) 
	{
		t.playercontrol.regentime=Timer();
		t.playercontrol.regentick=Timer();
	}
	else
	{
		t.playercontrol.regentime=0;
		t.playercontrol.regentick=0;
	}

	//  Reset jetpack
	t.playercontrol.jetpackmode=0;
	t.playercontrol.jetpackthrust_f=0.0;
	t.playercontrol.jetpackfuel_f=0;

	//  Player misc settings
	t.playercontrol.disablemusicreset=0;

	//  Reset gun collected count
	t.guncollectedcount=0;

	//  Init physics system
	ODEStart (   ); g.gphysicssessionactive=1;

	//  Set starting water Line (  )
	terrain_updatewaterphysics ( );

	//  Create terrain collision
	t.tgenerateterraindirtyregiononly=0;
	timestampactivity(0,"_physics_createterraincollision");
	physics_createterraincollision ( );
	t.tgenerateterraindirtyregiononly=0;

	#ifdef WICKEDENGINE
	// MAX also has virtual trees that need physics
	physics_createvirtualtreecylinders();
	#endif

	//  Player Controller Object
	if (  ObjectExist(t.aisystem.objectstartindex) == 0 ) 
	{
		//  normally created by AI which precedes Physics initialisations
		MakeObjectCube (  t.aisystem.objectstartindex,10 );
	}
	HideObject (  t.aisystem.objectstartindex );

	// moved player physics setup closer to main loop
	//t.freezeplayerposonly = 0;
	//physics_setupplayer ( );

	//  set default player gravity
	t.playercontrol.gravityactive=1;
	t.playercontrol.gravityactivepress=0;
	t.playercontrol.lockatheight=0;
	t.playercontrol.lockatheightpress=0;

	//  Setup entity physics
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		physics_prepareentityforphysics ( );
	}

	//  Introduce all characters and entities to the physics universe
	timestampactivity(0,"Introduce all characters");
	for ( g.charanimindex = 1 ; g.charanimindex <= g.charanimindexmax; g.charanimindex++ )
	{
		// get physics object for this character
		t.tphyobj=t.charanimstates[g.charanimindex].obj;

		// 111115 - but exclude any third person character
		if ( t.playercontrol.thirdperson.enabled == 1 && t.playercontrol.thirdperson.characterindex == g.charanimindex ) 
			t.tphyobj = 0;

		// if object still requires Y adjustment check
		if ( t.tphyobj>0 ) 
		{
			t.e = t.charanimstates[g.charanimindex].e;
			bool bCharacterUsesPhysics = false;
			#ifdef WICKEDENGINE
			t.entityelement[t.e].usingphysicsnow = 0;
			if (t.entityprofile[t.entityelement[t.e].bankindex].physics != 0) bCharacterUsesPhysics = true;
			#else
			bCharacterUsesPhysics = true;
			#endif
			if (bCharacterUsesPhysics == true)
			{
				// get entity index associated with character
				physics_setupcharacter ();
			}
		}
	}

	// Ensure the LUA mouse is always reset
	lua_deactivatemouse();

	// player physics setup closer to main loop
	t.freezeplayerposonly = 0;
	physics_setupplayer ( );
}

void physics_finalize ( void )
{
	ODEFinalizeWorld();
}

// one way flood fill to reduce terrain polys
struct sLargeQuadListItem
{
	int iX1;
	int iZ1;
	int iX2;
	int iZ2;
};
//int g_iQuadGrid[201][201];
int g_iQuadGrid[401][401];
std::vector<sLargeQuadListItem> g_LargeQuadList;
std::vector<sLargeQuadListItem> g_RefinedLargeQuadList;
void physics_processheightsusingfloodfill (int iX, int iZ, int iGridAtX, int iGridAtZ, float fOneTileSize, int iFullResolutionSize, int iResolutionSIze, int iHeightDataSize, float* pfHeightData)
{
	// stage 0 - we mark this as used right now so no overlapping behaviors occur
	int iUniqueTileID = 1 + g_LargeQuadList.size();
	g_iQuadGrid[iX][iZ] = iUniqueTileID;

	// the right/bottom edge needs full resolution (to join seamlessly with next area)
	int iX1 = iX;
	int iZ1 = iZ;
	int iX2 = iX;
	int iZ2 = iZ;
	if (1)//iX < iResolutionSIze && iZ < iResolutionSIze)
	{
		// stage 1 - create plane from this quad (iX,iZ)
		GGPLANE thisPlane;
		int iRealX = (iGridAtX*iResolutionSIze);
		int iRealZ = (iGridAtZ*iResolutionSIze);
		int iGetHeightX = (iRealX + iX);
		int iGetHeightZ = (iRealZ + iZ);
		float fHeight = 0.0f;
		int iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
		if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
		GGVECTOR3 vecPos0 = GGVECTOR3(iX * fOneTileSize, fHeight, iZ * fOneTileSize);
		iGetHeightX = (iRealX + iX + 1);
		iGetHeightZ = (iRealZ + iZ);
		iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
		if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
		GGVECTOR3 vecPos1 = GGVECTOR3((iX + 1) * fOneTileSize, fHeight, iZ * fOneTileSize);
		iGetHeightX = (iRealX + iX);
		iGetHeightZ = (iRealZ + iZ + 1);
		iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
		if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
		GGVECTOR3 vecPos2 = GGVECTOR3(iX * fOneTileSize, fHeight, (iZ + 1) * fOneTileSize);
		GGPlaneFromPoints(&thisPlane, &vecPos0, &vecPos1, &vecPos2);

		// stage 2 - expand quad in both axis while within threshold
		bool bExpanding = true;
		float fThreshold = 2.0f;
		int iTryX = iX;
		int iTryZ = iZ;
		bool bCanExpandX = true;
		bool bCanExpandZ = true;
		while (bExpanding == true)
		{
			// expand in directions where g_iQuadGrid is free
			if (bCanExpandX == true)
			{
				iTryX++;
				for (int iAllTheZs = iZ1; iAllTheZs <= iZ2; iAllTheZs++)
				{
					if (g_iQuadGrid[iTryX][iAllTheZs] == 0)
					{
						iGetHeightX = (iRealX + iTryX);
						iGetHeightZ = (iRealZ + iAllTheZs);
						fHeight = 0.0f;
						iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
						if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
						GGVECTOR3 vecTryPos = GGVECTOR3(iTryX * fOneTileSize, fHeight, iAllTheZs * fOneTileSize);
						float fDeviationFromPlane = GGPlaneDotCoord(&thisPlane, &vecTryPos);
						if (fabs(fDeviationFromPlane) > fThreshold)
						{
							// this position cannot be added to the current plane/largequad
							bCanExpandX = false;
						}
					}
					else
					{
						// hit a used tile, also cannot expand in this direction any more
						bCanExpandX = false;
					}
				}
				if (bCanExpandX == true)
				{
					// this is fine, keep going in the X direction
					iX2 = iTryX;
					for (int z = iZ1; z <= iZ2; z++)
						for (int x = iX1; x <= iX2; x++)
							g_iQuadGrid[x][z] = iUniqueTileID;
				}
				if (iTryX == iResolutionSIze) bCanExpandX = false;
			}
			if (bCanExpandZ == true)
			{
				iTryZ++;
				for (int iAllTheXs = iX1; iAllTheXs <= iX2; iAllTheXs++)
				{
					if (g_iQuadGrid[iAllTheXs][iTryZ] == 0)
					{
						iGetHeightX = (iRealX + iAllTheXs);
						iGetHeightZ = (iRealZ + iTryZ);
						fHeight = 0.0f;
						iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
						if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
						GGVECTOR3 vecTryPos = GGVECTOR3(iAllTheXs * fOneTileSize, fHeight, iTryZ * fOneTileSize);
						float fDeviationFromPlane = GGPlaneDotCoord(&thisPlane, &vecTryPos);
						if (fabs(fDeviationFromPlane) > fThreshold)
						{
							// this position cannot be added to the current plane/largequad
							bCanExpandZ = false;
						}
					}
					else
					{
						// hit a used tile, also cannot expand in this direction any more
						bCanExpandZ = false;
					}
				}
				if (bCanExpandZ == true)
				{
					// this is fine, keep going in the Z direction
					iZ2 = iTryZ;
					for (int z = iZ1; z <= iZ2; z++)
						for (int x = iX1; x <= iX2; x++)
							g_iQuadGrid[x][z] = iUniqueTileID;
				}
				if (iTryZ == iResolutionSIze) bCanExpandZ = false;
			}

			// if threshold exceeded in both directions, cannot expand any more
			if (bCanExpandX == false && bCanExpandZ == false)
				bExpanding = false;
		}
	}

	// stage 3 - create a 'larger' quad for this expanded area
	sLargeQuadListItem item;
	item.iX1 = iX1;
	item.iZ1 = iZ1;
	item.iX2 = iX2;
	item.iZ2 = iZ2;
	g_LargeQuadList.push_back(item);
}

/*
// global anomoly grid for recursive step
struct sAnomListType
{
	int p1;
	int p2;
	int p3;
	int p4;
};
bool g_bAnom[200][200];
std::vector<sAnomListType> g_anomlist;
void physics_processanomgridtile (int iX1, int iZ1, int iX2, int iZ2, float fOneTileSize, int iResolutionSIze)
{
	bool bAnyAnomInThisArea = false;
	if (iZ2 > iZ1 || iX2 > iX1)
	{
		for (int z = iZ1; z <= iZ2; z++)
		{
			for (int x = iX1; x <= iX2; x++)
			{
				if (g_bAnom[x][z] == true)
				{
					bAnyAnomInThisArea = true;
				}
			}
		}
	}
	if (bAnyAnomInThisArea == true )
	{
		// divide and recurse
		int iSizeXHalf = (iX2 - iX1) / 2;
		int iSizeZHalf = (iZ2 - iZ1) / 2;
		physics_processanomgridtile (iX1, iZ1, iX1 + iSizeXHalf, iZ1 + iSizeZHalf, fOneTileSize, iResolutionSIze); // top left
		physics_processanomgridtile (iX1 + iSizeXHalf + 1, iZ1, iX2, iZ1 + iSizeZHalf, fOneTileSize, iResolutionSIze); // top right
		physics_processanomgridtile (iX1, iZ1 + iSizeZHalf + 1, iX1 + iSizeXHalf, iZ2, fOneTileSize, iResolutionSIze); // bottom left
		physics_processanomgridtile (iX1 + iSizeXHalf + 1, iZ1 + iSizeZHalf + 1, iX2, iZ2, fOneTileSize, iResolutionSIze); // bottom right
	}
	else
	{
		// take four corners as a single poly (and so doing, ignore unnecessary detail for the purpose of physics)
		sAnomListType anomitem;
		anomitem.p1 = iX1 + (iZ1*iResolutionSIze);
		anomitem.p2 = iX1 + 1 + (iZ1*iResolutionSIze);
		anomitem.p3 = iX1 + ((iZ1 + 1)*iResolutionSIze);
		anomitem.p4 = iX1 + 1 + ((iZ1 + 1) *iResolutionSIze);
		g_anomlist.push_back(anomitem);
	}
}
*/

void physics_createterraincollision ( void )
{
	#ifdef WICKEDENGINE
	// we use the 'play area' to reduce 'test level' load time (concentrating only on the area you are using in the level)
	timestampactivity(0, "detect and define play area");
	// calculate bounds of static features
	static float fLastPlayerStartX = -1.0f;
	static float fLastPlayerStartZ = -1.0f;
	float fPlayerStartX = CameraPositionX(0);
	float fPlayerStartZ = CameraPositionZ(0);
	GGVECTOR3 vecMin, vecMax;
	vecMin = GGVECTOR3(fPlayerStartX, 0, fPlayerStartZ);
	vecMax = GGVECTOR3(fPlayerStartX, 0, fPlayerStartZ);
	vecMin -= GGVECTOR3(-500, 0, -500);
	vecMax += GGVECTOR3(500, 0, 500);
	int iObjectCount = 0;

	for (int e = 1; e <= g.entityelementlist; e++)
	{
		if ( 1 ) // must include start marker and all dynamic objects (everything) for full coverage of level
		{
			int iObj = t.entityelement[e].obj;
			if (iObj > 0)
			{
				if (ObjectExist(iObj) == 1)
				{
					if (vecMin.x > ObjectPositionX(iObj)) vecMin.x = ObjectPositionX(iObj);
					if (vecMin.y > ObjectPositionY(iObj)) vecMin.y = ObjectPositionY(iObj);
					if (vecMin.z > ObjectPositionZ(iObj)) vecMin.z = ObjectPositionZ(iObj);
					if (vecMax.x < ObjectPositionX(iObj)) vecMax.x = ObjectPositionX(iObj);
					if (vecMax.y < ObjectPositionY(iObj)) vecMax.y = ObjectPositionY(iObj);
					if (vecMax.z < ObjectPositionZ(iObj)) vecMax.z = ObjectPositionZ(iObj);
					iObjectCount++;
				}
			}
		}
		int entid = t.entityelement[e].bankindex;
		if (entid > 0)
		{
			if (t.entityprofile[entid].ismarker == 1)
			{
				// player start marker
				fPlayerStartX = t.entityelement[e].x;
				fPlayerStartZ = t.entityelement[e].z;
			}
		}
	}
	#ifdef WICKEDENGINE
	float fEditableSizeHalved = GGTerrain_GetEditableSize();
	t.terraineditableareasizeminx = -fEditableSizeHalved;
	t.terraineditableareasizeminz = -fEditableSizeHalved;
	t.terraineditableareasizemaxx = fEditableSizeHalved;
	t.terraineditableareasizemaxz = fEditableSizeHalved;
	#endif
	bool bAnythingHasMoved = false;
	if (fLastPlayerStartX != fPlayerStartX || fLastPlayerStartZ != fPlayerStartZ ) bAnythingHasMoved = true;
	fLastPlayerStartX = fPlayerStartX;
	fLastPlayerStartZ = fPlayerStartZ;
	g_vPlayAreaMin = vecMin - GGVECTOR3(1000, 0, 1000);
	g_vPlayAreaMax = vecMax + GGVECTOR3(1000, 0, 1000);
	if (g_vPlayAreaMin.x < -50000) g_vPlayAreaMin.x = -50000;
	if (g_vPlayAreaMin.x > 50000)  g_vPlayAreaMin.x = 50000;
	if (g_vPlayAreaMin.z < -50000) g_vPlayAreaMin.z = -50000;
	if (g_vPlayAreaMin.z > 50000)  g_vPlayAreaMin.z = 50000;
	if (g_vPlayAreaMax.x < -50000) g_vPlayAreaMax.x = -50000;
	if (g_vPlayAreaMax.x > 50000)  g_vPlayAreaMax.x = 50000;
	if (g_vPlayAreaMax.z < -50000) g_vPlayAreaMax.z = -50000;
	if (g_vPlayAreaMax.z > 50000)  g_vPlayAreaMax.z = 50000;

	// above values are used by navmesh
	ODECreateGGTerrain();
	return;

	// this is there the terrain physics is created
	timestampactivity(0, "create terrain physics");
	#ifdef GGTERRAIN_USE_NEW_TERRAIN
	float fPlayAreaSizeX = g_vPlayAreaMax.x - g_vPlayAreaMin.x; // soon to be relegated to 'nav clusters'
	float fPlayAreaSizeZ = g_vPlayAreaMax.z - g_vPlayAreaMin.z;
	float fEditableSizeX = t.terraineditableareasizemaxx - t.terraineditableareasizeminx;
	float fEditableSizeZ = t.terraineditableareasizemaxz - t.terraineditableareasizeminz;

	// if terrain has been changed
	bool bSkipTerrainPhyGen = false;
	if ( bSkipTerrainPhyGen == false )
	{
		// create terrain physics from GGTerrain Height Data
		float fEditableSizeHalved = GGTerrain_GetEditableSize();
		float fEditableSize = fEditableSizeHalved * 2.0f;
		int iDivideGrid = 20;
		int iFullResolutionSize = 5000;// PE: 5000 produce a maxVertex of 0x0000d9d4 (6000 go above 0xFFFF).
		int maxVertex = 0;
		int iHeightDataSize = iFullResolutionSize * iFullResolutionSize;
		float* pfHeightData = new float[iHeightDataSize];
		if (pfHeightData)
		{
			// get height data and other terrain settings
			bool bUseOriginalLOD = true; // need real correct height data, even if far away! Prevents player falling through terrain
			int iResult = GGTerrain_GetHeightMapEx(iFullResolutionSize, iFullResolutionSize, pfHeightData, bUseOriginalLOD);
			int iResolutionSIze = iFullResolutionSize / iDivideGrid;
			float fOneTileSize = fEditableSize / (iFullResolutionSize - 1);

			// init for ALL editable terrain (but slow until move to terrain gen UI and latest bullet3)
			int iGridFromX = 0;
			int iGridFromZ = 0;
			int iGridToX = iDivideGrid;
			int iGridToZ = iDivideGrid;

			// so we reduce to only the playable area to speed up test level experiences
			bool bLimitPhysicsToPlayableArea = false;
			if (bLimitPhysicsToPlayableArea == true)
			{			
				int iCameraGridX1 = iFullResolutionSize - (((fEditableSizeHalved - g_vPlayAreaMin.x) / fOneTileSize));
				int iCameraGridZ1 = iFullResolutionSize - (((fEditableSizeHalved - g_vPlayAreaMin.z) / fOneTileSize));
				iCameraGridX1 /= (iFullResolutionSize / iDivideGrid);
				iCameraGridZ1 /= (iFullResolutionSize / iDivideGrid);
				if (iCameraGridX1 < 2) iCameraGridX1 = 2;
				if (iCameraGridZ1 < 2) iCameraGridZ1 = 2;
				iGridFromX = iCameraGridX1 - 2;
				iGridFromZ = iCameraGridZ1 - 2;
				int iCameraGridX2 = iFullResolutionSize - (((fEditableSizeHalved - g_vPlayAreaMax.x) / fOneTileSize));
				int iCameraGridZ2 = iFullResolutionSize - (((fEditableSizeHalved - g_vPlayAreaMax.z) / fOneTileSize));
				iCameraGridX2 /= (iFullResolutionSize / iDivideGrid);
				iCameraGridZ2 /= (iFullResolutionSize / iDivideGrid);
				if (iCameraGridX2 > iDivideGrid - 2) iCameraGridX2 = iDivideGrid - 2;
				if (iCameraGridZ2 > iDivideGrid - 2) iCameraGridZ2 = iDivideGrid - 2;
				iGridToX = iCameraGridX2 + 2;
				iGridToZ = iCameraGridZ2 + 2;
			}

			// if area of terrain changed, need to wipe out old terrain cache files so can create new ones
			static int iGridFromXLast = -1;
			static int iGridToXLast = -1;
			static int iGridFromZLast = -1;
			static int iGridToZLast = -1;
			static cstr oldprojectfilename_s = "";
			if (g.projectfilename_s != oldprojectfilename_s)
			{
				//PE: Also if loaded another level, clear.
				iGridFromXLast = -1;
				oldprojectfilename_s = g.projectfilename_s;
			}
			if (iGridFromX != iGridFromXLast || iGridToX != iGridToXLast || iGridFromZ != iGridFromZLast || iGridToZ != iGridToZLast)
			{
				iGridFromXLast = iGridFromX;
				iGridToXLast = iGridToX;
				iGridFromZLast = iGridFromZ;
				iGridToZLast = iGridToZ;
				extern void gridedit_emptyallterrainobjfiles();
				cstr pStoreOld = GetDir();
				if (PathExist (g.mysystem.levelBankTestMap_s.Get()) == 0)
				{
					SetDir (cstr(g.fpscrootdir_s + "\\Files\\").Get());
					if (PathExist ("levelbank") == 0)
					{
						MakeDirectory ("levelbank");
						SetDir ("levelbank");
					}
					if (PathExist ("testmap") == 0)
					{
						MakeDirectory ("testmap");
						SetDir ("testmap");
					}
				}
				else
				{
					//PE: We must resolve this, so we dont use the standalone folder but the writefolder where everything is stored.
					char resolvefolder[MAX_PATH];
					strcpy(resolvefolder, g.mysystem.levelBankTestMap_s.Get());
					GG_GetRealPath(resolvefolder, 1);
					SetDir (resolvefolder);
				}
				gridedit_emptyallterrainobjfiles();
				SetDir (pStoreOld.Get());
			}
			// all grids for now
			int iIterCount = 0;
			int iTerrainPhyObjIndex = 0;
			int iDataSizeX = iGridToX - iGridFromX;
			int iDataSizeZ = iGridToZ - iGridFromZ;
			for (int iGridAtZ = iGridFromZ; iGridAtZ <= iGridToZ; iGridAtZ++)
			{
				for (int iGridAtX = iGridFromX; iGridAtX <= iGridToX; iGridAtX++)
				{
					// terrain object
					t.tphysicsterrainobjstart = t.terrain.objectstartindex + 1000;
					int iTerrainPhyObj = t.tphysicsterrainobjstart + iTerrainPhyObjIndex;
					iTerrainPhyObjIndex++;
					char pTerrainBVHChunkFilename[MAX_PATH];
					sprintf(pTerrainBVHChunkFilename, "%s\\Files\\levelbank\\testmap\\terrainobj%d.bullet", g.fpscrootdir_s.Get(), iTerrainPhyObj);
					GG_GetRealPath(pTerrainBVHChunkFilename, 1);

					// if this terrain object has been changed, delete its file, so it will regenerate
					if (g_bModifiedThisTerrainGrid[iGridAtX][iGridAtZ] == true)
					{
						if (FileExist(pTerrainBVHChunkFilename) == 1) DeleteFileA(pTerrainBVHChunkFilename);
						g_bModifiedThisTerrainGrid[iGridAtX][iGridAtZ] = false;
					}

					// report progress
					char pProgressStr[256];
					float fProgressPercentage = (float(((iGridAtZ - iGridFromZ)*iDataSizeX) + (iGridAtX - iGridFromX)) / (iDataSizeX*iDataSizeZ))*100.0f;
					if (fProgressPercentage > 100.0f) fProgressPercentage = 100.0f;
					if (t.game.gameisexe == 0 && g_iLastProgressPercentage != (int)fProgressPercentage)
					{
						g_iLastProgressPercentage = (int)fProgressPercentage / 3;
						sprintf(pProgressStr, "CREATING TERRAIN PHYSICS - [%d] : %d\\100 Complete", iIterCount, (int)fProgressPercentage);
						t.screenprompt_s = pProgressStr;
						printscreenprompt(t.screenprompt_s.Get());
					}

					// LB: New system to (a) speed up terrain physics generation and (b) improve accuracy for sharp and steep terrain details
					bool bNewFloodFillTerrainPhysicsReducer = true;
					if (bNewFloodFillTerrainPhysicsReducer == true)
					{
						// relative position in overall height field
						int iRealX = (iGridAtX*iResolutionSIze);
						int iRealZ = (iGridAtZ*iResolutionSIze);

						// one way flood fill uses no iteration for performance
						g_LargeQuadList.clear();
						memset(g_iQuadGrid, 0, sizeof(g_iQuadGrid));
						for (int z = 0; z < iResolutionSIze; z++)
						{
							for (int x = 0; x < iResolutionSIze; x++)
							{
								if (g_iQuadGrid[x][z] == 0)
								{
									physics_processheightsusingfloodfill (x, z, iGridAtX, iGridAtZ, fOneTileSize, iFullResolutionSize, iResolutionSIze, iHeightDataSize, pfHeightData);
								}
							}
						}

						// go through original large quads
						int iLargeQuadListSizeBeforeAdditions = g_LargeQuadList.size();
						g_RefinedLargeQuadList.clear();
						for (int quad = 0; quad < g_LargeQuadList.size(); quad++)
						{
							int iX1 = g_LargeQuadList[quad].iX1;
							int iZ1 = g_LargeQuadList[quad].iZ1;
							int iX2 = g_LargeQuadList[quad].iX2;
							int iZ2 = g_LargeQuadList[quad].iZ2;
							//if (iX2 > iX1 || iZ2 > iZ1) real solution is allow single strips to be divided !!!
							if (iX2 == iX1 && iZ2 == iZ1)
							{
								// no need to divide a SINGLE grid item
							}
							else
							{
								/* bottom edges act as glue for any tops, not the solution!
								// top edge
								int iTopEdgeUsed = -1;
								if (iZ1 > 0)
								{
									iTopEdgeUsed = iZ1;
									int iNeighboughID = g_iQuadGrid[iX1][iZ2 - 1] - 1;
									if (iNeighboughID >= 0)
									{
										if (g_LargeQuadList[iNeighboughID].iX1 == iX1 && g_LargeQuadList[iNeighboughID].iX2 == iX2)
										{
											// the neighbor lines up exactly, no need to fragment this edge
											iTopEdgeUsed = -1;
										}
										else
										{
											// we need to scan this edge and create fragments of neighboring quads to line up to
											int iCurrentX = iX1;
											int iCurrentID = iNeighboughID;
											for (int x = iX1; x <= iX2; x++)
											{
												iNeighboughID = g_iQuadGrid[x][iZ1 - 1] - 1;
												if (iNeighboughID >= 0)
												{
													if (iNeighboughID != iCurrentID)
													{
														// fill quad grid
														for (int fillx = iCurrentX; fillx < x - 1; fillx++)
														{
															int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
															g_iQuadGrid[fillx][iZ1] = iUniqueTileID;
														}

														// need a new line up quad here
														sLargeQuadListItem item;
														item.iX1 = iCurrentX;
														item.iX2 = x - 1;
														item.iZ1 = iZ1;
														item.iZ2 = iZ1;
														g_RefinedLargeQuadList.push_back(item);

														// set up for next one
														iCurrentX = x;
														iCurrentID = iNeighboughID;
													}
												}
											}
											if (iZ2 > iZ1)
											{
												// multiple rows deep, the norm
												if (iCurrentX <= iX2)
												{
													// fill quad grid
													for (int fillx = iCurrentX; fillx < iX2; fillx++)
													{
														int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
														g_iQuadGrid[fillx][iZ1] = iUniqueTileID;
													}

													// final line-up quad on this edge
													sLargeQuadListItem item;
													item.iX1 = iCurrentX;
													item.iX2 = iX2;
													item.iZ1 = iZ1;
													item.iZ2 = iZ1;
													g_RefinedLargeQuadList.push_back(item);
												}
												// and finally reduce the original on this side
												g_LargeQuadList[quad].iZ1 = iZ1 + 1;
											}
											else
											{
												// only a single row deep
												if (iCurrentX <= iX2)
												{
													// so instead of using a new one, use existing orig
													g_LargeQuadList[quad].iX1 = iCurrentX;
													g_LargeQuadList[quad].iX2 = iX2;
													g_LargeQuadList[quad].iZ1 = iZ1;
													g_LargeQuadList[quad].iZ2 = iZ1;
												}
												else
												{
													// if the last one created was indeed the last one
													// edit the old orig and delete from g_RefinedLargeQuadList list
													int iLastNewItemIndex = g_RefinedLargeQuadList.size();
													g_LargeQuadList[quad] = g_RefinedLargeQuadList[iLastNewItemIndex - 1];
													g_RefinedLargeQuadList.erase(g_RefinedLargeQuadList.begin() + iLastNewItemIndex);
												}
											}
										}
									}
									else
									{
										// error - seems we have a missing neightbor
										iTopEdgeUsed = -1;
									}
								}
								*/

								// bottom edge
								if (iZ2 < iResolutionSIze )// && iTopEdgeUsed != iZ2)
								{
									int iNeighboughID = g_iQuadGrid[iX1][iZ2 + 1] - 1;
									if (iNeighboughID >= 0)
									{
										if (g_LargeQuadList[iNeighboughID].iX1 == iX1 && g_LargeQuadList[iNeighboughID].iX2 == iX2)
										{
											// the neighbor lines up exactly, no need to fragment this edge
										}
										else
										{
											// we need to scan this edge and create fragments of neighboring quads to line up to
											int iCurrentX = iX1;
											int iCurrentID = iNeighboughID;
											for (int x = iX1; x <= iX2; x++)
											{
												iNeighboughID = g_iQuadGrid[x][iZ2 + 1] - 1;
												if (iNeighboughID >= 0)
												{
													if (iNeighboughID != iCurrentID)
													{
														// fill quad grid
														for (int fillx = iCurrentX; fillx < x - 1; fillx++)
														{
															int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
															g_iQuadGrid[fillx][iZ2] = iUniqueTileID;
														}

														// need a new line up quad here
														sLargeQuadListItem item;
														item.iX1 = iCurrentX;
														item.iX2 = x - 1;
														item.iZ1 = iZ2;
														item.iZ2 = iZ2;
														g_RefinedLargeQuadList.push_back(item);

														// set up for next one
														iCurrentX = x;
														iCurrentID = iNeighboughID;
													}
												}
											}
											if (iZ2 > iZ1)
											{
												// multiple rows deep, the norm
												if (iCurrentX <= iX2)
												{
													// fill quad grid
													for (int fillx = iCurrentX; fillx < iX2; fillx++)
													{
														int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
														g_iQuadGrid[fillx][iZ2] = iUniqueTileID;
													}

													// final line-up quad on this edge
													sLargeQuadListItem item;
													item.iX1 = iCurrentX;
													item.iX2 = iX2;
													item.iZ1 = iZ2;
													item.iZ2 = iZ2;
													g_RefinedLargeQuadList.push_back(item);
												}
												// and finally reduce the original on this side
												g_LargeQuadList[quad].iZ2 = iZ2 - 1;
											}
											else
											{
												// only a single row deep
												if (iCurrentX <= iX2)
												{
													// so instead of using a new one, use existing orig
													g_LargeQuadList[quad].iX1 = iCurrentX;
													g_LargeQuadList[quad].iX2 = iX2;
													g_LargeQuadList[quad].iZ1 = iZ2;
													g_LargeQuadList[quad].iZ2 = iZ2;
												}
												else
												{
													// if the last one created was indeed the last one
													// edit the old orig and delete from g_RefinedLargeQuadList list
													int iLastNewItemIndex = g_RefinedLargeQuadList.size();
													g_LargeQuadList[quad] = g_RefinedLargeQuadList[iLastNewItemIndex- 1];
													g_RefinedLargeQuadList.erase(g_RefinedLargeQuadList.begin() + iLastNewItemIndex);
												}
											}
										}
									}
									else
									{
										// error - seems we have a missing neightbor
									}
								}
							}
							if (g_RefinedLargeQuadList.size() > 0)
							{
								for (int addquad = 0; addquad < g_RefinedLargeQuadList.size(); addquad++)
								{
									g_LargeQuadList.push_back(g_RefinedLargeQuadList[addquad]);
								}
								g_RefinedLargeQuadList.clear();
							}
						}
						for (int quad = 0; quad < g_LargeQuadList.size(); quad++)
						{
							int iX1 = g_LargeQuadList[quad].iX1;
							int iZ1 = g_LargeQuadList[quad].iZ1;
							int iX2 = g_LargeQuadList[quad].iX2;
							int iZ2 = g_LargeQuadList[quad].iZ2;
							if (iX2 == iX1 && iZ2 == iZ1)
							{
								// no need to divide single grid
							}
							else
							{
								// right edge
								if (iX2 < iResolutionSIze)
								{
									int iNeighboughID = g_iQuadGrid[iX2 + 1][iZ1] - 1;
									if (iNeighboughID >= 0)
									{
										if (g_LargeQuadList[iNeighboughID].iZ1 == iZ1 && g_LargeQuadList[iNeighboughID].iZ2 == iZ2)
										{
											// the neighbor lines up exactly, no need to fragment this edge
										}
										else
										{
											// we need to scan this edge and create fragments of neighboring quads to line up to
											int iCurrentZ = iZ1;
											int iCurrentID = iNeighboughID;
											for (int z = iZ1; z <= iZ2; z++)
											{
												iNeighboughID = g_iQuadGrid[iX2 + 1][z] - 1;
												if (iNeighboughID >= 0)
												{
													if (iNeighboughID != iCurrentID)
													{
														// fill quad grid
														for (int fillz = iCurrentZ; fillz < z - 1; fillz++)
														{
															int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
															g_iQuadGrid[iX2][fillz] = iUniqueTileID;
														}

														// need a new line up quad here
														sLargeQuadListItem item;
														item.iZ1 = iCurrentZ;
														item.iZ2 = z - 1;
														item.iX1 = iX2;
														item.iX2 = iX2;
														g_RefinedLargeQuadList.push_back(item);

														// set up for next one
														iCurrentZ = z;
														iCurrentID = iNeighboughID;
													}
												}
											}
											if (iX2 > iX1)
											{
												// multi-row
												if (iCurrentZ <= iZ2)
												{
													// fill quad grid
													for (int fillz = iCurrentZ; fillz < iZ2; fillz++)
													{
														int iUniqueTileID = 1 + g_RefinedLargeQuadList.size() + iLargeQuadListSizeBeforeAdditions;
														g_iQuadGrid[iX2][fillz] = iUniqueTileID;
													}

													// final line-up quad on this edge
													sLargeQuadListItem item;
													item.iZ1 = iCurrentZ;
													item.iZ2 = iZ2;
													item.iX1 = iX2;
													item.iX2 = iX2;
													g_RefinedLargeQuadList.push_back(item);
												}
												// and finally reduce the original on this side
												g_LargeQuadList[quad].iX2 = iX2 - 1;
											}
											else
											{
												// single row
												if (iCurrentZ <= iZ2)
												{
													// so instead of using a new one, use existing orig
													g_LargeQuadList[quad].iZ1 = iCurrentZ;
													g_LargeQuadList[quad].iZ2 = iZ2;
													g_LargeQuadList[quad].iX1 = iX2;
													g_LargeQuadList[quad].iX2 = iX2;
												}
												else
												{
													// if the last one created was indeed the last one
													// edit the old orig and delete from g_RefinedLargeQuadList list
													int iLastNewItemIndex = g_RefinedLargeQuadList.size();
													g_LargeQuadList[quad] = g_RefinedLargeQuadList[iLastNewItemIndex - 1];
													g_RefinedLargeQuadList.erase(g_RefinedLargeQuadList.begin() + iLastNewItemIndex);
												}
											}
										}
									}
									else
									{
										// error - seems we have a missing neightbor
									}
								}
							}
							if (g_RefinedLargeQuadList.size() > 0)
							{
								for (int addquad = 0; addquad < g_RefinedLargeQuadList.size(); addquad++)
								{
									g_LargeQuadList.push_back(g_RefinedLargeQuadList[addquad]);
								}
								g_RefinedLargeQuadList.clear();
							}
						}

						// create verts from largerquad list
						int iVertexIndex = 0;
						int iVertFloatIndex = 0;
						int iVertIndex[4];
						int iIndicesIndex = 0;
						int iQuadListsCount = g_LargeQuadList.size() + g_RefinedLargeQuadList.size();
						float* pNewVertData = new float[iQuadListsCount * 4 * 3]; // max size needed
						int iRefGridSize = ((iResolutionSIze + 2)*(iResolutionSIze + 2));
						int* pRefVertGrid = new int[iRefGridSize];
						memset(pRefVertGrid, 0, iRefGridSize*sizeof(int));
						DWORD* pNewTerrainIndexData = new DWORD[iQuadListsCount * 6];

						// first pass creates the unique and shared verts (and faces)
						for (int twoquadlists = 0; twoquadlists < 2; twoquadlists++)
						{
							int quadcount = 0;
							if (twoquadlists == 0) quadcount = g_LargeQuadList.size();
							if (twoquadlists == 1) quadcount = g_RefinedLargeQuadList.size();
							for (int quad = 0; quad < quadcount; quad++)
							{
								// for each quad, create vert positions
								for (int four = 0; four < 4; four++)
								{
									int iX, iZ;
									if (twoquadlists == 0)
									{
										if (four == 0) { iX = g_LargeQuadList[quad].iX1; iZ = g_LargeQuadList[quad].iZ1; }
										if (four == 1) { iX = g_LargeQuadList[quad].iX2; iZ = g_LargeQuadList[quad].iZ1; }
										if (four == 2) { iX = g_LargeQuadList[quad].iX1; iZ = g_LargeQuadList[quad].iZ2; }
										if (four == 3) { iX = g_LargeQuadList[quad].iX2; iZ = g_LargeQuadList[quad].iZ2; }
									}
									if (twoquadlists == 1)
									{
										if (four == 0) { iX = g_RefinedLargeQuadList[quad].iX1; iZ = g_RefinedLargeQuadList[quad].iZ1; }
										if (four == 1) { iX = g_RefinedLargeQuadList[quad].iX2; iZ = g_RefinedLargeQuadList[quad].iZ1; }
										if (four == 2) { iX = g_RefinedLargeQuadList[quad].iX1; iZ = g_RefinedLargeQuadList[quad].iZ2; }
										if (four == 3) { iX = g_RefinedLargeQuadList[quad].iX2; iZ = g_RefinedLargeQuadList[quad].iZ2; }
									}
									// ensure the correct locations for the corners are worked out
									int iCornerX = iX, iCornerZ = iZ;
									if (four == 0) { iCornerX += 0; iCornerZ += 0; }
									if (four == 1) { iCornerX += 1; iCornerZ += 0; }
									if (four == 2) { iCornerX += 0; iCornerZ += 1; }
									if (four == 3) { iCornerX += 1; iCornerZ += 1; }
									int iRefGridOffset = (iCornerZ*(iResolutionSIze + 2)) + iCornerX;
									if (pRefVertGrid[iRefGridOffset] == 0)
									{
										// create unique vertex at this point
										iVertIndex[four] = iVertexIndex;
										float fHeight = 0.0f;
										int iGetHeightX = (iRealX + iCornerX);
										int iGetHeightZ = (iRealZ + iCornerZ);
										int iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
										if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];
										pNewVertData[iVertFloatIndex + 0] = iCornerX * fOneTileSize;
										pNewVertData[iVertFloatIndex + 2] = iCornerZ * fOneTileSize;
										pNewVertData[iVertFloatIndex + 1] = fHeight;
										iVertFloatIndex += 3;
										pRefVertGrid[iRefGridOffset] = 1 + iVertexIndex;
										iVertexIndex++;
									}
									else
									{
										// can share a vertex
										iVertIndex[four] = pRefVertGrid[iRefGridOffset] - 1;
									}
								}

								// then make the polygoon (face)
								pNewTerrainIndexData[iIndicesIndex + 0] = iVertIndex[0];
								pNewTerrainIndexData[iIndicesIndex + 1] = iVertIndex[1];
								pNewTerrainIndexData[iIndicesIndex + 2] = iVertIndex[2];
								pNewTerrainIndexData[iIndicesIndex + 3] = iVertIndex[1];
								pNewTerrainIndexData[iIndicesIndex + 4] = iVertIndex[3];
								pNewTerrainIndexData[iIndicesIndex + 5] = iVertIndex[2];
								iIndicesIndex += 6;
							}
						}

						// prepare mesh object with required mesh allocation
						DWORD dwVertexCount = iVertexIndex;
						DWORD dwIndexCount = iIndicesIndex;

						if (ObjectExist(iTerrainPhyObj) == 1) DeleteObject (iTerrainPhyObj);
						MakeObjectPlane (iTerrainPhyObj, 1, 1);
						sObject* pObject = GetObjectData(iTerrainPhyObj);
						delete pObject->ppMeshList[0];
						sMesh* pMesh = new sMesh();
						
						//PE: Check if 32bit buffers works in physics. NO: BVH dont support 32bit, and crash. Must lower resolution.
						//SetupMeshFVFData (pMesh, GGFVF_XYZ, dwVertexCount, dwIndexCount, true);

						if (dwVertexCount > maxVertex)
							maxVertex = dwVertexCount;

						SetupMeshFVFData(pMesh, GGFVF_XYZ, dwVertexCount, dwIndexCount, false);

						pMesh->iPrimitiveType = GGPT_TRIANGLELIST;
						pMesh->iDrawVertexCount = pMesh->dwVertexCount;
						pMesh->iDrawPrimitives = dwIndexCount / 3;
						pObject->ppMeshList[0] = pMesh;
						pObject->pFrame->pMesh = pMesh;
						// create mesh vertx and indices
						float* pVertPtr = (float*)pMesh->pVertexData;
						for (int v = 0; v < iVertFloatIndex; v++)
						{
							*(pVertPtr) = pNewVertData[v];
							pVertPtr++;
						}
						WORD* pIndicePtr = (WORD*)pMesh->pIndices;
						for (int i = 0; i < iIndicesIndex; i++)
						{
							*(pIndicePtr) = pNewTerrainIndexData[i];
							pIndicePtr++;
						}
						// create a mesh from the terrain heights
						float fBottomLeftCorner = -fEditableSize / 2;
						float fShiftForGridAtX = iRealX * fOneTileSize;
						float fShiftForGridAtZ = iRealZ * fOneTileSize;
						PositionObject (iTerrainPhyObj, fBottomLeftCorner + fShiftForGridAtX, 0, fBottomLeftCorner + fShiftForGridAtZ);
						SetObjectArbitaryValue (iTerrainPhyObj, 12345);
						// submit terrain object to physics system
						ODESetMeshFilename(pTerrainBVHChunkFilename);
						ODECreateStaticTerrainMesh (iTerrainPhyObj);
						ODESetMeshFilename("");
						t.tphysicsterrainobjend = iTerrainPhyObj;
						HideObject(iTerrainPhyObj);
						iIterCount++;

						// free resources
						if (pRefVertGrid)
						{
							delete pRefVertGrid;
							pRefVertGrid = NULL;
						}
						if (pNewVertData)
						{
							delete pNewVertData;
							pNewVertData = NULL;
						}
						if (pNewTerrainIndexData)
						{
							delete pNewTerrainIndexData;
							pNewTerrainIndexData = NULL;
						}
					}
					else
					{
						// prepare mesh object with required mesh allocation
						if (ObjectExist(iTerrainPhyObj) == 1) DeleteObject (iTerrainPhyObj);
						MakeObjectPlane (iTerrainPhyObj, 1, 1);
						sObject* pObject = GetObjectData(iTerrainPhyObj);
						delete pObject->ppMeshList[0];
						sMesh* pMesh = new sMesh();
						DWORD dwVertexCount = ((iResolutionSIze + 1) * (iResolutionSIze + 1));
						DWORD dwIndexCount = ((iResolutionSIze) * (iResolutionSIze)) * 6;
						SetupMeshFVFData (pMesh, GGFVF_XYZ, dwVertexCount, dwIndexCount, false);
						pMesh->iPrimitiveType = GGPT_TRIANGLELIST;
						pMesh->iDrawVertexCount = pMesh->dwVertexCount;
						pMesh->iDrawPrimitives = dwIndexCount / 3;
						pObject->ppMeshList[0] = pMesh;
						pObject->pFrame->pMesh = pMesh;
						// scan height data from editable area of iResolutionSIze
						float* pVertPtr = (float*)pMesh->pVertexData;
						int iVertIndex = 0;
						int iRealX = (iGridAtX*iResolutionSIze);
						int iRealZ = (iGridAtZ*iResolutionSIze);
						for (int z = 0; z < iResolutionSIze + 1; z++)
						{
							int iGetHeightZ = (iRealZ + z);
							int iVertAtLeftside = iVertIndex;
							for (int x = 0; x < iResolutionSIze + 1; x++)
							{
								// get raw height
								int iGetHeightX = (iRealX + x);
								float fHeight = 0.0f;
								int iOffset = (iGetHeightZ*iFullResolutionSize) + iGetHeightX;
								if (iOffset < iHeightDataSize) fHeight = pfHeightData[iOffset];

								// write vert required at this XZ
								*(pVertPtr + 0) = x * fOneTileSize;
								*(pVertPtr + 1) = fHeight;
								*(pVertPtr + 2) = z * fOneTileSize;
								pVertPtr += 3;
								iVertIndex++;
							}
						}
						// now write indices to describe polygons connecting verts
						WORD* pIndicePtr = (WORD*)pMesh->pIndices;
						for (int z = 0; z < iResolutionSIze; z++)
						{
							for (int x = 0; x < iResolutionSIze; x++)
							{
								int iTL = (z*(iResolutionSIze + 1)) + (x + 0);
								int iTR = (z*(iResolutionSIze + 1)) + (x + 1);
								int iBL = ((z + 1)*(iResolutionSIze + 1)) + (x + 0);
								int iBR = ((z + 1)*(iResolutionSIze + 1)) + (x + 1);
								*(pIndicePtr + 0) = iTL;
								*(pIndicePtr + 1) = iTR;
								*(pIndicePtr + 2) = iBL;
								*(pIndicePtr + 3) = iTR;
								*(pIndicePtr + 4) = iBR;
								*(pIndicePtr + 5) = iBL;
								pIndicePtr += 6;
							}
						}
						// create a mesh from the terrain heights
						float fBottomLeftCorner = -fEditableSize / 2;
						float fShiftForGridAtX = iRealX * fOneTileSize;
						float fShiftForGridAtZ = iRealZ * fOneTileSize;
						PositionObject (iTerrainPhyObj, fBottomLeftCorner + fShiftForGridAtX, 0, fBottomLeftCorner + fShiftForGridAtZ);
						SetObjectArbitaryValue (iTerrainPhyObj, 12345);
						// submit terrain object to physics system
						ODESetMeshFilename(pTerrainBVHChunkFilename);
						ODECreateStaticTerrainMesh (iTerrainPhyObj);
						ODESetMeshFilename("");
						HideObject(iTerrainPhyObj);
						iIterCount++;
					}
				}
			}

			// clean up resources
			delete pfHeightData;
			pfHeightData = NULL;
		}
	}
	#else
	//g_pTerrain->CreatePhysicsMeshes();
	#endif
	timestampactivity(0, "terrian physics complete");
	#else
	// takes tgenerateterraindirtyregiononly (0-full/1-only refresh terrain.dirtyxy)
	// Which terrain collision style
	t.tphysicsterrainobjstart=t.terrain.objectstartindex+1000;
	if ( t.terrain.TerrainID>0 ) 
	{
		//  new physics command which creates the perfect 12+12 collision geometry
		timestampactivity(0,"get terrian sectors");
		t.TerrainID=t.terrain.TerrainID;
		t.terrain.TerrainLODOBJStart=t.tphysicsterrainobjstart;
		for ( t.i = 0 ; t.i <= (int)BT_GetSectorCount(t.TerrainID,0)-1; t.i++ )
		{
			if (  BT_GetSectorExcluded(t.TerrainID,0,t.i) == 0 ) 
			{
				t.sopx_f=BT_GetSectorPositionX(t.TerrainID,0,t.i);
				t.sopy_f=BT_GetSectorPositionY(t.TerrainID,0,t.i);
				t.sopz_f=BT_GetSectorPositionZ(t.TerrainID,0,t.i);
				t.tokay=0;
				if (  t.tgenerateterraindirtyregiononly == 0  )  t.tokay = 1;
				if (  t.tgenerateterraindirtyregiononly == 1 ) 
				{
					//  only re-create if within field of dirty area
					t.tsopradx=abs(t.terrain.dirtyx2-t.terrain.dirtyx1)*2;
					t.tsopradz=abs(t.terrain.dirtyz2-t.terrain.dirtyz1)*2;
					if (  t.sopx_f >= ((t.terrain.dirtyx1-t.tsopradx)*50)-1500 && t.sopx_f <= ((t.terrain.dirtyx2+t.tsopradx)*50)+1500 ) 
					{
						if (  t.sopz_f >= ((t.terrain.dirtyz1-t.tsopradz)*50)-1500 && t.sopz_f <= ((t.terrain.dirtyz2+t.tsopradz)*50)+1500 ) 
						{
							t.tokay=1;
						}
					}
				}
				if (  t.tokay == 1 ) 
				{
					if (  ObjectExist(t.tphysicsterrainobjstart+t.i) == 1  )  DeleteObject (  t.tphysicsterrainobjstart+t.i );
					BT_MakeSectorObject (  t.TerrainID,0,t.i,t.tphysicsterrainobjstart+t.i );
					PositionObject (  t.tphysicsterrainobjstart+t.i,t.sopx_f,t.sopy_f,t.sopz_f );
					SetObjectArbitaryValue (  t.tphysicsterrainobjstart+t.i,0 );
					BT_MakeSectorObject (  t.TerrainID,0,t.i,0 );
					ExcludeOn (  t.tphysicsterrainobjstart+t.i );
					SetObjectMask (  t.tphysicsterrainobjstart+t.i,0 );
					CloneMeshToNewFormat (  t.tphysicsterrainobjstart+t.i,0x002 );
					if (  t.tgenerateterraindirtyregiononly == 1  )  ODEDestroyObject (  t.tphysicsterrainobjstart+t.i );
					t.tterrainsegmentuntouched=1;
					for ( t.tscanzz = -1600 ; t.tscanzz <= 1600 ; t.tscanzz+= 100 )
					{
						for ( t.tscanxx = -1600 ; t.tscanxx <= 1600 ; t.tscanxx+= 100 )
						{
							t.h_f=BT_GetGroundHeight(t.terrain.TerrainID,t.sopx_f+t.tscanxx,t.sopz_f+t.tscanzz);
							if (  t.h_f != 600.0 && t.h_f != -600 ) 
							{
								t.tterrainsegmentuntouched=0;
							}
						}
					}
					if (  t.tterrainsegmentuntouched == 1 ) 
					{
						//  if NO terrain unulations, can use quicker Box (  shape )
						DeleteObject (  t.tphysicsterrainobjstart+t.i );
						MakeObjectPlane (  t.tphysicsterrainobjstart+t.i, 3200, 3200 );
						PositionObject (  t.tphysicsterrainobjstart+t.i,t.sopx_f,600.0,t.sopz_f );
						XRotateObject ( t.tphysicsterrainobjstart+t.i, 90 );
						SetObjectArbitaryValue (  t.tphysicsterrainobjstart+t.i,0 );
						ExcludeOn (  t.tphysicsterrainobjstart+t.i );
						SetObjectMask (  t.tphysicsterrainobjstart+t.i,0 );
						CloneMeshToNewFormat (  t.tphysicsterrainobjstart+t.i,0x002 );
						ODECreateStaticTerrainMesh (  t.tphysicsterrainobjstart+t.i );
					}
					else
					{
						ODECreateStaticTerrainMesh (  t.tphysicsterrainobjstart+t.i );
					}
					DeleteObject (  t.tphysicsterrainobjstart+t.i );
				}
			}
		}
		t.terrain.TerrainLODOBJFinish=(t.tphysicsterrainobjstart+t.i)-1;
		timestampactivity(0,"get terrian sectors complete");
	}
	else
	{
		// super flat terrain (one quad)
		if ( ObjectExist(t.tphysicsterrainobjstart) == 0 ) 
		{
			MakeObjectBox ( t.tphysicsterrainobjstart,512*100,100,512*100 );
			HideObject ( t.tphysicsterrainobjstart );
			PositionObject ( t.tphysicsterrainobjstart,256*100,g.gdefaultterrainheight-50,256*100 );
			SetObjectArbitaryValue ( t.tphysicsterrainobjstart,0 );
		}
		ODECreateStaticBox ( t.tphysicsterrainobjstart );
	}
	#endif
}

struct sVTreeObj
{
	int iID;
	float fX;
	float fY;
	float fZ;
	bool bActive;
};
std::vector<sVTreeObj> g_VTreeObj;

void physics_createvirtualtreecylinders (void)
{
	#ifdef WICKEDENGINE
	// going to use GGTrees_GetClosest to position kenetic cylinders so player cannot walk through virtual trees
	if (ObjectExist(g.virtualtreeobjectstart) == 0)
	{
		// create once for rest of software lifecycle (Wicked slows when create/delete objects)
		for (int iVT = 0; iVT < PHYSICS_VIRTUALTREE_MAX; iVT++)
		{
			// create cylinder for virtual tree
			int iPhyObjID = g.virtualtreeobjectstart + iVT;
			MakeObjectSphere(iPhyObjID, 9);
			HideObject(iPhyObjID);
			sObject* pObject = GetObjectData(iPhyObjID);
			if (pObject)
			{
				WickedCall_SetObjectCastShadows(pObject, false);
			}
		}
	}

	// start new Vtrees list
	g_VTreeObj.clear();
	#endif
}

void physics_freevirtualtreecylinders (void)
{
	// just hide, we will need them next time
	for (int iVT = 0; iVT < PHYSICS_VIRTUALTREE_MAX; iVT++)
	{
		// create cylinder for virtual tree
		int iPhyObjID = g.virtualtreeobjectstart + iVT;
		if (ObjectExist(iPhyObjID) == 1)
		{
			ODEDestroyObject(iPhyObjID);
			HideObject(iPhyObjID);
		}
	}
}

void physics_managevirtualtreecylinders (void)
{
	#ifdef WICKEDENGINE
	// first start assuming all existing visible trees may not show again
	for (int vti = 0; vti < g_VTreeObj.size(); vti++)
		g_VTreeObj[vti].bActive = false;

	// scan for virtual trees
	if (ggtrees_global_params.draw_enabled == 1)
	{
		float fRadiusOfScan = 200.0f;
		GGTrees::GGTreePoint* pOutPoints = NULL;
		int iTreeCount = GGTrees::GGTrees_GetClosest (CameraPositionX(0), CameraPositionZ(0), fRadiusOfScan, &pOutPoints);
		if (pOutPoints)
		{
			for (int n = 0; n < iTreeCount; n++)
			{
				// this virtual tree pos
				GGVECTOR3 vecTreePos = GGVECTOR3(pOutPoints[n].x, pOutPoints[n].y, pOutPoints[n].z);

				// see if tree in the visible list
				int vti = 0;
				for (; vti < g_VTreeObj.size(); vti++)
				{
					if (g_VTreeObj[vti].iID > 0 && g_VTreeObj[vti].fX == vecTreePos.x && g_VTreeObj[vti].fY == vecTreePos.y && g_VTreeObj[vti].fZ == vecTreePos.z)
						break;
				}
				if (vti < g_VTreeObj.size())
				{
					// yes we still know about this tree
					g_VTreeObj[vti].bActive = true;
				}
				else
				{
					// ah, new tree, add to list
					// first see if we have a free slot in exsiting list
					int vti = 0;
					for (; vti < g_VTreeObj.size(); vti++)
					{
						if (g_VTreeObj[vti].bActive == false)
							break;
					}
					if (vti < g_VTreeObj.size())
					{
						// use existing entry in list
					}
					else
					{
						// new list entry
						if (g_VTreeObj.size() < PHYSICS_VIRTUALTREE_MAX)
						{
							sVTreeObj newtree;
							g_VTreeObj.push_back(newtree);
							vti = g_VTreeObj.size() - 1;
						}
						else
						{
							// no room to add to visible list
							vti = -1;
						}
					}
					// entry details
					if (vti != -1)
					{
						int iPhyObjID = g.virtualtreeobjectstart + vti;
						g_VTreeObj[vti].iID = iPhyObjID;
						g_VTreeObj[vti].fX = vecTreePos.x;
						g_VTreeObj[vti].fY = vecTreePos.y;
						g_VTreeObj[vti].fZ = vecTreePos.z;
						g_VTreeObj[vti].bActive = true;

						// delete any old one
						ODEDestroyObject(iPhyObjID);

						// position dynamically
						PositionObject(iPhyObjID, vecTreePos.x, vecTreePos.y, vecTreePos.z);

						// ShowObject(iPhyObjID); //Debug only.
						HideObject(iPhyObjID);

						// now create our physics object afresh
						SetObjectArbitaryValue (iPhyObjID, 6);
						ODECreateStaticCylinder (iPhyObjID, vecTreePos.x, vecTreePos.y, vecTreePos.z, 10, 500, 10, 0, 0, 0);
					}
				}
			}
			delete pOutPoints;
		}
		// finally remove any from list which have become inactive
		for (int vti = 0; vti < g_VTreeObj.size(); vti++)
		{
			if (g_VTreeObj[vti].bActive == false)
			{
				if (g_VTreeObj[vti].iID > 0)
				{
					int iPhyObjID = g.virtualtreeobjectstart + vti;
					if (ObjectExist(iPhyObjID) == 1)
					{
						ODEDestroyObject(iPhyObjID);
						HideObject(iPhyObjID);
					}
					g_VTreeObj[vti].iID = 0;
				}
			}
		}
	}
	#endif
}

void physics_prepareentityforphysics ( void )
{
	//  takes E and ENTID
	t.tphyobj=t.entityelement[t.e].obj;
	if (  t.entid>0 && t.tphyobj>0 ) 
	{
		// Entity has a different collision mode to the parent object in the FPE file...
		#ifdef WICKEDENGINE
		int iStoreEntityIndex = t.entid;
		int iStoreOriginalCollisionMode = t.entityprofile[iStoreEntityIndex].collisionmode;
		if (t.e < t.entityelement.size() && t.entityelement[t.e].eleprof.iOverrideCollisionMode != -1)
		{
			t.entityprofile[t.entid].collisionmode = t.entityelement[t.e].eleprof.iOverrideCollisionMode;
		}
		#endif

		// now use collisionindex
		t.tnophysics=0;
		if (  t.entityprofile[t.entid].ismarker != 0  )  t.tnophysics = 1;
		if (  t.entityprofile[t.entid].collisionmode == 11  )  t.tnophysics = 1;
		if (  t.entityprofile[t.entid].collisionmode == 12  )  t.tnophysics = 1;
		if (  t.entityelement[t.e].eleprof.physics == 0  )  t.tnophysics = 1;
		if (  t.entityelement[t.e].eleprof.physics == 2  )  t.tnophysics = 1;
		if (  t.entityprofile[t.entid].isammo == 1  )  t.tnophysics = 1;
		if (  Len(t.entityprofile[t.entid].isweapon_s.Get())>1  )  t.tnophysics = 1;
		if (  t.tnophysics == 1 ) 
		{
			//  no physics
		}
		else
		{
			if (  t.entityprofile[t.entid].isebe != 0 ) 
			{
				// EBE structure from cubes
				physics_setupebestructure ( );
				t.entityelement[t.e].usingphysicsnow=1;
			}
			else
			{
				if ( t.entityprofile[t.entid].ischaracter == 1 && t.entityelement[t.e].eleprof.isimmobile == 0 )
				{
					//  physics objects belong to Ghost AI Objects (set outside of this function, i.e. LUA-Entity.cpp)
				}
				else
				{
					// static or dynamic
					t.tstatic=t.entityelement[t.e].staticflag;
					if (  t.entityelement[t.e].eleprof.isimmobile == 1  )  t.tstatic = 1;

					// physics object is faux-character capsule (zombies, custom characters)
					if ( t.entityprofile[t.entid].collisionmode == 21 || t.entityprofile[t.entid].collisionmode == 22 )
					{
						// create capsule (to be controlled as entity-driven, i.e. MoveForward)
						physics_setupcharacter ( );
					}
					else
					{
						// create solid entities
						if (t.entityprofile[t.entid].collisionmode == 1)
						{
							t.tshape = 2;
						}
						else if (t.entityprofile[t.entid].collisionmode == 8)
						{
							t.tshape = 8; // new version of '2' but detects for OBJ
						}
						else if (t.entityprofile[t.entid].collisionmode == 9)
						{
							t.tshape = 9;
						}
						else if (t.entityprofile[t.entid].collisionmode == 10)
						{
							t.tshape = 10;
						}
						else if (t.entityprofile[t.entid].collisionmode == 2)
						{
							t.tshape = 6;
						}
						else if (t.entityprofile[t.entid].collisionmode == 3)
						{
							t.tshape = 7;
						}
						else
						{
							if (  t.entityprofile[t.entid].collisionmode >= 1000 ) 
							{
								t.tshape=t.entityprofile[t.entid].collisionmode;
							}
							else
							{
								if (  t.entityprofile[t.entid].collisionmode >= 50 && t.entityprofile[t.entid].collisionmode<60 ) 
								{
									t.tshape=3;
									t.tstatic=1;
								}
								else
								{
									t.tshape=1;
								}
							}
						}
						//  check if it has a list of physics objects from the importer, collisionmode 40
						if (  t.entityprofile[t.entid].physicsobjectcount > 0 && t.entityprofile[t.entid].collisionmode  ==  40 ) 
						{
							t.tshape = 4;
						}
						t.tweight=t.entityelement[t.e].eleprof.phyweight;
						t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
						t.tcollisionscaling=t.entityprofile[t.entid].collisionscaling;
						physics_setupobject ( );
						t.entityelement[t.e].usingphysicsnow=1;
					}
				}
			}
		}

		// ...and restore parent when done
		#ifdef WICKEDENGINE
		t.entityprofile[iStoreEntityIndex].collisionmode = iStoreOriginalCollisionMode;
		#endif
	}
}

void physics_setupplayernoreset ( void )
{
	//  create character controller for player
	PositionObject (  t.aisystem.objectstartindex,t.terrain.playerx_f,t.terrain.playery_f,t.terrain.playerz_f );
	if ( t.freezeplayerposonly==0 ) RotateObject (  t.aisystem.objectstartindex,t.terrain.playerax_f,t.terrain.playeray_f,t.terrain.playeraz_f );
	SetObjectArbitaryValue (  t.aisystem.objectstartindex,6 );
	ODECreateDynamicCharacterController (  t.aisystem.objectstartindex,t.playercontrol.gravity_f,t.playercontrol.fallspeed_f,t.playercontrol.climbangle_f );
}

void physics_setupplayer ( void )
{
	physics_setupplayernoreset ( );
	if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
	{
		if ( t.freezeplayerposonly==0 ) RotateCamera (  0,t.terrain.playerax_f,t.terrain.playeray_f,t.terrain.playeraz_f );
	}
}

void physics_disableplayer ( void )
{
	ODEDestroyObject (  t.aisystem.objectstartindex );
}

void physics_setupcharacter ( void )
{
	// only if physics is active for this character
	if (t.entityprofile[t.entityelement[t.e].bankindex].physics != 0)
	{
		// create physics for this character/faux-character object
		SetObjectArbitaryValue (t.tphyobj, 6);
		if (t.entityelement[t.e].eleprof.isimmobile == 0)
		{
			// 190718 - remove t.terrain.adjaboveground_f from enemy terrain relative positioning
			// ensure CHARACTER do not spawn UNDER the terrain
			#ifdef WICKEDENGINE
			t.tgroundheight_f = BT_GetGroundHeight(t.terrain.TerrainID, ObjectPositionX(t.tphyobj), ObjectPositionZ(t.tphyobj));
			#else
			if (t.terrain.TerrainID > 0)
			{
				t.tgroundheight_f = BT_GetGroundHeight(t.terrain.TerrainID, ObjectPositionX(t.tphyobj), ObjectPositionZ(t.tphyobj));
			}
			else
			{
				//t.tgroundheight_f=1000.0+t.terrain.adjaboveground_f;
				t.tgroundheight_f = g.gdefaultterrainheight;
			}
			#endif
			t.tgroundheight_f = t.tgroundheight_f + 2.5;
			// 291116 - account for object vecCenter (so characters with Y=0=Floor are not unjustly raised)
			// NOW DONE EARLIER SO NO NEED TO ADJUST FOR COL CENTER TWICE (see calling function)
			//float fAccountForVecCenter = GetObjectCollisionCenterY ( t.tphyobj );
			if (ObjectPositionY(t.tphyobj) <= t.tgroundheight_f)
			{
				PositionObject (t.tphyobj, ObjectPositionX(t.tphyobj), t.tgroundheight_f, ObjectPositionZ(t.tphyobj));
			}
			else
			{
				PositionObject (t.tphyobj, ObjectPositionX(t.tphyobj), ObjectPositionY(t.tphyobj), ObjectPositionZ(t.tphyobj));
			}
			t.tfinalscale_f = g.gcharactercapsulescale_f * ((t.entityprofile[t.entityelement[t.e].bankindex].scale + 0.0f) / 100.0f);
			float fWeight = t.entityelement[t.e].eleprof.phyweight;
			float fFriction = t.entityelement[t.e].eleprof.phyfriction;
			ODECreateDynamicCapsule (t.tphyobj, t.tfinalscale_f, 0.0, fWeight, fFriction, -1);
		}
		else
		{
			// 290515 - fixes scifi DLC characters floating (ISIMMOBILE=1) PositionObject ( t.tphyobj,ObjectPositionX(t.tphyobj),ObjectPositionY(t.tphyobj)+(ObjectSizeY(t.tphyobj,1)/2),ObjectPositionZ(t.tphyobj) );
			PositionObject (t.tphyobj, ObjectPositionX(t.tphyobj), ObjectPositionY(t.tphyobj), ObjectPositionZ(t.tphyobj));
			ODECreateStaticCapsule (t.tphyobj);
		}
		t.entityelement[t.e].usingphysicsnow = 1;
	}
	else
	{
		t.entityelement[t.e].usingphysicsnow = 0;
	}
}

void physics_setupebestructure ( void )
{
	//  create EBE physics for this object. Takes tphyobj and entid and e
	if ( t.tphyobj>0 ) 
	{
		if ( ObjectExist(t.tphyobj) == 1 ) 
		{
			ebe_physics_setupebestructure ( t.tphyobj, t.e );
		}
	}
}

void physics_setupobject ( void )
{
	// default is no special handling of OBJ collision meshes
	ODESetOBJLoadingFilename("");

	// create physics for this object. Takes tphyobj and entid and e
	if ( t.tphyobj>0 ) 
	{
		if ( ObjectExist(t.tphyobj) == 1 ) 
		{
			// hull decomp take time - offer ability to save and reload
			if (t.tshape == 10)
			{
				char pNoFPE[MAX_PATH];
				strcpy(pNoFPE, t.entitybank_s[t.entid].Get());
				pNoFPE[strlen(pNoFPE) - 4] = 0;
				char pObjectFilename[MAX_PATH];
				sprintf(pObjectFilename, "%s\\Files\\entitybank\\%s.bullet", g.fpscrootdir_s.Get(), pNoFPE);
				ODESetMeshFilename(pObjectFilename);
				GG_GetRealPath(pObjectFilename, 0);
				if (FileExist(pObjectFilename) == 0)
				{
					// use only the name
					char pJustName[MAX_PATH];
					strcpy(pJustName, "");
					for (int n = strlen(pNoFPE) - 1; n > 0; n--)
					{
						if (pNoFPE[n] == '/' || pNoFPE[n] == '\\')
						{
							sprintf(pJustName, " - %s", pNoFPE + n + 1);
							break;
						}
					}
					char pHullDecompWait[256];
					sprintf(pHullDecompWait, "HULL DECOMPOSITION%s", pJustName);
					t.screenprompt_s = pHullDecompWait;
					#ifdef WICKEDENGINE
					extern DWORD g_SensibleMessageTimer;
					g_SensibleMessageTimer = 1;
					printscreenprompt(t.screenprompt_s.Get());
					#endif
				}
			}

			extern bool physics_playground;
			SetObjectArbitaryValue (  t.tphyobj,t.entityprofile[t.entid].materialindex );
			#ifdef VRTECH
			if ( t.tstatic == 1 ) // now allow physics entities in multiplayer || t.game.runasmultiplayer == 1 ) 
			#else
			if ( t.tstatic == 1 || t.game.runasmultiplayer == 1 ) 
			#endif
			{
				// if static, need to ensure FIXNEWY pivot is respected
				if ( t.tstatic == 1 ) 
				{
					t.tstaticfixnewystore_f=ObjectAngleY(t.tphyobj);
					RotateObject (  t.tphyobj,ObjectAngleX(t.tphyobj),ObjectAngleY(t.tphyobj)+t.entityprofile[t.entid].fixnewy,ObjectAngleZ(t.tphyobj) );
				}

				// if special polygon collision mode (that uses OBJ) handle now)
				if (t.tshape == 8)
				{
					// set the FPE filename so the .OBJ or _COL.obj can be checked and used if present
					char pFullFPEFilename[MAX_PATH];
					sprintf(pFullFPEFilename, "%s\\Files\\entitybank\\%s", g.fpscrootdir_s.Get(), t.entitybank_s[t.entid].Get());
					ODESetOBJLoadingFilename(pFullFPEFilename);

					// and now treat as regular polygon collision mesh
					t.tshape = 2; 
				}

				//  create the physics now
				if (physics_playground)
				{
					t.entityelement[t.e].staticflag = 0;
					ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 1); //Turn everything into convex hull.
				}
				else if (t.tshape >= 1000 && t.tshape < 2000)
				{
					ODECreateStaticBox (  t.tphyobj,t.tshape-1000 );
				}
				else if ( t.tshape >= 2000 && t.tshape < 3000 ) 
				{
					ODECreateStaticTriangleMesh (  t.tphyobj,t.tshape-2000 );
				}
				else if ( t.tshape == 1 ) 
				{
					ODECreateStaticBox ( t.tphyobj );
				}
				else if (t.tshape == 6)
				{
					ODECreateStaticSphere ( t.tphyobj );
				}
				else if (t.tshape == 7)
				{
					ODECreateStaticCylinder ( t.tphyobj );
				}
				else if ( t.tshape == 2 || t.tshape == 9 || t.tshape == 10)
				{
					if (  t.tshape == 2 ) 
					{
						if (  t.tcollisionscaling != 100 ) 
						{
							ODECreateStaticTriangleMesh (  t.tphyobj,-1,t.tcollisionscaling );
						}
						else
						{
							ODECreateStaticTriangleMesh (  t.tphyobj );
						}
					}
					else
					{
						if (t.tshape == 10)
						{
							ODECreateStaticTriangleMesh (t.tphyobj, -1, t.tcollisionscaling, 2);
						}
						else
						{
							ODECreateStaticTriangleMesh (t.tphyobj, -1, t.tcollisionscaling, 1);
						}
					}
				}
				else if ( t.tshape == 3 ) 
				{
					physics_setuptreecylinder ( );
				}
				// tshape 4 is a list of physics objects from the importer
				else if ( t.tshape == 4 ) 
				{
					physics_setupimportershapes ( );
				}
				// if static, restore object before leaving
				if ( t.tstatic == 1 ) 
				{
					RotateObject (  t.tphyobj,ObjectAngleX(t.tphyobj),t.tstaticfixnewystore_f,ObjectAngleZ(t.tphyobj) );
				}
			}
			else
			{
				// objects will fall through Floor (  if they are perfectly sitting on it )
				PositionObject ( t.tphyobj, ObjectPositionX(t.tphyobj), ObjectPositionY(t.tphyobj)+0.1, ObjectPositionZ(t.tphyobj) );

				if(physics_playground)
				{
					ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 1);
				}
				else
				if ( t.tshape == 6 )     
				{
					// Sphere
					ODECreateDynamicSphere(t.tphyobj, t.tweight, t.tfriction, 0.01f);
				}
				else if ( t.tshape == 7 ) 
				{
					// Cylinder
					ODECreateDynamicCylinder(t.tphyobj, t.tweight, t.tfriction, 0.01f);
				}
				else if (t.tshape == 9)
				{
					// Dynamic convex hull
					ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 1);
				}
				else if (t.tshape == 10)
				{
					// Dynamic hull decomp
					ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 2);
				}
				else
				{
					// box
					ODECreateDynamicBox(t.tphyobj, -1, 0, t.tweight, t.tfriction, -1);
				}

				#ifdef WICKEDENGINE
				// apply zero gravity if ticked
				if (t.entityelement[t.e].eleprof.iAffectedByGravity == 0)
				{
					ODESetNoGravity(t.tphyobj, 0);
				}
				#endif
			}
		}
	}
	ODESetMeshFilename("");
	ODESetOBJLoadingFilename("");
}

int physics_findfreegamerealtimeobj ( void )
{
	int iFreeObj = g.gamerealtimeobjoffset;
	while ( ObjectExist ( iFreeObj ) == 1 && iFreeObj < g.gamerealtimeobjoffsetmax )
	{
		iFreeObj++;
	}
	if ( iFreeObj == g.gamerealtimeobjoffsetmax )
	{
		if ( ObjectExist ( iFreeObj ) == 1 ) DeleteObject ( iFreeObj );
	}
	return iFreeObj;
}

void physics_freeallgamerealtimeobjs ( void )
{
	for ( int iObj = g.gamerealtimeobjoffset; iObj <= g.gamerealtimeobjoffsetmax; iObj++ )
		if ( ObjectExist ( iObj ) == 1 ) 
			DeleteObject ( iObj );
}

void physics_setupimportershapes ( void )
{
	// flag to control if debug collision boxes should be left
	bool bLeaveDebugCollisionBoxes = false;
	if ( g.globals.showdebugcollisonboxes == 1 ) bLeaveDebugCollisionBoxes = true;

	// get collision boxes data from entity to make importer collision shapes
	if ( t.entid > MAX_ENTITY_PHYSICS_BOXES*2  ) 
	{
		Dim2 ( t.entityphysicsbox , t.entid , MAX_ENTITY_PHYSICS_BOXES   );
	}
	ODEStartStaticObject (  t.tphyobj );
	float fMoveToObjectWorldX = ObjectPositionX ( t.tphyobj );
	float fMoveToObjectWorldY = ObjectPositionY ( t.tphyobj );
	float fMoveToObjectWorldZ = ObjectPositionZ ( t.tphyobj );
	for ( t.tcount = 0 ; t.tcount <= t.entityprofile[t.entid].physicsobjectcount-1; t.tcount++ )
	{
		if (  ObjectExist(g.tempimporterlistobject)  )  DeleteObject (  g.tempimporterlistobject );
		int iObjectToUse = g.tempimporterlistobject;
		if ( bLeaveDebugCollisionBoxes == true ) iObjectToUse = physics_findfreegamerealtimeobj();
		t.tescale=t.entityprofile[t.entid].scale;
		if (  t.tescale>0 ) 
		{
			t.tnewscalex_f=t.tescale+t.entityelement[t.e].scalex;
			t.tnewscaley_f=t.tescale+t.entityelement[t.e].scaley;
			t.tnewscalez_f=t.tescale+t.entityelement[t.e].scalez;
		}
		else
		{
			t.tnewscalex_f=100+t.entityelement[t.e].scalex;
			t.tnewscaley_f=100+t.entityelement[t.e].scaley;
			t.tnewscalez_f=100+t.entityelement[t.e].scalez;
		}
		t.tnewsizex_f=(t.tnewscalex_f/100.0)*(t.entityphysicsbox[t.entid][t.tcount].SizeX);
		t.tnewsizey_f=(t.tnewscaley_f/100.0)*(t.entityphysicsbox[t.entid][t.tcount].SizeY);
		t.tnewsizez_f=(t.tnewscalez_f/100.0)*(t.entityphysicsbox[t.entid][t.tcount].SizeZ);
		MakeObjectBox ( iObjectToUse, t.tnewsizex_f, t.tnewsizey_f, t.tnewsizez_f );
		float tNewOffX = (t.tnewscalex_f/100.0) * t.entityphysicsbox[t.entid][t.tcount].OffX;
		float tNewOffY = (t.tnewscaley_f/100.0) * t.entityphysicsbox[t.entid][t.tcount].OffY;
		float tNewOffZ = (t.tnewscalez_f/100.0) * t.entityphysicsbox[t.entid][t.tcount].OffZ;
		t.tocy_f=ObjectSizeY(t.tphyobj,1)/2.0;
		PositionObject ( iObjectToUse, tNewOffX, t.tocy_f + tNewOffY, tNewOffZ );
		RotateObject ( iObjectToUse, t.entityphysicsbox[t.entid][t.tcount].RotX , t.entityphysicsbox[t.entid][t.tcount].RotY , t.entityphysicsbox[t.entid][t.tcount].RotZ );
		//SetObjectArbitaryValue ( iObjectToUse,t.entityprofile[t.entid].materialindex );
		ODEAddStaticObjectBox ( t.tphyobj, iObjectToUse, t.entityprofile[t.entid].materialindex );
		if ( bLeaveDebugCollisionBoxes == true ) 
		{
			FixObjectPivot ( iObjectToUse );
			RotateObject ( iObjectToUse, t.entityelement[t.e].rx, t.entityelement[t.e].ry, t.entityelement[t.e].rz );
			GGVECTOR3 vecOffset = GGVECTOR3 ( tNewOffX, t.tocy_f + tNewOffY, tNewOffZ );
			sObject* pObjectPtr = GetObjectData ( t.tphyobj );
			GGVec3TransformCoord ( &vecOffset, &vecOffset, &pObjectPtr->position.matRotation );
			PositionObject ( iObjectToUse, fMoveToObjectWorldX+vecOffset.x, fMoveToObjectWorldY+vecOffset.y, fMoveToObjectWorldZ+vecOffset.z );
		}
	}
	if ( ObjectExist(g.tempimporterlistobject)  )  DeleteObject (  g.tempimporterlistobject );
	ODEEndStaticObject (  t.tphyobj, 0 );
}

void physics_setuptreecylinder ( void )
{
	//  takes; tphyobj and entid and sets up a tree cylinder
	if (  t.tphyobj < 1  )  return;
	if (  ObjectExist(t.tphyobj)  ==  0  )  return;
	if (  t.entid < 1  )  return;

	//  Tree height (adjusted for scale)
	t.tSizeY_f = ObjectSizeY(t.tphyobj,1);

	//  if have ABS position from AI OBSTACLE calc, use that instead
	if (  t.entityelement[t.e].abscolx_f != -1 ) 
	{
		t.tFinalX_f = t.entityelement[t.e].abscolx_f;
		t.tFinalZ_f = t.entityelement[t.e].abscolz_f;
	}
	else
	{
		t.tFinalX_f = ObjectPositionX(t.tphyobj);
		t.tFinalZ_f = ObjectPositionZ(t.tphyobj);
	}
	t.tFinalY_f = ObjectPositionY(t.tphyobj) + (t.tSizeY_f/2.0);

	//  if have ABS radius from AI OBSTACLE calc, use that instead
	if (  t.entityelement[t.e].abscolradius_f != -1 ) 
	{
		t.tSizeX_f = t.entityelement[t.e].abscolradius_f;
		t.tSizeZ_f = t.entityelement[t.e].abscolradius_f;
	}
	else
	{
		t.tSizeX_f = 20;
		t.tSizeZ_f = 20;
	}

	//  increase size by 25%
	t.tSizeX_f=t.tSizeX_f*1.25;
	t.tSizeZ_f=t.tSizeZ_f*1.25;

	//  now create our physics object
	SetObjectArbitaryValue (  t.tphyobj,6 );
	ODECreateStaticCylinder (  t.tphyobj,t.tFinalX_f,t.tFinalY_f,t.tFinalZ_f,t.tSizeX_f,t.tSizeY_f,t.tSizeZ_f,0,0,0 );
}

void physics_disableobject ( void )
{
	ODEDestroyObject (  t.tphyobj );
}

void physics_beginsimulation ( void )
{
	//t.ptimer=PerformanceTimer() ; t.pfreq=PerformanceFrequency()/1000 ; t.ptimer=t.ptimer/t.pfreq;
	//t.machineindependentphysicsupdate=t.ptimer;
	t.machineindependentphysicsupdate = timeGetSecond();
}

void physics_pausephysics ( void )
{
	//t.pfreq1=PerformanceFrequency()/1000;
	//t.ptimer1=PerformanceTimer() ; t.ptimer1=t.ptimer1/t.pfreq1;
	t.ptimer1 = timeGetSecond();
}

void physics_resumephysics ( void )
{
	//t.ptimer2=PerformanceTimer() ; t.ptimer2=t.ptimer2/t.pfreq1;
	t.ptimer2 = timeGetSecond();
	t.machineindependentphysicsupdate=t.machineindependentphysicsupdate+(t.ptimer2-t.ptimer1);
}

#ifdef WICKEDENGINE
#include "..\..\..\WICKEDREPO\WickedEngine\wiProfiler.h"
#endif
void physics_loop ( void )
{
	#ifdef WICKEDENGINE
	auto range = wiProfiler::BeginRangeCPU("Max - Physics (All)");
	#endif

	#ifdef WICKEDENGINE
	// shuffle virtual trees about as the player needs
	physics_managevirtualtreecylinders();
	#endif

	// Player control
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_player");
	auto range2 = wiProfiler::BeginRangeCPU("Max - Physics (Player)");
	physics_player ( );
	wiProfiler::EndRange(range2);

	//  Update physics system
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling timeGetSecond");
	t.tphysicsadvance_f = timeGetSecond() - t.machineindependentphysicsupdate;
	if (  t.tphysicsadvance_f >= (1.0/120.0) ) 
	{
		//  only process physics once we reach the minimum substep constant
		if ( t.tphysicsadvance_f>0.05f ) t.tphysicsadvance_f = 0.05f;
		t.machineindependentphysicsupdate = timeGetSecond();
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling ODEUpdate");
		range2 = wiProfiler::BeginRangeCPU("Max - Physics (Update)");
		ODEUpdate ( t.tphysicsadvance_f );
		wiProfiler::EndRange(range2);
	}
	#ifdef WICKEDENGINE
	if (BPhys_GetDebugDrawerMode() != 0)
	{
		range2 = wiProfiler::BeginRangeCPU("Max - Physics (Debug)");
		physics_render_debug_meshes();
		wiProfiler::EndRange(range2);
	}
	wiProfiler::EndRange(range);
	#endif
}

void physics_free ( void )
{
	#ifdef WICKEDENGINE
	// just hide the virtual tree cylinders until we use them again
	physics_freevirtualtreecylinders();
	#endif

	// remove any game realtime objects (used for debugging collision boxes, possible LUA spawned 3D objects, etc)
	physics_freeallgamerealtimeobjs();

	// free terrain physics object
	if ( t.terrain.superflat == 0 ) 
	{
		for ( t.tobj = t.terrain.TerrainLODOBJStart ; t.tobj <= t.terrain.TerrainLODOBJFinish; t.tobj++ )
		{
			if (  ObjectExist(t.tobj) == 1 ) 
			{
				ODEDestroyObject (  t.tobj );
			}
		}
	}
	else
	{
		#ifdef WICKEDENGINE
		//PE: Need to free all terrain phy objects, 7000+ , remember we keep the pMem1,pMem2 memory with data that need to be freed.
		if (ObjectExist(t.tphysicsterrainobjstart) == 1)
		{
			if (t.tphysicsterrainobjend > t.tphysicsterrainobjstart)
			{
				for (int obj = t.tphysicsterrainobjstart; obj <= t.tphysicsterrainobjend; obj++)
				{
					ODEDestroyObject(obj);
					DeleteObject(obj);
				}
			}
			else
			{
				ODEDestroyObject(t.tphysicsterrainobjstart);
				DeleteObject(t.tphysicsterrainobjstart);
			}
		}
		#else
		if ( ObjectExist(t.tphysicsterrainobjstart) == 1 ) 
		{
			ODEDestroyObject (  t.tphysicsterrainobjstart );
			DeleteObject (  t.tphysicsterrainobjstart );
		}
		#endif
	}

	//  detatch entity from physics
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.obj=t.entityelement[t.e].obj;
		if ( t.obj>0 ) 
		{
			if ( ObjectExist(t.obj) == 1 ) 
			{
				if ( t.entityelement[t.e].usingphysicsnow == 1 ) 
				{
					t.tphyobj=t.obj  ; physics_disableobject ( );
					t.entityelement[t.e].usingphysicsnow=0;
				}
			}
		}
	}


	//  Clean-up physics system
	#ifdef WICKEDENGINE
	extern int g_iDevToolsOpen;
	if (g_iDevToolsOpen != 0)
	{
		physics_set_debug_draw(0);
	}
	#endif

	ODEEnd(); g.gphysicssessionactive=0;
}

void physics_explodesphere ( void )
{
	//  takes texplodex#,texplodey#,texplodez#,texploderadius#
	t.tstrengthofexplosion_f = t.tDamage_f;

	//  detect if player within radius and apply damage
	t.tdx_f=ObjectPositionX(t.aisystem.objectstartindex)-t.texplodex_f;
	t.tdy_f=ObjectPositionY(t.aisystem.objectstartindex)-t.texplodey_f;
	t.tdz_f=ObjectPositionZ(t.aisystem.objectstartindex)-t.texplodez_f;
	t.tdd_f=Sqrt(abs(t.tdx_f*t.tdx_f)+abs(t.tdy_f*t.tdy_f)+abs(t.tdz_f*t.tdz_f));
	if (  t.tdd_f<t.texploderadius_f ) 
	{
		//  apply camera shake for nearby explosion
		t.playercontrol.camerashake_f=(((t.texploderadius_f*2)-(t.tdd_f/(t.texploderadius_f*2)))*t.tstrengthofexplosion_f)/150.0/20.0;
		#ifndef WICKEDENGINE
		if (  t.playercontrol.camerashake_f > 25.0f  )  t.playercontrol.camerashake_f = 25.0f;
		#endif
	}
	if (  t.tdd_f<t.texploderadius_f ) 
	{
		// apply damage
		t.te=-1;
		t.tdamage = (1.0f - (t.tdd_f / t.texploderadius_f)) * t.tstrengthofexplosion_f;
		if ( t.tdamage > t.tstrengthofexplosion_f ) t.tdamage = t.tstrengthofexplosion_f;
		if ( t.game.runasmultiplayer == 1 ) 
		{
			t.tsteamwasnetworkdamage = 0;
			if ( t.entityelement[t.texplodesourceEntity].mp_networkkill == 1 ) 
			{
				// 13032015 0XX - Team Multiplayer
				if ( g.mp.team  ==  0 || g.mp.friendlyfireoff  ==  0 || t.mp_team[t.entityelement[t.texplodesourceEntity].mp_killedby]  !=  t.mp_team[g.mp.me] ) 
				{
					t.tsteamwasnetworkdamage = 1;
				}
			}
		}
		if ( t.game.runasmultiplayer  ==  1 ) 
		{
			// 13032015 0XX - Team Multiplayer
			// Can't kill yourself if friendly fire is off
			if ( t.tsteamwasnetworkdamage  ==  1 || g.mp.friendlyfireoff  ==  0 || g.mp.damageWasFromAI  ==  1 ) 
			{
				physics_player_takedamage ( );
			}
		}
		else
		{
			physics_player_takedamage ( );
		}
		if ( t.game.runasmultiplayer == 1 ) t.tsteamwasnetworkdamage = 0;

		//  apply force to push player
		t.playercontrol.pushangle_f = atan2deg(t.tdx_f,t.tdz_f);
		float fForceRelativeToDamage = t.tstrengthofexplosion_f / 30.0f;
		t.playercontrol.pushforce_f = (1.0f-(t.tdd_f/t.texploderadius_f))*fForceRelativeToDamage;
	}
	//  if the explosion was caused by another player, we let them handle it rather than us
	if (  t.game.runasmultiplayer  ==  1 ) 
	{
		if (  t.entityelement[t.texplodesourceEntity].mp_networkkill  ==  1 && g.mp.damageWasFromAI  ==  0 ) 
		{
			return;
		}
	}
	//  create a sphere of force at this location
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  g.mp.damageWasFromAI  ==  0 ) 
		{
			if (  t.texplodesourceEntity > 0 && t.e == t.texplodesourceEntity  )  t.entid = 0;
		}
		if (  t.entid > 0 && t.entityelement[t.e].obj > 0 ) 
		{
			// 220618 - use center of object, not coordinate of entity XYZ
			float fCenterOfEntityX = ObjectPositionX(t.entityelement[t.e].obj) + GetObjectCollisionCenterX(t.entityelement[t.e].obj);
			float fCenterOfEntityY = ObjectPositionY(t.entityelement[t.e].obj) + GetObjectCollisionCenterY(t.entityelement[t.e].obj);
			float fCenterOfEntityZ = ObjectPositionZ(t.entityelement[t.e].obj) + GetObjectCollisionCenterZ(t.entityelement[t.e].obj);
			t.tdx_f = fCenterOfEntityX - t.texplodex_f;
			t.tdy_f = fCenterOfEntityY - t.texplodey_f;
			t.tdz_f = fCenterOfEntityZ - t.texplodez_f;
			t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f)+abs(t.tdy_f*t.tdy_f)+abs(t.tdz_f*t.tdz_f));

			if ( t.tdd_f<t.texploderadius_f ) 
			{
				// 220618 - before apply actual entity damage/effect, ensure a line of sight exists (could be behind wall/door)
				float fRayDestFromExplosionX = fCenterOfEntityX - t.texplodex_f;
				float fRayDestFromExplosionY = fCenterOfEntityY - t.texplodey_f;
				float fRayDestFromExplosionZ = fCenterOfEntityZ - t.texplodez_f;
				fRayDestFromExplosionX /= t.tdd_f;
				fRayDestFromExplosionY /= t.tdd_f;
				fRayDestFromExplosionZ /= t.tdd_f;
				fRayDestFromExplosionX *= t.texploderadius_f;
				fRayDestFromExplosionY *= t.texploderadius_f;
				fRayDestFromExplosionZ *= t.texploderadius_f;
				fRayDestFromExplosionX += t.texplodex_f;
				fRayDestFromExplosionY += t.texplodey_f;
				fRayDestFromExplosionZ += t.texplodez_f;

				#ifndef WICKEDENGINE
				if (g.lightmappedobjectoffset >= g.lightmappedobjectoffsetfinish)
					t.ttt = IntersectAll(87000, 87000 + g.merged_new_objects - 1, 0, 0, 0, 0, 0, 0, -123);
				else
					t.ttt = IntersectAll( g.lightmappedobjectoffset, g.lightmappedobjectoffsetfinish, t.brayx1_f, t.brayy1_f, t.brayz1_f, 0, 0, 0, -123 );
				#endif
				t.tintersectvalue = IntersectAll( g.entityviewstartobj, g.entityviewendobj, 
					                              t.texplodex_f, t.texplodey_f, t.texplodez_f, 
					                              fRayDestFromExplosionX, fRayDestFromExplosionY, fRayDestFromExplosionZ, 
					                              t.entityelement[ t.e ].obj );
				if ( t.tintersectvalue == 0 || t.tintersectvalue == t.entityelement[ t.texplodesourceEntity ].obj )
				{
					//t.tdamage = ( t.texploderadius_f - t.tdd_f ) * t.tstrengthofexplosion_f;
					//t.tdamageforce = (t.texploderadius_f - t.tdd_f) * t.tstrengthofexplosion_f;
					t.tdamage = (1.0f - (t.tdd_f / t.texploderadius_f)) * t.tstrengthofexplosion_f;
					t.tdamageforce = (1.0f - (t.tdd_f / t.texploderadius_f)) * (t.tstrengthofexplosion_f);
					t.brayx1_f = t.texplodex_f;
					t.brayy1_f = t.texplodey_f;
					t.brayz1_f = t.texplodez_f;
					t.brayx2_f = fCenterOfEntityX;
					t.brayy2_f = fCenterOfEntityY;
					t.brayz2_f = fCenterOfEntityZ;
					t.braydx_f = t.brayx2_f-t.brayx1_f;
					t.braydz_f = t.brayz2_f-t.brayz1_f;
					t.braydist_f = Sqrt(abs(t.braydx_f*t.braydx_f)+abs(t.braydz_f*t.braydz_f));
					if ( t.braydist_f < 75 ) t.brayy2_f = t.texplodey_f + 100.0;
					if ( t.tdamageforce > 150 ) t.tdamageforce = 150;
					t.tdamagesource = 2;
					t.ttte = t.e ; entity_applydamage( ); t.e = t.ttte;
					//  inform darkAI of the explosion
					t.tsx_f = t.entityelement[t.e].x; 
					t.tsz_f = t.entityelement[t.e].z;
					darkai_makeexplosionsound ( );
				}
			}
		}
	}

	//  Reset flag for ai damage
	g.mp.damageWasFromAI = 0;
}

void physics_player_init ( void )
{
	// One Player In Single Player Game
	t.plrid=1;
	t.tnostartmarker=1;
	g.flashLightKeyEnabled = true;

	//  Initialise player settings
	if ( t.game.levelplrstatsetup == 1 )
	{
		// starting stats
		t.playercontrol.startlives=0;
		t.playercontrol.startstrength=100;
		t.playercontrol.startviolent=1;
		t.playercontrol.starthasweapon=0;
		t.playercontrol.starthasweaponqty=0;
		t.playercontrol.speedratio_f=1.0;
		t.playercontrol.hurtfall=100;
		t.playercontrol.canrun=1;
		t.player[t.plrid].lives=t.playercontrol.startlives;
		t.player[t.plrid].health=t.playercontrol.startstrength;
		t.player[t.plrid].powers.level=100;

		//  act on start marker
		t.playercontrol.hurtfall=0;
		t.playercontrol.speedratio_f=1.0;
	}
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entityprofile[t.entid].ismarker == 1 ) 
		{
			//  Player Start Marker Settings
			t.terrain.playerx_f=t.entityelement[t.e].x;
			t.terrain.playery_f=t.entityelement[t.e].y;
			t.terrain.playerz_f=t.entityelement[t.e].z;

			void FixEulerZInverted(float &ax, float &ay, float &az);
			float ax, ay, az;
			ax = t.entityelement[t.e].rx;
			ay = t.entityelement[t.e].ry;
			az = t.entityelement[t.e].rz;
			FixEulerZInverted(ax,ay,az);

			//ay = t.entityelement[t.e].ry

			t.terrain.playerax_f=0;
			t.terrain.playeray_f = ay;
			t.camangy_f=t.terrain.playeray_f;
			t.terrain.playeraz_f=0;
			t.playercontrol.finalcameraangley_f=t.terrain.playeray_f;

			//  Player Global Settings for this level
			if ( t.game.levelplrstatsetup == 1 )
			{
				//PE: @Maciej we still use lives in other systems.
				#ifdef WICKEDENGINE
				t.playercontrol.startlives=9999;
				#else
				t.playercontrol.startlives=t.entityelement[t.e].eleprof.lives;
				#endif
				t.playercontrol.startstrength=t.entityelement[t.e].eleprof.strength;
				if (  t.playercontrol.thirdperson.enabled == 1 ) 
				{
					t.tprotagoniste=t.playercontrol.thirdperson.charactere;
					t.playercontrol.starthasweapon=t.entityelement[t.tprotagoniste].eleprof.hasweapon;
				}
				else
				{
					t.playercontrol.starthasweapon=t.entityelement[t.e].eleprof.hasweapon;
				}
				t.playercontrol.starthasweaponqty=t.entityelement[t.e].eleprof.quantity;
				t.playercontrol.startviolent=t.entityelement[t.e].eleprof.isviolent;
				t.playercontrol.speedratio_f=t.entityelement[t.e].eleprof.speed/100.0;
				t.playercontrol.hurtfall=t.entityelement[t.e].eleprof.hurtfall;

				t.playercontrol.iPlayHeartBeatSoundOff = t.entityelement[t.e].eleprof.perentityflags & 1;
				t.playercontrol.iShowScreenBloodOff = (t.entityelement[t.e].eleprof.perentityflags & (1 << 1)) >> 1;
				#ifdef WICKEDENGINE
				t.playercontrol.fWeaponDamageMultiplier = t.entityelement[t.e].eleprof.weapondamagemultiplier;
				t.playercontrol.fMeleeDamageMultiplier = t.entityelement[t.e].eleprof.meleedamagemultiplier;
				#endif

				// new property of player start marker to disable flashlight
				if (t.entityelement[t.e].eleprof.usespotlighting == 1 )
					g.flashLightKeyEnabled = false;
				else
					g.flashLightKeyEnabled = true;

				// 050416 - if in parental mode, ensure no weapon at start
				if ( g.quickparentalcontrolmode == 2 )
				{
					// only ban modern day weapons, not fireball
					int iPlrGunID = t.weaponindex=t.playercontrol.starthasweapon;
					if ( iPlrGunID > 0 )
					{
						if ( strnicmp ( t.gun[iPlrGunID].name_s.Get(), "modernday", 9 ) == NULL )
						{
							t.playercontrol.starthasweapon = 0;
							t.playercontrol.starthasweaponqty = 0;
							t.playercontrol.startviolent = 0;
						}
					}
				}

				//  Populate lives and health with default player
				t.player[t.plrid].lives=t.playercontrol.startlives;
				t.player[t.plrid].health=t.playercontrol.startstrength;

				//  Start Marker present
				t.tnostartmarker=0;
			}
			else
			{
				// level 2 and above do not control start stats or weaponry
			}
		}
	}

	#ifdef WICKEDENGINE
	float fEditableSizeHalved = GGTerrain_GetEditableSize();
	t.terraineditableareasizeminx = -fEditableSizeHalved;
	t.terraineditableareasizeminz = -fEditableSizeHalved;
	t.terraineditableareasizemaxx = fEditableSizeHalved;
	t.terraineditableareasizemaxz = fEditableSizeHalved;
	if (t.terrain.playerx_f < t.terraineditableareasizeminx + 100.0f) t.terrain.playerx_f = t.terraineditableareasizeminx + 100.0f;
	if (t.terrain.playerx_f > t.terraineditableareasizemaxx - 100.0f) t.terrain.playerx_f = t.terraineditableareasizemaxx - 100.0f;
	if (t.terrain.playerz_f < t.terraineditableareasizeminz + 100.0f) t.terrain.playerz_f = t.terraineditableareasizeminz + 100.0f;
	if (t.terrain.playerz_f > t.terraineditableareasizemaxz - 100.0f) t.terrain.playerz_f = t.terraineditableareasizemaxz - 100.0f;
	#endif

	//  If no player start marker, reset player physics tweakables
	if ( t.game.levelplrstatsetup == 1 )
	{
		if ( t.tnostartmarker == 1 ) physics_inittweakables ( );
	}

	#ifdef VRTECH
	// if multiplayer mode, change start position to the multiplayer start marker default
	if ( t.game.runasmultiplayer == 1 ) 
	{
		// store good one
		float fGoodX = t.terrain.playerx_f;
		float fGoodY = t.terrain.playery_f;
		float fGoodZ = t.terrain.playerz_f;
		float fGoodA = t.terrain.playeray_f;

		// chose a multiplayer start position at random
		int iChoose = 1;
		if ( t.tmpstartindex > 1 ) iChoose = 1 + (rand() % t.tmpstartindex);
		t.terrain.playerx_f=t.mpmultiplayerstart[iChoose].x;
		t.terrain.playery_f=t.mpmultiplayerstart[iChoose].y;
		t.terrain.playerz_f=t.mpmultiplayerstart[iChoose].z;
		t.terrain.playeray_f=t.mpmultiplayerstart[iChoose].angle;
		t.camangy_f=t.terrain.playeray_f;
		if ( t.terrain.playerx_f < 100 )
		{
			// no start position, revert to regular start marker
			t.terrain.playerx_f = fGoodX;
			t.terrain.playery_f = fGoodY;
			t.terrain.playerz_f = fGoodZ;
			t.terrain.playeray_f = fGoodA;
			t.camangy_f=t.terrain.playeray_f;
		}
		t.playercontrol.finalcameraangley_f=t.terrain.playeray_f;
	}
	#endif

	//  Player start height (marker or no)
	#ifdef WICKEDENGINE
	t.tbestterrainplayery_f = BT_GetGroundHeight(t.terrain.TerrainID, t.terrain.playerx_f, t.terrain.playerz_f) + t.terrain.adjaboveground_f;
	#else
	if (  t.terrain.TerrainID>0 ) 
	{
		t.tbestterrainplayery_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.playerx_f,t.terrain.playerz_f)+t.terrain.adjaboveground_f;
	}
	else
	{
		t.tbestterrainplayery_f=g.gdefaultterrainheight+t.terrain.adjaboveground_f;
	}
	#endif

	//  also ensure ABOVE water Line (  )
	if (  t.tbestterrainplayery_f<t.terrain.waterliney_f+20+t.terrain.adjaboveground_f ) 
	{
		t.tbestterrainplayery_f=t.terrain.waterliney_f+20+t.terrain.adjaboveground_f;
	}
	if (  t.terrain.playery_f == 0 ) 
	{
		t.terrain.playery_f=t.tbestterrainplayery_f;
	}
	else
	{
		t.terrain.playery_f=t.terrain.playery_f+t.terrain.adjaboveground_f;
		if (  t.terrain.playery_f<t.tbestterrainplayery_f  )  t.terrain.playery_f = t.tbestterrainplayery_f;
	}

	// Select weapon if start marker specifies it
	//for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
	//{
	//	t.weaponslot[t.ws].got=0;
	//	t.weaponslot[t.ws].pref=0;
	//}
	if ( t.game.levelplrstatsetup == 1 )
	{
		//Sometimes the player would spawn with a "phantom weapon" where the weapon slot would be occupied and obstruct collecting/using weapons
		//This makes sure that the player start the level with no weapons and unoccupied weapon slots
		physics_player_resetWeaponSlots();

		// Noite for MD - be aware that the code is also used by other products, so wrap new code with the WICKEDENGINE ifdef
		#ifdef WICKEDENGINE
		// Adds Unarmoured Gloves to players inventory on spawn
		// This 'fix' was unnecessary as the user can specify what weaponm the player starts with, including the unarmed gloves (see Weapon propery in player start)
		// t.weaponindex = 1;
		// physics_player_addweapon();
		// Note for MD - also worth testing the section after adding code, you would have seen selecting the Rifle to start with gets overridden with this code in a nasty way :)
		#endif

		if (  t.playercontrol.starthasweapon>0 ) 
		{
			t.weaponindex=t.playercontrol.starthasweapon;
			t.tqty=t.playercontrol.starthasweaponqty;
			physics_player_addweapon ( );
		}
	}
	else
	{
		// if have weapon from previous level session, activate it
		if ( t.lastgunid > 0 ) 
		{
			g.autoloadgun = t.lastgunid;
			t.lastgunid = 0;
		}
	}

	// and only use this flag once per game
	t.game.levelplrstatsetup = 0;

	//  OpenFileMap (  for IDE access )
	if (  t.plrfilemapaccess == 0 && t.game.gameisexe == 0 ) 
	{
		#ifdef FPSEXCHANGE
		OpenFileMap (  11, "FPSEXCHANGE" );
		SetEventAndWait (  11 );
		#endif
		t.plrfilemapaccess=1;
	}
}

void physics_player_free ( void )
{
	if (  t.plrfilemapaccess == 1 ) 
	{
		t.plrfilemapaccess=0;
	}
}

//Dave Performance
int physics_player_listener_delay = 0;
void physics_player ( void )
{
	if ( t.game.runasmultiplayer == 0 || g.mp.noplayermovement == 0 ) 
	{
		#ifdef VRTECH
		if ( t.aisystem.processplayerlogic == 1 ) 
		#else
		if ( t.aisystem.processplayerlogic == 1 || t.conkit.editmodeactive != 0 ) 
		#endif
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_player_gatherkeycontrols");
			physics_player_gatherkeycontrols ( );
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_player_control");
			physics_player_control ( );
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling gun_update_hud");
			gun_update_hud ( );
			if ( ++physics_player_listener_delay > 3 )
			{
				physics_player_listener_delay = 0;
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_player_listener");
				physics_player_listener ( );
			}
		}
		else
		{
			// prevent player physics movement
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling ODEControlDynamicCharacterController");
			ODEControlDynamicCharacterController ( t.aisystem.objectstartindex, 0, 0, 0, 0, t.aisystem.playerducking, 0, 0, 0 );

			//PE: Make sure to rotate set camera to make shadows work in freezemode.
			if (t.freezeplayerposonly == 0)
				RotateObject(t.aisystem.objectstartindex, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f);
			if (g.luacameraoverride != 2 && g.luacameraoverride != 3)
			{
				// lee - 140820 - when this flag is set with Q&A script, terrain suddenly gets shadow!
				if (t.freezeplayerposonly == 0) 
					RotateCamera(0, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f);
				else
					RotateCamera(0, t.freezeplayerax, t.freezeplayeray, t.freezeplayeraz);
			}

		}
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_player_handledeath");
		physics_player_handledeath ( );
	}
}

void physics_player_gatherkeycontrols ( void )
{
	// Camera in control of player one
	t.plrid=1;

	// Key configuration
	t.plrkeyW=17;
	t.plrkeyA=30;
	t.plrkeyS=31;
	t.plrkeyD=32;
	t.plrkeyQ=16;
	t.plrkeyE=18;
	t.plrkeyF=33;
	t.plrkeyC=46;
	t.plrkeyZ=44;
	t.plrkeyR=19;
	t.plrkeySPACE=57;
	t.plrkeyRETURN=28;
	t.plrkeySHIFT=42;
	t.plrkeySHIFT2=54;
	t.plrkeyF12=88;
	t.plrkeyJ=36;

	// from SETUP.INI config keys
	if ( t.listkey[1]>0  )  t.plrkeyW = t.listkey[1];
	if ( t.listkey[2]>0  )  t.plrkeyS = t.listkey[2];
	if ( t.listkey[3]>0  )  t.plrkeyA = t.listkey[3];
	if ( t.listkey[4]>0  )  t.plrkeyD = t.listkey[4];
	if ( t.listkey[5]>0  )  t.plrkeySPACE = t.listkey[5];
	if ( t.listkey[6]>0  )  t.plrkeyC = t.listkey[6];
	if ( t.listkey[7]>0  )  t.plrkeyRETURN = t.listkey[7];
	if ( t.listkey[8]>0  )  t.plrkeyR = t.listkey[8];
	if ( t.listkey[9]>0  )  t.plrkeyQ = t.listkey[9];
	if ( t.listkey[10]>0  )  t.plrkeyE = t.listkey[10];
	if ( t.listkey[11]>0  )  t.plrkeySHIFT = t.listkey[11];

	// Read keys from config, and use in player control actions
	if ( g.walkonkeys == 1 ) 
	{
		if ( KeyState(g.keymap[t.plrkeyW]) ==1 ) { t.plrkeyW=1 ; t.plrkeySLOWMOTION=0 ;} else t.plrkeyW=0;
		if ( KeyState(g.keymap[t.plrkeyA]) ==1 ) { t.plrkeyA=1 ; t.plrkeySLOWMOTION=0 ;} else t.plrkeyA=0;
		if ( KeyState(g.keymap[t.plrkeyS]) ==1 ) { t.plrkeyS=1 ; t.plrkeySLOWMOTION=0 ;} else t.plrkeyS=0;
		if ( KeyState(g.keymap[t.plrkeyD]) ==1 ) { t.plrkeyD=1 ; t.plrkeySLOWMOTION=0 ;} else t.plrkeyD=0;
	}
	if ( g.arrowkeyson == 1 ) 
	{
		t.tplrkeySLOWMOTIONold=t.plrkeySLOWMOTION;
		if ( UpKey() == 1 ) { t.plrkeyW = 1  ; t.plrkeySLOWMOTION = 1; }
		if ( LeftKey() == 1 ) { t.plrkeyA = 1  ; t.plrkeySLOWMOTION = 1; }
		if ( DownKey() == 1 ) { t.plrkeyS = 1  ; t.plrkeySLOWMOTION = 1; }
		if ( RightKey() == 1 ) { t.plrkeyD = 1  ; t.plrkeySLOWMOTION = 1; }
		if ( t.tplrkeySLOWMOTIONold != t.plrkeySLOWMOTION ) 
		{
			t.null=MouseMoveX() ; t.null=MouseMoveY();
			t.cammousemovex_f=0 ; t.cammousemovey_f=0;
			t.tFinalCamX_f=ObjectPositionX(t.aisystem.objectstartindex);
			t.tFinalCamY_f=ObjectPositionY(t.aisystem.objectstartindex);
			t.tFinalCamZ_f=ObjectPositionZ(t.aisystem.objectstartindex);
		}
	}
	if ( KeyState(g.keymap[t.plrkeySHIFT]) == 1 && g.runkeys == 1 && t.jumpaction == 0  )  t.plrkeySHIFT = 1; else t.plrkeySHIFT = 0;
	if ( KeyState(g.keymap[t.plrkeySHIFT2]) == 1 && g.runkeys == 1 && t.jumpaction == 0  )  t.plrkeySHIFT2 = 1; else t.plrkeySHIFT2 = 0;

	#ifdef VRTECH
	// when in vr mode
	if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
	{
		// lee - 040619 - detect trigger and use as t.plrkeyE
		if ( GGVR_RightController_Trigger() > 0.5 || GGVR_LeftController_Trigger() > 0.5 )
			t.plrkeyE = 1;

		// lee - 040619 - detect grip button and use as run
		if ( GGVR_RightController_Grip() == 1 || GGVR_LeftController_Grip() == 1 )
			t.plrkeySHIFT = 1;
	}
	#endif

	if ( t.conkit.editmodeactive != 0 ) 
	{
		// FPS 3D Editing Mode - keys elsewhere
	}
	else
	{
		// FPS Gaming Mode
		if ( KeyState(g.keymap[t.plrkeySPACE]) == 1 && g.jumponkey == 1  )  t.plrkeySPACE = 1; else t.plrkeySPACE = 0;
		if ( KeyState(g.keymap[t.plrkeyQ]) == 1 && g.peekonkeys == 1  )  t.plrkeyQ = 1; else t.plrkeyQ = 0;
		if ( KeyState(g.keymap[t.plrkeyE]) == 1 && g.peekonkeys == 1  )  t.plrkeyE = 1; else t.plrkeyE = 0;
		if ( KeyState(g.keymap[t.plrkeyF]) == 1  )  t.plrkeyF = 1; else t.plrkeyF = 0;
		if ( KeyState(g.keymap[t.plrkeyC]) == 1 && g.crouchonkey == 1  )  t.plrkeyC = 1; else t.plrkeyC = 0;
		if ( ControlKey() == 1  )  t.plrkeyC = 1;
		if ( KeyState(g.keymap[t.plrkeyZ]) == 1  )  t.plrkeyZ = 1; else t.plrkeyZ = 0;
		if ( KeyState(g.keymap[t.plrkeyR]) == 1  )  t.plrkeyR = 1; else t.plrkeyR = 0;
		if ( KeyState(g.keymap[t.plrkeyRETURN]) == 1  )  t.plrkeyRETURN = 1; else t.plrkeyRETURN = 0;
		if ( KeyState(g.keymap[t.plrkeyJ]) == 1  )  t.plrkeyJ = 1; else t.plrkeyJ = 0;
	}

	// XBOX/Controller Keys
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking XBOX/Controller Keys");
	if ( g.gxbox == 1 ) 
	{
		if ( JoystickFireC() == 1 ) 
		{
			t.plrkeyR = 1;
		}
		if ( JoystickFireD() == 1 )  // also duplicated in LUA.cpp
		{
			t.plrkeyE = 1;
		}
	}
	if ( g.gxbox == 1 ) 
	{
		if ( g.walkonkeys == 1 ) 
		{
			if ( JoystickY()<-850  )  t.plrkeyW = 1;
			if ( JoystickY()>850  )  t.plrkeyS = 1;
			if ( JoystickX()<-850  )  t.plrkeyA = 1;
			if ( JoystickX()>850  )  t.plrkeyD = 1;
		}
		if ( JoystickFireA() == 1 && g.jumponkey == 1   )  t.plrkeySPACE = 1;
		if ( g.gxboxcontrollertype == 0 ) 
		{
			// XBOX360 Controller
			if ( JoystickFireXL(8) == 1 && g.crouchonkey == 1  )  t.plrkeyC = 1;
			if ( JoystickFireXL(9) == 1  )  t.plrkeyZ = 1;
		}
		if ( g.gxboxcontrollertype == 1 ) 
		{
			// Dual Action
			if ( JoystickFireXL(10) == 1 && g.crouchonkey == 1  )  t.plrkeyC = 1;
			if ( JoystickFireXL(11) == 1  )  t.plrkeyZ = 1;
			if ( JoystickFireXL(4) == 1 && g.runkeys == 1  )  t.plrkeySHIFT = 1;
			if ( JoystickFireXL(6) == 1 && g.runkeys == 1  )  t.plrkeySHIFT = 1;
		}
		if ( g.gxboxcontrollertype == 2 ) 
		{
			// Dual Action F310
			if ( JoystickFireXL(10) == 1 && g.crouchonkey == 1  )  t.plrkeyC = 1;
			if ( JoystickFireXL(11) == 1  )  t.plrkeyZ = 1;
			if ( JoystickFireXL(4) == 1 && g.runkeys == 1  )  t.plrkeySHIFT = 1;
			if ( JoystickFireXL(6) == 1 && g.runkeys == 1  )  t.plrkeySHIFT = 1;
		}
	}

	#ifdef VRTECH
	// VR Support - take extra input from VR controllers
	if ( g.vrglobals.GGVREnabled > 0 && g.vrglobals.GGVRUsingVRSystem == 1 )
	{
		// Intialize the player to the start position and rotation and setup the GGVR Player Object
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking VR controllers");
		if (g.vrglobals.GGVRInitialized == 0)
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling GGVR_SetPlayerPosition");
			GGVR_SetPlayerPosition(t.tFinalCamX_f, BT_GetGroundHeight(t.terrain.TerrainID, t.tFinalCamX_f, t.tFinalCamZ_f), t.tFinalCamZ_f);
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling GGVR_SetPlayerRotation");

			// this sets the origin based on the current camera zero (ARG!)
			// should only set based on player angle (minus HMD influence) as HMD added later at right time for smooth headset viewing!
			//GGVR_SetPlayerRotation(0, CameraAngleY(t.terrain.gameplaycamera), 0);
			//GGVR_SetPlayerRotation(0, t.camangy_f, 0); actually not called in main loop (actually in postproces with call to GGVR_SetPlayerAngleY)
			GGVR_SetPlayerRotation(0, 0, 0);

			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling GGVR_UpdatePlayer(false,terrainID)");
			if ( g.gproducelogfiles == 2 ) 
			{
				char pWhatIsPath[1024];
				GetCurrentDirectoryA ( 1024, pWhatIsPath );
				timestampactivity(0,pWhatIsPath);
			}
			try
			{
				int iBatchStart = g.batchobjectoffset;
				int iBatchEnd = g.batchobjectoffset + g.merged_new_objects + 1;
				GGVR_UpdatePlayer(false,t.terrain.TerrainID,g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,g.entityviewstartobj,g.entityviewendobj, iBatchStart, iBatchEnd);
			}
			catch(...)
			{
				timestampactivity(0,"try catch failed when calling GGVR_UpdatePlayer");
			}
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"getting GGVR_GetHMDOffsets");
			g.vrglobals.GGVR_Old_XposOffset = GGVR_GetHMDOffsetX();
			g.vrglobals.GGVR_Old_ZposOffset = GGVR_GetHMDOffsetZ();
			g.vrglobals.GGVR_Old_Yangle = GGVR_GetPlayerAngleY();
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"g.vrglobals.GGVRInitialized");
			g.vrglobals.GGVRInitialized = 1;
		}
		if (g.walkonkeys == 1)
		{
			if ( GGVR_RightController_JoyY() >  0.5 || GGVR_LeftController_JoyY() >  0.5 ) t.plrkeyW = 1;
			if ( GGVR_RightController_JoyY() < -0.5 || GGVR_LeftController_JoyY() < -0.5)  t.plrkeyS = 1;
		}
		g.vrglobals.GGVR_XposOffset = GGVR_GetHMDOffsetX();
		g.vrglobals.GGVR_ZposOffset = GGVR_GetHMDOffsetZ();
		g.vrglobals.GGVR_XposOffsetChange = g.vrglobals.GGVR_XposOffset - g.vrglobals.GGVR_Old_XposOffset;
		g.vrglobals.GGVR_ZposOffsetChange = g.vrglobals.GGVR_ZposOffset - g.vrglobals.GGVR_Old_ZposOffset;
		g.vrglobals.GGVR_Old_XposOffset = g.vrglobals.GGVR_XposOffset;
		g.vrglobals.GGVR_Old_ZposOffset = g.vrglobals.GGVR_ZposOffset;
	}
	#endif

	// Automated actions (script control)
	switch ( g.playeraction ) 
	{
		case 1 : t.player[1].state.firingmode = 1; break ;
		case 2 : t.gunzoommode = 1 ; break ;
		case 3 : t.player[1].state.firingmode = 2 ; break ;
		case 4 : g.forcecrouch = 1 ; break ;
		case 5 : t.plrkeySPACE = 1 ; break ;
		case 6 : t.plrkeyE = 1 ; break ;
		case 7 : t.plrkeyQ = 1 ; break ;
		case 8 : t.plrkeyRETURN = 1 ; break ;
		case 9 : t.tmouseclick = 1 ; break ;
		case 10 : t.tmouseclick = 2 ; break ;
		case 11 : 
		{
			// ensure weapon unjams affect both modes if sharing ammo
			g.firemodes[t.gunid][g.firemode].settings.jammed = 1; 
			if ( t.gun[t.gunid].settings.modessharemags == 1 ) 
			{
				g.firemodes[t.gunid][0].settings.jammed = 1;
				g.firemodes[t.gunid][1].settings.jammed = 1;
			}
		}
		break;
	}

	// Third person disables crouch/zoom/RMB
	t.playercontrol.camrightmousemode=0;
	if ( t.playercontrol.thirdperson.enabled == 1 ) 
	{
		g.forcecrouch=0 ; t.plrkeyC=0;
		t.gunzoommode = 0 ; if ( t.tmouseclick == 2 ) { t.tmouseclick = 0  ; t.playercontrol.camrightmousemode = 1; }
	}

	//  Free weapon jam if reload used (possible relocate these to gun module
	if ( t.player[1].state.firingmode == 2 ) //&& t.gunzoommode == 0 ) 
	{
		// unjam or reload animation to unjam weapon
		g.plrreloading=1;
		g.firemodes[t.gunid][g.firemode].settings.shotsfired=0;
		// play free jam animation if it exists
		if ( g.firemodes[t.gunid][g.firemode].action2.clearjam.s != 0 && g.firemodes[t.gunid][g.firemode].settings.jammed == 1 ) 
		{
			// come out of zoom if in it
			if ( t.gunzoommode >=8 ) t.gunzoommode = 11; // catches all states of a zoomed in state

			// play anim to fix jam
			g.plrreloading=2;
			g.custstart=g.firemodes[t.gunid][g.firemode].action2.clearjam.s;
			g.custend=g.firemodes[t.gunid][g.firemode].action2.clearjam.e;
			t.gunmode=9998;
		}
		g.firemodes[t.gunid][g.firemode].settings.shotsfired=0;

		// ensure weapon unjams affect both modes if sharing ammo
		g.firemodes[t.gunid][g.firemode].settings.jammed = 0;
		if ( t.gun[t.gunid].settings.modessharemags == 1 ) 
		{
			g.firemodes[t.gunid][0].settings.jammed = 0;
			g.firemodes[t.gunid][1].settings.jammed = 0;
		}
	}

	// Forced key controls (script control)
	if ( g.forcemove>0  )  t.plrkeyW = 1;
	if ( g.forcemove<0  )  t.plrkeyS = 1;
	if ( g.forcecrouch == 1 && g.playeraction != 4  )  g.forcecrouch = 0;
	if ( g.playeraction != 4  )  g.playeraction = 0;

	// interrogate IDE to see if we have input focus
	if ( t.game.gameisexe == 0 ) 
	{
		t.plrhasfocus=1;
		if ( t.plrfilemapaccess == 1 ) 
		{
			#ifdef VRTECH
			// if VR, disable this as WMR changes the focus window
			if ( g.vrglobals.GGVREnabled > 0 )
			{
				// window focus can be switched to HMD window
			}
			else
			{
				// normal behavior
				t.plrhasfocus=1;
				#if !defined(ENABLEIMGUI) || defined(USEOLDIDE)
				t.plrhasfocus=GetFileMapDWORD( 11, 148 );
				#endif
			}
			#else
			t.plrhasfocus=GetFileMapDWORD( 11, 148 );
			#endif
		}
	}

	// If player no health (dead), cannot control anything
	if ( t.player[t.plrid].health <= 0 || t.plrhasfocus == 0 ) 
	{
		t.plrkeyW=0;
		t.plrkeyA=0;
		t.plrkeyS=0;
		t.plrkeyD=0;
		t.plrkeyQ=0;
		t.plrkeyE=0;
		t.plrkeyF=0;
		t.plrkeyC=0;
		t.plrkeyZ=0;
		t.plrkeyR=0;
		t.plrkeySPACE=0;
		t.plrkeyRETURN=0;
		t.plrkeySHIFT=0;
		t.plrkeySHIFT2=0;
		t.plrkeyJ=0;
	}
}

void physics_no_gun_zoom ( void )
{
	//t.realfovdegree_f=t.visuals.CameraFOVZoomed_f;//*g.airmod_camfov_f;
	g.realfov_f=t.visuals.CameraFOV_f;//+t.realfovdegree_f;
	if ( g.realfov_f < 15 ) g.realfov_f = 15;
	SetCameraFOV ( g.realfov_f );
	SetCameraFOV ( 2, g.realfov_f );
}

void physics_getcorrectjumpframes ( int entid, float* fStartFrame, float* fHoldFrame, float* fResumeFrame, float* fFinishFrame )
{
	// use frames stored in VAULT animation
	t.q = t.entityprofile[entid].startofaianim + t.csi_stoodvault[1];
	*fStartFrame = t.entityanim[entid][t.q].start;
	*fFinishFrame = t.entityanim[entid][t.q].finish;

	// jump hold animation frames overridden in FPE
	if ( t.entityprofile[entid].jumphold > 0 )
		*fHoldFrame = t.entityprofile[entid].jumphold;
	else
		*fHoldFrame = t.entityanim[entid][t.q].finish - 10;

	// jump resume frame to indicate when can resume movement
	if ( t.entityprofile[entid].jumpresume > 0 )
		*fResumeFrame = t.entityprofile[entid].jumpresume;
	else
		*fResumeFrame = t.entityanim[entid][t.q].finish;
}

void physics_player_control_F9 ( void )
{
	// completely skip use of mousemovexy so LUA mouse system can use it for its own pointer
	if ( g.luaactivatemouse != 1 )
	{
		// No player control if dead, but use up mousemoves to prevent sudden move on respawn or if in multiplayer and respawning
		if ( t.player[t.plrid].health == 0 || t.plrhasfocus == 0 || (t.game.runasmultiplayer == 1 && g.mp.respawnLeft != 0) ) 
		{
			t.tMousemove_f = MouseMoveX() + MouseMoveY();
		}

		// Tab Mode 2, low FPS screen and construction kit menus require mouse.
		t.tConkitMenusOpen = t.conkit.make.paint.menuOpen + t.conkit.make.held.menuOpen + t.conkit.make.feature.menuOpen;
		if ( g.tabmode<2 && g.lowfpswarning != 1 && t.tConkitMenusOpen  ==  0 ) 
		{
			// Manipulate camera angle using mouselook
			if ( g.gminvert == 1 )  t.ttmousemovey = MouseMoveY()*-1; else t.ttmousemovey = MouseMoveY();
			if ( t.plrkeySLOWMOTION == 1 && g.globals.smoothcamerakeys == 1 ) 
			{
				t.cammousemovex_f=t.cammousemovex_f+(MouseMoveX()/10.0);
				t.cammousemovey_f=t.cammousemovey_f+(t.ttmousemovey/10.0);
				t.cammousemovex_f=t.cammousemovex_f*0.94;
				t.cammousemovey_f=t.cammousemovey_f*0.94;
			}
			else
			{
				t.cammousemovex_f=MouseMoveX();
				t.cammousemovey_f=t.ttmousemovey;
			}
			if ( t.plrhasfocus == 1 ) 
			{
				PositionMouse ( g_pGlob->dwWindowWidth/2, g_pGlob->dwWindowHeight );
			}
		}
		else
		{
			t.null=MouseMoveX() ; t.null=MouseMoveY();
			t.cammousemovex_f=0 ; t.cammousemovey_f=0;
		}
		if ( g.gxbox == 1 ) 
		{
			t.tjoyrotx_f=((JoystickTwistX()+0.0)-32768.0)/32768.0;
			t.tjoyroty_f=((JoystickTwistY()+0.0)-32768.0)/32768.0;
			if ( t.tjoyrotx_f>-0.2 && t.tjoyrotx_f<0.2  )  t.tjoyrotx_f = 0;
			if ( t.tjoyroty_f>-0.2 && t.tjoyroty_f<0.2  )  t.tjoyroty_f = 0;
			t.cammousemovex_f=t.tjoyrotx_f*6.0;
			t.cammousemovey_f=t.tjoyroty_f*6.0;
		}

		// Modifly rotation speed by zoom amount
		t.tturnspeedmodifier_f=1.0;
		if ( t.gunid>0 ) 
		{
			t.tturnspeedmodifier_f=g.firemodes[t.gunid][g.firemode].settings.plrturnspeedmod;
			if ( t.plrzoomin_f>1 ) 
			{
				// use gunspec settings to control relative speed of zoom mouselook
				t.tturnspeedmodifier_f=t.tturnspeedmodifier_f*g.firemodes[t.gunid][g.firemode].settings.zoomturnspeed;
			}
		}
		t.tRotationDivider_f=8.0/t.tturnspeedmodifier_f;
		t.camangx_f=CameraAngleX(t.terrain.gameplaycamera)+(t.cammousemovey_f/t.tRotationDivider_f);
		//if ( g.vrglobals.GGVREnabled == 0 ) // no VR with F9 mode currently!
		//{
		t.camangy_f=t.playercontrol.finalcameraangley_f+(t.cammousemovex_f/t.tRotationDivider_f);
		//}
	}

	// Cap look up/down angle so cannot wrap around
	if ( t.camangx_f<-80  )  t.camangx_f = -80;
	if ( t.camangx_f>85  )  t.camangx_f = 85;

	// Rotate player camera
	t.tsuspendplayercontrols=0;
	if ( t.player[t.plrid].health>0 && t.tsuspendplayercontrols == 0 ) 
	{
		// when camea rotation overridden, prevent rotation
		if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
		{
			RotateCamera (  t.terrain.gameplaycamera,t.camangx_f,t.camangy_f,0 );
			t.playercontrol.cx_f=CameraAngleX(t.terrain.gameplaycamera);
			t.playercontrol.cy_f=CameraAngleY(t.terrain.gameplaycamera);
			t.playercontrol.cz_f=CameraAngleZ(t.terrain.gameplaycamera);
		}

		// when camera overridden, do not allow movement
		if ( g.luacameraoverride != 1 && g.luacameraoverride != 3 )
		{
			if ( t.conkit.editmodeactive != 0 ) 
			{
				if ( t.plrkeySHIFT == 1 || t.plrkeySHIFT2 == 1 ) 
				{
					t.playercontrol.basespeed_f=4.0;
				}
				else
				{
					if ( t.inputsys.keycontrol == 1 ) 
					{
						t.playercontrol.basespeed_f=0.25;
					}
					else
					{
						t.playercontrol.basespeed_f=1.5;
					}
				}
				t.playercontrol.isrunning=0;
			}
			else
			{
				if ( t.aisystem.playerducking == 1 ) 
				{
					t.playercontrol.basespeed_f=0.5;
					t.playercontrol.isrunning=0;
				}
				else
				{
					if ( t.playercontrol.canrun == 1 && (t.plrkeySHIFT == 1 || t.plrkeySHIFT2 == 1) ) 
					{
						// sprinting speed, modified by directions.
						t.playercontrol.basespeed_f = 2.0;
						if ( t.playercontrol.thirdperson.enabled == 1 && t.playercontrol.thirdperson.camerafollow == 1 ) 
						{
							// WASD run speed
						}
						else
						{
							if ( t.plrkeyS == 1 ) 
							{
								t.playercontrol.basespeed_f = 1.0;
							}
							else
							{
								if ( t.plrkeyW  ==  1 ) 
								{
									if ( t.plrkeyA == 1 || t.plrkeyD == 1 ) 
									{
										t.playercontrol.basespeed_f = 1.75;
									}
								}
								else
								{
									if ( t.plrkeyA == 1 || t.plrkeyD == 1 ) 
									{
										t.playercontrol.basespeed_f = 1.5;
									}
								}
							}
						}
						t.playercontrol.isrunning=1;
					}
					else
					{
						t.playercontrol.basespeed_f=1.0;
						t.playercontrol.isrunning=0;
					}
				}
			}
			t.playercontrol.maxspeed_f=t.playercontrol.topspeed_f*t.playercontrol.basespeed_f;
			t.playercontrol.movement=0;
			if ( t.plrkeyA == 1 ) 
			{
				t.playercontrol.movement=1;
				t.playercontrol.movey_f=t.playercontrol.cy_f-90;
				if ( t.plrkeyW == 1 ) t.playercontrol.movey_f=t.playercontrol.cy_f-45;
				if ( t.plrkeyS == 1 ) t.playercontrol.movey_f=t.playercontrol.cy_f-45-90 ;
			}
			else
			{
				if ( t.plrkeyD == 1 ) 
				{
					t.playercontrol.movement=1;
					t.playercontrol.movey_f=t.playercontrol.cy_f+90;
					if ( t.plrkeyW == 1 ) t.playercontrol.movey_f=t.playercontrol.cy_f+45;
					if ( t.plrkeyS == 1 ) t.playercontrol.movey_f=t.playercontrol.cy_f+45+90;
				}
				else
				{
					if ( t.plrkeyW == 1 ) { t.playercontrol.movement=1 ; t.playercontrol.movey_f=t.playercontrol.cy_f; }
					if ( t.plrkeyS == 1 ) { t.playercontrol.movement=1 ; t.playercontrol.movey_f=t.playercontrol.cy_f+180; }
				}
			}
		}
		else
		{
			// camera overridden players cannot move themselves
			t.playercontrol.movement=0;
		}
	}
	else
	{
		//  dead players cannot move themselves
		t.playercontrol.movement=0;
	}
	t.playercontrol.lastmovement=t.playercontrol.movement;

	//  Control speed and walk wobble
	if ( t.playercontrol.movement>0 ) 
	{
		t.tokay=0;
		if ( t.playercontrol.gravityactive == 1 ) 
		{
			if ( t.playercontrol.plrhitfloormaterial != 0  )  t.tokay = 1;
			if ( t.playercontrol.underwater == 1 && t.hardwareinfoglobals.nowater == 0  )  t.tokay = 1;
			if ( t.playercontrol.jumpmode == 1 && t.playercontrol.jumpmodecanaffectvelocitycountdown_f>0  )  t.tokay = 1;
		}
		else
		{
			t.tokay=1;
		}
		if ( t.tokay == 1 ) 
		{
			// increase plr speed using accel ratio
			t.playercontrol.speed_f=t.playercontrol.speed_f+(t.playercontrol.accel_f*g.timeelapsed_f*0.05);
			if ( t.playercontrol.speed_f>t.playercontrol.maxspeed_f*t.playercontrol.speedratio_f ) 
			{
				t.playercontrol.speed_f=t.playercontrol.maxspeed_f*t.playercontrol.speedratio_f;
			}
		}
		else
		{
			// with no Floor (  under player, reduce any speed for downhill and fall behavior )
			t.tdeductspeed_f=(t.playercontrol.maxspeed_f*t.playercontrol.speedratio_f)*g.timeelapsed_f*0.01;
			t.playercontrol.speed_f=t.playercontrol.speed_f-t.tdeductspeed_f;
			if ( t.playercontrol.speed_f<0.0  )  t.playercontrol.speed_f = 0.0;
		}
		if ( t.playercontrol.gravityactive == 1 && t.playercontrol.jumpmode != 1 ) 
		{
			// on ground
			t.playercontrol.wobble_f=WrapValue(t.playercontrol.wobble_f+(t.playercontrol.wobblespeed_f*t.ElapsedTime_f*t.playercontrol.basespeed_f*t.playercontrol.speedratio_f));
		}
		else
		{
			// in air
			t.playercontrol.wobble_f=CurveValue(0,t.playercontrol.wobble_f,3.0);
		}
	}
	else
	{
		t.tdeductspeed_f=(t.playercontrol.maxspeed_f*t.playercontrol.speedratio_f)*g.timeelapsed_f*0.5;
		t.playercontrol.speed_f=t.playercontrol.speed_f-t.tdeductspeed_f;
		if (  t.playercontrol.speed_f<0.0  )  t.playercontrol.speed_f = 0.0;
	}

	// Pass movement state to player array
	t.player[t.plrid].state.moving=0;
	if ( t.playercontrol.movement == 1 ) 
	{
		t.player[t.plrid].state.moving=1;
		if ( t.aisystem.playerducking != 0 ) 
		{
			t.player[t.plrid].state.moving=2;
		}
	}

	// Prevent player leaving terrain area
	if ( ObjectPositionX(t.aisystem.objectstartindex)<100 ) { t.playercontrol.pushangle_f = 90.0  ; t.playercontrol.pushforce_f = 1.0; }
	if ( ObjectPositionX(t.aisystem.objectstartindex)>51100 ) { t.playercontrol.pushangle_f = 270.0  ; t.playercontrol.pushforce_f = 1.0; }
	if ( ObjectPositionZ(t.aisystem.objectstartindex)<100 ) { t.playercontrol.pushangle_f = 0.0  ; t.playercontrol.pushforce_f = 1.0; }
	if ( ObjectPositionZ(t.aisystem.objectstartindex)>51100 ) { t.playercontrol.pushangle_f = 180.0  ; t.playercontrol.pushforce_f = 1.0; }

	// Reduce any player push force over time
	if ( t.playercontrol.pushforce_f>0 ) 
	{
		t.playercontrol.pushforce_f=t.playercontrol.pushforce_f-(g.timeelapsed_f/2.0);
		if ( t.playercontrol.pushforce_f<0 ) 
		{
			t.playercontrol.pushforce_f=0.0;
		}
	}

	// Get current player ground height stood on
	#ifdef WICKEDENGINE
	t.tplayerterrainheight_f = BT_GetGroundHeight(0, ObjectPositionX(t.aisystem.objectstartindex), ObjectPositionZ(t.aisystem.objectstartindex));
	#else
	if ( t.terrain.TerrainID>0 ) 
	{
		t.tplayerterrainheight_f=BT_GetGroundHeight(t.terrain.TerrainID,ObjectPositionX(t.aisystem.objectstartindex),ObjectPositionZ(t.aisystem.objectstartindex));
	}
	else
	{
		t.tplayerterrainheight_f=g.gdefaultterrainheight;
	}
	#endif

	// Control player using Physics character controller
	t.tgotonewheight=0;
	t.tjetpackverticalmove_f=0.0;
	if ( t.playercontrol.gravityactive == 0 ) 
	{
		// in air
		if ( t.playercontrol.lockatheight == 1 ) 
		{
			t.tjetpackverticalmove_f=0;
		}
		else
		{
			t.tjetpackverticalmove_f=WrapValue(CameraAngleX(t.terrain.gameplaycamera));
			if ( t.plrkeyS == 1  )  t.tjetpackverticalmove_f = t.tjetpackverticalmove_f-360.0;
			if ( t.plrkeyA == 1  )  t.tjetpackverticalmove_f = 0;
			if ( t.plrkeyD == 1  )  t.tjetpackverticalmove_f = 0;
		}

		// extra height control with mouse wheel
		if ( t.playercontrol.controlheight>0 || t.playercontrol.controlheightcooldown>0 ) 
		{
			t.playercontrol.storemovey=t.playercontrol.movey_f;
			if ( t.playercontrol.controlheight == 3 ) 
			{
				if ( t.terrain.TerrainID>0 ) 
				{
					t.tgotonewheight=g.gdefaultterrainheight+t.tplayerterrainheight_f;
					t.playercontrol.controlheight=0;
				}
			}
			else
			{
				if ( t.playercontrol.controlheightcooldown > 0.0f )
				{
					if ( t.playercontrol.controlheightcooldownmode == 1 ) 
					{
						t.playercontrol.movey_f=t.playercontrol.movey_f+180;
						t.tjetpackverticalmove_f=-270;
					}
					else
						t.tjetpackverticalmove_f=90;
				}
				else
				{
					if ( t.playercontrol.controlheight == 1 ) 
					{
						// move up
						t.playercontrol.movey_f=t.playercontrol.movey_f+180;
						t.tjetpackverticalmove_f=-270;
					}
					else
					{
						// move down
						t.tjetpackverticalmove_f=90;
					}
					t.playercontrol.controlheightcooldownmode = t.playercontrol.controlheight;
				}
			}
			if ( t.playercontrol.controlheightcooldown > 0 )
			{
				t.playercontrol.controlheightcooldown = t.playercontrol.controlheightcooldown - 1.0f;
				t.playercontrol.speed_f=0;
			}
			else
				t.playercontrol.speed_f=t.playercontrol.maxspeed_f*t.playercontrol.speedratio_f;
		}
	}
	else
	{
		// regular gravity
		if ( t.playercontrol.plrhitfloormaterial == 0 && t.playercontrol.jumpmode != 1 ) 
		{
			t.tjetpackverticalmove_f=25.0;
		}
	}

	// handle slope climb prevention
	t.steepstart_f=30;
	t.steepslopeweight_f=1.0;
	if ( t.tgotonewheight == 0 ) 
	{
		// Player direction and speed
		t.tfinalplrmovey_f=t.playercontrol.movey_f;
		t.tfinalplrspeed_f=t.playercontrol.speed_f*2;
		// If not in air due to jetpack thurst
		if (  t.playercontrol.jetpackthrust_f == 0 ) 
		{
			// Player speed affected by gun speed modifier and zoom mode modifier
			if ( t.gunid>0 ) 
			{
				if ( t.game.runasmultiplayer == 0 && g.globals.enableplrspeedmods == 1 ) 
				{
					// only for single player action - MP play is too muddy!
					if ( g.firemodes[t.gunid][g.firemode].settings.isempty == 1 ) 
					{
						t.tfinalplrspeed_f=t.tfinalplrspeed_f*g.firemodes[t.gunid][g.firemode].settings.plremptyspeedmod;
					}
					else
					{
						t.tfinalplrspeed_f=t.tfinalplrspeed_f*g.firemodes[t.gunid][g.firemode].settings.plrmovespeedmod;
					}
					if ( t.plrzoomin_f>1 ) 
					{
						t.tfinalplrspeed_f=t.tfinalplrspeed_f*g.firemodes[t.gunid][g.firemode].settings.zoomwalkspeed;
					}
					if ( (t.gunmode >= 121 && t.gunmode <= 126) || (t.gunmode>= 700 && t.gunmode <= 707) || (t.gunmode >= 7000 && t.gunmode<= 7099) ) 
					{
						t.tfinalplrspeed_f=t.tfinalplrspeed_f*g.firemodes[t.gunid][g.firemode].settings.plrreloadspeedmod;
					}
				}
			}
			// Determine slope angle of plr direction
			if ( t.terrain.TerrainID>0 ) 
			{
				t.tplrx_f=ObjectPositionX(t.aisystem.objectstartindex);
				t.tplry_f=ObjectPositionY(t.aisystem.objectstartindex);
				t.tplrz_f=ObjectPositionZ(t.aisystem.objectstartindex);
				t.tplrgroundy_f=t.tplry_f-34.0;
				t.tplrx2_f=NewXValue(t.tplrx_f,t.tfinalplrmovey_f,20.0);
				t.tplrz2_f=NewZValue(t.tplrz_f,t.tfinalplrmovey_f,20.0);
				t.tplrgroundy2_f=BT_GetGroundHeight(t.terrain.TerrainID,t.tplrx2_f,t.tplrz2_f);
				t.tplrgroundy2_f=BT_GetGroundHeight(t.terrain.TerrainID,t.tplrx2_f,t.tplrz2_f);
				if ( t.tplrgroundy2_f>t.tplrgroundy_f ) 
				{
					t.tsteepvalue_f=t.tplrgroundy2_f-t.tplrgroundy_f;
					if ( t.tsteepvalue_f>t.steepstart_f ) 
					{
						// terrain steeper than 45 degrees starts to slow down move speed
						t.tfinalplrspeed_f=t.tfinalplrspeed_f-((t.tsteepvalue_f-t.steepstart_f)/t.steepslopeweight_f);
						if ( t.tfinalplrspeed_f<0  )  t.tfinalplrspeed_f = 0;
					}
				}
			}
		}

		// Move player using Bullet character controller
		if ( g.globals.riftmode>0  )  t.tfinalplrmovey_f = t.tfinalplrmovey_f+0;
		if ( t.playercontrol.isrunning == 1  )  t.tjetpackverticalmove_f = 0;
		ODESetDynamicCharacterController (t.aisystem.objectstartindex, t.terrain.waterliney_f, 0, 0, 0, 0, 0, 0);
		ODEControlDynamicCharacterController ( t.aisystem.objectstartindex,t.tfinalplrmovey_f,t.tjetpackverticalmove_f,t.tfinalplrspeed_f,t.playerjumpnow_f,t.aisystem.playerducking,t.playercontrol.pushangle_f,t.playercontrol.pushforce_f,t.playercontrol.jetpackthrust_f );
		if ( t.playercontrol.gravityactive == 0 ) 
		{
			if ( t.playercontrol.controlheight>0 ) 
			{
				t.playercontrol.movey_f=t.playercontrol.storemovey;
			}
		}
	}
	else
	{
		// shift player to new height position ('M' key)
		physics_disableplayer ( );
		t.terrain.playerx_f=ObjectPositionX(t.aisystem.objectstartindex);
		t.terrain.playery_f=t.tgotonewheight;
		t.terrain.playerz_f=ObjectPositionZ(t.aisystem.objectstartindex);
		t.terrain.playerax_f=0;
		t.terrain.playeray_f=CameraAngleY(0);
		t.camangy_f=t.terrain.playeray_f;
		t.terrain.playeraz_f=0;
		t.playercontrol.finalcameraangley_f=t.terrain.playeray_f;
		physics_setupplayer ( );
		ODESetWorldGravity (  0,0,0 );
		if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
		{
			RotateCamera (  t.terrain.gameplaycamera,90,CameraAngleY(t.terrain.gameplaycamera),CameraAngleZ(t.terrain.gameplaycamera) );
		}
	}

	// Place camera at player object position (stood or ducking)
	if ( t.aisystem.playerducking != 0 ) 
	{
		t.tsubtleeyeadjustment_f=10.0;
	}
	else
	{
		t.tsubtleeyeadjustment_f=30.0;
	}
	t.tadjustbasedonwobbley_f=(Cos(t.playercontrol.wobble_f)*t.playercontrol.wobbleheight_f);
	if ( t.plrkeySLOWMOTION == 1 && g.globals.smoothcamerakeys == 1 ) 
	{
		t.tFinalCamX_f=CurveValue(ObjectPositionX(t.aisystem.objectstartindex),t.tFinalCamX_f,10.0);
		t.tFinalCamY_f=CurveValue(t.tsubtleeyeadjustment_f+ObjectPositionY(t.aisystem.objectstartindex)-t.tadjustbasedonwobbley_f,t.tFinalCamY_f,10.0);
		t.tFinalCamZ_f=CurveValue(ObjectPositionZ(t.aisystem.objectstartindex),t.tFinalCamZ_f,10.0);
	}
	else
	{
		t.tleanangle_f=CameraAngleY(t.terrain.gameplaycamera)+t.playercontrol.leanoverangle_f;
		t.tleanoverx_f=NewXValue(0,t.tleanangle_f,t.playercontrol.leanover_f);
		t.tleanoverz_f=NewZValue(0,t.tleanangle_f,t.playercontrol.leanover_f);
		t.tFinalCamX_f = ObjectPositionX(t.aisystem.objectstartindex)+t.tleanoverx_f;
		t.tFinalCamY_f = t.tsubtleeyeadjustment_f+ObjectPositionY(t.aisystem.objectstartindex)-t.tadjustbasedonwobbley_f;
		t.tFinalCamZ_f = ObjectPositionZ(t.aisystem.objectstartindex)+t.tleanoverz_f;
	}

	// record present camera angles
	t.playercontrol.finalcameraanglex_f = CameraAngleX(t.terrain.gameplaycamera);
	t.playercontrol.finalcameraangley_f = CameraAngleY(t.terrain.gameplaycamera);
	t.playercontrol.finalcameraanglez_f = CameraAngleZ(t.terrain.gameplaycamera);

	// update camera position
	if ( g.luacameraoverride != 1 && g.luacameraoverride != 3 )
	{
		PositionCamera (  t.terrain.gameplaycamera,t.tFinalCamX_f+t.tshakex_f,t.tFinalCamY_f+t.tshakey_f,t.tFinalCamZ_f+t.tshakez_f );
	}

	// Apply flinch (for when hit hit)
	if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
	{
		t.playercontrol.flinchx_f=CurveValue(0,t.playercontrol.flinchx_f,20);
		t.playercontrol.flinchy_f=CurveValue(0,t.playercontrol.flinchy_f,20);
		t.playercontrol.flinchz_f=CurveValue(0,t.playercontrol.flinchz_f,20);
		t.playercontrol.flinchcurrentx_f=CurveValue(t.playercontrol.flinchx_f,t.playercontrol.flinchcurrentx_f,10.0);
		t.playercontrol.flinchcurrenty_f=CurveValue(t.playercontrol.flinchy_f,t.playercontrol.flinchcurrenty_f,10.0);
		t.playercontrol.flinchcurrentz_f=CurveValue(t.playercontrol.flinchz_f,t.playercontrol.flinchcurrentz_f,10.0);
		RotateCamera (  t.terrain.gameplaycamera,CameraAngleX(t.terrain.gameplaycamera)+t.playercontrol.flinchcurrentx_f,CameraAngleY(t.terrain.gameplaycamera)+t.playercontrol.flinchcurrenty_f,CameraAngleZ(t.terrain.gameplaycamera)+t.playercontrol.flinchcurrentz_f+(t.playercontrol.leanover_f/-10.0) );
	}
}

void physics_player_control_LUA ( void )
{
	// when LUA global ready, call LUA player control function
	if ( t.playercontrol.gameloopinitflag == 0 )
	{
		// F9 Edit Mode Controls internal
		if ( t.conkit.editmodeactive != 0 )
		{
			// F9 movement control (for some reason LuaCall does not run PlayerControl in global.lua)??!
			physics_player_control_F9();
		}
		else
		{
			// Feed in-game mappable keys
			LuaSetInt ( "g_PlrKeyW", t.plrkeyW );
			LuaSetInt ( "g_PlrKeyA", t.plrkeyA );
			LuaSetInt ( "g_PlrKeyS", t.plrkeyS );
			LuaSetInt ( "g_PlrKeyD", t.plrkeyD );
			LuaSetInt ( "g_PlrKeyQ", t.plrkeyQ );
			LuaSetInt ( "g_PlrKeyE", t.plrkeyE );
			LuaSetInt ( "g_PlrKeyF", t.plrkeyF );
			LuaSetInt ( "g_PlrKeyC", t.plrkeyC );
			LuaSetInt ( "g_PlrKeyZ", t.plrkeyZ );
			LuaSetInt ( "g_PlrKeyR", t.plrkeyR );
			LuaSetInt ( "g_PlrKeySPACE", t.plrkeySPACE );
			LuaSetInt ( "g_PlrKeyRETURN", t.plrkeyRETURN );
			LuaSetInt ( "g_PlrKeySHIFT", t.plrkeySHIFT );
			LuaSetInt ( "g_PlrKeySHIFT2", t.plrkeySHIFT2 );
			LuaSetInt ( "g_PlrKeyJ", t.plrkeyJ );

			// Call externaliszed script
			LuaSetFunction ( "PlayerControl", 0, 0 );
			LuaCall();
		}

		#ifdef WICKEDENGINE
		// Wicked has no interest in intercepting this event internally!
		#else
		// System to detect when player (for whatever reason) drops BELOW the terrain
		if ( ObjectPositionY(t.aisystem.objectstartindex)<t.tplayerterrainheight_f-1030.0 ) 
		{
			// player should NEVER be here - drastically recreate player
			t.terrain.playerx_f=ObjectPositionX(t.aisystem.objectstartindex);
			t.terrain.playery_f=t.tplayerterrainheight_f+20.0+t.terrain.adjaboveground_f;
			t.terrain.playerz_f=ObjectPositionZ(t.aisystem.objectstartindex);
			t.terrain.playerax_f=0;
			t.terrain.playeray_f=CameraAngleY(0);
			t.camangy_f=t.terrain.playeray_f;
			t.terrain.playeraz_f=0;
			t.playercontrol.finalcameraangley_f=t.terrain.playeray_f;
			physics_disableplayer ( );
			physics_setupplayernoreset ( );
		}
		#endif
	}
}

void physics_player_control ( void )
{
	// No player controls when in editing mode
	if ( t.plrhasfocus == 0 ) return;

	// Gather input data
	t.k_s=Lower(Inkey() );

	// Invincibe Mode - God Mode
	if ( g.ggodmodestate == 1 && t.k_s == "i" ) t.player[1].health = 99999;

	// Get MouseClick (except when in TAB TAB mode)
	// Mode 1 = ignore A, C, D buttons of controller
	if ( g.tabmode < 2 ) 
		t.tmouseclick = control_mouseclick_mode(1);
	else
		t.tmouseclick=0;

	// Set input data for LUA call
	LuaSetInt ( "g_KeyPressJ", t.plrkeyJ );
	LuaSetInt ( "g_MouseClickControl", t.tmouseclick );
	physics_player_control_LUA();

	// Apply colour to shader
	SetVector4 ( g.terrainvectorindex, t.playercontrol.redDeathFog_f, 0, 0, 0 );
	t.tColorVector = g.terrainvectorindex;
	postprocess_setscreencolor ( );
}

void physics_player_handledeath ( void )
{
	// handle player death
	if (  t.game.runasmultiplayer == 0 ) 
	{
		//  Handle player death - only for single player
		if (  t.playercontrol.deadtime>0 ) 
		{
			//  control sequence
			if (  t.playercontrol.thirdperson.enabled == 0 ) 
			{
				if (  CameraAngleZ(0)<45 ) 
				{
					if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
					{
						ZRotateCamera (  0,CameraAngleZ(0)+5.0 );
					}
				}
			}
			//  when death pause over
			if (  t.aisystem.processplayerlogic == 1 ) 
			{
				if (  Timer()>t.playercontrol.deadtime ) 
				{
					if (  t.playercontrol.startlives>0 && t.player[t.plrid].lives == 0 && t.game.gameisexe == 1 ) 
					{
						//  280415 - GAME OVER flag!
						t.game.gameloop=0 ; t.game.lostthegame=1;
					}
					else
					{
						//  move player to start
						physics_player_gotolastcheckpoint ( );
						#ifdef WICKEDENGINE
						//PE: Restart any ambient music tracks.
						if (t.gamevisuals.bEndableAmbientMusicTrack)
						{
							//PE: start any ambient music tracks.
							int iFreeSoundID = g.temppreviewsoundoffset + 3;
							if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
							if (FileExist(t.visuals.sAmbientMusicTrack.Get()) == 1)
							{
								LoadSound(t.visuals.sAmbientMusicTrack.Get(), iFreeSoundID, 0, 1);
								if (SoundExist(iFreeSoundID) == 1)
								{
									LoopSound(iFreeSoundID);
									SetSoundVolume(iFreeSoundID, t.visuals.iAmbientMusicTrackVolume);
								}
							}
						}
						int iFreeSoundID = g.temppreviewsoundoffset + 5;
						if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
						if (FileExist(t.visuals.sCombatMusicTrack.Get()) == 1)
						{
							LoadSound(t.visuals.sCombatMusicTrack.Get(), iFreeSoundID, 0, 1);
							if (t.gamevisuals.bEnableCombatMusicTrack)
							{
								if (SoundExist(iFreeSoundID) == 1)
								{
									LoopSound(iFreeSoundID);
									SetSoundVolume(iFreeSoundID, t.visuals.iCombatMusicTrackVolume);
								}
							}
						}
						#endif
					}
				}
				else
				{
					//  screen fade now handled in _physics_player_handlehealth
				}
			}
		}
	}
}

void physics_player_reset_underwaterstate ( void )
{
	visuals_underwater_off ( );
	t.playercontrol.inwaterstate = 0;
	t.playercontrol.drowntimestamp = 0;
}

void physics_player_listener ( void )
{
	ScaleListener (  5.0  ); RotateListener (  0,CameraAngleY(),0 );
	PositionListener ( CameraPositionX(), CameraPositionY(), CameraPositionZ() );
}

void physics_player_takedamage ( void )
{
	// Receives; tdamage, te, tDrownDamageFlag
	// Uses tDrownDamageFlag to avoid blood splats and other non drowning damage effects.
	// This is set to 0 after takedamage is called, so doesn't need to be unset elsewhere
	// before calling this sub

	#ifdef PRODUCTV3
	// 090419 - special VR mode also disables concepts of being damaged
	if (g.vrqcontrolmode != 0) return; //g.gvrmode == 3 ) return;
	#endif

	// Apply player shake, even if immune to damage
	if ((t.tdamage > 0 && t.player[t.plrid].health > 0 && t.player[t.plrid].health < 99999) || t.tdamage > 65000)
	{
		float fMoreShakeIfMeleeBased = 1.0f;
		if (t.te > 0)
		{
			int gunid = t.entityelement[t.te].eleprof.hasweapon;
			if (gunid > 0)
			{
				if (t.gun[gunid].weapontype == 51) // melee weapon type is 51
				{
					fMoreShakeIfMeleeBased = 3.0f;
				}
			}
		}
		else
		{
			// if no entity inflicting, assume external or generic damage, so more shake!
			fMoreShakeIfMeleeBased = 2.5f;
		}
		t.playercontrol.camerashake_f = (t.tdamage / 100.0f) * 25.0f * fMoreShakeIfMeleeBased;
	}

	// player cannot be damaged when immune!
	if ( t.huddamage.immunity>0  )  return;

	// quite early if in F9 editing mode
	if ( t.conkit.editmodeactive != 0  )  return;

	#ifdef WICKEDENGINE
	extern bool bInvulnerableMode;
	if (bInvulnerableMode)
	{
		if(t.player[t.plrid].health < 100)
		t.player[t.plrid].health = 100;
	}
	#endif

	//  Apply player health damage
	if (  (t.tdamage>0 && t.player[t.plrid].health>0 && t.player[t.plrid].health<99999) || t.tdamage>65000 ) 
	{
		//  Flag player damage in health regen code
		if (  t.playercontrol.regentime>0  )  t.playercontrol.regentime = Timer();

		//  Deduct health from player
		if (t.playercontrol.startstrength > 0)
		{
			// deduct player health
			t.player[t.plrid].health = t.player[t.plrid].health - t.tdamage;
		}

		// if NOT drowning, do usual damage stuff
		if ( t.tDrownDamageFlag  ==  0 ) 
		{
			// Instruct HUD about player damage
			if ( t.playercontrol.startviolent != 0 && g.quickparentalcontrolmode != 2 )
			{
				if ( t.playercontrol.thirdperson.enabled == 1 ) 
				{
					//  third person character produces blood
					if (  t.te != -1 ) 
					{
						t.ttobj1=t.entityelement[t.playercontrol.thirdperson.charactere].obj;
						t.decalx1=ObjectPositionX(t.ttobj1);
						t.decaly1=ObjectPositionY(t.ttobj1)+(ObjectSizeY(t.ttobj1)/2);
						t.decalz1=ObjectPositionZ(t.ttobj1);
						t.ttobj2=t.entityelement[t.te].obj;
						t.decalx2=ObjectPositionX(t.ttobj2);
						t.decaly2=ObjectPositionY(t.ttobj2)+(ObjectSizeY(t.ttobj2)/2);
						t.decalz2=ObjectPositionZ(t.ttobj2);
						t.ttdx_f=t.decalx1-t.decalx2;
						t.ttdy_f=t.decaly1-t.decaly2;
						t.ttdz_f=t.decalz1-t.decalz2;
						t.ttdd_f=Sqrt(abs(t.ttdx_f*t.ttdx_f)+abs(t.ttdy_f*t.ttdy_f)+abs(t.ttdz_f*t.ttdz_f));
						t.ttdx_f=t.ttdx_f/t.ttdd_f;
						t.ttdy_f=t.ttdy_f/t.ttdd_f;
						t.ttdz_f=t.ttdz_f/t.ttdd_f;
						g.decalx=t.decalx1-(t.ttdx_f*5.0);
						g.decaly=t.decaly1-(t.ttdy_f*5.0);
						g.decalz=t.decalz1-(t.ttdz_f*5.0);
						g.decalx=(g.decalx-10)+Rnd(20);
						g.decaly=(g.decaly-20)+Rnd(40);
						g.decalz=(g.decalz-10)+Rnd(20);
						for ( t.iter = 1 ; t.iter<=  1+Rnd(1); t.iter++ )
						{
							decal_triggerbloodsplat ( );
						}
					}
				}
				else
				{
					if (  t.te != -1 ) 
					{
						//  only if entity caused damage
						new_damage_marker(t.te,ObjectPositionX(t.entityelement[t.te].obj),ObjectPositionY(t.entityelement[t.te].obj),ObjectPositionZ(t.entityelement[t.te].obj),t.tdamage);
					}
					else
					{
						//  hurt from non-entity source
						for ( t.iter = 0 ; t.iter<=  9; t.iter++ )
						{
							placeblood(50,0,0,0,0);
						}
					}
				}
			}
			//  Trigger player grunt noise
			if (  t.playercontrol.startviolent != 0 && g.quickparentalcontrolmode != 2 ) 
			{
				/*
				t.tgruntfrequency=0;
				if ( 1 ) // 100316 - v1.121b3 - t.player[t.plrid].health<t.playercontrol.startstrength*0.25 )
				{
					#ifdef VRTECH
					//  one in three chance of grunting and been a while..
					if ( (DWORD)(Timer()+3000) > t.playercontrol.timesincelastgrunt ) 
					{
						if (  Rnd(4) == 1  )  t.tgruntfrequency = 1;
					}
					#else
					// always grunt when hit - so we know we have been hit, but don't overlap within 500ms!
					if ( (DWORD)(Timer()+500) > t.playercontrol.timesincelastgrunt ) 
					{
						t.tgruntfrequency = 1;
					}
					#endif
				}
				else
				{
					//  one in ten chance of grunting
					if (  Rnd(11) == 1  )  t.tgruntfrequency = 1;
				}
				if (  t.tgruntfrequency == 1 ) 
				{
				*/
				if ((DWORD)(Timer() + 250) > t.playercontrol.timesincelastgrunt)
				{
					//  only every one in three or if been a while since we grunted
					t.playercontrol.timesincelastgrunt = Timer();
					int iLastOne = t.tplrhurt;
					bool bHaveUniqueSound = false;
					while (bHaveUniqueSound == false)
					{
						int iRandomHurt = Rnd(15);
						switch (iRandomHurt)
						{
							case 0: t.tplrhurt = 1; break;
							case 1: t.tplrhurt = 2; break;
							case 2: t.tplrhurt = 3; break;
							case 3: t.tplrhurt = 4; break;
							case 4: t.tplrhurt = 8; break;
							case 5: t.tplrhurt = 9; break;
							case 6: t.tplrhurt = 10; break;
							case 7: t.tplrhurt = 16; break;
							case 8: t.tplrhurt = 21; break;
							case 9: t.tplrhurt = 22; break;
							case 10: t.tplrhurt = 23; break;
							case 11: t.tplrhurt = 24; break;
							case 12: t.tplrhurt = 25; break;
							case 13: t.tplrhurt = 26; break;
							case 14: t.tplrhurt = 27; break;
							case 15: t.tplrhurt = 28; break;
						}
						if (iLastOne != t.tplrhurt) bHaveUniqueSound = true;
					}
					t.tsnd = t.playercontrol.soundstartindex + t.tplrhurt;
					playinternalsound(t.tsnd);
				}
			}
		}

		#ifdef WICKEDENGINE
		extern bool bInvulnerableMode;
		if (bInvulnerableMode)
		{
			if (t.player[t.plrid].health < 100)
				t.player[t.plrid].health = 100;
		}
		#endif

		//  Check if player health at zero
		if (  t.player[t.plrid].health <= 0 ) 
		{
			if (  t.game.runasmultiplayer  ==  1 ) 
			{
				if (  t.tsteamwasnetworkdamage  ==  1 ) 
				{
					if (  t.entityelement[t.texplodesourceEntity].mp_networkkill  ==  1 ) 
					{
						//  inform of network kill
						mp_networkkill ( );
					}
				}
			}
			//  player looses a life
			t.player[t.plrid].health=0;
			if (  t.playercontrol.startlives>0 ) 
			{
				//  only reduce lives if using lives
				if (  t.game.runasmultiplayer == 0 ) 
				{
					t.player[t.plrid].lives=t.player[t.plrid].lives-1;
					if (  t.player[t.plrid].lives <= 0 ) 
					{
						t.player[t.plrid].lives=0;
					}
				}
			}
			if ( t.playercontrol.startviolent != 0 && g.quickparentalcontrolmode != 2 )
			{
				if ( t.tDrownDamageFlag == 0 ) 
				{
					//  player grunts in deadness if this isn't death by drowning
					playinternalsound(t.playercontrol.soundstartindex+1);
				}
			}
			// if camera was overriden, take it back
			g.luacameraoverride = 0;
			// if was frozen, unfreeze for the restore
			t.aisystem.processplayerlogic = 1;
			// restore player zoom
			t.plrzoominchange=1 ; t.plrzoomin_f=0.0;
			gun_playerdead ( );
			// start death sequence for player
			t.playercontrol.deadtime=Timer()+2000;
			// make sure all music is stopped
			if (  t.playercontrol.disablemusicreset == 0 ) 
			{
				music_resetall ( );
			}
			//  if third person, also create ragdoll of protagonist
			if (  t.playercontrol.thirdperson.enabled == 1 ) 
			{
				t.ttte=t.playercontrol.thirdperson.charactere;
				t.tdamageforce=0;
				t.entityelement[t.ttte].health=1 ; t.tdamage=1;
				t.entityelement[t.ttte].ry=ObjectAngleY(t.entityelement[t.ttte].obj);
				t.tskiplayerautoreject=1;
				entity_applydamage ( );
				t.tskiplayerautoreject=0;
			}
		}
	}
}

void physics_player_gotolastcheckpoint ( void )
{
	//  move player to last checkpoint (or start marker if no checkpoint)
	t.terrain.playerx_f=t.playercheckpoint.x;
	t.terrain.playery_f=t.playercheckpoint.y;
	t.terrain.playerz_f=t.playercheckpoint.z;
	t.terrain.playerax_f=0;
	t.terrain.playeray_f=t.playercheckpoint.a;
	t.camangy_f=t.terrain.playeray_f;
	t.terrain.playeraz_f=0;
	t.playercontrol.finalcameraangley_f=t.terrain.playeray_f;
	physics_resetplayer_core ( );
}

void physics_resetplayer_core ( void )
{
	//  Cannot restart under water!
	if (  t.hardwareinfoglobals.nowater == 0 ) 
	{
		if (  t.terrain.playery_f<t.terrain.waterliney_f ) 
		{
			t.terrain.playery_f=t.terrain.waterliney_f+t.terrain.adjaboveground_f;
		}
	}

	//  if the player was previous underwater, set them above water and switch off underwater effects
	physics_player_reset_underwaterstate ( );

	//  disable and setup player
	physics_disableplayer ( );
	physics_setupplayer ( );

	//  restore health
	t.player[t.plrid].health=t.playercontrol.startstrength;

	//  ressurection cease fire allows player to escape shooters when respawn
	t.playercontrol.ressurectionceasefire=Timer()+3000;

	//  reset vegetation
	t.completelyfillvegarea=1;
	grass_loop ( );

	//  fade in game screen again
	t.postprocessings.fadeinvalue_f=0.0;

	//  player is immune for a while
	t.huddamage.immunity=1000;

	//  reset death state
	t.playercontrol.deadtime=0;
	if ( g.luacameraoverride != 2 && g.luacameraoverride != 3 )
	{
		ZRotateCamera (  0,0 );
	}

	//  red screen effect finish
	t.playercontrol.redDeathFog_f = 0;
	SetVector4 (  g.terrainvectorindex,0,0,0,0 );
	t.tColorVector = g.terrainvectorindex ; postprocess_setscreencolor ( );

	//  Stop any blood HUD
	resetblood();
	resetdamagemarker();

	//  Deal with sounds if not disabled via script
	if (  t.playercontrol.disablemusicreset == 0 ) 
	{
		//  Stop any incidental music
		game_stopallsounds();
		//  Stop any looping projectile sounds
		weapon_projectile_reset ( );
		//  play default music
		music_playdefault ( );

		#ifdef VRTECH
		//  Restore any sounds from last checkpoint/start marker
		for ( t.s = g.soundbankoffset ; t.s<=  g.soundbankoffsetfinish; t.s++ )
		{
			if (  t.soundloopcheckpoint[t.s] == 1 ) 
			{
				if (  SoundExist(t.s) == 1 ) 
				{
					LoopSound (  t.s );
				}
			}
		}
		#else
		// LB-Issue-262: play/loop resume any sounds that had been playing at the time 
		for ( t.s = g.soundbankoffset; t.s <= g.soundbankoffsetfinish; t.s++ )
		{
			if (t.soundloopcheckpoint[t.s] != 0)
			{
				if (t.soundloopcheckpoint[t.s] > 0 && SoundExist(t.s) == 1)
				{
					if (t.soundloopcheckpoint[t.s] == 3)
						LoopSound(t.s);
					else if (t.soundloopcheckpoint[t.s] == 1)
						PlaySound(t.s);
				}
			}
		}
		#endif
	}

	//  ensure all markers and waypoints remain hidden
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entityprofile[t.entid].ismarker != 0 ) 
		{
			t.obj=t.entityelement[t.e].obj;
			if (  t.obj>0 ) 
			{
				if (  ObjectExist(t.obj) == 1 ) 
				{
					HideObject (  t.obj );
				}
			}
		}
	}
	waypoint_hideall ( );

	// if third person, resurrect ragdoll protagonist
	if ( t.playercontrol.thirdperson.enabled == 1 ) 
	{
		// ensure character protagonist is respawned
		t.e=t.playercontrol.thirdperson.charactere;
		entity_lua_spawn_core ( );
		physics_player_thirdpersonreset ( );
		//PE: Something is disable Z depth buffer when 3rd die ?, just reenable.
		//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/330
		EnableObjectZWrite(t.entityelement[t.e].obj);
		EnableObjectZDepth(t.entityelement[t.e].obj);
	}
}

void physics_player_thirdpersonreset ( void )
{
	//  convert to clone so can animate it (ragdoll makes it an instance)
	t.tentityconverttoclonenotshared=1;
	t.tte=t.e ; entity_converttoclone ( );
	t.tentityconverttoclonenotshared=0;
	t.charanimstate.e=t.e ; t.obj=t.entityelement[t.e].obj;
	entity_setupcharobjsettings ( );
	//  and reset any transitional states in animation system (avoid freezing it)
	t.smoothanim[t.charanimstate.obj].transition=0;
	t.smoothanim[t.charanimstate.obj].playstarted=0;
	//  camera has smoothing in third person, so reset this
	if ( g.luacameraoverride != 1 && g.luacameraoverride != 3 )
	{
		PositionCamera (  t.terrain.gameplaycamera,t.terrain.playerx_f,t.terrain.playery_f,t.terrain.playerz_f );
	}

	// ensure depth is not written so no motion blur
	// apply special character shader so can uniquely change shader constants
	// without affecting other NPCs and trees, etc
	if ( t.obj>0 ) 
	{
		if ( ObjectExist(t.obj) == 1 ) 
		{
			int tttentid = t.entityelement[t.e].bankindex;
			int ttsourceobj = g.entitybankoffset + tttentid;
			if ( ttsourceobj > 0 )
			{
				if ( ObjectExist ( ttsourceobj ) == 1 )
				{
					if ( GetNumberOfFrames ( ttsourceobj ) > 0 )
					{
						// third person is animating
						SetObjectEffect ( t.obj, g.thirdpersoncharactereffect );
						SetEffectConstantF (  g.thirdpersoncharactereffect,"DepthWriteMode",0.0f );			
					}
					else
					{
						// third person is non-animating
						SetObjectEffect ( t.obj, g.thirdpersonentityeffect );
						SetEffectConstantF ( g.thirdpersonentityeffect, "DepthWriteMode", 0.0f );
					}
				}
			}
		}
	}
	//  and also treat the vweap attachment too
	t.tattachmentobj=t.entityelement[t.e].attachmentobj;
	if (  t.tattachmentobj>0 ) 
	{
		if (  ObjectExist(t.tattachmentobj) == 1 ) 
		{
			SetObjectEffect (  t.tattachmentobj,g.thirdpersonentityeffect );
			SetEffectConstantF (  g.thirdpersonentityeffect,"DepthWriteMode",0.0f );
		}
	}
}

void physics_player_addweapon ( void )
{
	//  takes weaponindex
	t.tweaponisnew=0;
	//  check all weapon slots
	t.gotweapon=0;
	for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
	{
		if (  t.weaponslot[t.ws].got == t.weaponindex  )  t.gotweapon = t.ws;
	}
	if (  t.gotweapon == 0 ) 
	{
		//  check if we have a slot preference
		t.tweaponisnew=1;
		t.gotweaponpref=0;
		for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
		{
			if (  t.weaponslot[t.ws].pref == t.weaponindex  )  t.gotweaponpref = t.ws;
		}
		//  add weapon
		if (  t.gotweaponpref == 0 ) 
		{
			//  find free slot
			for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
			{
				if (  t.weaponslot[t.ws].pref == 0  )  break;
			}
			if (  g.forcedslot != 0 ) { t.ws = g.forcedslot  ; t.gotweaponpref = t.ws ; g.forcedslot = 0; }
			//  count weapons for maximum slots. If exceeded, prevent pick up.
			t.weaps=0;
			for ( t.count = 1 ; t.count <= 10 ; t.count++ )
			{
				if (  t.weaponslot[t.count].pref != 0  )  ++t.weaps;
			}
			if (  t.weaps >= g.maxslots  )  t.ws = 100;
			if (  t.ws <= 10 ) 
			{
				//  add weapon into free slot and create pref for it
				t.weaponslot[t.ws].pref=t.weaponindex;
				t.weaponhud[t.ws]=t.gun[t.weaponindex].hudimage;
				//  mark weapon with 'possible' entity that held this weapon (for equipment activation)
				g.firemodes[t.weaponindex][0].settings.equipmententityelementindex=t.autoentityusedtoholdweapon;
			}
			else
			{
				//  no room for weapon in available slots
				t.ws=0;
			}
		}
		else
		{
			t.ws=t.gotweaponpref;
		}

		//  switch to collected weapon
		if (  g.autoswap == 1 && t.ws>0 ) 
		{
			//  insert as slot weapon
			t.weaponslot[t.ws].got=t.weaponindex;
			t.weaponslot[t.ws].invpos=t.weaponinvposition;
			g.autoloadgun=t.weaponindex;
			t.weaponkeyselection=t.ws;
			t.gotweapon=t.ws;
		}

		//  place details of weapon in slot
		if (  t.ws>0 ) 
		{
			//  insert as slot weapon
			t.weaponslot[t.ws].got=t.weaponindex;
			t.weaponslot[t.ws].invpos=t.weaponinvposition;
			t.gotweapon=t.ws;
			if (  t.gunid == 0 ) 
			{
				//  if no gun held, auto select collected
				g.autoloadgun=t.weaponindex;
				t.weaponkeyselection=t.ws;
			}
		}
	}

	//  weapons start with some ammo
	if (  t.gotweapon>0 ) 
	{
		t.tgunid=t.weaponslot[t.gotweapon].pref;
		if (  t.gun[t.tgunid].settings.weaponisammo == 0 ) 
		{
			//if (  t.tqty == 0  )  t.tqty = 1;
			if (t.tqty < 0)  t.tqty = 0; //PE: no ammo should be possible.
			if (  t.weaponammo[t.gotweapon] == 0 && t.tweaponisnew == 1 ) 
			{
				//  provide some alternative ammo (weaponammo+10)
				if (  t.gun[t.tgunid].settings.modessharemags == 0 ) 
				{
					//  080415 - only if not sharing ammo
					t.taltqty=t.tqty;
					if (  t.taltqty>g.firemodes[t.tgunid][1].settings.reloadqty ) 
					{
						t.altpool=g.firemodes[t.tgunid][1].settings.poolindex;
						t.weaponammo[t.gotweapon+10]=g.firemodes[t.tgunid][1].settings.reloadqty;
						if (  t.altpool>0  )  t.ammopool[t.altpool].ammo = t.ammopool[t.altpool].ammo+(t.taltqty-g.firemodes[t.tgunid][1].settings.reloadqty); else t.weaponclipammo[t.gotweapon+10] = t.taltqty-g.firemodes[t.tgunid][1].settings.reloadqty;
					}
					else
					{
						if (  t.gun[t.tgunid].settings.addtospare == 0 ) 
						{
							t.weaponammo[t.gotweapon+10]=t.taltaty;
						}
						else
						{
							if (  t.gun[t.tgunid].settings.canaddtospare == 1 ) 
							{
								t.altpool=g.firemodes[t.tgunid][1].settings.poolindex;
								if (  t.altpool == 0  )  t.weaponclipammo[t.gotweapon+10] = t.weaponclipammo[t.gotweapon+10]+t.taltqty; else t.ammopool[t.altpool].ammo = t.ammopool[t.altpool].ammo+t.taltqty;
							}
							if (  t.gun[t.tgunid].settings.canaddtospare == 0  )  t.weaponammo[t.gotweapon+10] = t.taltqty;
						}
					}
				}
				//  provide some primary ammo
				if (  t.tqty>g.firemodes[t.tgunid][0].settings.reloadqty ) 
				{
					//  gun has MAX slots of ammo, cannot exceed this!
					t.tpool=g.firemodes[t.tgunid][0].settings.poolindex;
					t.weaponammo[t.gotweapon]=g.firemodes[t.tgunid][0].settings.reloadqty;
					if (  t.tpool>0 ) 
					{
						t.ammopool[t.tpool].ammo=t.ammopool[t.tpool].ammo+(t.tqty-g.firemodes[t.tgunid][0].settings.reloadqty);
					}
					else
					{
						t.weaponclipammo[t.gotweapon]=t.tqty-g.firemodes[t.tgunid][0].settings.reloadqty;
					}
				}
				else
				{
					if (  t.gun[t.tgunid].settings.addtospare == 0 ) 
					{
						t.weaponammo[t.gotweapon]=t.tqty;
					}
					else
					{
						//  new gunspec addition "addtospare" this will allow it so picking up ammo
						//  with an empty weapon won't add the ammo directly into the clip
						if (  t.gun[t.tgunid].settings.canaddtospare == 1 ) 
						{
							t.tpool=g.firemodes[t.tgunid][0].settings.poolindex;
							if (  t.tpool == 0 ) 
							{
								t.weaponclipammo[t.gotweapon]=t.weaponclipammo[t.gotweapon]+t.tqty;
							}
							else
							{
								t.ammopool[t.tpool].ammo=t.ammopool[t.tpool].ammo+t.tqty;
							}
						}
						if (  t.gun[t.tgunid].settings.canaddtospare == 0 ) 
						{
							t.gun[t.tgunid].settings.canaddtospare=1;
							t.weaponammo[t.gotweapon]=t.tqty;
						}
					}
				}
			}
			else
			{
				t.tpool=g.firemodes[t.tgunid][0].settings.poolindex;
				t.altpool=g.firemodes[t.tgunid][1].settings.poolindex;
				if (  t.tpool == 0  )  t.weaponclipammo[t.gotweapon] = t.weaponclipammo[t.gotweapon]+t.tqty; else t.ammopool[t.tpool].ammo = t.ammopool[t.tpool].ammo+t.tqty;
				if (  t.altpool == 0  )  t.weaponclipammo[t.gotweapon+10] = t.weaponclipammo[t.gotweapon+10]+t.taltqty; else t.ammopool[t.altpool].ammo = t.ammopool[t.altpool].ammo+t.taltqty;
			}
		}
	}

	//  refresh gun count
	physics_player_refreshcount ( );

	//  if collected weapon, and is empty, trigger reload if gun anim able
	if (  t.gotweapon>0 ) 
	{
		t.tgunid=t.weaponslot[t.gotweapon].pref;
		if (  t.weaponammo[t.gotweapon] == 0 ) 
		{
			if (  t.gunmode >= 5 && t.gunmode<31 ) 
			{
				t.gunmode=121;
			}
		}
	}

return;

}

void physics_player_removeweapon ( void )
{

	//  check all weapon slots
	for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
	{
		if (  t.weaponslot[t.ws].got == t.weaponindex  )  break;
	}
	if (  t.ws <= 10 ) 
	{
		//  Ensure gun is removed (if applicable)
		if (  t.gunid>0 && t.weaponslot[t.ws].got == t.gunid ) 
		{
			g.autoloadgun=0;
		}
		//  drop weapon from slot
		t.weaponslot[t.ws].got=0;
		t.weaponslot[t.ws].invpos=0;
	}

	//  refresh gun count
	physics_player_refreshcount ( );

return;

}

void physics_player_resetWeaponSlots( void )
{
	for (t.ws = 1; t.ws <= 10; t.ws++)
	{
		t.weaponslot[t.ws].got = 0;
		t.weaponslot[t.ws].invpos = 0;
	}
}

void physics_player_refreshcount ( void )
{

	//  refresh gun count
	t.guncollectedcount=0;
	for ( t.ws = 1 ; t.ws<=  10; t.ws++ )
	{
		if (  t.weaponslot[t.ws].got>0  )  ++t.guncollectedcount;
	}

return;
}


// Set the drawing mode of the physics debug drawer.
void physics_set_debug_draw(int iDraw)
{
	#ifdef WICKEDENGINE
	BPhys_SetDebugDrawerMode(iDraw, t.visuals.iPhysicsDebugDrawTransforms, t.visuals.iPhysicsDebugDrawConstraints);	
	// Clean up.
	if (!iDraw)
	{
		for (int i = 0; i < t.iPhysicsDebugObjects.size(); i++)
		{
			if(ObjectExist(t.iPhysicsDebugObjects[i])) DeleteObject(t.iPhysicsDebugObjects[i]);
		}
		t.iPhysicsDebugObjects.clear();
		t.iPhysicsDebugObjectsToUpdate.clear();
		t.iPhysicsCreatedDynamicMesh = 0;
		t.iPhysicsCreatedStaticMesh = 0;
		t.iPhysicsDebugMaxOffset = 0;
		t.physicsDebugDrawData = nullptr;
		t.iPhysicsDebugDynamicOffsets.clear();
		BPhys_ClearDebugDrawData();
	}
	#endif // WICKEDENGINE
}

void physics_create_debug_mesh(float* data, int count, bool bStatic, int offset)
{
#ifdef WICKEDENGINE
	if (!data) return;

	// Find a free memblock.
	int iFound = 0;
	for (int i = 1; i <= 257; i++)
	{
		if (MemblockExist(i) == 0)
		{
			iFound = i;
			break;
		}
	}
	if (iFound == 0) return;

	// Find a free mesh id.
	//int iMesh = -1;
	//for (int i = 1; i < g_iRawMeshListCount; i++)
	//{
	//	if (GetMeshExist(i) == 0)
	//	{
	//		iMesh = i;
	//		break;
	//	}
	//}
	//if (iMesh < 1) return;

	// Find a free object slot.
	int obj = -1;
	for (int i = g.physicsdebugdraweroffset; i < g.physicsdebugdraweroffset + 200; i++)
	{
		if (ObjectExist(i) == 0)
		{
			obj = i;
			break;
		}
	}
	if (obj < 0) return;

	int vertsize = 12;
	int iSizeBytes = 0;
	int vertexCount = count / 3;
	vertexCount *= 9;
	iSizeBytes = vertsize * vertexCount;
	iSizeBytes += 12; // Add header bytes.
	MakeMemblock(iFound, iSizeBytes);

	// Write the memblock header.
	// FVF format.
	WriteMemblockDWord(iFound, 0, GGFVF_XYZ);
	// Size of single vertex - 3 x float: position, 3 x float: normal,1 x DWORD: diffuse, 2 x float: tex coords = 36 bytes.
	WriteMemblockDWord(iFound, 4, 12);
	// Number of vertices in the mesh.
	WriteMemblockDWord(iFound, 8, vertexCount);

	float x0, x1, x2, x3, x4, x5;
	float y0, y1, y2, y3, y4, y5;
	float z0, z1, z2, z3, z4, z5;
	int v = 0;
	float p0[3];
	float p1[3];
	float points[18];

	// Every 6 elements of data contain two points on the physics object.
	// data can contain the vertices of a previously created mesh, so start at offset (count at the last mesh creation).
	for (int i = offset; i <= count - 6; i += 6)
	{
		p0[0] = data[i]; p0[1] = data[i + 1]; p0[2] = data[i + 2];
		p1[0] = data[i + 3]; p1[1] = data[i + 4]; p1[2] = data[i + 5];

		physics_debug_make_prism_between_points(p0, p1, points);
		
		// Corners of the prism.
		x0 = points[0]; y0 = points[1]; z0 = points[2];
		x1 = points[3]; y1 = points[4]; z1 = points[5];
		x2 = points[6]; y2 = points[7]; z2 = points[8];
		x3 = points[9]; y3 = points[10]; z3 = points[11];
		x4 = points[12]; y4 = points[13]; z4 = points[14];
		x5 = points[15]; y5 = points[16]; z5 = points[17];

		// Form the faces of the prism from the corners.
		physics_add_vert_to_debug_mesh(x0, y0, z0, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x2, y2, z2, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x3, y3, z3, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x2, y2, z2, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x4, y4, z4, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x3, y3, z3, v, iFound);
		v++;

		physics_add_vert_to_debug_mesh(x1, y1, z1, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x2, y2, z2, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x4, y4, z4, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x2, y2, z2, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x5, y5, z5, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x4, y4, z4, v, iFound);
		v++;

		physics_add_vert_to_debug_mesh(x0, y0, z0, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x1, y1, z1, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x5, y5, z5, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x0, y0, z0, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x5, y5, z5, v, iFound);
		v++;
		physics_add_vert_to_debug_mesh(x3, y3, z3, v, iFound);
		v++;
	}

	// Keep track of the newly created object so we can delete it later.
	t.iPhysicsDebugObjects.push_back(obj);

	// Dynamic objects will need their vertices updated to prevent recreating the mesh each frame.
	if (!bStatic)
	{
		t.iPhysicsDebugObjectsToUpdate.push_back(obj);
		t.iPhysicsDebugDynamicOffsets.push_back(offset);
	}

	CreateMeshFromMemblock(obj, iFound);
	MakeObject(obj, obj, 0);

	sObject* pObject = GetObjectData(obj);
	int verts = pObject->ppMeshList[0]->dwVertexCount;

	SetObject(obj, 0, 0, 0, 0, 0, 0, 0);
	SetObjectCollisionOff(obj);
	DisableObjectZWrite(obj);
	SetObjectLight(obj, 0);
	SetObjectMask(obj, 1);

	// 150817 - GUI shader with DIFFUSE element included
	SetObjectEffect(obj, g.guidiffuseshadereffectindex);

	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
	WickedCall_RemoveObject(pObject);
	WickedCall_AddObject(pObject);
	WickedCall_TextureObject(pObject, NULL);
	WickedCall_SetObjectCastShadows(pObject, false);
	WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
	// wicked object does not use per-vertex diffuse color, so find first color and apply to whole object
	DWORD diffuse = Rgb(0, 255, 0);
	SetObjectDiffuse(obj, Rgb(0, 255, 0));

	// set alpha and transparency of this object
	SetObjectTransparency(obj, 2);
	SetAlphaMappingOn(obj, 75);// 25 );

	DeleteMemblock(iFound);

#endif // WICKEDENGINE
}

void physics_add_vert_to_debug_mesh(float x, float y, float z, int v, int memblock)
{
#ifdef WICKEDENGINE
	//  Position of vertex in memblock
	int pos = 12 + (v * 12);

	//  Set vertex position
	WriteMemblockFloat(memblock, pos + 0, x);
	WriteMemblockFloat(memblock, pos + 4, y);
	WriteMemblockFloat(memblock, pos + 8, z);
#endif // WICKEDENGINE
}

void physics_update_debug_mesh(float* data, int count, int objectID, int offsetLower, int offsetUpper)
{
#ifdef WICKEDENGINE
	float x0, x1, x2, x3, x4, x5;
	float y0, y1, y2, y3, y4, y5;
	float z0, z1, z2, z3, z4, z5;
	int v = 0;
	float p0[3];
	float p1[3];
	float points[18];

	if (!data) return;

	sObject* pObject = GetObjectData(objectID);
	if (!pObject)
	{
		return;
	}
	
	sMesh* pMesh = pObject->ppMeshList[0];
	if (!pMesh)
	{
		return;
	}
	
	LockVertexDataForLimbCore(objectID, 0, 1);
	for (int i = offsetLower; i <= offsetUpper - 6; i += 6)
	{
		// Every 6 elements of data contain two points on the physics object.
		p0[0] = data[i]; p0[1] = data[i + 1]; p0[2] = data[i + 2];
		p1[0] = data[i + 3]; p1[1] = data[i + 4]; p1[2] = data[i + 5];

		physics_debug_make_prism_between_points(p0, p1, points);

		// Corners of the prism.
		x0 = points[0]; y0 = points[1]; z0 = points[2];
		x1 = points[3]; y1 = points[4]; z1 = points[5];
		x2 = points[6]; y2 = points[7]; z2 = points[8];
		x3 = points[9]; y3 = points[10]; z3 = points[11];
		x4 = points[12]; y4 = points[13]; z4 = points[14];
		x5 = points[15]; y5 = points[16]; z5 = points[17];

		// Make each face of the prism (two triangles per face).
		SetVertexDataPosition(v++, x0, y0, z0);
		SetVertexDataPosition(v++, x2, y2, z2);
		SetVertexDataPosition(v++, x3, y3, z3);
		SetVertexDataPosition(v++, x2, y2, z2);
		SetVertexDataPosition(v++, x4, y4, z4);
		SetVertexDataPosition(v++, x3, y3, z3);
							 
		SetVertexDataPosition(v++, x1, y1, z1);
		SetVertexDataPosition(v++, x2, y2, z2);
		SetVertexDataPosition(v++, x4, y4, z4);
		SetVertexDataPosition(v++, x2, y2, z2);
		SetVertexDataPosition(v++, x5, y5, z5);
		SetVertexDataPosition(v++, x4, y4, z4);
							 
		SetVertexDataPosition(v++, x0, y0, z0);
		SetVertexDataPosition(v++, x1, y1, z1);
		SetVertexDataPosition(v++, x5, y5, z5);
		SetVertexDataPosition(v++, x0, y0, z0);
		SetVertexDataPosition(v++, x5, y5, z5);
		SetVertexDataPosition(v++, x3, y3, z3);
	}

	if (pMesh->dwVertexCount > count)
	{
		// If the amount of vertices has been reduced, fill the remaining vertex data with invalid values.
		// Prevents having to reallocate.
		//for (int i = v; i < pMesh->dwVertexCount; i++) SetVertexDataPosition(v++, 0.0f, 0.0f, 0.0f);
	}

	UnlockVertexData();

	WickedCall_UpdateMeshVertexData(pMesh);
#endif // WICKEDENGINE
}

void physics_debug_make_prism_between_points(float* p0, float* p1, float* results, float thickness)
{
#ifdef WICKEDENGINE
	XMFLOAT3 a;
	XMFLOAT3 b;
	XMVECTOR positionA;
	XMVECTOR direction;
	XMVECTOR normal;
	XMFLOAT3 worldUp;
	XMVECTOR temp;
	XMVECTOR dot;
	XMVECTOR right;
	XMVECTOR up;

	// for terrain physics debug!
	//float thickness = 0.12f;
	//float thickness = 5.12f; see from afar :)
	XMVECTOR points[6];

	// Calculate direction between the two points.
	a = XMFLOAT3(p0[0], p0[1], p0[2]);
	b = XMFLOAT3(p1[0], p1[1], p1[2]);
	direction = XMLoadFloat3(&b) - XMLoadFloat3(&a);
	normal = XMVector3Normalize(direction);

	// Calculate a vector that allows us to get another vector perpendicular to the direction vector.
	worldUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
	temp = XMLoadFloat3(&worldUp);
	dot = XMVector3Dot(normal, temp);
	positionA = XMLoadFloat3(&a);
	if (XMVectorGetX(dot) >= 0.98f || XMVectorGetX(dot) <= -0.98f)
	{
		// Normal and temp have the same direction, so change temp.
		worldUp = XMFLOAT3(1.0f, 0.0f, 0.0f);
		temp = XMLoadFloat3(&worldUp);
	}

	// Calculate the other two basis vectors, giving 3 orthogonal vectors.
	right = XMVector3Cross(normal, temp);
	right = XMVector3Normalize(right);

	up = XMVector3Cross(right, normal);
	up = XMVector3Normalize(up);

	// Calculate 3 points about the normal vector (that pass through the right and up vectors)
	for (int j = 0; j < 3; j++)
	{
		// 2.0944 radians = 120 degrees for 3 points around the positionA.
		points[j] = positionA + (thickness * cosf(2.0944f * j) * right) + (thickness * sinf(2.0944f * j) * up);

		// Calculate the corresponding point on the other triangle by moving the first point towards the second.
		points[j + 3] = points[j] + direction;
	}

	for (int j = 0; j < 6; j++)
	{
		results[3 * j + 0] = XMVectorGetX(points[j]);
		results[3 * j + 1] = XMVectorGetY(points[j]);
		results[3 * j + 2] = XMVectorGetZ(points[j]);
	}
#endif // WICKEDENGINE
}

// For editor only.
void physics_debug_add_object(int objectID)
{
	if(BPhys_GetDebugObjectCount() == 0) ODEStart();
	BPhys_AddDebugSingleObject(objectID);
	t.tphyobj = objectID;
	t.entid = objectID;
	physics_importer_create_temp();
}

// For editor only.
void physics_debug_remove_object(int objectID)
{
	BPhys_RemoveDebugSingleObject(objectID);
	if (BPhys_GetDebugObjectCount() == 0) ODEEnd();
}

// For editor only.
void physics_debug_draw()
{
	BPhys_DrawDebugObjects();
	physics_render_debug_meshes();
}

// For test game and editor.
void physics_render_debug_meshes()
{
	#ifdef WICKEDENGINE
	if (BPhys_GetDebugDrawerMode() != 0)
	{
		int elementCount = 0;

		if (t.iPhysicsCreatedStaticMesh == 0)
		{
			// Get all of the points in the static physics geometry.
			float* data = BPhys_GetStaticDebugDrawData(elementCount);

			// Create the static physics mesh.
			physics_create_debug_mesh(data, elementCount, true, 0);

			// Flag set to ensure we only get the static geometry once.
			t.iPhysicsCreatedStaticMesh = 1;

			// Clear the static physics data so we can get the dynamic data.
			BPhys_ClearDebugDrawData();
		}
		else
		{
			// Get the updated debug data.
			t.physicsDebugDrawData = BPhys_GetDynamicDebugDrawData(elementCount);

			if (t.iPhysicsCreatedDynamicMesh == 0)
			{
				// Create the debug mesh if it hasn't already been created.
				physics_create_debug_mesh(t.physicsDebugDrawData, elementCount, false, t.iPhysicsDebugMaxOffset);
				t.iPhysicsDebugMaxOffset = elementCount;
				t.iPhysicsCreatedDynamicMesh = 1;
			}
			else
			{
				// Update all of the debug meshes.
				for (int i = 0; i < t.iPhysicsDebugObjectsToUpdate.size(); i++)
				{
					int iOffsetUpper = elementCount;
					if (i < t.iPhysicsDebugObjectsToUpdate.size() - 1) iOffsetUpper = t.iPhysicsDebugDynamicOffsets[i + 1];

					physics_update_debug_mesh(t.physicsDebugDrawData, elementCount, t.iPhysicsDebugObjectsToUpdate[i],
						t.iPhysicsDebugDynamicOffsets[i], iOffsetUpper);
				}

				if (elementCount > t.iPhysicsDebugMaxOffset)
				{
					// Object has been added to the physics world, so a new physics mesh must be created.
					t.iPhysicsCreatedDynamicMesh = 0;
				}
			}
		}
	}
	#endif
}

void physics_importer_create_temp()
{
	#ifdef WICKEDENGINE
	if (t.importer.collisionshape == 0) t.tshape = 0;   // box
	if (t.importer.collisionshape == 1) t.tshape = 1;   // polygon
	if (t.importer.collisionshape == 2) t.tshape = 2;   // sphere
	if (t.importer.collisionshape == 3) t.tshape = 3;   // cylinder
	if (t.importer.collisionshape == 4) t.tshape = 9;   // hull
	if (t.importer.collisionshape == 5) t.tshape = 21;  // character collission
	if (t.importer.collisionshape == 6) t.tshape = 50;  // tree collision
	if (t.importer.collisionshape == 7) t.tshape = 11;  // no collision
	#endif

	if (t.tstatic == 1) // now allow physics entities in multiplayer || t.game.runasmultiplayer == 1 ) 
	{
		// if static, need to ensure FIXNEWY pivot is respected
		if (t.tstatic == 1)
		{
			//t.tstaticfixnewystore_f = ObjectAngleY(t.tphyobj);
			//RotateObject(t.tphyobj, ObjectAngleX(t.tphyobj), ObjectAngleY(t.tphyobj) + t.entityprofile[t.entid].fixnewy, ObjectAngleZ(t.tphyobj));
		}
		//  create the physics now
		
		if (t.tshape >= 1000 && t.tshape < 2000)
		{
			ODECreateStaticBox(t.tphyobj, t.tshape - 1000);
		}
		else if (t.tshape >= 2000 && t.tshape < 3000)
		{
			ODECreateStaticTriangleMesh(t.tphyobj, t.tshape - 2000);
		}
		else if (t.tshape == 1)
		{
			ODECreateStaticBox(t.tphyobj);
		}
		else if (t.tshape == 6)
		{
			ODECreateStaticSphere(t.tphyobj);
		}
		else if (t.tshape == 7)
		{
			ODECreateStaticCylinder(t.tphyobj);
		}
		else if (t.tshape == 2 || t.tshape == 9 || t.tshape == 10)
		{
			if (t.tshape == 2)
			{
				if (t.tcollisionscaling != 100)
				{
					ODECreateStaticTriangleMesh(t.tphyobj, -1, t.tcollisionscaling);
				}
				else
				{
					ODECreateStaticTriangleMesh(t.tphyobj);
				}
			}
			else
			{
				if (t.tshape == 10)
				{
					ODECreateStaticTriangleMesh(t.tphyobj, -1, t.tcollisionscaling, 2);
				}
				else
				{
					ODECreateStaticTriangleMesh(t.tphyobj, -1, t.tcollisionscaling, 1);
				}
			}
		}
		else if (t.tshape == 3)
		{
			physics_setuptreecylinder();
		}
		// tshape 4 is a list of physics objects from the importer
		else if (t.tshape == 4)
		{
			physics_setupimportershapes();
		}
		// if static, restore object before leaving
		if (t.tstatic == 1)
		{
			RotateObject(t.tphyobj, ObjectAngleX(t.tphyobj), t.tstaticfixnewystore_f, ObjectAngleZ(t.tphyobj));
		}
	}
	else
	{
		// objects will fall through Floor (  if they are perfectly sitting on it )
		PositionObject(t.tphyobj, ObjectPositionX(t.tphyobj), ObjectPositionY(t.tphyobj) + 0.1, ObjectPositionZ(t.tphyobj));

		if (t.tshape == 6)
		{
			// Sphere
			ODECreateDynamicSphere(t.tphyobj, t.tweight, t.tfriction, 0.01f);
		}
		else if (t.tshape == 7)
		{
			// Cylinder
			ODECreateDynamicCylinder(t.tphyobj, t.tweight, t.tfriction, 0.01f);
		}
		else if (t.tshape == 9)
		{
			// Dynamic convex hull
			ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 1);
		}
		else if (t.tshape == 10)
		{
			// Dynamic hull composition
			ODECreateDynamicTriangleMesh(t.tphyobj, t.tweight, t.tfriction, -1, 2);
		}
		else
		{
			// box
			ODECreateDynamicBox(t.tphyobj, -1, 0, t.tweight, t.tfriction, -1);
		}
	}
}

int physics_getmaterialindex (float fX, float fZ)
{
	#ifdef WICKEDENGINE
	int iMatID = GGTerrain_GetMaterialIndex(fX, fZ) & 0xFF;
	int iMaterialIndex = 0;
	if (iMatID >= 0 && iMatID < 32) iMaterialIndex = g_iMapMatIDToMatIndex[iMatID];
	return iMaterialIndex;
	#else
	return 0;
	#endif
}

#ifdef WICKEDENGINE
int physics_rayintersecttree (float fX, float fY, float fZ, float fToX, float fToY, float fToZ)
{
	float fHeightOfTreeDetect = 200.0f; //LB: Can be improved with geometry awareness (slower)
	for (int vti = 0; vti < g_VTreeObj.size(); vti++)
	{
		bool bRayTooLowOrHigh = false;
		if (fY < g_VTreeObj[vti].fY && fToY < g_VTreeObj[vti].fY) bRayTooLowOrHigh = true;
		if (fY > g_VTreeObj[vti].fY + fHeightOfTreeDetect && fToY > g_VTreeObj[vti].fY + fHeightOfTreeDetect) bRayTooLowOrHigh = true;
		if (bRayTooLowOrHigh==false)
		{
			// ray crosses Y area presence of tree
			float fCX = g_VTreeObj[vti].fX;
			float fCZ = g_VTreeObj[vti].fZ;
			float r = 15.0f;
			double x0 = fCX, y0 = fCZ;
			double x1 = fX, y1 = fZ;
			double x2 = fToX, y2 = fToZ;
			double A = y2 - y1;
			double B = x1 - x2;
			double C = x2 * y1 - x1 * y2;
			double a = (A*A) + (B*B);
			double b, c, d;
			const double eps = 1e-14;
			if (fabs(B) >= eps) 
			{
				b = 2 * (A * C + A * B * y0 - (B*B) * x0);
				c = (C*C) + 2 * B * C * y0 - (B*B) * ((r*r) - (x0*x0) - (y0*y0));
			}
			else 
			{
				b = 2 * (B * C + A * B * x0 - (A*A) * y0);
				c = (C*C) + 2 * A * C * x0 - (A*A) * ((r*r) - (x0*x0) - (y0*y0));
			}
			d = (b*b) - 4 * a * c;
			if (d > 0 )
			{
				return 1; // hit a tree
			}
		}
	}
	return 0;
}
#endif
