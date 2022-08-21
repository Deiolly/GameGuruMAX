//----------------------------------------------------
//--- GAMEGURU - M-Game
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"
#include "cOccluderThread.h"
#include "CGfxC.h"
#include "DarkLUA.h"
#include <algorithm>
#include <string>
#include <fstream>
#include <iterator>
#include "master.h"

#ifdef ENABLEIMGUI
	//PE: GameGuru IMGUI.
	#include "..\Imgui\imgui.h"
	#include "..\Imgui\imgui_impl_win32.h"
	#include "..\Imgui\imgui_gg_extras.h"
#endif

#ifdef WICKEDENGINE
#include ".\\..\..\\Guru-WickedMAX\\GPUParticles.h"
using namespace GPUParticles;
#include "GGTerrain\GGTerrain.h"
#include "GGTerrain\GGTrees.h"
using namespace GGTerrain;
using namespace GGTrees;
#include "GGRecastDetour.h"
GGRecastDetour g_RecastDetour;
bool g_bShowRecastDetourDebugVisuals = false;
int old_render_params2 = 0;
#endif

#ifdef STORYBOARD
extern int g_Storyboard_First_Level_Node;
extern int g_Storyboard_Current_Level;
extern char g_Storyboard_First_fpm[256];
extern char g_Storyboard_Current_fpm[256];
extern char g_Storyboard_Current_lua[256];
extern StoryboardStruct Storyboard;
#endif
// Externs
extern bool g_occluderOn;
extern bool	g_occluderf9Mode;

#ifdef VRTECH
extern bool g_bInTutorialMode;
#endif

// Globals
bool g_bInEditor = true;
int g_iMasterRootState = 0;
int g_iActivelyUsingVRNow = 0;
int g_iInGameMenuState = 0;
extern Master master;

// 
//  Game Module to manage all game flow
// 

#ifndef PRODUCTCLASSIC
extern int iLaunchAfterSync;
#endif
extern bool commonexecutable_loop_for_game(void);

void gameexecutable_init(void)
{
	// start game init code
	int iEXEGameIsVR = 0;
#ifdef WICKEDENGINE
	if (g.vrglobals.GGVREnabled > 0) iEXEGameIsVR = 1;

	//PE: Load in any imgui media used in standalone, special mode tabtab...
	SetMipmapNum(1); //PE: mipmaps not needed.
	image_setlegacyimageloading(true);

	LoadImage("editors\\uiv3\\ccp-none.png", CCP_NONE);
	LoadImage("editors\\uiv3\\ccp-empty.png", CCP_EMPTY);

	image_setlegacyimageloading(false);
	SetMipmapNum(-1);


#endif
	editor_previewmap_initcode(iEXEGameIsVR);
	#ifndef PRODUCTCLASSIC
	iLaunchAfterSync = 201;
	#endif
}

void gameexecutable_loop(void)
{
	// loop with special modes used when in test game or standalone game
	commonexecutable_loop_for_game();
}

void gameexecutable_finish(void)
{
	#ifdef WICKEDENGINE
	// no longer need fragmentation handler for 64bit engine
	#else
	// Only if not quitting standalone
	bool bUseFragmentationMainloop = false;
	if (t.game.allowfragmentation == 0 || t.game.allowfragmentation == 2)
	{
		if(t.game.allowfragmentation_mainloop != 0)
			bUseFragmentationMainloop = true;
	}
	if ( t.game.masterloop != 0 || bUseFragmentationMainloop )
	{
		// 250619 - very large levels can fragment 32 bit memory after a few levels
		// so this mode will restart the executable, and launch the new level
		// crude solution until 64 bit allows greater memory referencing
		if ( t.game.allowfragmentation == 2 )
		{
			// next level load or back to main menu (both require relaunch)
			if ( strlen(t.game.jumplevel_s.Get()) > 0 )
			{
				// next level
				//timestampactivity(0, "Next level...");
				SoundDestructor();
				SetDir("..");
				LPSTR pEXEName = Appname();
				cstr pCommandLineString = cstr("-reloadstandalonelevel") + t.game.jumplevel_s + ":" + Str(t.luaglobal.gamestatechange);
				ExecuteFile ( pEXEName, pCommandLineString.Get(), "", 0 );
				Sleep(8000);
				return;
			}
			else
			{
				// new main menu (except if t.game.masterloop == 0 in which case we are quitting)
				t.game.allowfragmentation = 0;
			}
		}

		// 131115 - standalone game sessions fragment memory over time, so launch new instance
		// of the game executable (with silencing command line) and then quit this 'fragmented'
		// session after a few seconds to allow for a decent transition
		if ( t.game.allowfragmentation == 0 )
		{
			// replaced master loop with EXE relaunch
			//timestampactivity(0, "Relaunch...");
			SoundDestructor();
			SetDir("..");
			LPSTR pEXEName = Appname();
			ExecuteFile ( pEXEName, "-reloadstandalone", "", 0 );
			Sleep(8000);
			return;
		}
	}
	#endif

	// Free before exit app
	mp_free ( );
}

// The occluder thread
cOccluderThread*	g_pOccluderThread = NULL;
float				g_fOccluderCamVelX = 0.0f;
float				g_fOccluderCamVelZ = 0.0f;
float				g_fOccluderLastCamX = 0.0f;
float				g_fOccluderLastCamZ = 0.0f;

#ifdef VRTECH
void game_scanfornewavatars ( bool bDynamicallyRecreateCharacters )
{
	// add any character creator player avatars in
	if ( t.bTriggerAvatarRescanAndLoad == true )
	{
		for ( t.tcustomAvatarCount = 0 ; t.tcustomAvatarCount <= MP_MAX_NUMBER_OF_PLAYERS-1; t.tcustomAvatarCount++ )
		{
			// check if there is a custom avatar (and not loaded)
			if ( t.mp_playerAvatars_s[t.tcustomAvatarCount] != "" && t.mp_playerAvatarLoaded[t.tcustomAvatarCount] == false ) 
			{
				// there is so lets built a temp fpe file from it
				t.ent_s = g.rootdir_s+"entitybank\\user\\charactercreatorplus\\customAvatar_"+Str(t.tcustomAvatarCount)+".fpe";
				t.avatarFile_s = t.ent_s;
				t.avatarString_s = t.mp_playerAvatars_s[t.tcustomAvatarCount];
				characterkitplus_makeMultiplayerCharacterCreatorAvatar ( );
				entity_addtoselection_core ( );
				characterkitplus_removeMultiplayerCharacterCreatorAvatar ( );
				t.tubindex[t.tcustomAvatarCount+2]=t.entid;
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].ischaracter=0;
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].collisionmode=12;
				// No lua script for player chars
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].aimain_s = "";
				// avatar is now loaded
				t.mp_playerAvatarLoaded[t.tcustomAvatarCount] = true;

				// additionally, when triggered, replace actual objects with new created ones above (for dynamic loading)
				if ( bDynamicallyRecreateCharacters == true )
				{
					t.e = t.mp_playerEntityID[t.tcustomAvatarCount];
					if ( t.e > 0 )
					{
						// update entity element with new character object (dynamically loaded during game)
						t.entityelement[t.e].bankindex = t.entid;

						// update entity object itself
						t.tupdatee = t.e; 
						entity_updateentityobj ( );
					}
				}
			}
		}
		t.bTriggerAvatarRescanAndLoad = false;

		// refreshes object masks of avatar heads
		t.visuals.refreshshaders = 1;
	}
}
#endif

#ifdef WIP_PROLOADLEVELTEXTURES
std::vector<std::string> preload_setup;
#endif

#ifdef WICKEDENGINE

uint32_t GetObjectNavMeshVertexCount( int iID )
{
	if ( !ConfirmObject ( iID ) ) return 0;

	sObject* pObject = g_ObjectList [ iID ];

	uint32_t numTotalVertices = 0;
	
	for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
	{
		sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
		if (pFrame)
		{
			sMesh* pMesh = pFrame->pMesh;
			if (pMesh)
			{
				if (pMesh->dwIndexCount == 0)
				{
					numTotalVertices += pMesh->dwVertexCount;
				}
				else
				{
					// has index data
					numTotalVertices += pMesh->dwIndexCount;
				}
			}
		}
	}

	return numTotalVertices;
}

void GetObjectNavMeshVertices( int iID, float* pVertices )
{
	if ( !ConfirmObject ( iID ) ) return;

	sObject* pObject = g_ObjectList [ iID ];

	uint32_t numTotalVertices = 0;

	// faces
	for (int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++)
	{
		sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
		if (pFrame)
		{
			sMesh* pMesh = pFrame->pMesh;
			if (pMesh)
			{			
				GGMATRIX matThisFrame;
				GGMatrixTranslation	(&matThisFrame, pFrame->vecOffset.x, pFrame->vecOffset.y, pFrame->vecOffset.z);
				
				float* pVertData = (float*) pMesh->pVertexData;
				uint32_t stride = pMesh->dwFVFSize / 4;

				if (pMesh->dwIndexCount == 0)
				{
					// has no indice data
					
					for (DWORD dwV = 0; dwV < pMesh->dwVertexCount; dwV ++)
					{
						GGVECTOR3 vecXYZ = GGVECTOR3( pVertData[0], pVertData[1], pVertData[2] );
						GGVec3TransformCoord(&vecXYZ, &vecXYZ, &matThisFrame);

						uint32_t index = (numTotalVertices + dwV) * 3;

						pVertices[ index + 0 ] = vecXYZ.x;
						pVertices[ index + 1 ] = vecXYZ.y;
						pVertices[ index + 2 ] = vecXYZ.z;

						pVertData += stride;
					}

					numTotalVertices += pMesh->dwVertexCount;
				}
				else
				{
					// has indice data
					for (DWORD dwI = 0; dwI < pMesh->dwIndexCount; dwI++ )
					{
						DWORD dwV = pMesh->pIndices[ dwI ];
						
						uint32_t vIndex = dwV * stride;
						GGVECTOR3 vecXYZ = GGVECTOR3( pVertData[vIndex], pVertData[vIndex + 1], pVertData[vIndex + 2] );
						GGVec3TransformCoord(&vecXYZ, &vecXYZ, &matThisFrame);

						uint32_t index = (numTotalVertices + dwI) * 3;

						pVertices[ index + 0 ] = vecXYZ.x;
						pVertices[ index + 1 ] = vecXYZ.y;
						pVertices[ index + 2 ] = vecXYZ.z;
					}

					numTotalVertices += pMesh->dwIndexCount;
				}
			}
		}
	}
}

void game_createnavmeshfromlevel ( bool bForceGeneration )
{
	// area around any entities on map
	GGVECTOR3 vecMinArea = GGVECTOR3( 999999,  999999,  999999);
	GGVECTOR3 vecMaxArea = GGVECTOR3(-999999, -999999, -999999);
	float editableSize = GGTerrain_GetEditableSize();

	// work out hash for all "static" objects
	double dSuperHash = 0;
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		int entid = t.entityelement[e].bankindex;
		// build the bounding box using both static objects and characters, in case there are no static objects near a character
		if (t.entityelement[e].staticflag == 1 || t.entityprofile[entid].ischaracter == 1)
		{
			int iObj = t.entityelement[e].obj;
			if (iObj > 0)
			{
				if (ObjectExist(iObj) == 1)
				{
					// establish bounds of static objects
					GGVECTOR3 vecPos = GGVECTOR3(ObjectPositionX(iObj), ObjectPositionY(iObj), ObjectPositionZ(iObj));
					if ( vecPos.x > editableSize || vecPos.x < -editableSize || vecPos.z > editableSize || vecPos.z < -editableSize ) continue;
					if (vecPos.x > vecMaxArea.x) vecMaxArea.x = vecPos.x;
					if (vecPos.z > vecMaxArea.z) vecMaxArea.z = vecPos.z;
					if (vecPos.x < vecMinArea.x) vecMinArea.x = vecPos.x;
					if (vecPos.z < vecMinArea.z) vecMinArea.z = vecPos.z;

					// calculating superhash
					dSuperHash += iObj;
					dSuperHash += vecPos.x;
					dSuperHash += vecPos.y;
					dSuperHash += vecPos.z;
					dSuperHash += ObjectAngleX(iObj);
					dSuperHash += ObjectAngleY(iObj);
					dSuperHash += ObjectAngleZ(iObj);
					dSuperHash += ObjectScaleX(iObj);
					dSuperHash += ObjectScaleY(iObj);
					dSuperHash += ObjectScaleZ(iObj);
				}
			}
		}
	}
		
	if (dSuperHash == 0)
	{
		// failing all else, just use camera position
		vecMinArea.x = vecMaxArea.x = CameraPositionX(0);
		vecMinArea.z = vecMaxArea.z = CameraPositionZ(0);
	}

	// toggling trees means recalcing for tree obstacles
	dSuperHash += ggtrees_global_params.draw_enabled;

	// exit early if no change detected in static arrangement
	static double dLastSuperHash = -1;
	if (bForceGeneration == false)
	{
		if (dLastSuperHash != -1 && dSuperHash == dLastSuperHash) return;
	}
	dLastSuperHash = dSuperHash;

	// create monster object representing level geometry
	int iBuildAllLevelMesh = g.meshgeneralwork;
	int iBuildAllLevelObj = g.tempobjectoffset + 0;
	if (ObjectExist(iBuildAllLevelObj) == 1) DeleteObject(iBuildAllLevelObj);

	// always expand bounding box in case characters move around
	vecMinArea.x -= 1000;
	vecMaxArea.x += 1000;
	vecMinArea.z -= 1000;
	vecMaxArea.z += 1000;

	/*
	// FOR PAUL - initial code to scan ALL objects in the level and reveal the coordinates of the characters that NEED navmesh support
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		int entid = t.entityelement[e].bankindex;
		if (t.entityprofile[entid].ischaracter == 1)
		{
			// this character has a starting position, and an assumed range represented as a multiple of their given view range
			GGVECTOR3 vecCharacterPosition = GGVECTOR3(t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z);
			float fAssumedGameRange = t.entityelement[e].eleprof.conerange * 4;
		}
	}
	*/

	// terrain geometry
	int iLimbIndex = 1;
	int iTerrainObj = iBuildAllLevelObj;
	bool bCreateTerrainMesh = true;
	if (bCreateTerrainMesh == true )
	{
		// generate polygons for required area
		int iFirstLOD = 2;
		GGVECTOR3* pvecVerts = NULL;
		int iTerrainFloorVertexCount = GGTerrain_GetTriangleList(&pvecVerts, vecMinArea.x, vecMinArea.z, vecMaxArea.x, vecMaxArea.z, iFirstLOD);
		if (iTerrainFloorVertexCount > 0 )
		{
			// divide up into 16-bit size meshes
			int iVertStart =  0;
			int iPiecesCount = iTerrainFloorVertexCount / 65535;
			for (int iPieces = 0; iPieces <= iPiecesCount; iPieces++)
			{
				// create a mesh from polygons
				int newobj = g.tempobjectoffset + 1;
				int iTerrainNavMeshObj = newobj;
				if (iPieces == 0) iTerrainNavMeshObj = iBuildAllLevelObj;
				if (ObjectExist(iTerrainNavMeshObj) == 1) DeleteObject (iTerrainNavMeshObj);
				MakeObjectPlane (iTerrainNavMeshObj, 1, 1);
				sObject* pObject = GetObjectData(iTerrainNavMeshObj);
				delete pObject->ppMeshList[0];
				sMesh* pMesh = new sMesh();

				// full piece size or remaining
				DWORD dwVertexCount = iTerrainFloorVertexCount - iVertStart;
				if (dwVertexCount > 65535) dwVertexCount = 65535;

				// resize object for this
				DWORD dwIndexCount = dwVertexCount;
				SetupMeshFVFData (pMesh, GGFVF_XYZ, dwVertexCount, dwIndexCount, false);
				pMesh->iPrimitiveType = GGPT_TRIANGLELIST;
				pMesh->iDrawVertexCount = pMesh->dwVertexCount;
				pMesh->iDrawPrimitives = dwIndexCount / 3;
				pObject->ppMeshList[0] = pMesh;
				pObject->pFrame->pMesh = pMesh;
				float* pVertPtr = (float*)pMesh->pVertexData;
				for (int v = 0; v < dwVertexCount; v++)
				{
					*(pVertPtr + 0) = pvecVerts[iVertStart + v].x;
					*(pVertPtr + 1) = pvecVerts[iVertStart + v].y;
					*(pVertPtr + 2) = pvecVerts[iVertStart + v].z;
					pVertPtr += 3;
				}
				WORD* pIndicePtr = (WORD*)pMesh->pIndices;
				for (int i = 0; i < dwIndexCount; i++)
				{
					*(pIndicePtr) = i;
					pIndicePtr++;
				}

				// for next piece (if any)
				iVertStart += 65535;

				// add this terrain piece as mesh to main OBJ
				if (iPieces > 0)
				{
					// after iniutial creation, add new meshes
					MakeMeshFromObject(iBuildAllLevelMesh, iTerrainNavMeshObj);
					AddLimb(iBuildAllLevelObj, iLimbIndex, iBuildAllLevelMesh);
					iLimbIndex++;

					// remove tempo mesh/obj
					if (ObjectExist(iTerrainNavMeshObj) == 1) DeleteObject (iTerrainNavMeshObj);
				}
			}

			// ensure main OIBJ is hidden during process
			HideObject(iBuildAllLevelObj);

			// free when complete
			if (pvecVerts)
			{
				delete pvecVerts;
				pvecVerts = NULL;
			}
		}
		else
		{
			// inexplicavble that any part of the terrain measuring 1000x1000 produces NO polys!!
			MakeObjectPlane (iBuildAllLevelObj, 1, 1);
			HideObject(iBuildAllLevelObj);
		}
	}
	else
	{
		MakeObjectPlane (iBuildAllLevelObj, 1, 1);
		HideObject(iBuildAllLevelObj);
	}

	// all static objects in level
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		if (t.entityelement[e].staticflag == 1)
		{
			int iObj = t.entityelement[e].obj;
			int iBankindex = t.entityelement[e].bankindex;
			bool bValid = true;
			if (t.entityprofile[iBankindex].ismarker != 0) bValid = false;
			if (t.entityprofile[iBankindex].collisionmode == 11) bValid = false;
			if (t.entityprofile[iBankindex].collisionmode == 12) bValid = false;
			if (t.entityprofile[iBankindex].isammo == 1) bValid = false;
			if (t.entityprofile[iBankindex].isweapon_s.Len() > 1) bValid = false;

			if (bValid && iObj > 0 && ObjectExist(iObj) == 1)
			{
				GGVECTOR3 vecPos = GGVECTOR3(ObjectPositionX(iObj), ObjectPositionY(iObj), ObjectPositionZ(iObj));
				if ( vecPos.x > editableSize || vecPos.x < -editableSize || vecPos.z > editableSize || vecPos.z < -editableSize ) continue;

				//PE: Add physics shapes here.
				if (GetMeshExist(iBuildAllLevelMesh) == 1) DeleteMesh(iBuildAllLevelMesh);
				if (iBankindex > 0 && t.entityprofile[iBankindex].collisionmode >= 50 && t.entityprofile[iBankindex].collisionmode < 60)
				{
					int newobj = g.tempobjectoffset + 1;
					if (ObjectExist(newobj)) DeleteObject(newobj);
					MakeObjectCylinder(newobj ,1);

					t.tSizeY_f = ObjectSizeY(iObj, 1);

					//  if have ABS position from AI OBSTACLE calc, use that instead
					if (t.entityelement[e].abscolx_f != -1)
					{
						t.tFinalX_f = t.entityelement[e].abscolx_f;
						t.tFinalZ_f = t.entityelement[e].abscolz_f;
					}
					else
					{
						t.tFinalX_f = ObjectPositionX(iObj);
						t.tFinalZ_f = ObjectPositionZ(iObj);
					}
					t.tFinalY_f = ObjectPositionY(iObj) + (t.tSizeY_f / 2.0);

					//  if have ABS radius from AI OBSTACLE calc, use that instead
					if (t.entityelement[e].abscolradius_f != -1)
					{
						t.tSizeX_f = t.entityelement[e].abscolradius_f;
						t.tSizeZ_f = t.entityelement[e].abscolradius_f;
					}
					else
					{
						t.tSizeX_f = 20;
						t.tSizeZ_f = 20;
					}

					//  increase size by 25%
					t.tSizeX_f = t.tSizeX_f*1.25;
					t.tSizeZ_f = t.tSizeZ_f*1.25;

					ScaleObject(newobj, t.tSizeX_f*100, t.tSizeY_f*100, t.tSizeZ_f*100);
					MakeMeshFromObject(iBuildAllLevelMesh, newobj);
					AddLimb(iBuildAllLevelObj, iLimbIndex, iBuildAllLevelMesh);
					OffsetLimb(iBuildAllLevelObj, iLimbIndex, ObjectPositionX(iObj), ObjectPositionY(iObj)+(t.tSizeY_f*0.5), ObjectPositionZ(iObj));
					iLimbIndex++;
				}
				else
				{
					// to make a cleaner NAVMESH, stairs are better as ramps, so use OBJ if present for this purpose
					// if object uses convex hull, see if there is an OBJ we can swap inplace of the objects full mesh
					int iObjToUseForNavMesh = iObj;
					bool bHeavyPOlyShapesShouldCheckForOBJ = false;
					if (t.entityprofile[iBankindex].collisionmode == 1) bHeavyPOlyShapesShouldCheckForOBJ = true; // polygon
					if (t.entityprofile[iBankindex].collisionmode == 8) bHeavyPOlyShapesShouldCheckForOBJ = true; // polygon with OBJ
					if (t.entityprofile[iBankindex].collisionmode == 9) bHeavyPOlyShapesShouldCheckForOBJ = true; // convex hull
					if (t.entityprofile[iBankindex].collisionmode == 10) bHeavyPOlyShapesShouldCheckForOBJ = true; // hull decomp
					if (bHeavyPOlyShapesShouldCheckForOBJ == true)
					{
						char pNoFPE[MAX_PATH];
						strcpy(pNoFPE, t.entitybank_s[iBankindex].Get());
						pNoFPE[strlen(pNoFPE) - 4] = 0;
						char pOBJCollisionMesh[MAX_PATH];
						sprintf(pOBJCollisionMesh, "%s\\Files\\entitybank\\%s.obj", g.fpscrootdir_s.Get(), pNoFPE);
						GG_GetRealPath(pOBJCollisionMesh, 0);
						if (FileExist(pOBJCollisionMesh) == 0)
						{
							sprintf(pOBJCollisionMesh, "%s\\Files\\entitybank\\%s_COL.obj", g.fpscrootdir_s.Get(), pNoFPE);
							GG_GetRealPath(pOBJCollisionMesh, 0);
						}
						if (FileExist(pOBJCollisionMesh) == 1)
						{
							// can optimize this by keeping the low poly OBJ, perhaps add to DBO as a LOD??
							if (ObjectExist(g.temp2objectoffset) == 1) DeleteObject(g.temp2objectoffset);
							LoadObject (pOBJCollisionMesh, g.temp2objectoffset);
							iObjToUseForNavMesh = g.temp2objectoffset;
							RotateObject(iObjToUseForNavMesh, ObjectAngleX(iObj), ObjectAngleY(iObj), ObjectAngleZ(iObj));
							ScaleObject(iObjToUseForNavMesh, ObjectScaleX(iObj), ObjectScaleY(iObj), ObjectScaleZ(iObj));
						}
						// for now I disable OBJ creation, the ideal way is to get an artist to provide a good OBJ mesh for nav mesh
						//else
						//{
						//	// all hulls should have an OBJ collision mesh, so save one now
						//	// and it also gives user opportunity to load OBJ into 3D modeller and improve the mesh for navmesh
						//	GG_GetRealPath(pOBJCollisionMesh, 1);
						//	float fStoreAX = ObjectAngleX(iObj);
						//	float fStoreAY = ObjectAngleY(iObj);
						//	float fStoreAZ = ObjectAngleZ(iObj);
						//	float fStoreSX = ObjectScaleX(iObj);
						//	float fStoreSY = ObjectScaleY(iObj);
						//	float fStoreSZ = ObjectScaleZ(iObj);
						//	RotateObject(iObj, 0, 0, 0);
						//	ScaleObject(iObj, 100, 100, 100);
						//	SaveObject(pOBJCollisionMesh, iObj);
						//	RotateObject(iObj, fStoreAX, fStoreAY, fStoreAZ);
						//	ScaleObject(iObj, fStoreSX, fStoreSY, fStoreSZ);
						//}							
					}
					// regular mesh from object
					MakeMeshFromObject(iBuildAllLevelMesh, iObjToUseForNavMesh);
					AddLimb(iBuildAllLevelObj, iLimbIndex, iBuildAllLevelMesh);
					OffsetLimb(iBuildAllLevelObj, iLimbIndex, ObjectPositionX(iObj), ObjectPositionY(iObj), ObjectPositionZ(iObj));
					iLimbIndex++;
				}
			}
		}
	}

	// simple obstacle object/mesh to punch into navmesh
	if (ObjectExist(g.temp2objectoffset)) DeleteObject(g.temp2objectoffset); //PE: Got 7007 if any tree cylinder are created.
	MakeObjectBox (g.temp2objectoffset, 10, 200, 10);
	MakeMeshFromObject (g.meshgeneralwork2, g.temp2objectoffset);
	DeleteObject(g.temp2objectoffset);

	// solve issue of large delay when nothing in scene (hack for now, need to find real reason)
	if (iLimbIndex == 1)
	{
		AddLimb(iBuildAllLevelObj, iLimbIndex, g.meshgeneralwork2);
		OffsetLimb(iBuildAllLevelObj, iLimbIndex, 0 - 800, 0 - 5, 0 - 800);
		iLimbIndex++;
	}

	// add virtual trees into the navmesh (quick solution)
	if (ggtrees_global_params.draw_enabled == 1)
	{
		float fPlayAreaRadiusX = (vecMaxArea.x - vecMinArea.x) / 2;
		float fPlayAreaRadiusZ = (vecMaxArea.z - vecMinArea.z) / 2;
		float fPlayAreaCenterX = vecMinArea.x + fPlayAreaRadiusX;
		float fPlayAreaCenterZ = vecMinArea.z + fPlayAreaRadiusZ;
		float fPlayAreaRadius = fPlayAreaRadiusX;
		if (fPlayAreaRadiusZ > fPlayAreaRadius) fPlayAreaRadius = fPlayAreaRadiusZ;
		GGTrees::GGTreePoint* pOutPoints = NULL;
		int iTreeCount = GGTrees::GGTrees_GetClosest (fPlayAreaCenterX, fPlayAreaCenterZ, fPlayAreaRadius, &pOutPoints);
		if (pOutPoints)
		{
			for (int n = 0; n < iTreeCount; n++)
			{
				GGVECTOR3 vecTreePos = GGVECTOR3(pOutPoints[n].x, pOutPoints[n].y, pOutPoints[n].z);
				AddLimb(iBuildAllLevelObj, iLimbIndex, g.meshgeneralwork2);
				OffsetLimb(iBuildAllLevelObj, iLimbIndex, vecTreePos.x, vecTreePos.y, vecTreePos.z);
				iLimbIndex++;
			}
			delete pOutPoints;
		}
	}

	float* pVertices = 0;
	uint32_t numVertices = 0;

	const bool saveObject = false;
	if ( saveObject )
	{
		#ifdef WICKEDENGINE
		char pProgressStr[256];
		sprintf_s(pProgressStr, 256, "SAVING NAV MESH TOPOGRAPHY - %d\\100 Complete", 11);
		void printscreenprompt(char*);
		printscreenprompt(pProgressStr);
		#endif

		// save level geometry object as OBJ
		LPSTR pAllLevelGeometryFilename = "levelbank\\testmap\\rawlevelgeometry.obj";
		if (FileExist(pAllLevelGeometryFilename)) DeleteFileA(pAllLevelGeometryFilename);
		SaveObjectEx(pAllLevelGeometryFilename, iBuildAllLevelObj, true); //PE: Use 8% of the time optimize it.
	}
	else
	{
		#ifdef WICKEDENGINE
		char pProgressStr[256];
		sprintf_s(pProgressStr, 256, "GENERATING NAV MESH VERTICES - %d\\100 Complete", 11);
		void printscreenprompt(char*);
		printscreenprompt(pProgressStr);
		#endif

		numVertices = GetObjectNavMeshVertexCount( iBuildAllLevelObj );
		pVertices = new float[ numVertices*3 ];
		GetObjectNavMeshVertices( iBuildAllLevelObj, pVertices );
	}

	// can now delete massive object
	DeleteObject(iBuildAllLevelObj);

	// generate nav mesh using recast on OBJ
	g_RecastDetour.buildall( pVertices, numVertices );

	if ( pVertices ) delete [] pVertices;
}

void game_updatenavmeshsystem(void)
{
	// render any debug objects (such as nav mesh)
	if (g_bShowRecastDetourDebugVisuals == true)
		g_RecastDetour.handleDebugRender();
	else
		g_RecastDetour.cleanupDebugRender();
}
#endif

void game_masterroot_gameloop_initcode(int iUseVRTest)
{
	//PE: For this to work we need to support decrypting files.
	#ifdef WIP_PROLOADLEVELTEXTURES
	//PE: START level proloader here.
	if (t.game.gameisexe == 1 || t.game.runasmultiplayer == 1)
	{
		//Get fpm level name.
		if (t.game.runasmultiplayer == 1)
		{
			// Multiplayer FPM loading
			g.projectfilename_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
		}
		else
		{
			// Single player
			if (Len(t.game.jumplevel_s.Get()) > 0)
			{
				// can override jumplevel with 'advanced warning level filename' when LOAD level from MAIN MENU
				if (strcmp(t.game.pAdvanceWarningOfLevelFilename, "") != NULL)
				{
					t.game.jumplevel_s = t.game.pAdvanceWarningOfLevelFilename;
					strcpy(t.game.pAdvanceWarningOfLevelFilename, "");
				}

				// work out first level from exe name (copied to jumplevel_s)
				g.projectfilename_s = g.mysystem.mapbank_s + t.game.jumplevel_s;
				if (cstr(Lower(Right(g.projectfilename_s.Get(), 4))) != ".fpm")
					g.projectfilename_s = g.projectfilename_s + ".fpm";
			}

			// 050316 - if not there, try all subfolders
			if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + g.projectfilename_s).Get()) == 0)
			{
				// go into mapbank folder
				cstr tthisold_s = "";
				tthisold_s = GetDir();
				SetDir(g.mysystem.mapbankAbs_s.Get());

				// scan for ALL files/folders
				ChecklistForFiles();
				for (int c = 1; c <= ChecklistQuantity(); c++)
				{
					if (ChecklistValueA(c) != 0)
					{
						// only folders
						cstr tfolder_s = ChecklistString(c);
						if (tfolder_s != "." && tfolder_s != "..")
						{
							// skip . and .. folders
							cstr newlevellocation = g.mysystem.mapbank_s + tfolder_s + "\\" + t.game.jumplevel_s;
							if (cstr(Lower(Right(newlevellocation.Get(), 4))) != ".fpm")
								newlevellocation = newlevellocation + ".fpm";

							// does this guessed file location exist
							if (FileExist(cstr(g.fpscrootdir_s + "\\Files\\" + newlevellocation).Get()) == 1)
							{
								// found the level inside a nested folder
								g.projectfilename_s = newlevellocation;
								break;
							}
						}
					}
				}
				SetDir(tthisold_s.Get());
			}

		}
		preload_setup.clear();
		std::string sString = g.projectfilename_s.Get();
		replaceAll(sString, "\\", "_");
		replaceAll(sString, ".", "_");
		replaceAll(sString, ":", "_");

		//PE: Save preload information.
		char szRealFilename[MAX_PATH];
		strcpy_s(szRealFilename, MAX_PATH, "preloadinfo\\");
		strcat(szRealFilename, sString.c_str());
		GG_GetRealPath(szRealFilename, 1);

		// Open the File
		std::ifstream in(szRealFilename);
		if (in)
		{
			std::string str;
			while (std::getline(in, str))
			{
				if (str.size() > 0)
					preload_setup.push_back(str);
			}
			in.close();
		}
		if (preload_setup.size() > 0)
		{
			timestampactivity(0, "_using_preload_informations");

			image_preload_files_wait();
			image_preload_files_start();
			for (int i = 0; i < preload_setup.size(); i++)
			{
				image_preload_files_add( (char *) preload_setup[i].c_str(), -1);
			}
			image_preload_files_finish();
		}
	}
	#endif
	// also hide rendering of 3D while we set up a new level
	SyncMaskOverride ( 0 );

	// Loading page
	timestampactivity(0,"_titles_loadingpageupdate");
	if ( t.game.gameisexe == 1 ) 
	{
		//PE: When getting here everything was faded out.
		t.postprocessings.fadeinvalue_f = 1.0f;
		//g.globals.hidelowfpswarning = 0; // this overrides the SETUP.INI setting
		HideOrShowLUASprites(false);
		EnableAllSprites(); // the disable is called in DarkLUA by ResetFade() black out command when load game position

		//titles_loadingpage ( );
		timestampactivity(0,"LUA script : loading");
		sky_hide();
		t.game.levelloadprogress = 0;
		titleslua_init ( );
		titleslua_main ( "loading" );
		sky_show();
		titleslua_main_loopcode();
		t.game.levelloadprogress=0  ; titles_loadingpageupdate ( );
	}

	// Extract level files from FPM
	if ( t.game.runasmultiplayer == 1 ) 
	{
		// Multiplayer FPM loading
		g.projectfilename_s=g.mysystem.editorsGrideditAbs_s+"__multiplayerlevel__.fpm";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm";
		t.trerfeshvisualsassets=1;
		mapfile_loadproject_fpm ( );
		t.game.jumplevel_s="";
	}
	else
	{
		// Single player
		if ( Len(t.game.jumplevel_s.Get())> 0 ) //PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
		{
			// can override jumplevel with 'advanced warning level filename' when LOAD level from MAIN MENU
			if ( strcmp ( t.game.pAdvanceWarningOfLevelFilename, "" ) != NULL )
			{
				t.game.jumplevel_s = t.game.pAdvanceWarningOfLevelFilename;
				strcpy ( t.game.pAdvanceWarningOfLevelFilename, "" );
			}

			// work out first level from exe name (copied to jumplevel_s)
			g.projectfilename_s = g.mysystem.mapbank_s + t.game.jumplevel_s;
			//PE: In new code ifused can now include mapbank
			if (pestrcasestr(t.game.jumplevel_s.Get(), "mapbank\\"))
				g.projectfilename_s = t.game.jumplevel_s;
			if ( cstr(Lower(Right(g.projectfilename_s.Get(),4))) != ".fpm" )
				g.projectfilename_s=g.projectfilename_s+".fpm";

			// 050316 - if not there, try all subfolders
			if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+g.projectfilename_s).Get()) == 0 ) 
			{
				// go into mapbank folder
				cstr tthisold_s =  "";
				tthisold_s=GetDir();
				SetDir ( g.mysystem.mapbankAbs_s.Get() );

				// scan for ALL files/folders
				ChecklistForFiles (  );
				for ( int c = 1 ; c<=  ChecklistQuantity(); c++ )
				{
					if (  ChecklistValueA(c) != 0 ) 
					{
						// only folders
						cstr tfolder_s = ChecklistString(c);
						if ( tfolder_s != "." && tfolder_s != ".." ) 
						{
							// skip . and .. folders
							cstr newlevellocation = g.mysystem.mapbank_s + tfolder_s + "\\" + t.game.jumplevel_s;
							if ( cstr(Lower(Right(newlevellocation.Get(),4))) != ".fpm" )
								newlevellocation = newlevellocation + ".fpm";

							// does this guessed file location exist
							if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+newlevellocation).Get()) == 1 ) 
							{
								// found the level inside a nested folder
								g.projectfilename_s = newlevellocation; 
								break;
							}
						}
					}
				}
				SetDir ( tthisold_s.Get() );
			}

			// finally load the level in
			mapfile_loadproject_fpm ( );
			t.visuals=t.gamevisuals;
			t.game.jumplevel_s="";
		}
	}

	// reload gunspecs
	if (g.reloadWeaponGunspecs == 1)
	{
		gun_scaninall_dataonly();
	}

	// we first load extra guns into gun array EARLY (ahead of entity data load which assigns gunids to isweapon hasweapon)
	gun_tagmpgunstolist ( );

	// help keep progress bar instant and moving
	#ifdef WICKEDENGINE
	char pProgressStr[256];
	sprintf_s(pProgressStr, 256, "PREPARING TEST LEVEL - %d\\100 Complete", 1);
	void printscreenprompt(char*);
	printscreenprompt(pProgressStr);
	#endif

	// just load the entity data for now (rest in _game_loadinleveldata)
	timestampactivity(0,"_game_loadinentitiesdatainlevel");
	if ( t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
	{
		//  extra precaution, delete any old entities and LM objects
		if ( t.game.runasmultiplayer == 1 ) 
		{
			entity_delete ( );
			lm_removeold ( );
		}
		#ifdef WIP_PROLOADLEVELTEXTURES
		//PE: Record preload informations here.
		preload_setup.clear();
		#endif

		game_loadinentitiesdatainlevel ( );

		#ifdef WIP_PROLOADLEVELTEXTURES
		std::string sString = g.projectfilename_s.Get();
		replaceAll(sString, "\\", "_");
		replaceAll(sString, ".", "_");
		replaceAll(sString, ":", "_");

		//PE: Save preload information.
		char szRealFilename[MAX_PATH];
		strcpy_s(szRealFilename, MAX_PATH, "preloadinfo\\");
		strcat(szRealFilename, sString.c_str());
		GG_GetRealPath(szRealFilename, 1);
		std::ofstream output_file(szRealFilename);
		std::ostream_iterator<std::string> output_iterator(output_file, "\n");
		std::copy(preload_setup.begin(), preload_setup.end(), output_iterator);
		preload_setup.clear();
		#endif
	}

	// Load any extra material sounds associated with new entities (i.e. material(m).usedinlevel=1?)
	// NOTE: Level can collect materials (and material depth) and apply here to quicken material loader (2s)
	material_loadsounds ( 0 );

	// if multiplayer, detect spawn positions and add extra UBER characters
	if ( t.game.runasmultiplayer == 1 ) 
	{
		// these are the multiplayer start markers
		t.tnumberofstartmarkers = 0;
		g.mp.team = 0;
		g.mp.coop = 0;
		for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
		{
			t.mpmultiplayerstart[t.tc].active=0;
		}
		t.plrindex=1;
		t.tfoundAMultiplayerScript = 0;
		for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
		{
			// reset all updates
			t.entityelement[t.e].mp_updateOn = 0;
			t.entityelement[t.e].mp_isLuaChar = 0;
			t.entityelement[t.e].mp_rotateType = 0;
			t.entid=t.entityelement[t.e].bankindex;
			if ( t.entid>0 ) 
			{
				#ifdef VRTECH
				#else
				if ( t.entityprofile[t.entid].ismarker == 7 && t.plrindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
				{
					// to ensure mp game script always runs from any distance
					t.entityelement[t.e].eleprof.phyalways = 1;
					if ( t.entityelement[t.e].eleprof.aimain_s == "" ) 
					{
						t.entityelement[t.e].eleprof.aimain_s = "multiplayer_firstto10.lua";
					}
					if ( t.entityelement[t.e].eleprof.teamfield  !=  0 ) 
					{
						g.mp.team = 1;
					}
	
					//  only let one marker end up with a script otherwise we end up running the same script 8 times
					if ( t.tfoundAMultiplayerScript == 0 ) 
					{
						t.tfoundAMultiplayerScript = 1;
						// 12032015 0XX - Team Multiplayer - check for team mode
						if ( FileOpen(3)  ==  1  )  CloseFile (  3 );
						t.strwork = "" ; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
						OpenToRead (  3, t.strwork.Get() );
						g.mp.friendlyfireoff = 0;
						while ( FileEnd(3) == 0 ) 
						{
							t.tScriptLine_s = ReadString (  3 );
							t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
							if (  FindSubString(t.tScriptLine_s.Get(),"setmultiplayergamefriendlyfireoff") > 0 && FindSubString(t.tScriptLine_s.Get(),"--SetMultiplayerGameFriendlyFireOff")  <=  0 && FindSubString(t.tScriptLine_s.Get(),"-- SetMultiplayerGameFriendlyFireOff") <=  0 ) 
							{
								g.mp.friendlyfireoff = 1;
							}
						}
						CloseFile (  3 );
					}
					else
					{
						t.entityelement[t.e].eleprof.aimain_s = "";
					}
					++t.plrindex;
				}
				else
				{
					if (  t.entityprofile[t.entid].ischaracter  ==  0 ) 
					{
						if (  FileOpen(3)  ==  1  )  CloseFile (  3 );
						if (  t.entityelement[t.e].eleprof.aimain_s  !=  "" ) 
						{
							if (  FileExist(t.entityelement[t.e].eleprof.aimain_s.Get())  ==  1 ) 
							{
								t.strwork = ""; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
								OpenToRead (  3, t.strwork.Get() );
								while (  FileEnd(3)  ==  0 ) 
								{
									t.tScriptLine_s = ReadString (  3 );
									t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
									//  are the using ai?
									if (  FindSubString(t.tScriptLine_s.Get(),"AIEntityGoToPosition") > 0 ) 
									{
										t.entityelement[t.e].mp_isLuaChar = 1;
									}
								}
								CloseFile (  3 );
							}
						}
					}
				}
				#endif
			}
		}

		// Build multiplayer start markers
		t.thaveTeamAMarkers = 0;
		t.thaveTeamBMarkers = 0;
		t.tmpstartindex = 1;
		for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if ( t.entid>0 ) 
			{
				if ( t.entityprofile[t.entid].ismarker == 7 && t.tmpstartindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
				{
					// add start markers for free for all or team a
					if ( t.entityelement[t.e].eleprof.teamfield < 2 ) 
					{
						// a spawn GetPoint ( for the multiplayer )
						t.mpmultiplayerstart[t.tmpstartindex].active=1;
						t.mpmultiplayerstart[t.tmpstartindex].x=t.entityelement[t.e].x;
						// added 10 onto the y otherwise the players fall through the ground
						t.mpmultiplayerstart[t.tmpstartindex].y=t.entityelement[t.e].y+50;
						t.mpmultiplayerstart[t.tmpstartindex].z=t.entityelement[t.e].z;
						t.mpmultiplayerstart[t.tmpstartindex].angle=t.entityelement[t.e].ry;
						t.thaveTeamAMarkers = 1;
						++t.tnumberofstartmarkers;
						++t.tmpstartindex;
					}
				}
			}
		}
		// add team b markers if in team mode
		#ifdef VRTECH
		#else
		if ( g.mp.team == 1 ) 
		{
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if ( t.entid>0 ) 
				{
					if ( t.entityprofile[t.entid].ismarker == 7 && t.tmpstartindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
					{
						// add start markers for team b
						if ( t.entityelement[t.e].eleprof.teamfield == 2 ) 
						{
							// a spawn GetPoint (  for the multiplayer )
							t.mpmultiplayerstart[t.tmpstartindex].active=1;
							t.mpmultiplayerstart[t.tmpstartindex].x=t.entityelement[t.e].x;
							// added 10 onto the y otherwise the players fall through the ground
							t.mpmultiplayerstart[t.tmpstartindex].y=t.entityelement[t.e].y+50;
							t.mpmultiplayerstart[t.tmpstartindex].z=t.entityelement[t.e].z;
							t.mpmultiplayerstart[t.tmpstartindex].angle=t.entityelement[t.e].ry;
							t.thaveTeamBMarkers = 1;
							++t.tnumberofstartmarkers;
							++t.tmpstartindex;
						}
					}
				}
			}
		}
		#endif

		// check for coop mode
		g.mp.coop = 0;
		#ifdef VRTECH
		#else
		if ( g.mp.team == 1 ) 
		{
			if ( (t.thaveTeamAMarkers  ==  1 && t.thaveTeamBMarkers  ==  0) || (t.thaveTeamAMarkers  ==  0 && t.thaveTeamBMarkers  ==  1) || (t.thaveTeamAMarkers  ==  0 && t.thaveTeamBMarkers  ==  0) ) 
			{
				g.mp.coop = 1;
				mp_setupCoopTeam ( );
			}
		}
		#endif

		// perhaps it is a solo game with a start maker only
		#ifdef VRTECH
		bool bHaveRegularStartMarker = false;
		#endif
		if ( g.mp.coop == 0 && t.tnumberofstartmarkers == 0 ) 
		{
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if ( t.entid>0 ) 
				{
					if ( t.entityprofile[t.entid].ismarker == 1 ) 
					{
						// a spawn GetPoint ( for the multiplayer )
						#ifdef VRTECH
						bHaveRegularStartMarker = true;
						#endif
						t.mpmultiplayerstart[1].active=1;
						t.mpmultiplayerstart[1].x=t.entityelement[t.e].x;
						// added 10 onto the y otherwise the players fall through the ground
						t.mpmultiplayerstart[1].y=t.entityelement[t.e].y+50;
						t.mpmultiplayerstart[1].z=t.entityelement[t.e].z;
						t.mpmultiplayerstart[1].angle=t.entityelement[t.e].ry;
						t.entityelement[t.e].eleprof.phyalways = 1;

						#ifdef VRTECH
						#else
						// switch it to multiplayer script
						t.entityelement[t.e].eleprof.aimain_s = "multiplayer_firstto10.lua";
						t.tnumberofstartmarkers = 1;
						g.mp.coop = 1;
						g.mp.team = 1;
						mp_setupCoopTeam ( );

						// Check for friendly fire off
						if ( FileOpen(3) == 1 )  CloseFile ( 3 );
						t.strwork ="" ; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
						OpenToRead (  3, t.strwork.Get() );
						g.mp.friendlyfireoff = 0;
						while (  FileEnd(3)  ==  0 ) 
						{
							t.tScriptLine_s = ReadString (  3 );
							t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
							if (  FindSubString(t.tScriptLine_s.Get(),"setmultiplayergamefriendlyfireoff") > 0 && FindSubString(t.tScriptLine_s.Get(),"--SetMultiplayerGameFriendlyFireOff")  <=  0 && FindSubString(t.tScriptLine_s.Get(),"-- SetMultiplayerGameFriendlyFireOff") <=  0 ) 
							{
								g.mp.friendlyfireoff = 1;
							}
						}
						CloseFile (  3 );
						#endif
					}
				}
			}
		}

		//  if multiplayer and not coop, disable ai characters
		#ifdef PHOTONMP
			// Photon retains all characters in map
		#else
			if ( t.game.runasmultiplayer == 1 && g.mp.coop == 0 ) 
			{
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if ( t.entid>0 ) 
				{
					if ( t.entityprofile[t.entid].ischaracter  ==  1 ) 
					{
						t.entityelement[t.e].destroyme=1;
					}
				}
			}
			}
		#endif

		// if multiplayer and coop, setup ai for switching who control them, depending on gameplay circumstances
		#ifdef VRTECH
		#else
		if ( t.game.runasmultiplayer == 1 && g.mp.coop == 1 ) 
		{
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if ( t.entid>0 ) 
				{
					if ( t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar ) 
					{
						t.entityelement[t.e].mp_coopControlledByPlayer = -1;
					}
				}
			}
		}
		#endif

		// if no multiplayer markers, put some at the default height
		#ifdef VRTECH
		if ( t.tnumberofstartmarkers == 0 && bHaveRegularStartMarker == false ) 
		#else
		if ( t.tnumberofstartmarkers == 0 ) 
		#endif
		{
			for ( t.tloop = 1; t.tloop <= MP_MAX_NUMBER_OF_PLAYERS; t.tloop++ )
			{
				t.mpmultiplayerstart[t.tloop].active=1;
				t.mpmultiplayerstart[t.tloop].x=GGORIGIN_X;
				//  added 10 onto the y otherwise the players fall through the ground
				t.mpmultiplayerstart[t.tloop].y=BT_GetGroundHeight(t.terrain.TerrainID,GGORIGIN_X,GGORIGIN_Z)+50;
				t.mpmultiplayerstart[t.tloop].z=GGORIGIN_Z;
				t.mpmultiplayerstart[t.tloop].angle=0;
			}
		}
		#ifdef VRTECH
		#else
		// if coop and only 1 marker, make some more
		if ( g.mp.coop == 1 && t.tnumberofstartmarkers == 1 ) 
		{
			for ( t.tloop = 2 ; t.tloop <= MP_MAX_NUMBER_OF_PLAYERS; t.tloop++ )
			{
				t.mpmultiplayerstart[t.tloop].active=1;
				t.mpmultiplayerstart[t.tloop].x=t.mpmultiplayerstart[1].x;
				//  added 10 onto the y otherwise the players fall through the ground
				t.mpmultiplayerstart[t.tloop].y=t.mpmultiplayerstart[1].y;
				t.mpmultiplayerstart[t.tloop].z=t.mpmultiplayerstart[1].z;
				t.mpmultiplayerstart[t.tloop].angle=t.mpmultiplayerstart[1].angle;
			}
		}
		#endif

		// reserve max multiplayer characters (all weapon animations included)
		Dim ( t.tubindex,2+MP_MAX_NUMBER_OF_PLAYERS  );
		#ifdef WICKEDENGINE
		t.ent_s=g.rootdir_s+"charactercreatorplus\\Uber Character.fpe";
		#else
		#ifdef PHOTONMP
			t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Character.fpe";
		#else
			t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Soldier.fpe";
		#endif
		#endif
		entity_addtoselection_core ( );
		t.tubindex[0]=t.entid;
		t.entityprofile[t.tubindex[0]].ischaracter=0;
		t.entityprofile[t.tubindex[0]].collisionmode=12;
		t.entityprofile[t.tubindex[0]].aimain_s = "";

		#ifdef PHOTONMP
			// No teams - no combat!
		#else
			if ( g.mp.team == 1 && g.mp.coop == 0 ) 
			{
			t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Soldier Red.fpe";
			entity_addtoselection_core ( );
			t.tubindex[1]=t.entid;
			t.entityprofile[t.tubindex[1]].ischaracter=0;
			t.entityprofile[t.tubindex[1]].collisionmode=12;
			// No lua script for player chars
			t.entityprofile[t.tubindex[1]].aimain_s = "";
			t.tti = 1;
			}
			#ifdef VRTECH
			#else
			// add any character creator player avatars in
			for ( t.tcustomAvatarCount = 0 ; t.tcustomAvatarCount<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tcustomAvatarCount++ )
			{
			//  check if there is a custom avatar
			if (  t.mp_playerAvatars_s[t.tcustomAvatarCount]  !=  "" ) 
			{
				//  there is so lets built a temp fpe file from it
				t.ent_s=g.rootdir_s+"entitybank\\user\\charactercreator\\customAvatar_"+Str(t.tcustomAvatarCount)+".fpe";
				t.avatarFile_s = t.ent_s;
				t.avatarString_s = t.mp_playerAvatars_s[t.tcustomAvatarCount];
				characterkit_makeMultiplayerCharacterCreatorAvatar ( );
				entity_addtoselection_core ( );
				characterkit_removeMultiplayerCharacterCreatorAvatar ( );
				t.tubindex[t.tcustomAvatarCount+2]=t.entid;
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].ischaracter=0;
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].collisionmode=12;
				// No lua script for player chars
				t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].aimain_s = "";
			}
			}
			#endif
		#endif

		#ifdef VRTECH
		// add any character creator player avatars in
		for ( t.tcustomAvatarCount = 0 ; t.tcustomAvatarCount <= MP_MAX_NUMBER_OF_PLAYERS-1; t.tcustomAvatarCount++ )
		{
			t.mp_playerAvatarLoaded[t.tcustomAvatarCount] = false;
		}
		t.bTriggerAvatarRescanAndLoad = true;
		game_scanfornewavatars ( false );
		#endif

		// store ttiswitch for tti as multiplayer avatars can upset the 0->1 switching!
		t.ttiswitch = 1;
		for ( t.plrindex = 1 ; t.plrindex <= MP_MAX_NUMBER_OF_PLAYERS; t.plrindex++ )
		{
			// Add the max number of players into the level if there are start markers or not
			if ( g.mp.team == 1 && g.mp.coop == 0 ) 
			{
				t.ttiswitch = 1 - t.ttiswitch;
			}
			else
			{
				t.ttiswitch = 0;
			}
			t.tti = t.ttiswitch;

			// check if the player has their own avatar
			if ( t.mp_playerAvatars_s[t.plrindex-1] != "" ) 
			{
				t.tti = t.plrindex-1+2;
			}

			t.ubercharacterindex = t.tubindex[t.tti];
			t.entitymaintype=1 ; t.entitybankindex=t.ubercharacterindex;
			t.gridentityeditorfixed=0;
			t.gridentitystaticmode=0;
			t.gridentityhasparent=0;
			t.tfoundone = 0;
			if ( t.mpmultiplayerstart[t.plrindex].active == 1 ) 
			{
				t.tfoundone = 1;
				t.gridentityposx_f=t.mpmultiplayerstart[t.plrindex].x;
				t.gridentityposy_f=t.mpmultiplayerstart[t.plrindex].y;
				t.gridentityposz_f=t.mpmultiplayerstart[t.plrindex].z;
			}
			else
			{
				t.tonetotry = t.plrindex/2;
				if ( t.tonetotry > 0 ) 
				{
					t.tfoundone = 1;
					if ( t.mpmultiplayerstart[t.tonetotry].active == 1 ) 
					{
						t.gridentityposx_f=t.mpmultiplayerstart[t.tonetotry].x;
						t.gridentityposy_f=t.mpmultiplayerstart[t.tonetotry].y;
						t.gridentityposz_f=t.mpmultiplayerstart[t.tonetotry].z;
					}
				}
			}
			if ( t.tfoundone == 0 ) 
			{
				if ( t.mpmultiplayerstart[1].active == 1 ) 
				{
					t.gridentityposx_f=t.mpmultiplayerstart[1].x;
					t.gridentityposy_f=t.mpmultiplayerstart[1].y;
					t.gridentityposz_f=t.mpmultiplayerstart[1].z;
				}
			}
			t.gridentityrotatex_f=0;
			t.gridentityrotatey_f=t.mpmultiplayerstart[t.plrindex].angle;
			t.gridentityrotatez_f=0;
			t.gridentityrotatequatmode = 0;
			t.gridentityrotatequatx_f = 0;
			t.gridentityrotatequaty_f = 0;
			t.gridentityrotatequatz_f = 0;
			t.gridentityrotatequatw_f = 1;
			t.gridentityscalex_f=100;
			t.gridentityscaley_f=100;
			t.gridentityscalez_f=100;
			entity_fillgrideleproffromprofile ( );
			entity_addentitytomap_core ( );
			t.mpmultiplayerstart[t.plrindex].ghostentityindex=t.e;

			// Grab the entity number for steam to use
			t.mp_playerEntityID[t.plrindex-1] = t.e;
			t.entityprofile[t.ubercharacterindex].ismultiplayercharacter=1;
			t.entityprofile[t.ubercharacterindex].hasweapon_s="";
			t.entityprofile[t.ubercharacterindex].hasweapon=0;
			t.entityprofile[t.ubercharacterindex].aimain_s = "";
		}

		#ifdef VRTECH
		//need this later in game for dynamic avatar loading
		//UnDim ( t.tubindex );
		#else
		UnDim ( t.tubindex );
		#endif
	}

	// in standalone, no IDE feeding test level, so load it in
	timestampactivity(0,"_game_loadinleveldata");
	if ( t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
	{
		game_loadinleveldata ( );
	}

	//  Prepare this level
	t.game.levelplrstatsetup = 1; //PE: Make sure to setup new "player start marker" settings.
	#ifdef WICKEDENGINE
	if (t.game.gameisexe == 1) loadingpageprogress(5);
	#endif

	// help keep progress bar instant and moving
	#ifdef WICKEDENGINE
	sprintf_s(pProgressStr, 256, "LOADING LEVEL RESOURCES - %d\\100 Complete", 2);
	void printscreenprompt(char*);
	printscreenprompt(pProgressStr);
	#endif
	game_preparelevel ( );
	game_preparelevel_forplayer ( );

	#ifdef WICKEDENGINE
	/* now have QUAT so can improve on this hack!
	//PE: Make sure everything is setup correct.
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		if (t.entityelement[t.e].obj > 0 && t.e < g.entityelementlist)
		{
			int masterid = t.entityelement[t.e].bankindex;
			if (masterid > 0 && (t.entityprofile[masterid].ragdoll == 1 || t.entityprofile[masterid].ischaracter == 1))
			{
				//PE: In new quat rotation we can get x=-180 z=180 or -180 and everything gets inverted. so make sure when char and ragdoll we use x=0,z=0.
				//t.entityelement[t.e].rx = 0;
				//t.entityelement[t.e].rz = 0;
				if (fabs(t.entityelement[t.e].rx) >= 170 && fabs(t.entityelement[t.e].rz) >= 170)
				{
					//LB: not pretty or ideal, what is the best method of un-inverting a euler rotation from -180,44,-179/etc to 0,44+180,0?
					t.entityelement[t.e].rx = 0;
					t.entityelement[t.e].ry += 180.0f;
					t.entityelement[t.e].rz = 0;
				}
			}
		}
	}
	*/
	//LB: some corruption in older levels, can correct here (level editor also corrects, but not for levels loaded and ran)
	for (t.e = 1; t.e <= g.entityelementlist; t.e++)
	{
		if (t.entityelement[t.e].obj > 0 && t.e < g.entityelementlist)
		{
			int masterid = t.entityelement[t.e].bankindex;
			if (masterid > 0 && (t.entityprofile[masterid].ragdoll == 1 || t.entityprofile[masterid].ischaracter == 1))
			{
				entity_calculateeuleryfromquat(t.e);
			}
		}
	}
	#else
	// VRQ and Classic can have characters lying on floor, etc, so preserve X and z rotations
	#endif

	#ifdef WICKEDENGINE
	sprintf_s(pProgressStr, 256, "FINALIZING LEVEL DATA - %d\\100 Complete", 15);
	void printscreenprompt(char*);
	printscreenprompt(pProgressStr);
	#endif
	game_preparelevel_finally ( );

	// Load any light map objects if available
	timestampactivity(0,"load lightmapped objects");
	lm_loadscene ( );
			
	g.merged_new_objects = 0;
	if ( t.tlmloadsuccess == 0  ) 
	{ 
		//&& !g.disable_drawcall_optimizer
		//###################################################################
		//#### PE: Very simple but effectively draw call optimizer       ####
		//#### Could be made more intelligent when time allow :)         ####
		//#### On a 2000 object level it takes below 2 sec to run.       ####
		//#### setup.ini "drawcalloptimizer=1" will optimize everything  ####
		//#### setup.ini "drawcalloptimizer=0" only fpe settings counts. ####
		//#### .fpe "drawcalloptimizer=1" will optimize this object.     ####
		//#### .fpe "drawcalloptimizer=0" will NOT optimize object.      ####
		//#### .fpe "drawcalloptimizeroff=1" will NOT optimize object.   ####
		//#### (drawcalloptimizeroff is used when you optimize           ####
		//####  everything but have problems with a object )             ####
		//#### .fpe "drawcallscaleadjust" adjust scale.                  ####
		//###################################################################

		#define DC_DISTANCE 1000
		#define MAX_DRAWPRIMITIVES 32765 // max faces.
		#define MAX_DRAWVERTEX 65520 // max vertex 65530.

		timestampactivity(0, "draw call optimizer.");
		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			if (t.entityelement[t.e].obj > 0 && t.e < g.entityelementlist )
			{
				t.entityelement[t.e].dc_merged = false;
				if (t.entityelement[t.e].draw_call_obj > 0) {

					if (t.entityelement[t.e].draw_call_obj > 0 && ObjectExist(t.entityelement[t.e].draw_call_obj) == 1) {
						DeleteObject(t.entityelement[t.e].draw_call_obj);
						t.entityelement[t.e].draw_call_obj = 0;
					}
				}
			}
		}

		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			t.entid = t.entityelement[t.e].bankindex;
			t.obj = t.entityelement[t.e].obj;

			if (t.obj > 0 && t.e < g.entityelementlist && t.entityelement[t.e].dc_merged == false && (g.globals.drawcalloptimizer==1 || t.entityprofile[t.entid].drawcalloptimizer == 1) && t.entityprofile[t.entid].drawcalloptimizeroff == 0 && t.entityprofile[t.entid].isimmobile != 1 && t.entityelement[t.e].eleprof.isimmobile != 1 && t.entityelement[t.e].eleprof.spawnatstart == 1)
			{

				struct OrderByObjectDistance
				{
					bool operator()(int pObjectA, int pObjectB)
					{
								
						if (t.entityelement[pObjectA].dc_distance < t.entityelement[pObjectB].dc_distance)
							return true;

						if (t.entityelement[pObjectA].dc_distance == t.entityelement[pObjectB].dc_distance)
							return true;
						return false;
					}
				};

				int nextObjeid = 0;
				std::vector< int >     vObjList;
				if (1 == 1) {
					//PE: Sort a list by object,distance to increase hit rate.
							
					if (ObjectExist(t.obj)) {
						//for (int i = t.e + 1; i <= g.entityelementlist; i++) {
						for (int i = 1; i <= g.entityelementlist; i++) {
							int testobj = t.entityelement[i].obj;
							int iEntid = t.entityelement[i].bankindex;
									
							if (testobj > 0 && i != t.e && ObjectExist(testobj) && t.entityelement[i].dc_merged == false && t.entityprofile[iEntid].isimmobile != 1 && t.entityelement[i].eleprof.isimmobile != 1 && t.entityelement[i].staticflag == 1 && t.entityelement[i].eleprof.spawnatstart == 1) {

								sObject* pObject = g_ObjectList[t.obj];
								int instanceonly = 0;
								if (pObject && pObject->pInstanceOfObject) {
									pObject = pObject->pInstanceOfObject;
								}

								sObject* pObjectTest = g_ObjectList[testobj];
								if (pObjectTest && pObjectTest->pInstanceOfObject) {
									pObjectTest = pObjectTest->pInstanceOfObject;
								}

								if (pObject && pObjectTest && pObject == pObjectTest) {

									t.tdx_f = t.entityelement[t.e].x - t.entityelement[i].x;
									t.tdz_f = t.entityelement[t.e].z - t.entityelement[i].z;
									t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));

									t.entityelement[i].dc_distance = t.tdd_f;
									vObjList.push_back(i);
								}

							}
						}
						//Sort list
						std::sort(vObjList.begin(), vObjList.end(), OrderByObjectDistance());

					}
				}

				if (vObjList.size() > 0)
					nextObjeid = vObjList[0];

				int glueid = t.entityelement[nextObjeid].bankindex;
				int glueobj = t.entityelement[nextObjeid].obj;

				if (vObjList.size() > 0 && glueobj > 0 && ObjectExist(t.obj) && ObjectExist(glueobj))
				{
					bool validshader = false;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_basic.fx") == 0)
						validshader = true;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_tree.fx") == 0)
						validshader = true;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\entity_basic.fx") == 0)
						validshader = true;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_illum.fx") == 0)
						validshader = true;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "") == 0)
						validshader = true;
					if(t.entityprofile[t.entid].animmax == 0)
						validshader = true;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\treea_basic.fx") == 0)
						validshader = false;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_anim.fx") == 0)
						validshader = false;
					#ifdef VRTECH
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_animwithtran.fx") == 0)
						validshader = false;
					#endif
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_treea.fx") == 0)
						validshader = false;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_anim8bone.fx") == 0)
						validshader = false;
					if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_animwithtran.fx") == 0)
						validshader = false;
					if (t.entityprofile[t.entid].animmax > 0)
						validshader = false;
							
					if (validshader && t.entityprofile[t.entid].ismarker == 0 && t.entityprofile[t.entid].isebe == 0 && t.entityprofile[t.entid].transparency == 0 && t.entityelement[nextObjeid].staticflag == 1 && t.entityprofile[t.entid].isimmobile != 1 && t.entityelement[t.e].eleprof.isimmobile != 1 && t.entityelement[t.e].eleprof.spawnatstart == 1 && t.entityelement[t.e].staticflag == 1)
					{

						//Validate if same master object.
						sObject* pObject = g_ObjectList[t.obj];
						int instanceonly = 0;
						if (pObject && pObject->pInstanceOfObject) {
							pObject = pObject->pInstanceOfObject;
							instanceonly++;
						}

						sObject* pObject2 = g_ObjectList[glueobj];
						if (pObject2 && pObject2->pInstanceOfObject) {
							pObject2 = pObject2->pInstanceOfObject;
							instanceonly++;
						}

						int iMeshWithTexture = -1;
						int iFrameVertex = 0;
						std::vector< int > vUniqueImageIds;
						vUniqueImageIds.clear();
						if (pObject)
						{
							for (int i = 0; i < pObject->iFrameCount; i++)
							{
								if (pObject->ppFrameList[i]->pMesh)
								{
									sMesh* pMesh = pObject->ppFrameList[i]->pMesh;

									if( pMesh->pTextures )
									{
										//PE: Count how many different images is used.
										vUniqueImageIds.push_back(pMesh->pTextures[0].iImageID);
										//strcpy(tmp, pMesh->pTextures[0].pName);
									}

									iMeshWithTexture = i;
									iFrameVertex += pObject->ppFrameList[i]->pMesh->dwVertexCount;
									//break;
								}
							}
						}

						if (iFrameVertex > 0x8000) //Cant merge to many vertex.
							instanceonly = 0;

						//PE: Allow users to also drawcall optimize multimaterial objects, if set in fpe.
						if (t.entityprofile[t.entid].drawcalloptimizer != 1)
						{
							if (vUniqueImageIds.size() > 1)
							{
								instanceonly = 0;
							}
							//PE: Cant do multi material for now.
							//CloneObject(destobj, t.obj, 101); do not support this.
							if (iMeshWithTexture >= 0 && pObject->ppFrameList[iMeshWithTexture]->pMesh) {
								if (pObject->ppFrameList[iMeshWithTexture]->pMesh->bUseMultiMaterial)
									instanceonly = 0;
							}
						}

						int iMultiMatCount = GetMultiMaterialCount(t.obj);
						if (iMultiMatCount > 0) {
							//PE: Allows you to force even multi-textured object to be batched.
							if(t.entityprofile[t.entid].drawcalloptimizer != 1)
								instanceonly = 0;
						}

						//PE: Keep objects distance below DC_DISTANCE for best culling.
						if (pObject && pObject2 && instanceonly >= 2  && t.entityelement[nextObjeid].dc_distance < DC_DISTANCE && g.merged_new_objects < 2890 ) {
									
							if (pObject == pObject2) {
								//Same master glue it.

								if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);

								float gluescalex = ObjectScaleX(glueobj);
								float gluescaley = ObjectScaleY(glueobj);
								float gluescalez = ObjectScaleZ(glueobj);

								float src_scalex = ObjectScaleX(t.obj);
								float src_scaley = ObjectScaleY(t.obj);
								float src_scalez = ObjectScaleZ(t.obj);

								float scaleadjust = t.entityprofile[t.entid].drawcallscaleadjust;

								int tmpobj = (g.merged_new_objects+100) + 87000; //PE: TODO change 85000
								if (tmpobj < g_iObjectListCount && g_ObjectList[tmpobj])
								{
									if (g_ObjectList[tmpobj]->pFrame)
									{
										DeleteObject(tmpobj);
									}
								}

								CloneObject(tmpobj, t.obj);

								//PE: Hmm the lod removal could be improved.
								int bestlod = -1;
								PerformCheckListForLimbs(tmpobj);
								for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
								{
									t.tname_s = Lower(ChecklistString(t.c));

									LPSTR pRightFive = "";
									if (strlen(t.tname_s.Get()) > 5)
										pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;

									if ( (t.tname_s == "lod_0" || stricmp(pRightFive,"_lod0") == 0 ) ) bestlod = 0;
									if ( (t.tname_s == "lod_1" || stricmp(pRightFive,"_lod1") == 0 ) && (bestlod < 0 || bestlod > 1) )  bestlod = 1;
									if ( (t.tname_s == "lod_2" || stricmp(pRightFive,"_lod2") == 0 ) && (bestlod < 0) )  bestlod = 2;

									if (t.entityprofile[t.entid].resetlimbmatrix == 1)
									{
										OffsetLimb(tmpobj, t.c - 1, 0, 0, 0, 0);
									}
								}
								if (bestlod >= 0) {
									for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
									{
										t.tname_s = Lower(ChecklistString(t.c));
										LPSTR pRightFive = "";
										if (strlen(t.tname_s.Get()) > 5)
											pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;

										if (bestlod == 0 && ( t.tname_s == "lod_1" || t.tname_s == "lod_2" || t.tname_s == "lod_3") ) {
											RemoveLimb(tmpobj, t.c - 1);
										}
										if (bestlod == 0 && (stricmp(pRightFive, "_lod1") == 0 || stricmp(pRightFive, "_lod2") == 0 || stricmp(pRightFive, "_lod3") == 0 )) {
											RemoveLimb(tmpobj, t.c - 1);
										}
										if (bestlod == 1 && (t.tname_s == "lod_2" || stricmp(pRightFive, "_lod2") == 0) ) {
											RemoveLimb(tmpobj, t.c - 1);
										}
										if (bestlod == 2 && (t.tname_s == "lod_3" || stricmp(pRightFive, "_lod3") == 0 ) ) {
											RemoveLimb(tmpobj, t.c - 1);
										}
									}
								}

								PositionObject(tmpobj, 0, 0, 0); //PE: Need to be at 0,0,0
								ScaleObject(tmpobj, 100, 100, 100); //PE: no scale.

								int iAfterPolygonTotal = 0;
								int iAfterVertex = 0;

								sObject* pAfterObject = g_ObjectList[tmpobj];
								if (pAfterObject)
								{
									if (pAfterObject->iMeshCount>0)
									{
										for (int iM = 0; iM<pAfterObject->iMeshCount; iM++)
										{
											sMesh* pMesh = pAfterObject->ppMeshList[iM];
											if (pMesh)
											{
												iAfterPolygonTotal += pMesh->iDrawPrimitives;
												iAfterVertex += pMesh->dwVertexCount;
											}
										}
									}
								}
//										printf("after %d vertex %d", iAfterPolygonTotal, iAfterVertex);
						

								if (pAfterObject && pAfterObject->iMeshCount == 0) {
									//CloneObject failed.
									if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);
								}
								else {
									MakeMeshFromObject(g.meshlightmapwork, tmpobj); //PE: mesh from source.
								}

								if( (iAfterPolygonTotal * 2 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 2 < MAX_DRAWVERTEX)  && GetMeshExist(g.meshlightmapwork) == 1)
								{
									int destobj = g.merged_new_objects + 87000; //PE: TODO change 85000 , perhaps reverse from 90000 ?
									if (g_ObjectList[destobj])
									{
										if (g_ObjectList[destobj]->pFrame)
										{
											DeleteObject(destobj);
										}
									}

									t.tmasterx_f = ObjectPositionX(t.obj);
									t.tmastery_f = ObjectPositionY(t.obj);
									t.tmasterz_f = ObjectPositionZ(t.obj);

									//CloneObject(destobj, tmpobj); //We use the cleaned tmpobj instead of t.obj
									MakeObject(destobj, g.meshlightmapwork, -1); //Use mesh to prevent any transforms.

									int testypos = 0; //50,150
									PositionObject(destobj, 0, 0, 0); //PE: Need to be at 0,0,0
									ScaleObject(destobj, 100, 100, 100); //PE: no scale.

									float src_angx = ObjectAngleX(t.obj);
									float src_angy = ObjectAngleY(t.obj);
									float src_angz = ObjectAngleZ(t.obj);

									PerformCheckListForLimbs(destobj);
									AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

									t.tox_f = ObjectPositionX(glueobj) - t.tmasterx_f;
									t.toy_f = ObjectPositionY(glueobj) - t.tmastery_f;
									t.toz_f = ObjectPositionZ(glueobj) - t.tmasterz_f;

									OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

									//PE: not needed anymore fixed above when we reset scale to 100,100,100
//											if (t.entityprofile[t.entid].scale != 100 ) {
//												float scalejust = t.entityprofile[t.entid].scale - 100;
//												gluescalex -= scalejust;
//												gluescaley -= scalejust;
//												gluescalez -= scalejust;
//											}
											
									RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj), ObjectAngleY(glueobj), ObjectAngleZ(glueobj));
									ScaleLimb(destobj, ChecklistQuantity(), gluescalex + scaleadjust, gluescaley + scaleadjust, gluescalez + scaleadjust);
									for (int i = ChecklistQuantity()-1; i >= 0; i--) {
										RotateLimb(destobj, i, src_angx, src_angy, src_angz);
										ScaleLimb(destobj, i, ObjectScaleX(t.obj) + scaleadjust, ObjectScaleY(t.obj) + scaleadjust, ObjectScaleZ(t.obj) + scaleadjust);
									}

									//PE: TODO if possible add one more object.

									bool additionaladded2 = false;
									int glueid2 = 0;
									int glueobj2 = 0;
							
									if ((iAfterPolygonTotal * 3 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 3 < MAX_DRAWVERTEX) && vObjList.size() > 1 ) {
										nextObjeid = vObjList[1];

										glueid2 = t.entityelement[nextObjeid].bankindex;
										glueobj2 = t.entityelement[nextObjeid].obj;

										sObject* pObject3 = g_ObjectList[glueobj2];
										if (pObject3 && pObject3->pInstanceOfObject) {
											pObject3 = pObject3->pInstanceOfObject;
											if (pObject3 == pObject2) {
												t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
												t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
												t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
												if (t.tdd_f < DC_DISTANCE) {
													//Object ok add.
													float gluescalex2 = ObjectScaleX(glueobj2);
													float gluescaley2 = ObjectScaleY(glueobj2);
													float gluescalez2 = ObjectScaleZ(glueobj2);
													//Its the same master so reuse g.meshlightmapwork
													PerformCheckListForLimbs(destobj);
													AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

													t.tox_f = ObjectPositionX(glueobj2) - t.tmasterx_f;
													t.toy_f = ObjectPositionY(glueobj2) - t.tmastery_f;
													t.toz_f = ObjectPositionZ(glueobj2) - t.tmasterz_f;

													OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

													RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj2), ObjectAngleY(glueobj2), ObjectAngleZ(glueobj2));
													ScaleLimb(destobj, ChecklistQuantity(), gluescalex2 + scaleadjust, gluescaley2 + scaleadjust, gluescalez2 + scaleadjust);
													additionaladded2 = true;
												}
											}
										}
									}


									bool additionaladded3 = false;
									int glueid3 = 0;
									int glueobj3 = 0;
									if ((iAfterPolygonTotal * 4 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 4 < MAX_DRAWVERTEX)  && vObjList.size() > 2) {
										nextObjeid = vObjList[2];

										glueid3 = t.entityelement[nextObjeid].bankindex;
										glueobj3 = t.entityelement[nextObjeid].obj;

										sObject* pObject4 = g_ObjectList[glueobj3];
										if (pObject4 && pObject4->pInstanceOfObject) {
											pObject4 = pObject4->pInstanceOfObject;
											if (pObject4 == pObject2) {
												t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
												t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
												t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
												if (t.tdd_f < DC_DISTANCE) {
													//Object ok add.
													float gluescalex3 = ObjectScaleX(glueobj3);
													float gluescaley3 = ObjectScaleY(glueobj3);
													float gluescalez3 = ObjectScaleZ(glueobj3);
													//Its the same master so reuse g.meshlightmapwork
													PerformCheckListForLimbs(destobj);
													AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

													t.tox_f = ObjectPositionX(glueobj3) - t.tmasterx_f;
													t.toy_f = ObjectPositionY(glueobj3) - t.tmastery_f;
													t.toz_f = ObjectPositionZ(glueobj3) - t.tmasterz_f;

													OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

													RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj3), ObjectAngleY(glueobj3), ObjectAngleZ(glueobj3));
													ScaleLimb(destobj, ChecklistQuantity(), gluescalex3 + scaleadjust, gluescaley3 + scaleadjust, gluescalez3 + scaleadjust);
													additionaladded3 = true;
												}
											}
										}
									}

									bool additionaladded4 = false;
									int glueid4 = 0;
									int glueobj4 = 0;
									if ((iAfterPolygonTotal * 5 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 5 < MAX_DRAWVERTEX)  && vObjList.size() > 3) {
										nextObjeid = vObjList[3];

										glueid4 = t.entityelement[nextObjeid].bankindex;
										glueobj4 = t.entityelement[nextObjeid].obj;

										sObject* pObject5 = g_ObjectList[glueobj4];
										if (pObject5 && pObject5->pInstanceOfObject) {
											pObject5 = pObject5->pInstanceOfObject;
											if (pObject5 == pObject2) {
												t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
												t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
												t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
												if (t.tdd_f < DC_DISTANCE) {
													//Object ok add.
													float gluescalex4 = ObjectScaleX(glueobj4);
													float gluescaley4 = ObjectScaleY(glueobj4);
													float gluescalez4 = ObjectScaleZ(glueobj4);
													//Its the same master so reuse g.meshlightmapwork
													PerformCheckListForLimbs(destobj);
													AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

													t.tox_f = ObjectPositionX(glueobj4) - t.tmasterx_f;
													t.toy_f = ObjectPositionY(glueobj4) - t.tmastery_f;
													t.toz_f = ObjectPositionZ(glueobj4) - t.tmasterz_f;

													OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

													RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj4), ObjectAngleY(glueobj4), ObjectAngleZ(glueobj4));
													ScaleLimb(destobj, ChecklistQuantity(), gluescalex4 + scaleadjust, gluescaley4 + scaleadjust, gluescalez4 + scaleadjust);
													additionaladded4 = true;
												}
											}
										}
									}

									bool additionaladded5 = false;
									int glueid5 = 0;
									int glueobj5 = 0;
									if ((iAfterPolygonTotal * 6 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 6 < MAX_DRAWVERTEX)  && vObjList.size() > 4) {
										nextObjeid = vObjList[4];

										glueid5 = t.entityelement[nextObjeid].bankindex;
										glueobj5 = t.entityelement[nextObjeid].obj;

										sObject* pObject6 = g_ObjectList[glueobj5];
										if (pObject6 && pObject6->pInstanceOfObject) {
											pObject6 = pObject6->pInstanceOfObject;
											if (pObject6 == pObject2) {
												t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
												t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
												t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
												if (t.tdd_f < DC_DISTANCE) {
													//Object ok add.
													float gluescalex5 = ObjectScaleX(glueobj5);
													float gluescaley5 = ObjectScaleY(glueobj5);
													float gluescalez5 = ObjectScaleZ(glueobj5);
													//Its the same master so reuse g.meshlightmapwork
													PerformCheckListForLimbs(destobj);
													AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

													t.tox_f = ObjectPositionX(glueobj5) - t.tmasterx_f;
													t.toy_f = ObjectPositionY(glueobj5) - t.tmastery_f;
													t.toz_f = ObjectPositionZ(glueobj5) - t.tmasterz_f;

													OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

													RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj5), ObjectAngleY(glueobj5), ObjectAngleZ(glueobj5));
													ScaleLimb(destobj, ChecklistQuantity(), gluescalex5 + scaleadjust, gluescaley5 + scaleadjust, gluescalez5 + scaleadjust);
													additionaladded5 = true;
												}
											}
										}
									}

									bool additionaladded6 = false;
									int glueid6 = 0;
									int glueobj6 = 0;
									if ((iAfterPolygonTotal * 7 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 7 < MAX_DRAWVERTEX)  && vObjList.size() > 5) {
										nextObjeid = vObjList[5];

										glueid6 = t.entityelement[nextObjeid].bankindex;
										glueobj6 = t.entityelement[nextObjeid].obj;

										sObject* pObject7 = g_ObjectList[glueobj6];
										if (pObject7 && pObject7->pInstanceOfObject) {
											pObject7 = pObject7->pInstanceOfObject;
											if (pObject7 == pObject2) {
												t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
												t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
												t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
												if (t.tdd_f < DC_DISTANCE) {
													//Object ok add.
													float gluescalex6 = ObjectScaleX(glueobj6);
													float gluescaley6 = ObjectScaleY(glueobj6);
													float gluescalez6 = ObjectScaleZ(glueobj6);
													//Its the same master so reuse g.meshlightmapwork
													PerformCheckListForLimbs(destobj);
													AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

													t.tox_f = ObjectPositionX(glueobj6) - t.tmasterx_f;
													t.toy_f = ObjectPositionY(glueobj6) - t.tmastery_f;
													t.toz_f = ObjectPositionZ(glueobj6) - t.tmasterz_f;

													OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

													RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj6), ObjectAngleY(glueobj6), ObjectAngleZ(glueobj6));
													ScaleLimb(destobj, ChecklistQuantity(), gluescalex6 + scaleadjust, gluescaley6 + scaleadjust, gluescalez6 + scaleadjust);
													additionaladded6 = true;
												}
											}
										}
									}

									//PE: Merge everything into a single mesh.
									DeleteMesh(g.meshlightmapwork);
									MakeMeshFromObject(g.meshlightmapwork, destobj);
									DeleteObject(destobj);
									MakeObject(destobj, g.meshlightmapwork, -1);
									PositionObject(destobj, t.tmasterx_f, t.tmastery_f + testypos, t.tmasterz_f);

									//PE: TODO
									//PE: Objects like plant 08.fpe get VERY large ? , need to be checked why!
									//float objectsizex = ObjectSizeX(destobj);
									//if (objectsizex > 1000) {
									//	SetObjectMask(destobj, 1);
									//}

									if (t.entityprofile[t.entid].canseethrough == 1)
									{
										SetObjectCollisionProperty(destobj, 1);
									}
									if (t.entityprofile[t.entid].ischaracter == 0)
									{
										if (t.entityprofile[t.entid].collisionmode == 11)
										{
											SetObjectCollisionProperty(destobj, 1);
										}
									}
									if (t.entityprofile[t.entid].cullmode >= 0)
									{
										if (t.entityprofile[t.entid].cullmode != 0)
										{
											SetObjectCull(destobj, 0);
										}
										else
										{
											SetObjectCull(destobj, 1);
										}
									}
									SetObjectSpecularPower(destobj, t.entityelement[t.entid].eleprof.specularperc / 100.0f);

									if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);
												
									CloneObject(destobj, t.obj, 101); //PE: Copy textures only.

									SetObjectStatic(destobj, true); //Mark as static.

									//PE: TODO store t.e and glueobj under pObject->draw_call_obj
									//PE: So we can access them directly in drawobject.
									//pObject->draw_call_obj = g_ObjectList[destobj];

									//Disable if any LOD setup from original object.
									if (bestlod >= 0) {
										SetObjectLOD(destobj, 1, 50000);
										SetObjectLOD(destobj, 2, 50000);
									}

									t.entityelement[t.e].draw_call_obj = destobj;
									t.entityelement[t.e].dc_obj[0] = glueobj;
									t.entityelement[t.e].dc_entid[0] = vObjList[0];
									t.entityelement[vObjList[0]].dc_merged = true;
									if (additionaladded2) {
										t.entityelement[t.e].dc_obj[1] = glueobj2;
										t.entityelement[t.e].dc_entid[1] = vObjList[1];
										t.entityelement[vObjList[1]].dc_merged = true;
										HideObject(glueobj2);
									}
									if (additionaladded3) {
										t.entityelement[t.e].dc_obj[2] = glueobj3;
										t.entityelement[t.e].dc_entid[2] = vObjList[2];
										t.entityelement[vObjList[2]].dc_merged = true;
										HideObject(glueobj3);
									}
									if (additionaladded4) {
										t.entityelement[t.e].dc_obj[3] = glueobj4;
										t.entityelement[t.e].dc_entid[3] = vObjList[3];
										t.entityelement[vObjList[3]].dc_merged = true;
										HideObject(glueobj4);
									}
									if (additionaladded5) {
										t.entityelement[t.e].dc_obj[4] = glueobj5;
										t.entityelement[t.e].dc_entid[4] = vObjList[4];
										t.entityelement[vObjList[4]].dc_merged = true;
										HideObject(glueobj5);
									}
									if (additionaladded6) {
										t.entityelement[t.e].dc_obj[5] = glueobj6;
										t.entityelement[t.e].dc_entid[5] = vObjList[5];
										t.entityelement[vObjList[5]].dc_merged = true;
										HideObject(glueobj6);
									}
									//Hide org objects.
									HideObject(t.obj);
									HideObject(glueobj);

									SetObjectEffect(t.entityelement[t.e].draw_call_obj, t.entityprofile[t.entid].usingeffect );
									ShowObject(t.entityelement[t.e].draw_call_obj);

									g.merged_new_objects++;

									// NOTE: Does this mean batched objects will not benefit from these important flags (some models have OpenGL normals and the auto-generated tangents shift about)											
									// PE: Ups my bad , actually we would use pObject->draw_call_obj so it would have all the original settings,
									// PE: but not yet, so just added this again, cant remember why i commented it out ? seams to work fine :)
									DWORD dwArtFlags = 0;
									if (t.entityprofile[t.entid].invertnormal == 1) dwArtFlags = 1;
									if (t.entityprofile[t.entid].preservetangents == 1) dwArtFlags |= 1 << 1;
									SetObjectArtFlags(destobj, dwArtFlags, 0.0f);

								}

								DeleteObject(tmpobj);
							}
						}
					}
				}
			}
		}
	}

	#ifdef WICKEDENGINE
	#ifdef NEWMAXAISYSTEM
	// Create nav mesh from entire level geometry
	timestampactivity(0, "Attempt to create nav mesh");
	if (t.game.gameisexe == 1)
	{
		loadingpageprogress(5);
		game_createnavmeshfromlevel ( true );
	}
	else
	{
		game_createnavmeshfromlevel ( false );
	}
	#endif
	#endif

	// Setup variables for main game loop
	#ifdef WICKEDENGINE
	t.screenprompt_s = "STARTING GAME";
	if (t.game.gameisexe == 0)  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0, t.screenprompt_s.Get());
	#endif
	game_init ( );

	// Helpful prompt for start of test game
	if ( t.game.gameisexe == 0 && t.game.runasmultiplayer == 0 ) 
	{
		#ifdef VRTECH
		if (g_bInTutorialMode == true)
		{
			t.visuals.generalprompt_s = "PRESS ESCAPE TO RETURN TO TUTORIAL";
		}
		else
		{
			#ifdef WICKEDENGINE
			t.visuals.generalprompt_s = "Press TAB to see framerate or ESCAPE to exit test";
			#else
			t.visuals.generalprompt_s = "TAB for settings, F9 for 3D edit, Esc to quit.";
			#endif
		}
		#else
		t.visuals.generalprompt_s="TAB for settings, F9 for 3D edit, F10 for snapshot, Esc to quit.";
		#endif
		t.visuals.generalpromptstatetimer=Timer()+123;
	}
	else
	{
		if ( t.game.runasmultiplayer == 1 ) 
		{
			#ifdef PHOTONMP
				t.visuals.generalpromptstatetimer=Timer()+1000;
				#ifdef PRODUCTV3
				t.visuals.generalprompt_s="Welcome to VR Quest(r) Social VR";
				#else
				t.visuals.generalprompt_s="Welcome to GameGuru MAX Multiplayer";
				#endif
			#else
				t.visuals.generalpromptstatetimer=Timer()+1000;
				t.visuals.generalprompt_s="Press RETURN to Chat";
			#endif
		}
		else
		{
			t.visuals.generalpromptstatetimer=0;
		}

		//PE: start any animations that use startanimingame > 0. ( standalone ).
		for (t.tte = 1; t.tte <= g.entityelementlist; t.tte++)
		{
			//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/341
			// hide EBE markers
			int iIndex = t.entityelement[t.tte].bankindex;
			if (t.entityprofile[iIndex].isebe != 0)
			{
				t.tobj = t.entityelement[t.tte].obj;
				if (t.tobj>0)
				{
					if (ObjectExist(t.tobj) == 1)
					{
						HideLimb(t.tobj, 0);
					}
				}
			}

			t.entid = t.entityelement[t.tte].bankindex;
			t.tttsourceobj = g.entitybankoffset + t.entityelement[t.tte].bankindex;
			t.tobj = t.entityelement[t.tte].obj;
			if (t.tobj > 0)
			{
				if (ObjectExist(t.tobj) == 1)
				{
					//PE: Possible fix for issues:
					//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/206
					//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/273
					//PE: need testing.
					if (t.entityprofile[t.entid].ischaracter == 1) {
						//Char should always have z depth , but somehow its removed somewhere.

						//PE: check t.entityprofile[t.tentid].zdepth == 0
						EnableObjectZDepth(t.tobj);
					}

					#ifdef WICKEDENGINE
					//PE: Make sure we reset all animations. mainly for lua controlled objects like doors
					if (t.entityprofile[t.entid].animmax > 0)
					{
						SetObjectFrame(t.tttsourceobj, 0);
						StopObject(t.tttsourceobj);
						SetObjectFrame(t.tobj, 0);
						StopObject(t.tobj);
					}
					#endif

					if (t.entityprofile[t.entid].startanimingame > 0) {
						if (t.entityprofile[t.entid].animmax > 0) {
							t.q = t.entityprofile[t.entid].startanimingame - 1;
							SetObjectFrame(t.tttsourceobj, 0);
							LoopObject(t.tttsourceobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
							SetObjectFrame(t.tobj, 0);
							LoopObject(t.tobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
						}
					}
				}
			}
		}

	}

	//  setup spin values, this rotates the player 360 degrees at the start to kill initial
	//  stutter issues, during this time we don't want low FPS warnings
	t.postprocessings.spinfill=0 ; t.postprocessings.spinfillStartAng_f=CameraAngleY(0);

	//  apply any settings
	timestampactivity(0,"immediate title settings applied");
	if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
	{
		titles_immediateupdatesound ( );
		titles_immediateupdategraphics ( );
	}

	//  for multiplayer, check if there is a jetpack in the level and grab the model to place on players back
	if (  t.game.runasmultiplayer == 1 ) 
	{
		for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if (  t.entid>0 ) 
			{
				if (  cstr(Lower(Left(t.entityprofileheader[t.entid].desc_s.Get(),8)))  ==   "jet pack"  && ObjectExist(g.steamplayermodelsoffset+120)  ==  0 ) 
				{
					CloneObject (  g.steamplayermodelsoffset+120,t.entityelement[t.e].obj );
					YRotateObject (  g.steamplayermodelsoffset+120,180 );
					FixObjectPivot (  g.steamplayermodelsoffset+120 );
				}
			}
		}
	}

	#ifdef VRTECH
	//  check for character creator characters just before game starts
	///characterkit_checkForCharacters ( );
	#else
	characterkit_checkForCharacters ( );
	#endif

	//  Clear screen of any artifacts
	titles_loadingpagefree();
	CLS (  Rgb(0,0,0) );

	// In EXE running, override cameras with no mask until title/loading done
	SyncMaskOverride ( 0xFFFFFFFF );

	// resort texture list to ignore objects set to be ignored
	DoTextureListSort ( );

	// if reloading standalone level, need to restore basic stats from LUA save file
	if ( g.iStandaloneIsReloading == 2 )
	{
		// call LUA function from game which updates stats via LUA script
		char pLUACustomLoadCall[256];
		strcpy ( pLUACustomLoadCall, "GameLoopLoadStats" );
		LuaSetFunction ( pLUACustomLoadCall, 0, 0 ); 
		LuaCall ( );
	}

	// one final command to improve static physics performance
	physics_finalize ( );

	// Wipe out mouse deltas
	t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;

	//  Tab mode LOW FPS Warning
	g.tabmode=0 ; g.lowfpstarttimer=Timer();

	//  Game loop will run while single level is in play
	t.huddamage.immunity=1000;
	t.game.gameloop=1;
	g.timeelapsed_f=0;
	#ifdef VRTECH
	t.luaglobal.scriptprompttype = 0;
	#endif
	t.luaglobal.scriptprompt_s="";
	t.luaglobal.scriptprompttime=0;
	t.luaglobal.scriptprompttextsize=0;
	t.luaglobal.scriptprompt3dtime=0;
	strcpy ( t.luaglobal.scriptprompt3dtext, "" );

	#ifdef VRTECH
	// 260220 - for some reason, Social VR sets view 0,0,1,1, and does not set it back!
	// so we do so here to ensure we see the game
	SetCameraView(0, 0, 0, GetDisplayWidth(), GetDisplayHeight());
	#endif			

	#ifdef WICKEDENGINE
	// no more prompts, reset system so next time we can have a 2 seconds grace before any prompts (see printscreenprompt)
	t.screenprompt_s = "";
	printscreenprompt(t.screenprompt_s.Get());
	t.postprocessings.fadeinvalue_f = 0.0; //PE: Fade in level.
	#endif

	// prompt
	if ( g.gproducelogfiles == 2 )
		timestampactivity(0,"main game loop begins in deep debug trace mode");
	else
		timestampactivity(0,"main game loop begins");

	#ifdef WICKEDENGINE
	extern int iTriggerGrassTreeUpdate;
	iTriggerGrassTreeUpdate = 5; //PE: Make sure trees and grass height is set after terrain has finish.

	//Stop any menu background music ...
	int iFreeSoundID = g.temppreviewsoundoffset + 2;
	if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
	{
		// stop currently playing preview
		StopSound(iFreeSoundID);
	}

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
	iFreeSoundID = g.temppreviewsoundoffset + 5;
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

	//PE: Disable all terrain editor tools in testgame.
	old_render_params2 = ggterrain_global_render_params2.flags2;
	ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_MINI_MAP;
	ggterrain_global_render_params2.flags2 &= ~GGTERRAIN_SHADER_FLAG2_SHOW_BRUSH_SIZE;
	// The map bounds can optionally be shown in testgame.
	extern void TestLevel_ToggleBoundary(bool, bool);
	TestLevel_ToggleBoundary(t.showtestgame2dbounds, t.showtestgame3dbounds);

	//extern void TestLevel_ToggleTreeVegWater(bool, bool, bool);
	//TestLevel_ToggleTreeVegWater(t.showtestgametrees, t.showtestgameveg, t.showtestgamewater);

	#endif
}

void game_masterroot_gameloop_afterexitgamemenu(void)
{
	if (t.game.jumplevel_s == "")
	{
		// can override jumplevel with 'advanced warning level filename' when LOAD level from MAIN MENU
		if (strcmp(t.game.pAdvanceWarningOfLevelFilename, "") != NULL)
		{
			t.game.jumplevel_s = t.game.pAdvanceWarningOfLevelFilename;
			#ifdef STORYBOARD
			if (strlen(Storyboard.gamename) > 0)
			{
				//PE: We use t.game.pAdvanceWarningOfLevelFilename later.
			}
			else
			{
				strcpy(t.game.pAdvanceWarningOfLevelFilename, "");
			}
			#else
			strcpy(t.game.pAdvanceWarningOfLevelFilename, "");
			#endif
		}
	}
	if (t.game.jumplevel_s != "")
	{
		//Load level.
		t.game.gameloop = 0;
	}
			
	timestampactivity(0,"leaving options page");
	g.titlesettings.updateshadowsaswell=0;
	//PE: Clear g_iMouseDeltaZ
	MouseMoveZ();
	LuaSetInt("g_MouseWheel", 0); //PE: Reset g_MouseWheel
}

void game_masterroot_gameloop_afterescapepressed(void)
{
	if ( t.currentgunobj>0 ) { if ( ObjectExist(t.currentgunobj) == 1 ) { SetObjectSpeed ( t.currentgunobj,t.currentgunanimspeed_f); } }
	physics_resumephysics ( );
	entity_resumeanimations ( );
	t.aisystem.cumilativepauses=Timer()-t.tremembertimer;
	game_main_snapshotsoundresume ( );
	t.strwork = ""; t.strwork = t.strwork + "resuming game loop with flag "+Str(t.game.gameloop);
	timestampactivity(0, t.strwork.Get() );
	// Wipe out mouse deltas
	t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;

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

bool bMainLoopRunning = false;

bool game_masterroot_gameloop_loopcode(int iUseVRTest)
{
	// within gameloop, can have a in-game menu loop, so handle this if used
	if (g_iInGameMenuState == 1)
	{
		if (titleslua_main_loopcode() == true)
		{
			game_masterroot_gameloop_afterexitgamemenu();
			game_masterroot_gameloop_afterescapepressed();
			g_iInGameMenuState = 0;
		}
		else
		{
			return false;
		}
	}

	//  Game cycle loop
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"winddown mp_closeconnection");
	#ifdef VRTECH
	if ( t.game.gameloopwinddown == 1 )
	{
		if ( mp_closeconnection() == 1 )
		{
			t.game.gameloopwinddown = 0;
			t.game.gameloop = 0;
		}
	}
	#endif

	// detect if standalone is a foreground window
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"obtain plrhasfocus");
	if ( t.game.gameisexe == 1 )
	{
		#ifdef VRTECH
		if ( g.gvrmode == 3 )
		{
			t.plrhasfocus = 1;
		}
		else
		#else
		if(1)
		#endif
		{
			HWND hForeWnd = GetForegroundWindow();
			if ( GetWindowHandle() != hForeWnd ) 
				t.plrhasfocus = 0;
			else
				t.plrhasfocus = 1;
		}
	}

	// if controller active, also detect for START button press (same as ESCAPE)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"controller start button check");
	bool bControllerEscape = false;
	if ( g.gxbox > 0 && JoystickFireXL(9) == 1 ) bControllerEscape = true;

	//  trigger options page or exit test level
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"escape button check");
	if ( EscapeKey() == 1 || bControllerEscape == true ) 
	{
		t.tremembertimer=Timer();
		#ifdef VRTECH
		game_main_snapshotsoundloopcheckpoint ( );
		game_stopallsounds ();
		#else
		game_main_snapshotsound ( true );
		#endif
		while ( EscapeKey() != 0 ) {}
		darkai_character_freezeall ( );
		physics_pausephysics ( );
		entity_pauseanimations ( );
		if ( t.currentgunobj > 0 ) { if ( ObjectExist(t.currentgunobj)==1 ) { SetObjectSpeed ( t.currentgunobj,0) ; } }
		if ( t.playercontrol.jetobjtouse>0 ) 
		{
			if ( ObjectExist(t.playercontrol.jetobjtouse) == 1  )  SetObjectSpeed (  t.playercontrol.jetobjtouse,0 );
		}
		#ifdef VRTECH
		if ( t.game.gameisexe == 0 ) // no menu in multiplayer test mode && t.game.runasmultiplayer == 0 ) 
		{
			if ( t.game.runasmultiplayer == 1 )
			{
				// wait until connection closed, then exit game loop
				t.game.gameloopwinddown = 1; 
			}
			else
			{
				// leave right away
				t.game.gameloop=0; 
				t.game.levelloop=0; 
				t.game.masterloop=0;
			}
			if ( t.conkit.editmodeactive == 1 ) 
			{
				conkitedit_switchoff ( );
			}
		}
		#else
		if (  t.game.gameisexe == 0 && t.game.runasmultiplayer == 0 ) 
		{
			t.game.gameloop=0 ; t.game.levelloop=0 ; t.game.masterloop=0;
			if ( t.conkit.editmodeactive == 1 ) 
			{
				conkitedit_switchoff ( );
			}
		}
		#endif
		else
		{
			g.titlesettings.updateshadowsaswell = 1;
			timestampactivity(0, "entering options page");
			titleslua_init();
			g_iInGameMenuState = 1;
			titleslua_main("gamemenu");
			if (titleslua_main_loopcode() == true)
			{
				// title systen left right away, continue
				game_masterroot_gameloop_afterexitgamemenu();
			}
			else
			{
				// cycle
				return false;
			}
		}
		game_masterroot_gameloop_afterescapepressed();
		g_iInGameMenuState = 0;
	}

	// Fade in gamescreen (using post process shader)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"fade game screen logic");
	if ( t.postprocessings.fadeinvalue_f<1.0 ) 
	{
		// Hide Lua Sprites
		HideOrShowLUASprites ( true );
		#ifndef WICKEDENGINE
		//  Once spinfill is complete we can begin fading in. When complete we can enable low fps warnings again
		if ( t.postprocessings.spinfill < 360 ) 
		{
			t.postprocessings.spinfill += 10;
			if ( t.postprocessings.spinfill >= 360 )
			{
				// 020316 - v1.13b1 - restore to agreed start orientation
				RotateCamera ( 0, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f );
			}
			else
			{
				// spin around to fill video memory with immediate scenery
				RotateCamera (  0,0,t.postprocessings.spinfillStartAng_f + t.postprocessings.spinfill,0 );
			}

			// Wipe out mouse deltas during spin up
			t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;
		}
		else
		#endif
		{
			if ( t.postprocessings.fadeinvalue_f <= 0 )
			{
				// only if in test game mode, standalone already set volume values (in title.lua)						
				if ( t.game.gameisexe == 0 )
				{
					// set music and sound global volumes
					audio_volume_init ( );
				}
				else if (!FileExist("savegames\\sounds.dat")) {
					//PE: This is not always in standalone ? , so:
					audio_volume_init();
				}
				// and update internal volume values so music update can use volumes!
				audio_volume_update ( );
			}
			#ifdef WICKEDENGINE
			//PE: Wicked seams a littler faster then classic , strange ?
			t.postprocessings.fadeinvalue_f = t.postprocessings.fadeinvalue_f + (g.timeelapsed_f*0.08f);
			#else
			t.postprocessings.fadeinvalue_f=t.postprocessings.fadeinvalue_f+(g.timeelapsed_f*0.1f);
			#endif
			if (  t.postprocessings.fadeinvalue_f >= 1.0f ) 
			{
				#ifdef WICKEDENGINE
				//PE: Disable 3D hiding. we are all done and ready to play.
				extern int iKeepBackgroundForFrames;
				iKeepBackgroundForFrames = 0;
				#endif
				t.postprocessings.fadeinvalue_f=1.0f;
				//g.globals.hidelowfpswarning = 0; // this overrides the SETUP.INI setting
				HideOrShowLUASprites ( false );
				EnableAllSprites(); // the disable is called in DarkLUA by ResetFade() black out command when load game position
			}
		}
		#ifdef WICKEDENGINE
		extern bool bFakeStandaloneTest;
		if ( (t.game.gameisexe == 1 || bFakeStandaloneTest ) && t.postprocessings.fadeinvalue_f < 1.0) //PE: Only standalone and , not on last frame.
		{
			extern bool bImGuiInTestGame;
			extern bool bRenderTabTab;
			extern bool bBlockImGuiUntilNewFrame;
			extern bool bImGuiRenderWithNoCustomTextures;
			extern bool g_bNoGGUntilGameGuruMainCalled;
			extern bool bImGuiFrameState;

			if ((bImGuiInTestGame) && !bRenderTabTab && !bImGuiFrameState)
			{
				//We need a new frame.
				ImGui_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				bRenderTabTab = true;
				bBlockImGuiUntilNewFrame = false;
				bImGuiRenderWithNoCustomTextures = false;
			}

			ImGuiViewport* mainviewport = ImGui::GetMainViewport();
			if (mainviewport)
			{
				ImDrawList* drawlist = ImGui::GetForegroundDrawList(mainviewport);
				if (drawlist)
				{
					ImVec4 monitor_col = ImVec4(0.0, 0.0, 0.0, 1.0 - t.postprocessings.fadeinvalue_f); //Fade in.
					drawlist->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size, ImGui::GetColorU32(monitor_col));
				}
			}
		}
		#endif
		t.postprocessings.fadeinvalueupdate=1;
	}

	// handle fading
	#ifdef WICKEDENGINE
	bMainLoopRunning = true;
	#else
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"handle postprocess fading");
	if (  t.postprocessings.fadeinvalueupdate == 1 ) 
	{
		t.postprocessings.fadeinvalueupdate=0;
		if (  GetEffectExist(g.postprocesseffectoffset+0) == 1 ) 
		{
			SetVector4 (  g.terrainvectorindex,t.postprocessings.fadeinvalue_f,t.postprocessings.fadeinvalue_f,t.postprocessings.fadeinvalue_f,0 );
			SetEffectConstantV (  g.postprocesseffectoffset+0,"OverallColor",g.terrainvectorindex );
			SetEffectConstantV (  g.postprocesseffectoffset+2,"OverallColor",g.terrainvectorindex );
			SetEffectConstantV (  g.postprocesseffectoffset+4,"OverallColor",g.terrainvectorindex );
		}
		//  210115 - helps refresh scene at very start
		t.visuals.refreshshaders=1;
	}
	#endif

	//  Immunity when respawn
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"handle player immunity");
	if (  t.huddamage.immunity>0 ) 
	{
		t.huddamage.immunity=t.huddamage.immunity-(10*g.timeelapsed_f);
		if (  t.huddamage.immunity<0  )  t.huddamage.immunity = 0;
	}

	//  Run all game subroutines		
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_main_loop");
	game_main_loop ( );

	//  Update screen
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_sync");
	game_sync ( );

	// determine if end of game loop
	if (t.game.gameloop != 1)
		return true;
	else
		return false;
}

void game_masterroot_gameloop_afterloopcode(int iUseVRTest)
{
	// first save current level stats before reset LUA
	if ( t.game.allowfragmentation == 2 )
	{
		char pLUACustomSaveCall[256];
		strcpy ( pLUACustomSaveCall, "GameLoopSaveStats" );
		LuaSetFunction ( pLUACustomSaveCall, 0, 0 ); 
		LuaCall ( );
	}

	// free any lua activity (restore FOV if ingame activity there)
	timestampactivity(0,"finalising LUA system before reset");
	lua_quitting(); 

	// Rest any internal game variables
	game_main_stop ( );

	//PE: Draw call optimizer
//	if (!g.disable_drawcall_optimizer)
	{
		//PE: restore all states. delete all batched objects.
		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			t.entid = t.entityelement[t.e].bankindex;
			t.obj = t.entityelement[t.e].obj;

			if (t.obj > 0 && t.e < g.entityelementlist && ObjectExist(t.obj) == 1)
			{
				if (t.entityelement[t.e].draw_call_obj > 0) {

					if (t.entityelement[t.e].draw_call_obj > 0 && ObjectExist(t.entityelement[t.e].draw_call_obj) == 1) {
						DeleteObject(t.entityelement[t.e].draw_call_obj);
						t.entityelement[t.e].draw_call_obj = 0;
						if(ObjectExist(t.obj) == 1)
							ShowObject(t.obj);
					}

					if (t.entityelement[t.e].dc_obj[0] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[0]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[0]);
					if (t.entityelement[t.e].dc_obj[1] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[1]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[1]);
					if (t.entityelement[t.e].dc_obj[2] > 0 &&  ObjectExist(t.entityelement[t.e].dc_obj[2]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[2]);
					if (t.entityelement[t.e].dc_obj[3] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[3]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[3]);
					if (t.entityelement[t.e].dc_obj[4] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[4]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[4]);
					if (t.entityelement[t.e].dc_obj[5] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[5]) == 1)
						ShowObject(t.entityelement[t.e].dc_obj[5]);

					t.entityelement[t.e].dc_obj[0] = 0;
					t.entityelement[t.e].dc_obj[1] = 0;
					t.entityelement[t.e].dc_obj[2] = 0;
					t.entityelement[t.e].dc_obj[3] = 0;
					t.entityelement[t.e].dc_obj[4] = 0;
					t.entityelement[t.e].dc_obj[5] = 0;

					t.entityelement[t.e].dc_entid[0] = 0;
					t.entityelement[t.e].dc_entid[1] = 0;
					t.entityelement[t.e].dc_entid[2] = 0;
					t.entityelement[t.e].dc_entid[3] = 0;
					t.entityelement[t.e].dc_entid[4] = 0;
					t.entityelement[t.e].dc_entid[5] = 0;
				}
			}
		}
	}

	//PE: Moved here as standalone will delete all objects, so we could not free DCO objects.
	//  Free any level resources
	game_freelevel();

	// must reset LUA here for clean end-game-screens
	// ensure LUA is completely reset before loading new ones in
	// the free call is because game options menu init, but not freed back then
	timestampactivity(0,"resetting LUA system");
	titleslua_free ( );
	LuaReset (  );

	//PE: restore waterline.
	t.terrain.waterliney_f = g.gdefaultwaterheight;

	#ifdef VRTECH
	reset_env_particles();
	#endif

	//PE: restore sun position for editor.
	t.terrain.sundirectionx_f = t.terrain.skysundirectionx_f;
	t.terrain.sundirectiony_f = t.terrain.skysundirectiony_f;
	t.terrain.sundirectionz_f = t.terrain.skysundirectionz_f;

	// 240316 - additional cleanup
	mp_freefadesprite ( );

	// if VR, deactivate at this point
	//g_VR920RenderStereoNow = false;

	//  Advance level to 'next one' or 'win game'
	timestampactivity(0,"end of level stage");
	if (  t.game.gameisexe == 1 )
	{
		timestampactivity(0,"game is standalone exe");
		if (  t.game.quitflag == 0 ) 
		{
			timestampactivity(0,"game has not quit");
			bool bUseOldSystem = true;
#ifdef STORYBOARD
			if (strlen(Storyboard.gamename) > 0)
			{
				bUseOldSystem = false;
				if (t.game.lostthegame == 1)
				{
					//Get output link to lose screen from current level node.
					int a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm , 1);
					if (a == 2)
					{
						//Found a lose output to a screen.
						std::string script = g_Storyboard_Current_fpm;
						replaceAll(script, ".lua", "");
						char tmp[MAX_PATH];
						sprintf(tmp, "Project LUA script : %s", script.c_str());
						timestampactivity(0, tmp);
						sky_hide();
						if (script != "title")
						{
							titleslua_init();
							titleslua_main((char *)script.c_str());
							//PE: We need a blocking run or screen is not displayed. t.game.levelloop = 0; will start title screen.
							titleslua_blocking_run();
							titleslua_free();
						}
						sky_show();
						t.game.levelloop = 0;
					}
					else
					{
						timestampactivity(0, "Project LUA script : lose");
						sky_hide();
						titleslua_init();
						titleslua_main("lose");
						//PE: We need a blocking run or screen is not displayed. t.game.levelloop = 0; will start title screen.
						titleslua_blocking_run();
						titleslua_free();
						sky_show();
						t.game.levelloop = 0;
					}
				}
				else
				{
//					FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
//					g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
//					strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);

					if (strcmp(t.game.pAdvanceWarningOfLevelFilename, "") != NULL)
					{
						//From load game
						t.game.jumplevel_s = t.game.pAdvanceWarningOfLevelFilename;
						strcpy(t.game.pAdvanceWarningOfLevelFilename, "");
						//PE: Find g_Storyboard_Current_Level from t.game.jumplevel_s.

						for (int i = 0; i < STORYBOARD_MAXNODES; i++)
						{
							if (Storyboard.Nodes[i].used)
							{
								if (pestrcasestr(Storyboard.Nodes[i].level_name, t.game.jumplevel_s.Get()) != 0)
								{
									g_Storyboard_Current_Level = i;
									strcpy(g_Storyboard_Current_fpm, Storyboard.Nodes[i].level_name);
								}
							}
						}
						//	t.game.pAdvanceWarningOfLevelFilename
						//	t.game.jumplevel_s
					}
					else
					{
						//Get next level.
						int a = FindNextLevel(g_Storyboard_Current_Level, g_Storyboard_Current_fpm);
						if (a == 2)
						{
							//We got a screen jump to that screen.
							std::string script = g_Storyboard_Current_fpm;
							replaceAll(script, ".lua", "");
							char tmp[MAX_PATH];
							sprintf(tmp, "Project LUA script : %s", script.c_str());
							timestampactivity(0, tmp);
							if (script != "title")
							{
								sky_hide();
								titleslua_init();
								titleslua_main((char *)script.c_str());
								//PE: We need a blocking run or screen is not displayed. t.game.levelloop = 0; will start title screen.
								titleslua_blocking_run();
								titleslua_free();
								sky_show();
							}
							t.game.levelloop = 0;
						}
						else
						{
							//We got a new level, use it.
							std::string nextlevel = g_Storyboard_Current_fpm;
							replaceAll(nextlevel, "mapbank\\", "");
							replaceAll(nextlevel, ".fpm", "");
							char tmp[MAX_PATH];
							sprintf(tmp, "Project t.game.jumplevel_s : %s", nextlevel.c_str());
							timestampactivity(0, tmp);
							t.game.jumplevel_s = nextlevel.c_str();
						}
					}
				}
			}
#endif
			if (bUseOldSystem)
			{
				//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
				if (Len(t.game.jumplevel_s.Get()) > 0)
				{
					//  goes around and loads this level name
					timestampactivity(0, "game is loading non-linear level map:");
					timestampactivity(0, "t.game.jumplevel_s.Get()");
				}
				else
				{
					// win, lose or next level pages
					if (t.game.lostthegame == 1)
					{
						//titles_gamelostpage ( );
						timestampactivity(0, "LUA script : lose");
						sky_hide();
						titleslua_init();
						titleslua_main("lose");
						#ifdef WICKEDENGINE
						//PE: We need a blocking run or screen is not displayed. t.game.levelloop = 0; will start title screen.
						//PE: @Lee not sure if these works different in vrquest ? so only wicked for now.
						titleslua_blocking_run();
						#endif
						titleslua_free();
						sky_show();
						t.game.levelloop = 0;
					}
					else
					{
						t.game.level = t.game.level + 1;
						if (t.game.level > t.game.levelmax)
						{
							//titles_gamewonpage ( );
							timestampactivity(0, "LUA script : win");
							sky_hide();
							titleslua_init();
							titleslua_main("win");
							#ifdef WICKEDENGINE
							//PE: We need a blocking run or screen is not displayed. t.game.levelloop = 0; will start title screen.
							//PE: @Lee not sure if these works different in vrquest ? so only wicked for now.
							titleslua_blocking_run();
							#endif
							titleslua_free();
							sky_show();
							t.game.levelloop = 0;
						}
						else
						{
							//titles_completepage ( );
							timestampactivity(0, "LUA script : nextlevel");
							sky_hide();
							titleslua_init();
							titleslua_main("nextlevel");
							#ifdef WICKEDENGINE
							//PE: We need a blocking run or screen is not displayed.
							//PE: @Lee not sure if these works different in vrquest ? so only wicked for now.
							titleslua_blocking_run();
							#endif
							sky_show();
						}
					}
				}
			}

		}
	}
	t.game.quitflag=0;

	//  If was in multiplayer session, no level loop currently
	if (  t.game.runasmultiplayer == 1 ) 
	{
		t.game.levelloop=0;
	}

	if (g.memgeneratedump == 1) {
		timestampactivity(0, "DumpImageList after freeing level data.");
		DumpImageList(); // PE: Dump image usage after level.
	}

	#ifdef WICKEDENGINE
	g_RecastDetour.cleanupDebugRender();
	g_bShowRecastDetourDebugVisuals = false;
	#endif

	// 250619 - very large levels can fragment 32 bit memory after a few levels
	// so this mode will restart the executable, and launch the new level
	// crude solution until 64 bit allows greater memory referencing
	if ( t.game.allowfragmentation == 2 )
		t.game.levelloop = 0;

	#ifdef WICKEDENGINE
	if (t.gamevisuals.bEndableAmbientMusicTrack)
	{
		//PE: Stop any ambient music tracks.
		int iFreeSoundID = g.temppreviewsoundoffset + 3;
		if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
		{
			StopSound(iFreeSoundID);
		}
		if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
	}
	int iFreeSoundID = g.temppreviewsoundoffset + 5;
	if (SoundExist(iFreeSoundID) == 1 && SoundPlaying(iFreeSoundID) == 1)
	{
		StopSound(iFreeSoundID);
	}
	if (SoundExist(iFreeSoundID) == 1) DeleteSound(iFreeSoundID);
	//PE: restore old terrain settings.
	ggterrain_global_render_params2.flags2 = old_render_params2;
	#endif

}

bool game_masterroot_levelloop_initcode(int iUseVRTest)
{
	// first hide rendering of 3D while we set up
	SyncMaskOverride ( 0 );

	//  Optionally set resolution for game and setup for dependencies
	timestampactivity(0,"_game_setresolution");
	if (  t.game.set.resolution == 1 ) 
	{
		game_setresolution ( );
		game_postresolutionchange ( );
		t.game.set.resolution=0;
	}

	//  One-off splash screen or animation
	if (  t.game.set.initialsplashscreen == 1 ) 
	{
		t.game.set.initialsplashscreen=0;
	}

	//  Setup level progression settings
	t.game.level=1;
	t.game.levelmax=1;
	t.game.levelloop=1;
	t.game.levelendingcycle=0;
	t.game.lostthegame=0;
	t.game.jumplevel_s="";
	strcpy ( t.game.pAdvanceWarningOfLevelFilename, "" );

	//  specify first level to load (same name as executable)
	if (  t.game.gameisexe == 1 ) 
	{
		t.tapp_s=Appname();
		for ( t.n = Len(t.tapp_s.Get()) ; t.n >= 1 ; t.n+= -1 )
		{
			if (  t.tapp_s.Get()[t.n-1] == '\\' || t.tapp_s.Get()[t.n-1] == '/' ) 
			{
				t.tapp_s=Right(t.tapp_s.Get(),Len(t.tapp_s.Get())-t.n);
				break;
			}
		}
		t.game.jumplevel_s=Left(t.tapp_s.Get(),Len(t.tapp_s.Get())-4);

#ifdef STORYBOARD

		//PE: Check if we are using a storyboard project
		strcpy(Storyboard.gamename, "");
		load_storyboard(t.game.jumplevel_s.Get());
		if (strlen(Storyboard.gamename) > 0)
		{
			//PE: Got a project. find first level to load.
			FindFirstLevel(g_Storyboard_First_Level_Node, g_Storyboard_First_fpm);
			g_Storyboard_Current_Level = g_Storyboard_First_Level_Node;
			strcpy(g_Storyboard_Current_fpm, g_Storyboard_First_fpm);
			//Clean name.
			
			std::string sLevelTitle = g_Storyboard_First_fpm;
			replaceAll(sLevelTitle, ".fpm", "");
			replaceAll(sLevelTitle, "mapbank\\", "");
			t.game.jumplevel_s = sLevelTitle.c_str();
		}
//		char project[MAX_PATH];
//		strcpy(project, "projectbank\\");
//		strcat(project, t.game.jumplevel_s);
//		strcat(project, "\\project.dat");

#endif
	}

	//  Title init - If this is just a test game, we only need to set default volumes
	timestampactivity(0,"_titles_init");
	if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
	{
		titles_init ( );
	}
	else
	{
		t.gamesounds.sounds=100;
		t.gamesounds.music=100;
	}

	// Do title page
	timestampactivity(0,"_titles_titlepage");
	if (t.game.gameisexe == 1 && t.game.ignoretitle == 0)
	{
		sky_hide();
		titles_loadingpageinit();
		titleslua_init();

		// this is a special flag set when quit from game (avoids rogue IMGUI renders that have been deleted in the restart)
		#ifndef PRODUCTCLASSIC
		extern bool bBlockImGuiUntilFurtherNotice;
		bBlockImGuiUntilFurtherNotice = false;
		#endif

		// 250619 - for straight-through level loading, still need to handle title resources
		// (which include sound loading/playing - fixes 3D sound delay issue?!?)
		if (g.iStandaloneIsReloading == 2)
		{
			titleslua_main_inandout("title");
		}
		else
		{
			// start title system loop
			titleslua_main("title");
			return true;
		}
	}

	// no title system loop, continue
	game_masterroot_levelloop_initcode_aftertitleloop();
	return false;
}

void game_masterroot_levelloop_initcode_aftertitleloop(void)
{
	// if game executable and not ignoring title system
	if (t.game.gameisexe == 1 && t.game.ignoretitle == 0)
	{
		titleslua_free ( );
		sky_show();
	}

	// 250619 - can relaunch game executable at a specific level (reduce memory fragmentation)
	if ( g.iStandaloneIsReloading == 2 )
	{
		t.game.jumplevel_s = g.sStandaloneIsReloadingLevel;
		t.luaglobal.gamestatechange = atoi(g.sStandaloneIsReloadingLevelGameStatChange.Get());
		g.sStandaloneIsReloadingLevel = "";
		g.sStandaloneIsReloadingLevelGameStatChange = "";
	}

	// Standaline Multiplayer HOST/JOIN screen
	if ( t.game.runasmultiplayer == 1 ) 
	{
		// Multiplayer init
		#ifdef VRTECH
		mp_fullinit();
		#endif
		g.mp.mode = MP_MODE_MAIN_MENU;
		timestampactivity(0,"_titles_steampage");
		t.game.cancelmultiplayer=0;
		SetCameraView (  0,0,1,1 );
		titles_steampage ( );
		if ( t.game.cancelmultiplayer == 1 ) 
		{
			// user selected BACK (cancel multiplayer)
			#ifdef VRTECH
			mp_fullclose();
			#endif
			t.game.levelloop=0;
		}
		else
		{
			// proceed into level loop where multiplayer spawn markers are detected and ghosts loaded
		}
	}

	// Initialise gun system (transcends per-level initialisations)
	t.game.levelplrstatsetup = 1;
	gun_restart ( );
	gun_resetactivateguns ( );
}

bool game_masterroot_levelloop_loopcode(int iUseVRTest)
{
	// Level loop will run while level progression is in progress
	if (t.game.levelloop != 1)
		return true;
	else
		return false;
}

void game_masterroot_levelloop_afterloopcode(int iUseVRTest)
{
	// Free any game resources
	game_freegame ( );

	if ( t.game.runasmultiplayer == 1 ) 
	{
		mp_free_game ( );
		mp_cleanupGame ( );
		if (  g.mp.goBackToEditor  ==  1 ) 
		{
			g.mp.goBackToEditor = 0;
			t.game.masterloop = 0;
		}
	}

	// get rid of debris and particles that may be lingering
	explosion_cleanup ( );

	#ifdef WICKEDENGINE
	ravey_particles_hide_all_particles();
	#endif // WICKEDENGINE

	// if ignored title, exit now
	if (  t.game.ignoretitle == 1 && t.game.runasmultiplayer == 0  )  t.game.masterloop = 0;

	// Master loop end
	t.game.allowfragmentation_mainloop = t.game.masterloop;
	if (t.game.allowfragmentation == 0 || t.game.allowfragmentation == 2) t.game.masterloop = 0; //break;
}

void game_masterroot_initcode(int iUseVRTest)
{
	#ifdef VRTECH
	// prevent any VR if VRtest is off
	if ( iUseVRTest == 0 ) g.vrglobals.GGVRUsingVRSystem = 0;
	#endif
	#ifdef WICKEDENGINE
	g_iActivelyUsingVRNow = iUseVRTest;
	#endif

	//  Load all one-off non-graphics assets
	timestampactivity(0,"_game_oneoff_nongraphics");
	game_oneoff_nongraphics ( );

	//  Master loop will run until whole game terminated
	t.game.masterloop=1;
}

#ifdef WICKEDENGINE
bool bDisplayedEarlyAccess = false;
#endif
bool game_masterroot_loopcode(int iUseVRTest)
{
	#ifdef WICKEDENGINE
	extern bool bSpecialStandalone;
	if (t.game.gameisexe == 1 && !bDisplayedEarlyAccess && !bSpecialStandalone)
	{
		//PE: This is blocking so can do it like this.
		void early_access_strandalone_welcome(void);
		early_access_strandalone_welcome();
		bDisplayedEarlyAccess = true;
	}
	#endif

	// state engine to handle nested loops (master / level / gameloop)
	if (g_iMasterRootState == 0)
	{
		if (game_masterroot_levelloop_initcode(iUseVRTest) == true)
		{
			// go to title loop first (below)
			g_iMasterRootState = 51;
		}
		else
		{
			// title system not used, continue
			g_iMasterRootState = 1;
		}
	}
	if (g_iMasterRootState == 1)
	{
		if (game_masterroot_levelloop_loopcode(iUseVRTest) == true)
		{
			g_iMasterRootState = 6;
		}
		else
		{
			g_iMasterRootState = 2;
		}
	}
	if (g_iMasterRootState == 2)
	{
		//PE: TESTGAME t.game.gameisexe = 1 here will produce a loading screen and hide everything.
		//PE: TESTGAME use t.game.jumplevel_s to set what level to load.
		game_masterroot_gameloop_initcode(iUseVRTest);
		g_iMasterRootState = 3;
	}
	if (g_iMasterRootState == 3)
	{
		if (game_masterroot_gameloop_loopcode(iUseVRTest) == true)
		{
			g_iMasterRootState = 4;
		}
	}
	if (g_iMasterRootState == 4)
	{
		game_masterroot_gameloop_afterloopcode(iUseVRTest);
		g_iMasterRootState = 5;
	}
	if (g_iMasterRootState == 5)
	{
		if (t.game.levelloop == 1 && Len(t.game.jumplevel_s.Get()) > 0 )
		{
			g_iMasterRootState = 1;
		}
		else
		{
			g_iMasterRootState = 6;
		}
	}
	if (g_iMasterRootState == 6)
	{
		game_masterroot_levelloop_afterloopcode(iUseVRTest);
		g_iMasterRootState = 0;
	}

	// title system state control
	if (g_iMasterRootState == 51)
	{
		if ( titleslua_main_loopcode() == true )
		{
			game_masterroot_levelloop_initcode_aftertitleloop();
			g_iMasterRootState = 2;
		}
	}

	// determine if end of master loop
	if (t.game.masterloop != 1)
		return true;
	else
		return false;
}

void game_masterroot_afterloopcode(int iUseVRTest)
{
	// End splash if EXE is advertising
	if ( t.game.set.endsplash == 1 ) 
	{
		t.game.set.endsplash=0;
	}

	#ifdef VRTECH
	// restore VR activity (vrtest flag has done its job)
	g.vrglobals.GGVRUsingVRSystem = 1;
	#ifdef WICKEDENGINE
	g_iActivelyUsingVRNow = 0;
	master.StopVR();
	#endif
	// restore normal rendering activity when finish game run
	SyncMaskOverride ( 0xFFFFFFFF );
	// cannot rely on postprocess to restore, so do so here when return
	SetCameraView ( 0, 0, 0, GetDisplayWidth(), GetDisplayHeight() );
	#endif
}

void game_masterroot(int iUseVRTest)
{
	bool bRunLoop = true;
	game_masterroot_initcode(iUseVRTest);
	g_iMasterRootState = 0;
	while (bRunLoop == true)
	{
		if (game_masterroot_loopcode(iUseVRTest) == true) bRunLoop = false;
	}
	game_masterroot_afterloopcode(iUseVRTest);
}

/* preserve original masterroot code
void game_masterroot ( int iUseVRTest )
{
	#ifdef VRTECH
	// prevent any VR if VRtest is off
	if ( iUseVRTest == 0 ) g.vrglobals.GGVRUsingVRSystem = 0;
	#endif

	//  Load all one-off non-graphics assets
	timestampactivity(0,"_game_oneoff_nongraphics");
	game_oneoff_nongraphics ( );

	//  Master loop will run until whole game terminated
	t.game.masterloop=1;
	while ( t.game.masterloop == 1 ) 
	{
		// first hide rendering of 3D while we set up
		SyncMaskOverride ( 0 );

		//  Optionally set resolution for game and setup for dependencies
		timestampactivity(0,"_game_setresolution");
		if (  t.game.set.resolution == 1 ) 
		{
			game_setresolution ( );
			game_postresolutionchange ( );
			t.game.set.resolution=0;
		}

		//  One-off splash screen or animation
		if (  t.game.set.initialsplashscreen == 1 ) 
		{
			t.game.set.initialsplashscreen=0;
		}

		//  Setup level progression settings
		t.game.level=1;
		t.game.levelmax=1;
		t.game.levelloop=1;
		t.game.levelendingcycle=0;
		t.game.lostthegame=0;
		t.game.jumplevel_s="";
		strcpy ( t.game.pAdvanceWarningOfLevelFilename, "" );

		//  specify first level to load (same name as executable)
		if (  t.game.gameisexe == 1 ) 
		{
			t.tapp_s=Appname();
			for ( t.n = Len(t.tapp_s.Get()) ; t.n >= 1 ; t.n+= -1 )
			{
				if (  t.tapp_s.Get()[t.n-1] == '\\' || t.tapp_s.Get()[t.n-1] == '/' ) 
				{
					t.tapp_s=Right(t.tapp_s.Get(),Len(t.tapp_s.Get())-t.n);
					break;
				}
			}
			t.game.jumplevel_s=Left(t.tapp_s.Get(),Len(t.tapp_s.Get())-4);
		}

		//  Title init - If this is just a test game, we only need to set default volumes
		timestampactivity(0,"_titles_init");
		if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
		{
			titles_init ( );
		}
		else
		{
			t.gamesounds.sounds=100;
			t.gamesounds.music=100;
		}

		// Do title page
		timestampactivity(0,"_titles_titlepage");
		if ( t.game.gameisexe == 1 && t.game.ignoretitle == 0 )
		{
			sky_hide();
			titles_loadingpageinit();
			titleslua_init ( );

			// 250619 - for straight-through level loading, still need to handle title resources
			// (which include sound loading/playing - fixes 3D sound delay issue?!?)
			if ( g.iStandaloneIsReloading == 2 )
				titleslua_main_inandout ( "title" );
			else
				titleslua_main ( "title" );

			titleslua_free ( );
			sky_show();
		}

		// 250619 - can relaunch game executable at a specific level (reduce memory fragmentation)
		if ( g.iStandaloneIsReloading == 2 )
		{
			t.game.jumplevel_s = g.sStandaloneIsReloadingLevel;
			t.luaglobal.gamestatechange = atoi(g.sStandaloneIsReloadingLevelGameStatChange.Get());
			g.sStandaloneIsReloadingLevel = "";
			g.sStandaloneIsReloadingLevelGameStatChange = "";
		}

		// Standaline Multiplayer HOST/JOIN screen
		if ( t.game.runasmultiplayer == 1 ) 
		{
			// Multiplayer init
			#ifdef VRTECH
			mp_fullinit();
			#endif
			g.mp.mode = MP_MODE_MAIN_MENU;
			timestampactivity(0,"_titles_steampage");
			t.game.cancelmultiplayer=0;
			SetCameraView (  0,0,1,1 );
			titles_steampage ( );
			if ( t.game.cancelmultiplayer == 1 ) 
			{
				// user selected BACK (cancel multiplayer)
				#ifdef VRTECH
				mp_fullclose();
				#endif
				t.game.levelloop=0;
			}
			else
			{
				// proceed into level loop where multiplayer spawn markers are detected and ghosts loaded
			}
		}

		// Initialise gun system (transcends per-level initialisations)
		t.game.levelplrstatsetup = 1;
		gun_restart ( );
		gun_resetactivateguns ( );

		// Level loop will run while level progression is in progress
		while ( t.game.levelloop == 1 ) 
		{
			// also hide rendering of 3D while we set up a new level
			SyncMaskOverride ( 0 );

			// Loading page
			timestampactivity(0,"_titles_loadingpageupdate");
			if ( t.game.gameisexe == 1 ) 
			{
				//titles_loadingpage ( );
				timestampactivity(0,"LUA script : loading");
				sky_hide();
				titleslua_init ( );
				titleslua_main ( "loading" );
				sky_show();
				t.game.levelloadprogress=0  ; titles_loadingpageupdate ( );
			}

			// Extract level files from FPM
			if ( t.game.runasmultiplayer == 1 ) 
			{
				// Multiplayer FPM loading
				g.projectfilename_s=g.mysystem.editorsGrideditAbs_s+"__multiplayerlevel__.fpm";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm";
				t.trerfeshvisualsassets=1;
				mapfile_loadproject_fpm ( );
				t.game.jumplevel_s="";
			}
			else
			{
				// Single player
				if ( Len(t.game.jumplevel_s.Get())> 0 ) //PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
				{
					// can override jumplevel with 'advanced warning level filename' when LOAD level from MAIN MENU
					if ( strcmp ( t.game.pAdvanceWarningOfLevelFilename, "" ) != NULL )
					{
						t.game.jumplevel_s = t.game.pAdvanceWarningOfLevelFilename;
						strcpy ( t.game.pAdvanceWarningOfLevelFilename, "" );
					}

					// work out first level from exe name (copied to jumplevel_s)
					g.projectfilename_s = g.mysystem.mapbank_s + t.game.jumplevel_s;				
					if ( cstr(Lower(Right(g.projectfilename_s.Get(),4))) != ".fpm" )
						g.projectfilename_s=g.projectfilename_s+".fpm";

					// 050316 - if not there, try all subfolders
					if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+g.projectfilename_s).Get()) == 0 ) 
					{
						// go into mapbank folder
						cstr tthisold_s =  "";
						tthisold_s=GetDir();
						SetDir ( g.mysystem.mapbankAbs_s.Get() );

						// scan for ALL files/folders
						ChecklistForFiles (  );
						for ( int c = 1 ; c<=  ChecklistQuantity(); c++ )
						{
							if (  ChecklistValueA(c) != 0 ) 
							{
								// only folders
								cstr tfolder_s = ChecklistString(c);
								if ( tfolder_s != "." && tfolder_s != ".." ) 
								{
									// skip . and .. folders
									cstr newlevellocation = g.mysystem.mapbank_s + tfolder_s + "\\" + t.game.jumplevel_s;
									if ( cstr(Lower(Right(newlevellocation.Get(),4))) != ".fpm" )
										newlevellocation = newlevellocation + ".fpm";

									// does this guessed file location exist
									if ( FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+newlevellocation).Get()) == 1 ) 
									{
										// found the level inside a nested folder
										g.projectfilename_s = newlevellocation; 
										break;
									}
								}
							}
						}
						SetDir ( tthisold_s.Get() );
					}

					// finally load the level in
					mapfile_loadproject_fpm ( );
					t.visuals=t.gamevisuals;
					t.game.jumplevel_s="";
				}
			}

			// reload gunspecs
			if (g.reloadWeaponGunspecs == 1)
			{
				gun_scaninall_dataonly();
			}

			// we first load extra guns into gun array EARLY (ahead of entity data load which assigns gunids to isweapon hasweapon)
			gun_tagmpgunstolist ( );

			// just load the entity data for now (rest in _game_loadinleveldata)
			timestampactivity(0,"_game_loadinentitiesdatainlevel");
			if ( t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
			{
				//  extra precaution, delete any old entities and LM objects
				if ( t.game.runasmultiplayer == 1 ) 
				{
					entity_delete ( );
					lm_removeold ( );
				}
				game_loadinentitiesdatainlevel ( );
			}

			// Load any extra material sounds associated with new entities (i.e. material(m).usedinlevel=1?)
			// NOTE: Level can collect materials (and material depth) and apply here to quicken material loader (2s)
			material_loadsounds ( 0 );

			// if multiplayer, detect spawn positions and add extra UBER characters
			if ( t.game.runasmultiplayer == 1 ) 
			{
				// these are the multiplayer start markers
				t.tnumberofstartmarkers = 0;
				g.mp.team = 0;
				g.mp.coop = 0;
				for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
				{
					t.mpmultiplayerstart[t.tc].active=0;
				}
				t.plrindex=1;
				t.tfoundAMultiplayerScript = 0;
				for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
				{
					// reset all updates
					t.entityelement[t.e].mp_updateOn = 0;
					t.entityelement[t.e].mp_isLuaChar = 0;
					t.entityelement[t.e].mp_rotateType = 0;
					t.entid=t.entityelement[t.e].bankindex;
					if ( t.entid>0 ) 
					{
						#ifdef VRTECH
						#else
						if ( t.entityprofile[t.entid].ismarker == 7 && t.plrindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
						{
							// to ensure mp game script always runs from any distance
							t.entityelement[t.e].eleprof.phyalways = 1;
							if ( t.entityelement[t.e].eleprof.aimain_s == "" ) 
							{
								t.entityelement[t.e].eleprof.aimain_s = "multiplayer_firstto10.lua";
							}
							if ( t.entityelement[t.e].eleprof.teamfield  !=  0 ) 
							{
								g.mp.team = 1;
							}
	
							//  only let one marker end up with a script otherwise we end up running the same script 8 times
							if ( t.tfoundAMultiplayerScript == 0 ) 
							{
								t.tfoundAMultiplayerScript = 1;
								// 12032015 0XX - Team Multiplayer - check for team mode
								if ( FileOpen(3)  ==  1  )  CloseFile (  3 );
								t.strwork = "" ; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
								OpenToRead (  3, t.strwork.Get() );
								g.mp.friendlyfireoff = 0;
								while ( FileEnd(3) == 0 ) 
								{
									t.tScriptLine_s = ReadString (  3 );
									t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
									if (  FindSubString(t.tScriptLine_s.Get(),"setmultiplayergamefriendlyfireoff") > 0 && FindSubString(t.tScriptLine_s.Get(),"--SetMultiplayerGameFriendlyFireOff")  <=  0 && FindSubString(t.tScriptLine_s.Get(),"-- SetMultiplayerGameFriendlyFireOff") <=  0 ) 
									{
										g.mp.friendlyfireoff = 1;
									}
								}
								CloseFile (  3 );
							}
							else
							{
								t.entityelement[t.e].eleprof.aimain_s = "";
							}
							++t.plrindex;
						}
						else
						{
							if (  t.entityprofile[t.entid].ischaracter  ==  0 ) 
							{
								if (  FileOpen(3)  ==  1  )  CloseFile (  3 );
								if (  t.entityelement[t.e].eleprof.aimain_s  !=  "" ) 
								{
									if (  FileExist(t.entityelement[t.e].eleprof.aimain_s.Get())  ==  1 ) 
									{
										t.strwork = ""; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
										OpenToRead (  3, t.strwork.Get() );
										while (  FileEnd(3)  ==  0 ) 
										{
											t.tScriptLine_s = ReadString (  3 );
											t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
											//  are the using ai?
											if (  FindSubString(t.tScriptLine_s.Get(),"AIEntityGoToPosition") > 0 ) 
											{
												t.entityelement[t.e].mp_isLuaChar = 1;
											}
										}
										CloseFile (  3 );
									}
								}
							}
						}
						#endif
					}
				}

				// Build multiplayer start markers
				t.thaveTeamAMarkers = 0;
				t.thaveTeamBMarkers = 0;
				t.tmpstartindex = 1;
				for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
				{
					t.entid=t.entityelement[t.e].bankindex;
					if ( t.entid>0 ) 
					{
						if ( t.entityprofile[t.entid].ismarker == 7 && t.tmpstartindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
						{
							// add start markers for free for all or team a
							if ( t.entityelement[t.e].eleprof.teamfield < 2 ) 
							{
								// a spawn GetPoint ( for the multiplayer )
								t.mpmultiplayerstart[t.tmpstartindex].active=1;
								t.mpmultiplayerstart[t.tmpstartindex].x=t.entityelement[t.e].x;
								// added 10 onto the y otherwise the players fall through the ground
								t.mpmultiplayerstart[t.tmpstartindex].y=t.entityelement[t.e].y+50;
								t.mpmultiplayerstart[t.tmpstartindex].z=t.entityelement[t.e].z;
								t.mpmultiplayerstart[t.tmpstartindex].angle=t.entityelement[t.e].ry;
								t.thaveTeamAMarkers = 1;
								++t.tnumberofstartmarkers;
								++t.tmpstartindex;
							}
						}
					}
				}
				// add team b markers if in team mode
				#ifdef VRTECH
				#else
				if ( g.mp.team == 1 ) 
				{
					for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if ( t.entid>0 ) 
						{
							if ( t.entityprofile[t.entid].ismarker == 7 && t.tmpstartindex <= MP_MAX_NUMBER_OF_PLAYERS ) 
							{
								// add start markers for team b
								if ( t.entityelement[t.e].eleprof.teamfield == 2 ) 
								{
									// a spawn GetPoint (  for the multiplayer )
									t.mpmultiplayerstart[t.tmpstartindex].active=1;
									t.mpmultiplayerstart[t.tmpstartindex].x=t.entityelement[t.e].x;
									// added 10 onto the y otherwise the players fall through the ground
									t.mpmultiplayerstart[t.tmpstartindex].y=t.entityelement[t.e].y+50;
									t.mpmultiplayerstart[t.tmpstartindex].z=t.entityelement[t.e].z;
									t.mpmultiplayerstart[t.tmpstartindex].angle=t.entityelement[t.e].ry;
									t.thaveTeamBMarkers = 1;
									++t.tnumberofstartmarkers;
									++t.tmpstartindex;
								}
							}
						}
					}
				}
				#endif

				// check for coop mode
				g.mp.coop = 0;
				#ifdef VRTECH
				#else
				if ( g.mp.team == 1 ) 
				{
					if ( (t.thaveTeamAMarkers  ==  1 && t.thaveTeamBMarkers  ==  0) || (t.thaveTeamAMarkers  ==  0 && t.thaveTeamBMarkers  ==  1) || (t.thaveTeamAMarkers  ==  0 && t.thaveTeamBMarkers  ==  0) ) 
					{
						g.mp.coop = 1;
						mp_setupCoopTeam ( );
					}
				}
				#endif

				// perhaps it is a solo game with a start maker only
				#ifdef VRTECH
				bool bHaveRegularStartMarker = false;
				#endif
				if ( g.mp.coop == 0 && t.tnumberofstartmarkers == 0 ) 
				{
					for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if ( t.entid>0 ) 
						{
							if ( t.entityprofile[t.entid].ismarker == 1 ) 
							{
								// a spawn GetPoint ( for the multiplayer )
								#ifdef VRTECH
								bHaveRegularStartMarker = true;
								#endif
								t.mpmultiplayerstart[1].active=1;
								t.mpmultiplayerstart[1].x=t.entityelement[t.e].x;
								// added 10 onto the y otherwise the players fall through the ground
								t.mpmultiplayerstart[1].y=t.entityelement[t.e].y+50;
								t.mpmultiplayerstart[1].z=t.entityelement[t.e].z;
								t.mpmultiplayerstart[1].angle=t.entityelement[t.e].ry;
								t.entityelement[t.e].eleprof.phyalways = 1;

								#ifdef VRTECH
								#else
								// switch it to multiplayer script
								t.entityelement[t.e].eleprof.aimain_s = "multiplayer_firstto10.lua";
								t.tnumberofstartmarkers = 1;
								g.mp.coop = 1;
								g.mp.team = 1;
								mp_setupCoopTeam ( );

								// Check for friendly fire off
								if ( FileOpen(3) == 1 )  CloseFile ( 3 );
								t.strwork ="" ; t.strwork = t.strwork + "scriptbank\\"+t.entityelement[t.e].eleprof.aimain_s;
								OpenToRead (  3, t.strwork.Get() );
								g.mp.friendlyfireoff = 0;
								while (  FileEnd(3)  ==  0 ) 
								{
									t.tScriptLine_s = ReadString (  3 );
									t.tScriptLine_s = Lower(t.tScriptLine_s.Get());
									if (  FindSubString(t.tScriptLine_s.Get(),"setmultiplayergamefriendlyfireoff") > 0 && FindSubString(t.tScriptLine_s.Get(),"--SetMultiplayerGameFriendlyFireOff")  <=  0 && FindSubString(t.tScriptLine_s.Get(),"-- SetMultiplayerGameFriendlyFireOff") <=  0 ) 
									{
										g.mp.friendlyfireoff = 1;
									}
								}
								CloseFile (  3 );
								#endif
							}
						}
					}
				}

				//  if multiplayer and not coop, disable ai characters
				#ifdef PHOTONMP
				 // Photon retains all characters in map
				#else
				 if ( t.game.runasmultiplayer == 1 && g.mp.coop == 0 ) 
				 {
					for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if ( t.entid>0 ) 
						{
							if ( t.entityprofile[t.entid].ischaracter  ==  1 ) 
							{
								t.entityelement[t.e].destroyme=1;
							}
						}
					}
				 }
				#endif

				// if multiplayer and coop, setup ai for switching who control them, depending on gameplay circumstances
				#ifdef VRTECH
				#else
				if ( t.game.runasmultiplayer == 1 && g.mp.coop == 1 ) 
				{
					for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if ( t.entid>0 ) 
						{
							if ( t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar ) 
							{
								t.entityelement[t.e].mp_coopControlledByPlayer = -1;
							}
						}
					}
				}
				#endif

				// if no multiplayer markers, put some at the default height
				#ifdef VRTECH
				if ( t.tnumberofstartmarkers == 0 && bHaveRegularStartMarker == false ) 
				#else
				if ( t.tnumberofstartmarkers == 0 ) 
				#endif
				{
					for ( t.tloop = 1; t.tloop <= MP_MAX_NUMBER_OF_PLAYERS; t.tloop++ )
					{
						t.mpmultiplayerstart[t.tloop].active=1;
						t.mpmultiplayerstart[t.tloop].x=GGORIGIN_X;
						//  added 10 onto the y otherwise the players fall through the ground
						t.mpmultiplayerstart[t.tloop].y=BT_GetGroundHeight(t.terrain.TerrainID,GGORIGIN_X,GGORIGIN_Z)+50;
						t.mpmultiplayerstart[t.tloop].z=GGORIGIN_Z;
						t.mpmultiplayerstart[t.tloop].angle=0;
					}
				}
				#ifdef VRTECH
				#else
				// if coop and only 1 marker, make some more
				if ( g.mp.coop == 1 && t.tnumberofstartmarkers == 1 ) 
				{
					for ( t.tloop = 2 ; t.tloop <= MP_MAX_NUMBER_OF_PLAYERS; t.tloop++ )
					{
						t.mpmultiplayerstart[t.tloop].active=1;
						t.mpmultiplayerstart[t.tloop].x=t.mpmultiplayerstart[1].x;
						//  added 10 onto the y otherwise the players fall through the ground
						t.mpmultiplayerstart[t.tloop].y=t.mpmultiplayerstart[1].y;
						t.mpmultiplayerstart[t.tloop].z=t.mpmultiplayerstart[1].z;
						t.mpmultiplayerstart[t.tloop].angle=t.mpmultiplayerstart[1].angle;
					}
				}
				#endif

				// reserve max multiplayer characters (all weapon animations included)
				Dim ( t.tubindex,2+MP_MAX_NUMBER_OF_PLAYERS  );
				#ifdef WICKEDENGINE
				t.ent_s=g.rootdir_s+"charactercreatorplus\\Uber Character.fpe";
				#else
				#ifdef PHOTONMP
				 t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Character.fpe";
				#else
				 t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Soldier.fpe";
				#endif
				#endif
				entity_addtoselection_core ( );
				t.tubindex[0]=t.entid;
				t.entityprofile[t.tubindex[0]].ischaracter=0;
				t.entityprofile[t.tubindex[0]].collisionmode=12;
				t.entityprofile[t.tubindex[0]].aimain_s = "";

				#ifdef PHOTONMP
				 // No teams - no combat!
				#else
				 if ( g.mp.team == 1 && g.mp.coop == 0 ) 
				 {
					t.ent_s=g.rootdir_s+"entitybank\\characters\\Uber Soldier Red.fpe";
					entity_addtoselection_core ( );
					t.tubindex[1]=t.entid;
					t.entityprofile[t.tubindex[1]].ischaracter=0;
					t.entityprofile[t.tubindex[1]].collisionmode=12;
					// No lua script for player chars
					t.entityprofile[t.tubindex[1]].aimain_s = "";
					t.tti = 1;
				 }
				 #ifdef VRTECH
				 #else
				 // add any character creator player avatars in
				 for ( t.tcustomAvatarCount = 0 ; t.tcustomAvatarCount<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tcustomAvatarCount++ )
				 {
					//  check if there is a custom avatar
					if (  t.mp_playerAvatars_s[t.tcustomAvatarCount]  !=  "" ) 
					{
						//  there is so lets built a temp fpe file from it
						t.ent_s=g.rootdir_s+"entitybank\\user\\charactercreator\\customAvatar_"+Str(t.tcustomAvatarCount)+".fpe";
						t.avatarFile_s = t.ent_s;
						t.avatarString_s = t.mp_playerAvatars_s[t.tcustomAvatarCount];
						characterkit_makeMultiplayerCharacterCreatorAvatar ( );
						entity_addtoselection_core ( );
						characterkit_removeMultiplayerCharacterCreatorAvatar ( );
						t.tubindex[t.tcustomAvatarCount+2]=t.entid;
						t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].ischaracter=0;
						t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].collisionmode=12;
						// No lua script for player chars
						t.entityprofile[t.tubindex[t.tcustomAvatarCount+2]].aimain_s = "";
					}
				 }
				 #endif
				#endif

				#ifdef VRTECH
				// add any character creator player avatars in
				for ( t.tcustomAvatarCount = 0 ; t.tcustomAvatarCount <= MP_MAX_NUMBER_OF_PLAYERS-1; t.tcustomAvatarCount++ )
				{
					t.mp_playerAvatarLoaded[t.tcustomAvatarCount] = false;
				}
				t.bTriggerAvatarRescanAndLoad = true;
				game_scanfornewavatars ( false );
				#endif

				// store ttiswitch for tti as multiplayer avatars can upset the 0->1 switching!
				t.ttiswitch = 1;
				for ( t.plrindex = 1 ; t.plrindex <= MP_MAX_NUMBER_OF_PLAYERS; t.plrindex++ )
				{
					// Add the max number of players into the level if there are start markers or not
					if ( g.mp.team == 1 && g.mp.coop == 0 ) 
					{
						t.ttiswitch = 1 - t.ttiswitch;
					}
					else
					{
						t.ttiswitch = 0;
					}
					t.tti = t.ttiswitch;

					// check if the player has their own avatar
					if ( t.mp_playerAvatars_s[t.plrindex-1] != "" ) 
					{
						t.tti = t.plrindex-1+2;
					}

					t.ubercharacterindex = t.tubindex[t.tti];
					t.entitymaintype=1 ; t.entitybankindex=t.ubercharacterindex;
					t.gridentityeditorfixed=0;
					t.gridentitystaticmode=0;
					t.gridentityhasparent=0;
					t.tfoundone = 0;
					if ( t.mpmultiplayerstart[t.plrindex].active == 1 ) 
					{
						t.tfoundone = 1;
						t.gridentityposx_f=t.mpmultiplayerstart[t.plrindex].x;
						t.gridentityposy_f=t.mpmultiplayerstart[t.plrindex].y;
						t.gridentityposz_f=t.mpmultiplayerstart[t.plrindex].z;
					}
					else
					{
						t.tonetotry = t.plrindex/2;
						if ( t.tonetotry > 0 ) 
						{
							t.tfoundone = 1;
							if ( t.mpmultiplayerstart[t.tonetotry].active == 1 ) 
							{
								t.gridentityposx_f=t.mpmultiplayerstart[t.tonetotry].x;
								t.gridentityposy_f=t.mpmultiplayerstart[t.tonetotry].y;
								t.gridentityposz_f=t.mpmultiplayerstart[t.tonetotry].z;
							}
						}
					}
					if ( t.tfoundone == 0 ) 
					{
						if ( t.mpmultiplayerstart[1].active == 1 ) 
						{
							t.gridentityposx_f=t.mpmultiplayerstart[1].x;
							t.gridentityposy_f=t.mpmultiplayerstart[1].y;
							t.gridentityposz_f=t.mpmultiplayerstart[1].z;
						}
					}
					t.gridentityrotatex_f=0;
					t.gridentityrotatey_f=t.mpmultiplayerstart[t.plrindex].angle;
					t.gridentityrotatez_f=0;
					t.gridentityscalex_f=100;
					t.gridentityscaley_f=100;
					t.gridentityscalez_f=100;
					entity_fillgrideleproffromprofile ( );
					entity_addentitytomap_core ( );
					t.mpmultiplayerstart[t.plrindex].ghostentityindex=t.e;

					// Grab the entity number for steam to use
					t.mp_playerEntityID[t.plrindex-1] = t.e;
					t.entityprofile[t.ubercharacterindex].ismultiplayercharacter=1;
					t.entityprofile[t.ubercharacterindex].hasweapon_s="";
					t.entityprofile[t.ubercharacterindex].hasweapon=0;
					t.entityprofile[t.ubercharacterindex].aimain_s = "";
				}

				#ifdef VRTECH
				//need this later in game for dynamic avatar loading
				//UnDim ( t.tubindex );
				#else
				UnDim ( t.tubindex );
				#endif
			}

			// in standalone, no IDE feeding test level, so load it in
			timestampactivity(0,"_game_loadinleveldata");
			if ( t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
			{
				game_loadinleveldata ( );
			}

			//  Prepare this level
			game_preparelevel ( );
			game_preparelevel_forplayer ( );
			game_preparelevel_finally ( );

			// Load any light map objects if available
			timestampactivity(0,"load lightmapped objects");
			lm_loadscene ( );
			
			g.merged_new_objects = 0;
			if ( t.tlmloadsuccess == 0  ) { //&& !g.disable_drawcall_optimizer

				//###################################################################
				//#### PE: Very simple but effectively draw call optimizer       ####
				//#### Could be made more intelligent when time allow :)         ####
				//#### On a 2000 object level it takes below 2 sec to run.       ####
				//#### setup.ini "drawcalloptimizer=1" will optimize everything  ####
				//#### setup.ini "drawcalloptimizer=0" only fpe settings counts. ####
				//#### .fpe "drawcalloptimizer=1" will optimize this object.     ####
				//#### .fpe "drawcalloptimizer=0" will NOT optimize object.      ####
				//#### .fpe "drawcalloptimizeroff=1" will NOT optimize object.   ####
				//#### (drawcalloptimizeroff is used when you optimize           ####
				//####  everything but have problems with a object )             ####
				//#### .fpe "drawcallscaleadjust" adjust scale.                  ####
				//###################################################################

				#define DC_DISTANCE 1000
				#define MAX_DRAWPRIMITIVES 32765 // max faces.
				#define MAX_DRAWVERTEX 65520 // max vertex 65530.

				timestampactivity(0, "draw call optimizer.");
				for (t.e = 1; t.e <= g.entityelementlist; t.e++)
				{
					if (t.entityelement[t.e].obj > 0 && t.e < g.entityelementlist )
					{
						t.entityelement[t.e].dc_merged = false;
						if (t.entityelement[t.e].draw_call_obj > 0) {

							if (t.entityelement[t.e].draw_call_obj > 0 && ObjectExist(t.entityelement[t.e].draw_call_obj) == 1) {
								DeleteObject(t.entityelement[t.e].draw_call_obj);
								t.entityelement[t.e].draw_call_obj = 0;
							}
						}
					}
				}

				for (t.e = 1; t.e <= g.entityelementlist; t.e++)
				{
					t.entid = t.entityelement[t.e].bankindex;
					t.obj = t.entityelement[t.e].obj;

					if (t.obj > 0 && t.e < g.entityelementlist && t.entityelement[t.e].dc_merged == false && (g.globals.drawcalloptimizer==1 || t.entityprofile[t.entid].drawcalloptimizer == 1) && t.entityprofile[t.entid].drawcalloptimizeroff == 0 && t.entityprofile[t.entid].isimmobile != 1 && t.entityelement[t.e].eleprof.isimmobile != 1 && t.entityelement[t.e].eleprof.spawnatstart == 1)
					{

						struct OrderByObjectDistance
						{
							bool operator()(int pObjectA, int pObjectB)
							{
								
								if (t.entityelement[pObjectA].dc_distance < t.entityelement[pObjectB].dc_distance)
									return true;

								if (t.entityelement[pObjectA].dc_distance == t.entityelement[pObjectB].dc_distance)
									return true;
								return false;
							}
						};

						int nextObjeid = 0;
						std::vector< int >     vObjList;
						if (1 == 1) {
							//PE: Sort a list by object,distance to increase hit rate.
							
							if (ObjectExist(t.obj)) {
								//for (int i = t.e + 1; i <= g.entityelementlist; i++) {
								for (int i = 1; i <= g.entityelementlist; i++) {
									int testobj = t.entityelement[i].obj;
									int iEntid = t.entityelement[i].bankindex;
									
									if (testobj > 0 && i != t.e && ObjectExist(testobj) && t.entityelement[i].dc_merged == false && t.entityprofile[iEntid].isimmobile != 1 && t.entityelement[i].eleprof.isimmobile != 1 && t.entityelement[i].staticflag == 1 && t.entityelement[i].eleprof.spawnatstart == 1) {

										sObject* pObject = g_ObjectList[t.obj];
										int instanceonly = 0;
										if (pObject && pObject->pInstanceOfObject) {
											pObject = pObject->pInstanceOfObject;
										}

										sObject* pObjectTest = g_ObjectList[testobj];
										if (pObjectTest && pObjectTest->pInstanceOfObject) {
											pObjectTest = pObjectTest->pInstanceOfObject;
										}

										if (pObject && pObjectTest && pObject == pObjectTest) {

											t.tdx_f = t.entityelement[t.e].x - t.entityelement[i].x;
											t.tdz_f = t.entityelement[t.e].z - t.entityelement[i].z;
											t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));

											t.entityelement[i].dc_distance = t.tdd_f;
											vObjList.push_back(i);
										}

									}
								}
								//Sort list
								std::sort(vObjList.begin(), vObjList.end(), OrderByObjectDistance());

							}
						}

						if (vObjList.size() > 0)
							nextObjeid = vObjList[0];

						int glueid = t.entityelement[nextObjeid].bankindex;
						int glueobj = t.entityelement[nextObjeid].obj;

						if (vObjList.size() > 0 && glueobj > 0 && ObjectExist(t.obj) && ObjectExist(glueobj))
						{
							bool validshader = false;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_basic.fx") == 0)
								validshader = true;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_tree.fx") == 0)
								validshader = true;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\entity_basic.fx") == 0)
								validshader = true;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_illum.fx") == 0)
								validshader = true;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "") == 0)
								validshader = true;
							if(t.entityprofile[t.entid].animmax == 0)
								validshader = true;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\treea_basic.fx") == 0)
								validshader = false;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_anim.fx") == 0)
								validshader = false;
							#ifdef VRTECH
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_animwithtran.fx") == 0)
								validshader = false;
							#endif
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_treea.fx") == 0)
								validshader = false;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_anim8bone.fx") == 0)
								validshader = false;
							if (strcmp(Lower(t.entityprofile[t.entid].effect_s.Get()), "effectbank\\reloaded\\apbr_animwithtran.fx") == 0)
								validshader = false;
							if (t.entityprofile[t.entid].animmax > 0)
								validshader = false;
							
							if (validshader && t.entityprofile[t.entid].ismarker == 0 && t.entityprofile[t.entid].isebe == 0 && t.entityprofile[t.entid].transparency == 0 && t.entityelement[nextObjeid].staticflag == 1 && t.entityprofile[t.entid].isimmobile != 1 && t.entityelement[t.e].eleprof.isimmobile != 1 && t.entityelement[t.e].eleprof.spawnatstart == 1 && t.entityelement[t.e].staticflag == 1)
							{

								//Validate if same master object.
								sObject* pObject = g_ObjectList[t.obj];
								int instanceonly = 0;
								if (pObject && pObject->pInstanceOfObject) {
									pObject = pObject->pInstanceOfObject;
									instanceonly++;
								}

								sObject* pObject2 = g_ObjectList[glueobj];
								if (pObject2 && pObject2->pInstanceOfObject) {
									pObject2 = pObject2->pInstanceOfObject;
									instanceonly++;
								}

								int iMeshWithTexture = -1;
								int iFrameVertex = 0;
								if (pObject)
								{
									for (int i = 0; i < pObject->iFrameCount; i++)
									{
										if (pObject->ppFrameList[i]->pMesh)
										{
											iMeshWithTexture = i;
											iFrameVertex += pObject->ppFrameList[i]->pMesh->dwVertexCount;
											//break;
										}
									}
								}
								if(iFrameVertex > 0x8000) //Cant merge to many vertex.
									instanceonly = 0;
								//PE: Cant do multi material for now.
								//CloneObject(destobj, t.obj, 101); do not support this.
								if (iMeshWithTexture >= 0 && pObject->ppFrameList[iMeshWithTexture]->pMesh) {
									if (pObject->ppFrameList[iMeshWithTexture]->pMesh->bUseMultiMaterial)
										instanceonly = 0;
								}

								//PE: Keep objects distance below DC_DISTANCE for best culling.
								if (pObject && pObject2 && instanceonly >= 2  && t.entityelement[nextObjeid].dc_distance < DC_DISTANCE && g.merged_new_objects < 4980 ) {
									
									if (pObject == pObject2) {
										//Same master glue it.

										if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);

										float gluescalex = ObjectScaleX(glueobj);
										float gluescaley = ObjectScaleY(glueobj);
										float gluescalez = ObjectScaleZ(glueobj);

										float src_scalex = ObjectScaleX(t.obj);
										float src_scaley = ObjectScaleY(t.obj);
										float src_scalez = ObjectScaleZ(t.obj);

										float scaleadjust = t.entityprofile[t.entid].drawcallscaleadjust;

										int tmpobj = (g.merged_new_objects+100) + 87000; //PE: TODO change 85000
										if (g_ObjectList[tmpobj])
										{
											if (g_ObjectList[tmpobj]->pFrame)
											{
												DeleteObject(tmpobj);
											}
										}

										CloneObject(tmpobj, t.obj);

										//PE: Hmm the lod removal could be improved.
										int bestlod = -1;
										PerformCheckListForLimbs(tmpobj);
										for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
										{
											t.tname_s = Lower(ChecklistString(t.c));

											LPSTR pRightFive = "";
											if (strlen(t.tname_s.Get()) > 5)
												pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;

											if ( (t.tname_s == "lod_0" || stricmp(pRightFive,"_lod0") == 0 ) ) bestlod = 0;
											if ( (t.tname_s == "lod_1" || stricmp(pRightFive,"_lod1") == 0 ) && (bestlod < 0 || bestlod > 1) )  bestlod = 1;
											if ( (t.tname_s == "lod_2" || stricmp(pRightFive,"_lod2") == 0 ) && (bestlod < 0) )  bestlod = 2;

											if (t.entityprofile[t.entid].resetlimbmatrix == 1)
											{
												OffsetLimb(tmpobj, t.c - 1, 0, 0, 0, 0);
											}
										}
										if (bestlod >= 0) {
											for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
											{
												t.tname_s = Lower(ChecklistString(t.c));
												LPSTR pRightFive = "";
												if (strlen(t.tname_s.Get()) > 5)
													pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;

												if (bestlod == 0 && ( t.tname_s == "lod_1" || t.tname_s == "lod_2" || t.tname_s == "lod_3") ) {
													RemoveLimb(tmpobj, t.c - 1);
												}
												if (bestlod == 0 && (stricmp(pRightFive, "_lod1") == 0 || stricmp(pRightFive, "_lod2") == 0 || stricmp(pRightFive, "_lod3") == 0 )) {
													RemoveLimb(tmpobj, t.c - 1);
												}
												if (bestlod == 1 && (t.tname_s == "lod_2" || stricmp(pRightFive, "_lod2") == 0) ) {
													RemoveLimb(tmpobj, t.c - 1);
												}
												if (bestlod == 2 && (t.tname_s == "lod_3" || stricmp(pRightFive, "_lod3") == 0 ) ) {
													RemoveLimb(tmpobj, t.c - 1);
												}
											}
										}

										PositionObject(tmpobj, 0, 0, 0); //PE: Need to be at 0,0,0
										ScaleObject(tmpobj, 100, 100, 100); //PE: no scale.

										int iAfterPolygonTotal = 0;
										int iAfterVertex = 0;

										sObject* pAfterObject = g_ObjectList[tmpobj];
										if (pAfterObject)
										{
											if (pAfterObject->iMeshCount>0)
											{
												for (int iM = 0; iM<pAfterObject->iMeshCount; iM++)
												{
													sMesh* pMesh = pAfterObject->ppMeshList[iM];
													if (pMesh)
													{
														iAfterPolygonTotal += pMesh->iDrawPrimitives;
														iAfterVertex += pMesh->dwVertexCount;
													}
												}
											}
										}
//										printf("after %d vertex %d", iAfterPolygonTotal, iAfterVertex);
						

										if (pAfterObject && pAfterObject->iMeshCount == 0) {
											//CloneObject failed.
											if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);
										}
										else {
											MakeMeshFromObject(g.meshlightmapwork, tmpobj); //PE: mesh from source.
										}

										if( (iAfterPolygonTotal * 2 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 2 < MAX_DRAWVERTEX)  && GetMeshExist(g.meshlightmapwork) == 1)
										{
											int destobj = g.merged_new_objects + 87000; //PE: TODO change 85000 , perhaps reverse from 90000 ?
											if (g_ObjectList[destobj])
											{
												if (g_ObjectList[destobj]->pFrame)
												{
													DeleteObject(destobj);
												}
											}

											t.tmasterx_f = ObjectPositionX(t.obj);
											t.tmastery_f = ObjectPositionY(t.obj);
											t.tmasterz_f = ObjectPositionZ(t.obj);

											//CloneObject(destobj, tmpobj); //We use the cleaned tmpobj instead of t.obj
											MakeObject(destobj, g.meshlightmapwork, -1); //Use mesh to prevent any transforms.

											int testypos = 0; //50,150
											PositionObject(destobj, 0, 0, 0); //PE: Need to be at 0,0,0
											ScaleObject(destobj, 100, 100, 100); //PE: no scale.

											float src_angx = ObjectAngleX(t.obj);
											float src_angy = ObjectAngleY(t.obj);
											float src_angz = ObjectAngleZ(t.obj);

											PerformCheckListForLimbs(destobj);
											AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

											t.tox_f = ObjectPositionX(glueobj) - t.tmasterx_f;
											t.toy_f = ObjectPositionY(glueobj) - t.tmastery_f;
											t.toz_f = ObjectPositionZ(glueobj) - t.tmasterz_f;

											OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

											//PE: not needed anymore fixed above when we reset scale to 100,100,100
//											if (t.entityprofile[t.entid].scale != 100 ) {
//												float scalejust = t.entityprofile[t.entid].scale - 100;
//												gluescalex -= scalejust;
//												gluescaley -= scalejust;
//												gluescalez -= scalejust;
//											}
											
											RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj), ObjectAngleY(glueobj), ObjectAngleZ(glueobj));
											ScaleLimb(destobj, ChecklistQuantity(), gluescalex + scaleadjust, gluescaley + scaleadjust, gluescalez + scaleadjust);
											for (int i = ChecklistQuantity()-1; i >= 0; i--) {
												RotateLimb(destobj, i, src_angx, src_angy, src_angz);
												ScaleLimb(destobj, i, ObjectScaleX(t.obj) + scaleadjust, ObjectScaleY(t.obj) + scaleadjust, ObjectScaleZ(t.obj) + scaleadjust);
											}

											//PE: TODO if possible add one more object.

											bool additionaladded2 = false;
											int glueid2 = 0;
											int glueobj2 = 0;
							
											if ((iAfterPolygonTotal * 3 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 3 < MAX_DRAWVERTEX) && vObjList.size() > 1 ) {
												nextObjeid = vObjList[1];
												glueid2 = t.entityelement[nextObjeid].bankindex;
												glueobj2 = t.entityelement[nextObjeid].obj;

												sObject* pObject3 = g_ObjectList[glueobj2];
												if (pObject3 && pObject3->pInstanceOfObject) {
													pObject3 = pObject3->pInstanceOfObject;
													if (pObject3 == pObject2) {
														t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
														t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
														t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
														if (t.tdd_f < DC_DISTANCE) {
															//Object ok add.
															float gluescalex2 = ObjectScaleX(glueobj2);
															float gluescaley2 = ObjectScaleY(glueobj2);
															float gluescalez2 = ObjectScaleZ(glueobj2);
															//Its the same master so reuse g.meshlightmapwork
															PerformCheckListForLimbs(destobj);
															AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

															t.tox_f = ObjectPositionX(glueobj2) - t.tmasterx_f;
															t.toy_f = ObjectPositionY(glueobj2) - t.tmastery_f;
															t.toz_f = ObjectPositionZ(glueobj2) - t.tmasterz_f;

															OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

															RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj2), ObjectAngleY(glueobj2), ObjectAngleZ(glueobj2));
															ScaleLimb(destobj, ChecklistQuantity(), gluescalex2 + scaleadjust, gluescaley2 + scaleadjust, gluescalez2 + scaleadjust);
															additionaladded2 = true;
														}
													}
												}
											}


											bool additionaladded3 = false;
											int glueid3 = 0;
											int glueobj3 = 0;
											if ((iAfterPolygonTotal * 4 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 4 < MAX_DRAWVERTEX)  && vObjList.size() > 2) {
												nextObjeid = vObjList[2];
												glueid3 = t.entityelement[nextObjeid].bankindex;
												glueobj3 = t.entityelement[nextObjeid].obj;

												sObject* pObject4 = g_ObjectList[glueobj3];
												if (pObject4 && pObject4->pInstanceOfObject) {
													pObject4 = pObject4->pInstanceOfObject;
													if (pObject4 == pObject2) {
														t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
														t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
														t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
														if (t.tdd_f < DC_DISTANCE) {
															//Object ok add.
															float gluescalex3 = ObjectScaleX(glueobj3);
															float gluescaley3 = ObjectScaleY(glueobj3);
															float gluescalez3 = ObjectScaleZ(glueobj3);
															//Its the same master so reuse g.meshlightmapwork
															PerformCheckListForLimbs(destobj);
															AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

															t.tox_f = ObjectPositionX(glueobj3) - t.tmasterx_f;
															t.toy_f = ObjectPositionY(glueobj3) - t.tmastery_f;
															t.toz_f = ObjectPositionZ(glueobj3) - t.tmasterz_f;

															OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

															RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj3), ObjectAngleY(glueobj3), ObjectAngleZ(glueobj3));
															ScaleLimb(destobj, ChecklistQuantity(), gluescalex3 + scaleadjust, gluescaley3 + scaleadjust, gluescalez3 + scaleadjust);
															additionaladded3 = true;
														}
													}
												}
											}

											bool additionaladded4 = false;
											int glueid4 = 0;
											int glueobj4 = 0;
											if ((iAfterPolygonTotal * 5 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 5 < MAX_DRAWVERTEX)  && vObjList.size() > 3) {
												nextObjeid = vObjList[3];
												glueid4 = t.entityelement[nextObjeid].bankindex;
												glueobj4 = t.entityelement[nextObjeid].obj;

												sObject* pObject5 = g_ObjectList[glueobj4];
												if (pObject5 && pObject5->pInstanceOfObject) {
													pObject5 = pObject5->pInstanceOfObject;
													if (pObject5 == pObject2) {
														t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
														t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
														t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
														if (t.tdd_f < DC_DISTANCE) {
															//Object ok add.
															float gluescalex4 = ObjectScaleX(glueobj4);
															float gluescaley4 = ObjectScaleY(glueobj4);
															float gluescalez4 = ObjectScaleZ(glueobj4);
															//Its the same master so reuse g.meshlightmapwork
															PerformCheckListForLimbs(destobj);
															AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

															t.tox_f = ObjectPositionX(glueobj4) - t.tmasterx_f;
															t.toy_f = ObjectPositionY(glueobj4) - t.tmastery_f;
															t.toz_f = ObjectPositionZ(glueobj4) - t.tmasterz_f;

															OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

															RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj4), ObjectAngleY(glueobj4), ObjectAngleZ(glueobj4));
															ScaleLimb(destobj, ChecklistQuantity(), gluescalex4 + scaleadjust, gluescaley4 + scaleadjust, gluescalez4 + scaleadjust);
															additionaladded4 = true;
														}
													}
												}
											}

											bool additionaladded5 = false;
											int glueid5 = 0;
											int glueobj5 = 0;
											if ((iAfterPolygonTotal * 6 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 6 < MAX_DRAWVERTEX)  && vObjList.size() > 4) {
												nextObjeid = vObjList[4];
												glueid5 = t.entityelement[nextObjeid].bankindex;
												glueobj5 = t.entityelement[nextObjeid].obj;

												sObject* pObject6 = g_ObjectList[glueobj5];
												if (pObject6 && pObject6->pInstanceOfObject) {
													pObject6 = pObject6->pInstanceOfObject;
													if (pObject6 == pObject2) {
														t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
														t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
														t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
														if (t.tdd_f < DC_DISTANCE) {
															//Object ok add.
															float gluescalex5 = ObjectScaleX(glueobj5);
															float gluescaley5 = ObjectScaleY(glueobj5);
															float gluescalez5 = ObjectScaleZ(glueobj5);
															//Its the same master so reuse g.meshlightmapwork
															PerformCheckListForLimbs(destobj);
															AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

															t.tox_f = ObjectPositionX(glueobj5) - t.tmasterx_f;
															t.toy_f = ObjectPositionY(glueobj5) - t.tmastery_f;
															t.toz_f = ObjectPositionZ(glueobj5) - t.tmasterz_f;

															OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

															RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj5), ObjectAngleY(glueobj5), ObjectAngleZ(glueobj5));
															ScaleLimb(destobj, ChecklistQuantity(), gluescalex5 + scaleadjust, gluescaley5 + scaleadjust, gluescalez5 + scaleadjust);
															additionaladded5 = true;
														}
													}
												}
											}

											bool additionaladded6 = false;
											int glueid6 = 0;
											int glueobj6 = 0;
											if ((iAfterPolygonTotal * 7 < MAX_DRAWPRIMITIVES) && (iAfterVertex * 7 < MAX_DRAWVERTEX)  && vObjList.size() > 5) {
												nextObjeid = vObjList[5];
												glueid6 = t.entityelement[nextObjeid].bankindex;
												glueobj6 = t.entityelement[nextObjeid].obj;

												sObject* pObject7 = g_ObjectList[glueobj6];
												if (pObject7 && pObject7->pInstanceOfObject) {
													pObject7 = pObject7->pInstanceOfObject;
													if (pObject7 == pObject2) {
														t.tdx_f = t.entityelement[t.e].x - t.entityelement[nextObjeid].x;
														t.tdz_f = t.entityelement[t.e].z - t.entityelement[nextObjeid].z;
														t.tdd_f = Sqrt(abs(t.tdx_f*t.tdx_f) + abs(t.tdz_f*t.tdz_f));
														if (t.tdd_f < DC_DISTANCE) {
															//Object ok add.
															float gluescalex6 = ObjectScaleX(glueobj6);
															float gluescaley6 = ObjectScaleY(glueobj6);
															float gluescalez6 = ObjectScaleZ(glueobj6);
															//Its the same master so reuse g.meshlightmapwork
															PerformCheckListForLimbs(destobj);
															AddLimb(destobj, ChecklistQuantity(), g.meshlightmapwork);

															t.tox_f = ObjectPositionX(glueobj6) - t.tmasterx_f;
															t.toy_f = ObjectPositionY(glueobj6) - t.tmastery_f;
															t.toz_f = ObjectPositionZ(glueobj6) - t.tmasterz_f;

															OffsetLimb(destobj, ChecklistQuantity(), t.tox_f, t.toy_f, t.toz_f);

															RotateLimb(destobj, ChecklistQuantity(), ObjectAngleX(glueobj6), ObjectAngleY(glueobj6), ObjectAngleZ(glueobj6));
															ScaleLimb(destobj, ChecklistQuantity(), gluescalex6 + scaleadjust, gluescaley6 + scaleadjust, gluescalez6 + scaleadjust);
															additionaladded6 = true;
														}
													}
												}
											}

											//PE: Merge everything into a single mesh.
											DeleteMesh(g.meshlightmapwork);
											MakeMeshFromObject(g.meshlightmapwork, destobj);
											DeleteObject(destobj);
											MakeObject(destobj, g.meshlightmapwork, -1);
											PositionObject(destobj, t.tmasterx_f, t.tmastery_f + testypos, t.tmasterz_f);

											//PE: TODO
											//PE: Objects like plant 08.fpe get VERY large ? , need to be checked why!
											//float objectsizex = ObjectSizeX(destobj);
											//if (objectsizex > 1000) {
											//	SetObjectMask(destobj, 1);
											//}

											if (t.entityprofile[t.entid].canseethrough == 1)
											{
												SetObjectCollisionProperty(destobj, 1);
											}
											if (t.entityprofile[t.entid].ischaracter == 0)
											{
												if (t.entityprofile[t.entid].collisionmode == 11)
												{
													SetObjectCollisionProperty(destobj, 1);
												}
											}
											if (t.entityprofile[t.entid].cullmode >= 0)
											{
												if (t.entityprofile[t.entid].cullmode != 0)
												{
													SetObjectCull(destobj, 0);
												}
												else
												{
													SetObjectCull(destobj, 1);
												}
											}
											SetObjectSpecularPower(destobj, t.entityelement[t.entid].eleprof.specularperc / 100.0f);

											if (GetMeshExist(g.meshlightmapwork) == 1)  DeleteMesh(g.meshlightmapwork);
												
											CloneObject(destobj, t.obj, 101); //PE: Copy textures only.

											SetObjectStatic(destobj, true); //Mark as static.

											//PE: TODO store t.e and glueobj under pObject->draw_call_obj
											//PE: So we can access them directly in drawobject.
											//pObject->draw_call_obj = g_ObjectList[destobj];

											//Disable if any LOD setup from original object.
											if (bestlod >= 0) {
												SetObjectLOD(destobj, 1, 50000);
												SetObjectLOD(destobj, 2, 50000);
											}

											t.entityelement[t.e].draw_call_obj = destobj;
											t.entityelement[t.e].dc_obj[0] = glueobj;
											t.entityelement[t.e].dc_entid[0] = vObjList[0];
											t.entityelement[vObjList[0]].dc_merged = true;
											if (additionaladded2) {
												t.entityelement[t.e].dc_obj[1] = glueobj2;
												t.entityelement[t.e].dc_entid[1] = vObjList[1];
												t.entityelement[vObjList[1]].dc_merged = true;
												HideObject(glueobj2);
											}
											if (additionaladded3) {
												t.entityelement[t.e].dc_obj[2] = glueobj3;
												t.entityelement[t.e].dc_entid[2] = vObjList[2];
												t.entityelement[vObjList[2]].dc_merged = true;
												HideObject(glueobj3);
											}
											if (additionaladded4) {
												t.entityelement[t.e].dc_obj[3] = glueobj4;
												t.entityelement[t.e].dc_entid[3] = vObjList[3];
												t.entityelement[vObjList[3]].dc_merged = true;
												HideObject(glueobj4);
											}
											if (additionaladded5) {
												t.entityelement[t.e].dc_obj[4] = glueobj5;
												t.entityelement[t.e].dc_entid[4] = vObjList[4];
												t.entityelement[vObjList[4]].dc_merged = true;
												HideObject(glueobj5);
											}
											if (additionaladded6) {
												t.entityelement[t.e].dc_obj[5] = glueobj6;
												t.entityelement[t.e].dc_entid[5] = vObjList[5];
												t.entityelement[vObjList[5]].dc_merged = true;
												HideObject(glueobj6);
											}
											//Hide org objects.
											HideObject(t.obj);
											HideObject(glueobj);

											SetObjectEffect(t.entityelement[t.e].draw_call_obj, t.entityprofile[t.entid].usingeffect );
											ShowObject(t.entityelement[t.e].draw_call_obj);

											g.merged_new_objects++;

											// NOTE: Does this mean batched objects will not benefit from these important flags (some models have OpenGL normals and the auto-generated tangents shift about)											
											// PE: Ups my bad , actually we would use pObject->draw_call_obj so it would have all the original settings,
											// PE: but not yet, so just added this again, cant remember why i commented it out ? seams to work fine :)
											DWORD dwArtFlags = 0;
											if (t.entityprofile[t.entid].invertnormal == 1) dwArtFlags = 1;
											if (t.entityprofile[t.entid].preservetangents == 1) dwArtFlags |= 1 << 1;
											SetObjectArtFlags(destobj, dwArtFlags, 0.0f);

										}

										DeleteObject(tmpobj);
									}
								}
							}
						}
					}
				}
			}

			// Setup variables for main game loop
			timestampactivity(0,"initialise final game variables");
			game_init ( );

			// Helpful prompt for start of test game
			if ( t.game.gameisexe == 0 && t.game.runasmultiplayer == 0 ) 
			{
				#ifdef VRTECH
				if (g_bInTutorialMode == true)
				{
					t.visuals.generalprompt_s = "PRESS ESCAPE TO RETURN TO TUTORIAL";
				}
				else
				{
					t.visuals.generalprompt_s = "TAB for settings, F9 for 3D edit, Esc to quit.";
				}
				#else
				t.visuals.generalprompt_s="TAB for settings, F9 for 3D edit, F10 for snapshot, Esc to quit.";
				#endif
				t.visuals.generalpromptstatetimer=Timer()+123;
			}
			else
			{
				if ( t.game.runasmultiplayer == 1 ) 
				{
					#ifdef PHOTONMP
					 t.visuals.generalpromptstatetimer=Timer()+1000;
					 #ifdef PRODUCTV3
					  t.visuals.generalprompt_s="Welcome to VR Quest(r) Social VR";
					 #else
					  t.visuals.generalprompt_s="Welcome to GameGuru MAX Multiplayer";
					 #endif
					#else
					 t.visuals.generalpromptstatetimer=Timer()+1000;
					 t.visuals.generalprompt_s="Press RETURN to Chat";
					#endif
				}
				else
				{
					t.visuals.generalpromptstatetimer=0;
				}


				//PE: start any animations that use startanimingame > 0. ( standalone ).
				for (t.tte = 1; t.tte <= g.entityelementlist; t.tte++)
				{
					//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/341
					// hide EBE markers
					int iIndex = t.entityelement[t.tte].bankindex;
					if (t.entityprofile[iIndex].isebe != 0)
					{
						t.tobj = t.entityelement[t.tte].obj;
						if (t.tobj>0)
						{
							if (ObjectExist(t.tobj) == 1)
							{
								HideLimb(t.tobj, 0);
							}
						}
					}


					t.entid = t.entityelement[t.tte].bankindex;
					t.tttsourceobj = g.entitybankoffset + t.entityelement[t.tte].bankindex;
					t.tobj = t.entityelement[t.tte].obj;
					if (t.tobj > 0)
					{
						if (ObjectExist(t.tobj) == 1)
						{
							//PE: Possible fix for issues:
							//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/206
							//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/273
							//PE: need testing.
							if (t.entityprofile[t.entid].ischaracter == 1) {
								//Char should always have z depth , but somehow its removed somewhere.

								//PE: check t.entityprofile[t.tentid].zdepth == 0

								EnableObjectZDepth(t.tobj);
							}
							if (t.entityprofile[t.entid].startanimingame > 0) {
								if (t.entityprofile[t.entid].animmax > 0) {
									t.q = t.entityprofile[t.entid].startanimingame - 1;
									SetObjectFrame(t.tttsourceobj, 0);
									LoopObject(t.tttsourceobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
									SetObjectFrame(t.tobj, 0);
									LoopObject(t.tobj, t.entityanim[t.entid][t.q].start, t.entityanim[t.entid][t.q].finish);
								}
							}
						}
					}
				}

			}

			//  setup spin values, this rotates the player 360 degrees at the start to kill initial
			//  stutter issues, during this time we don't want low FPS warnings
			t.postprocessings.spinfill=0 ; t.postprocessings.spinfillStartAng_f=CameraAngleY(0);

			//  apply any settings
			timestampactivity(0,"immediate title settings applied");
			if (  t.game.gameisexe == 1 || t.game.runasmultiplayer == 1 ) 
			{
				titles_immediateupdatesound ( );
				titles_immediateupdategraphics ( );
			}

			//  for multiplayer, check if there is a jetpack in the level and grab the model to place on players back
			if (  t.game.runasmultiplayer == 1 ) 
			{
				for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
				{
					t.entid=t.entityelement[t.e].bankindex;
					if (  t.entid>0 ) 
					{
						if (  cstr(Lower(Left(t.entityprofileheader[t.entid].desc_s.Get(),8)))  ==   "jet pack"  && ObjectExist(g.steamplayermodelsoffset+120)  ==  0 ) 
						{
							CloneObject (  g.steamplayermodelsoffset+120,t.entityelement[t.e].obj );
							YRotateObject (  g.steamplayermodelsoffset+120,180 );
							FixObjectPivot (  g.steamplayermodelsoffset+120 );
						}
					}
				}
			}

			#ifdef VRTECH
			//  check for character creator characters just before game starts
			///characterkit_checkForCharacters ( );
			#else
			characterkit_checkForCharacters ( );
			#endif

			//  Clear screen of any artifacts
			titles_loadingpagefree();
			CLS (  Rgb(0,0,0) );

			// In EXE running, override cameras with no mask until title/loading done
			SyncMaskOverride ( 0xFFFFFFFF );

			// resort texture list to ignore objects set to be ignored
			DoTextureListSort ( );

			// if reloading standalone level, need to restore basic stats from LUA save file
			if ( g.iStandaloneIsReloading == 2 )
			{
				// call LUA function from game which updates stats via LUA script
				char pLUACustomLoadCall[256];
				strcpy ( pLUACustomLoadCall, "GameLoopLoadStats" );
				LuaSetFunction ( pLUACustomLoadCall, 0, 0 ); 
				LuaCall ( );
			}

			// one final command to improve static physics performance
			physics_finalize ( );

			// Wipe out mouse deltas
			t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;

			//  Tab mode LOW FPS Warning
			g.tabmode=0 ; g.lowfpstarttimer=Timer();

			//  Game loop will run while single level is in play
			t.huddamage.immunity=1000;
			t.game.gameloop=1;
			g.timeelapsed_f=0;
			#ifdef VRTECH
			t.luaglobal.scriptprompttype = 0;
			#endif
			t.luaglobal.scriptprompt_s="";
			t.luaglobal.scriptprompttime=0;
			t.luaglobal.scriptprompttextsize=0;
			t.luaglobal.scriptprompt3dtime=0;
			strcpy ( t.luaglobal.scriptprompt3dtext, "" );

			#ifdef VRTECH
			// 260220 - for some reason, Social VR sets view 0,0,1,1, and does not set it back!
			// so we do so here to ensure we see the game
			SetCameraView(0, 0, 0, GetDisplayWidth(), GetDisplayHeight());
			#endif
			
			//  Game cycle loop
			if ( g.gproducelogfiles == 2 )
				timestampactivity(0,"main game loop begins in deep debug trace mode");
			else
				timestampactivity(0,"main game loop begins");
			while ( t.game.gameloop == 1 ) 
			{
				// gameloop winddown (for multiplayer server exit time)
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"winddown mp_closeconnection");
				#ifdef VRTECH
				if ( t.game.gameloopwinddown == 1 )
				{
					if ( mp_closeconnection() == 1 )
					{
						t.game.gameloopwinddown = 0;
						t.game.gameloop = 0;
					}
				}
				#endif

				#ifdef ALPHAEXPIRESYSTEM
				 t.visuals.generalprompt_s = "This is a DEV BUILD ONLY - it is not the final version of GameGuru MAX!";
				 t.visuals.generalpromptstatetimer=Timer()+123;
				#endif

				// detect if standalone is a foreground window
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"obtain plrhasfocus");
				if ( t.game.gameisexe == 1 )
				{
					#ifdef VRTECH
					if ( g.gvrmode == 3 )
					{
						t.plrhasfocus = 1;
					}
					else
					#else
					if(1)
					#endif
					{
						HWND hForeWnd = GetForegroundWindow();
						if ( GetWindowHandle() != hForeWnd ) 
							t.plrhasfocus = 0;
						else
							t.plrhasfocus = 1;
					}
				}

				// if controller active, also detect for START button press (same as ESCAPE)
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"controller start button check");
				bool bControllerEscape = false;
				if ( g.gxbox > 0 && JoystickFireXL(9) == 1 ) bControllerEscape = true;

				//  trigger options page or exit test level
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"escape button check");
				if ( EscapeKey() == 1 || bControllerEscape == true ) 
				{
					t.tremembertimer=Timer();
					#ifdef VRTECH
					game_main_snapshotsoundloopcheckpoint ( );
					game_stopallsounds ();
					#else
					game_main_snapshotsound ( true );
					#endif
					while ( EscapeKey() != 0 ) {}
					darkai_character_freezeall ( );
					physics_pausephysics ( );
					entity_pauseanimations ( );
					if ( t.currentgunobj > 0 ) { if ( ObjectExist(t.currentgunobj)==1 ) { SetObjectSpeed ( t.currentgunobj,0) ; } }
					if ( t.playercontrol.jetobjtouse>0 ) 
					{
						if ( ObjectExist(t.playercontrol.jetobjtouse) == 1  )  SetObjectSpeed (  t.playercontrol.jetobjtouse,0 );
					}
					#ifdef VRTECH
					if ( t.game.gameisexe == 0 ) // no menu in multiplayer test mode && t.game.runasmultiplayer == 0 ) 
					{
						if ( t.game.runasmultiplayer == 1 )
						{
							// wait until connection closed, then exit game loop
							t.game.gameloopwinddown = 1; 
						}
						else
						{
							// leave right away
							t.game.gameloop=0; 
							t.game.levelloop=0; 
							t.game.masterloop=0;
						}
						if ( t.conkit.editmodeactive == 1 ) 
						{
							conkitedit_switchoff ( );
						}
					}
					#else
					if (  t.game.gameisexe == 0 && t.game.runasmultiplayer == 0 ) 
					{
						t.game.gameloop=0 ; t.game.levelloop=0 ; t.game.masterloop=0;
						if ( t.conkit.editmodeactive == 1 ) 
						{
							conkitedit_switchoff ( );
						}
					}
					#endif
					else
					{
						g.titlesettings.updateshadowsaswell=1;
						timestampactivity(0,"entering options page");
						titleslua_init ( );
						titleslua_main ( "gamemenu" );
						timestampactivity(0,"leaving options page");
						g.titlesettings.updateshadowsaswell=0;
						LuaSetInt("g_MouseWheel", MouseMoveZ()); //PE: need to be MouseMoveZ().
					}
					if ( t.currentgunobj>0 ) { if ( ObjectExist(t.currentgunobj) == 1 ) { SetObjectSpeed ( t.currentgunobj,t.currentgunanimspeed_f); } }
					physics_resumephysics ( );
					entity_resumeanimations ( );
					t.aisystem.cumilativepauses=Timer()-t.tremembertimer;
					game_main_snapshotsoundresume ( );
					t.strwork = ""; t.strwork = t.strwork + "resuming game loop with flag "+Str(t.game.gameloop);
					timestampactivity(0, t.strwork.Get() );
					// Wipe out mouse deltas
					t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;
				}

				// Fade in gamescreen (using post process shader)
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"fade game screen logic");
				if ( t.postprocessings.fadeinvalue_f<1.0 ) 
				{
					// Hide Lua Sprites
					HideOrShowLUASprites ( true );

					//  Once spinfill is complete we can begin fading in. When complete we can enable low fps warnings again
					if ( t.postprocessings.spinfill < 360 ) 
					{
						t.postprocessings.spinfill += 10;
						if ( t.postprocessings.spinfill >= 360 )
						{
							// 020316 - v1.13b1 - restore to agreed start orientation
							RotateCamera ( 0, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f );
						}
						else
						{
							// spin around to fill video memory with immediate scenery
							RotateCamera (  0,0,t.postprocessings.spinfillStartAng_f + t.postprocessings.spinfill,0 );
						}

						// Wipe out mouse deltas during spin up
						t.tMousemove_f = MouseMoveX() + MouseMoveY() + MouseZ(); t.tMousemove_f  = 0;
					}
					else
					{
						if ( t.postprocessings.fadeinvalue_f <= 0 )
						{
							// only if in test game mode, standalone already set volume values (in title.lua)						
							if ( t.game.gameisexe == 0 )
							{
								// set music and sound global volumes
								audio_volume_init ( );
							}
							else if (!FileExist("savegames\\sounds.dat")) {
								//PE: This is not always in standalone ? , so:
								audio_volume_init();
							}
							// and update internal volume values so music update can use volumes!
							audio_volume_update ( );
						}
						t.postprocessings.fadeinvalue_f=t.postprocessings.fadeinvalue_f+(g.timeelapsed_f*0.1f);
						if (  t.postprocessings.fadeinvalue_f >= 1.0f ) 
						{
							t.postprocessings.fadeinvalue_f=1.0f;
							//g.globals.hidelowfpswarning = 0; // this overrides the SETUP.INI setting
							HideOrShowLUASprites ( false );
							EnableAllSprites(); // the disable is called in DarkLUA by ResetFade() black out command when load game position
						}
					}
					t.postprocessings.fadeinvalueupdate=1;
				}

				// handle fading
				#ifdef WICKEDENGINE
				#else
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"handle postprocess fading");
				if (  t.postprocessings.fadeinvalueupdate == 1 ) 
				{
					t.postprocessings.fadeinvalueupdate=0;
					if (  GetEffectExist(g.postprocesseffectoffset+0) == 1 ) 
					{
						SetVector4 (  g.terrainvectorindex,t.postprocessings.fadeinvalue_f,t.postprocessings.fadeinvalue_f,t.postprocessings.fadeinvalue_f,0 );
						SetEffectConstantV (  g.postprocesseffectoffset+0,"OverallColor",g.terrainvectorindex );
						SetEffectConstantV (  g.postprocesseffectoffset+2,"OverallColor",g.terrainvectorindex );
						SetEffectConstantV (  g.postprocesseffectoffset+4,"OverallColor",g.terrainvectorindex );
					}
					//  210115 - helps refresh scene at very start
					t.visuals.refreshshaders=1;
				}
				#endif

				//  Immunity when respawn
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"handle player immunity");
				if (  t.huddamage.immunity>0 ) 
				{
					t.huddamage.immunity=t.huddamage.immunity-(10*g.timeelapsed_f);
					if (  t.huddamage.immunity<0  )  t.huddamage.immunity = 0;
				}

				//  Run all game subroutines		
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_main_loop");
				game_main_loop ( );

				//  Update screen
				if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_sync");
				game_sync ( );

			} //  Game cycle loop end

			// first save current level stats before reset LUA
			if ( t.game.allowfragmentation == 2 )
			{
				char pLUACustomSaveCall[256];
				strcpy ( pLUACustomSaveCall, "GameLoopSaveStats" );
				LuaSetFunction ( pLUACustomSaveCall, 0, 0 ); 
				LuaCall ( );
			}

			// free any lua activity (restore FOV if ingame activity there)
			timestampactivity(0,"finalising LUA system before reset");
			lua_quitting(); 

			// Rest any internal game variables
			game_main_stop ( );

			//PE: Draw call optimizer
//			if (!g.disable_drawcall_optimizer)
			{
				//PE: restore all states. delete all batched objects.
				for (t.e = 1; t.e <= g.entityelementlist; t.e++)
				{
					t.entid = t.entityelement[t.e].bankindex;
					t.obj = t.entityelement[t.e].obj;

					if (t.obj > 0 && t.e < g.entityelementlist && ObjectExist(t.obj) == 1)
					{
						if (t.entityelement[t.e].draw_call_obj > 0) {

							if (t.entityelement[t.e].draw_call_obj > 0 && ObjectExist(t.entityelement[t.e].draw_call_obj) == 1) {
								DeleteObject(t.entityelement[t.e].draw_call_obj);
								t.entityelement[t.e].draw_call_obj = 0;
								if(ObjectExist(t.obj) == 1)
									ShowObject(t.obj);
							}

							if (t.entityelement[t.e].dc_obj[0] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[0]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[0]);
							if (t.entityelement[t.e].dc_obj[1] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[1]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[1]);
							if (t.entityelement[t.e].dc_obj[2] > 0 &&  ObjectExist(t.entityelement[t.e].dc_obj[2]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[2]);
							if (t.entityelement[t.e].dc_obj[3] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[3]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[3]);
							if (t.entityelement[t.e].dc_obj[4] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[4]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[4]);
							if (t.entityelement[t.e].dc_obj[5] > 0 && ObjectExist(t.entityelement[t.e].dc_obj[5]) == 1)
								ShowObject(t.entityelement[t.e].dc_obj[5]);

							t.entityelement[t.e].dc_obj[0] = 0;
							t.entityelement[t.e].dc_obj[1] = 0;
							t.entityelement[t.e].dc_obj[2] = 0;
							t.entityelement[t.e].dc_obj[3] = 0;
							t.entityelement[t.e].dc_obj[4] = 0;
							t.entityelement[t.e].dc_obj[5] = 0;

							t.entityelement[t.e].dc_entid[0] = 0;
							t.entityelement[t.e].dc_entid[1] = 0;
							t.entityelement[t.e].dc_entid[2] = 0;
							t.entityelement[t.e].dc_entid[3] = 0;
							t.entityelement[t.e].dc_entid[4] = 0;
							t.entityelement[t.e].dc_entid[5] = 0;

						}
					}
				}
			}

			//PE: Moved here as standalone will delete all objects, so we could not free DCO objects.
			//  Free any level resources
			game_freelevel();

			// must reset LUA here for clean end-game-screens
			// ensure LUA is completely reset before loading new ones in
			// the free call is because game options menu init, but not freed back then
			timestampactivity(0,"resetting LUA system");
			titleslua_free ( );
			LuaReset (  );

			//PE: restore waterline.
			t.terrain.waterliney_f = g.gdefaultwaterheight;

			#ifdef VRTECH
			reset_env_particles();
			#endif

			//PE: restore sun position for editor.
			t.terrain.sundirectionx_f = t.terrain.skysundirectionx_f;
			t.terrain.sundirectiony_f = t.terrain.skysundirectiony_f;
			t.terrain.sundirectionz_f = t.terrain.skysundirectionz_f;


			// 240316 - additional cleanup
			mp_freefadesprite ( );

			// if VR, deactivate at this point
			//g_VR920RenderStereoNow = false;

			//  Advance level to 'next one' or 'win game'
			timestampactivity(0,"end of level stage");
			if (  t.game.gameisexe == 1 )
			{
				timestampactivity(0,"game is standalone exe");
				if (  t.game.quitflag == 0 ) 
				{
					timestampactivity(0,"game has not quit");
					//PE: issue https://github.com/TheGameCreators/GameGuruRepo/issues/444
					if (  Len(t.game.jumplevel_s.Get())> 0 ) 
					{
						//  goes around and loads this level name
						timestampactivity(0,"game is loading non-linear level map:");
						timestampactivity(0,"t.game.jumplevel_s.Get()");
					}
					else
					{
						// win, lose or next level pages
						if (  t.game.lostthegame == 1 ) 
						{
							//titles_gamelostpage ( );
							timestampactivity(0,"LUA script : lose");
							sky_hide();
							titleslua_init ( );
							titleslua_main ( "lose" );
							titleslua_free ( );
							sky_show();
							t.game.levelloop=0;
						}
						else
						{
							t.game.level=t.game.level+1;
							if (  t.game.level>t.game.levelmax ) 
							{
								//titles_gamewonpage ( );
								timestampactivity(0,"LUA script : win");
								sky_hide();
								titleslua_init ( );
								titleslua_main ( "win" );
								titleslua_free ( );
								sky_show();
								t.game.levelloop=0;
							}
							else
							{
								//titles_completepage ( );
								timestampactivity(0,"LUA script : nextlevel");
								sky_hide();
								titleslua_init ( );
								titleslua_main ( "nextlevel" );
								sky_show();
							}
						}
					}
				}
			}
			t.game.quitflag=0;

			//  If was in multiplayer session, no level loop currently
			if (  t.game.runasmultiplayer == 1 ) 
			{
				t.game.levelloop=0;
			}

			if (g.memgeneratedump == 1) {
				timestampactivity(0, "DumpImageList after freeing level data.");
				DumpImageList(); // PE: Dump image usage after level.
			}

			// 250619 - very large levels can fragment 32 bit memory after a few levels
			// so this mode will restart the executable, and launch the new level
			// crude solution until 64 bit allows greater memory referencing
			if ( t.game.allowfragmentation == 2 )
				t.game.levelloop = 0;

		// Level loop end
		}

		// Free any game resources
		game_freegame ( );

		if ( t.game.runasmultiplayer == 1 ) 
		{
			mp_free_game ( );
			mp_cleanupGame ( );
			if (  g.mp.goBackToEditor  ==  1 ) 
			{
				g.mp.goBackToEditor = 0;
				t.game.masterloop = 0;
			}
		}

		// get rid of debris and particles that may be lingering
		explosion_cleanup ( );

		// if ignored title, exit now
		if (  t.game.ignoretitle == 1 && t.game.runasmultiplayer == 0  )  t.game.masterloop = 0;

		// Master loop end
		if ( t.game.allowfragmentation == 0 || t.game.allowfragmentation == 2 ) break;
	}

	// End splash if EXE is advertising
	if ( t.game.set.endsplash == 1 ) 
	{
		t.game.set.endsplash=0;
	}

	#ifdef VRTECH
	// restore VR activity (vrtest flag has done its job)
	g.vrglobals.GGVRUsingVRSystem = 1;

	// restore normal rendering activity when finish game run
	SyncMaskOverride ( 0xFFFFFFFF );

	// cannot rely on postprocess to restore, so do so here when return
	SetCameraView ( 0, 0, 0, GetDisplayWidth(), GetDisplayHeight() );
	#endif
}
*/

void game_setresolution ( void )
{
	//  set game resolution here
	t.multisamplingfactor=0;
	t.multimonitormode=0;
	SetDisplayMode (  GetDesktopWidth(),GetDesktopHeight(),32,g.gvsync,t.multisamplingfactor,t.multimonitormode );
	SyncOn (   ); SyncRate (  0  ); Sync (   ); SetAutoCamOff (  );
	DisableEscapeKey (  );
}

void game_postresolutionchange ( void )
{
}

void game_oneoff_nongraphics ( void )
{
	// Trigger a sound stops initial slow-down?
	if (g.silentsoundoffset > 0)
	{
		if (SoundExist(g.silentsoundoffset) == 1)
		{
			PlaySound(g.silentsoundoffset);
			PositionSound(g.silentsoundoffset, 0, 0, 0);
		}
	}

	//  Force all weapons into weapon slots (initial default start)
	gun_gatherslotorder ( );
}

void game_loadinentitiesdatainlevel ( void )
{
	// Load player settings
	timestampactivity(0,"Load player config");
	mapfile_loadplayerconfig ( );

	// Load entity bank
	t.screenprompt_s="LOADING ENTITY BANK";
	if ( t.game.gameisexe == 0 ) printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	entity_loadbank ( );

	#ifdef VRTECH
	// Load entity elements
	t.screenprompt_s="LOADING ENTITY ELEMENTS";
	if ( t.game.gameisexe == 0 ) printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	#endif
	entity_loadelementsdata ( );
	timestampactivity(0, "e:entity_loadelementsdata()");

}

void game_loadinleveldata ( void )
{
	//  Load waypoints
	t.screenprompt_s="LOADING WAYPOINTS DATA";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	waypoint_loaddata ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	waypoint_recreateobjs ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Load terrain
	t.screenprompt_s="LOADING TERRAIN DATA";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	terrain_loaddata ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Recreate all entities in level
	t.screenprompt_s="CREATE ENTITY OBJECTS";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());

	for ( t.tupdatee = 1 ; t.tupdatee<=  g.entityelementlist; t.tupdatee++ )
	{
		entity_updateentityobj ( );
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	}
	t.terrain.terrainpainteroneshot=0;

	//  default start position is edit-camera XZ (Y done in physics init call)
	t.terrain.playerx_f=25000;
	t.terrain.playery_f=0;
	t.terrain.playerz_f=25000;
	t.terrain.playerax_f=0.0;
	t.terrain.playeray_f=0.0;
	t.terrain.playeraz_f=0.0;
	t.camangy_f=0;

	//  hide all markers
	t.screenprompt_s="GAME OBJECT CLEANUP";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.obj=t.entityelement[t.e].obj;
		if (  t.obj>0 ) 
		{
			if (  ObjectExist(t.obj) == 1 ) 
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entityprofile[t.entid].ismarker != 0 ) 
				{
					//  all markers must be hidden
					HideObject (  t.obj );
				}
				if (  t.entityprofile[t.entid].addhandlelimb>0 ) 
				{
					//  hide decal handles
					HideLimb (  t.obj,t.entityprofile[t.entid].addhandlelimb );

				}
			}
		}
	}
	waypoint_hideall ( );
}

#ifdef VRTECH
float GetWAVtoLIPProgress(void);
#endif

void game_preparelevel ( void )
{
	//Dave Performance - ensure sound volume is 0
	// 271115 - Dave, you just wiped out all dynamic music volume!
	//g.musicsystem.percentageVolume = 0;
	//t.audioVolume.music = 0;
	//t.audioVolume.sound = 0;
	//t.audioVolume.musicFloat = 0.0;
	//t.audioVolume.soundFloat = 0.0;

	//  Init music system first to make sure nothing is playing during the load sequence
	// 271115 - has to be here as LUA triggers Play Music during its INIT but if MUSIC_INIT was
	// called last it would stop the default music from playing, but setting volume to zero 
	// will achieve the same result of keeping music silent until ready
	music_init ( );
	t.audioVolume.musicFloat = 0;
	t.audioVolume.soundFloat = 0;

	//  Load all assets required to perform level
	timestampactivity(0,"_game_preparelevel:");

	//  Switch on post process if it was switched off (init called later in _finally subroutine)
	postprocess_on ( );

	//  (re)load any player sounds (player style specified in player start marker)
	material_loadplayersounds ( );

	//  init character sound
	character_sound_init ( );

	//  particles
	ravey_particles_init ( );
	#ifdef VRTECH
	reset_env_particles ( );
	#endif

	#ifdef WICKEDENGINE
	// bulletholes return at last
	bulletholes_init();
	#endif

	//  Allow Steam to refresh (so does not stall)
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  HUD graphics
	t.screenprompt_s = "LOADING HUD GRAPHICS";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	hud_init ( );

	//  Load sky spec (for any shaders later that require sun-pos)
	t.screenprompt_s="LOADING SKY";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	t.terrainskyspecinitmode=0;

	#ifdef WICKEDENGINE
	//PE: Remember sun angle.
	float oSx = t.visuals.SunAngleX;
	float oSy = t.visuals.SunAngleY;
	float oSz = t.visuals.SunAngleZ;
	#endif

	#ifdef WICKEDENGINE
	sky_skyspec_init( false );
	#else
	sky_skyspec_init ( );
	#endif

	#ifdef WICKEDENGINE
	//PE: In wicked we want to restore the sun angle from the map and not use skyspec.ini settings. (only when loading a old level).
	if (t.visuals.skyindex == 0 || t.visuals.bDisableSkybox)
	{
		//PE: Only if we re not using a simple skybox.
		t.terrain.sunrotationx_f = t.visuals.SunAngleX = oSx;
		t.terrain.sunrotationy_f = t.visuals.SunAngleY = oSy;
		t.terrain.sunrotationz_f = t.visuals.SunAngleZ = oSz;
	}
	#endif
	//  Update terrain height map so GetGroundHeight command will work
	t.screenprompt_s="UPDATED TERRAIN";
	timestampactivity(0,t.screenprompt_s.Get());
	//terrain_update ( );
	//PE: Dont actually render the terrain it will ruin the loading progress screen.

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Load weapon system
	#ifdef WICKEDENGINE
	// moved further down as entity eleprof hasweapon needs populating before know what guns the characters are using
	#else
	#ifdef VRTECH
	 t.screenprompt_s="LOADING NEW HUD";
	#else
	 t.screenprompt_s="LOADING NEW WEAPONS";
	#endif	
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	gun_activategunsfromentities ( );
	gun_setup ( );
	gun_loadonlypresent ( );
	#endif

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Load in HUD Layer assets
	t.screenprompt_s = "LOADING HUD LAYERS";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	hud_scanforhudlayers ( );

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Veg Creation
	t.screenprompt_s = "PREPARING VEGETATION";
	#ifdef WICKEDENGINE
	// already performed by the editor!
	#else
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	grass_init ( );
	#endif

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  setup terrain for in-game
	t.screenprompt_s = "LOADING WATER SYSTEM";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	terrain_start_play ( );
	terrain_water_init ( );
	terrain_water_loop ( );

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  setup terrain for in-game
	t.screenprompt_s="LOADING A.I SYSTEM";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	darkai_init ( );
	t.aisystem.containerpathmax=0;
	t.screenprompt_s="PREPARING A.I SYSTEM";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	darkai_preparedata ( );

	//  Reset waypoint for game activity
	#ifdef WICKEDENGINE
	if (t.showtestgameelements == 0)
	{
		t.screenprompt_s = "RESETTING WAYPOINTS A.I";
		#ifndef WICKEDENGINE
		if (t.game.gameisexe == 0)  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
		#endif
		timestampactivity(0, t.screenprompt_s.Get());
		waypoint_reset();
	}
	#else
	t.screenprompt_s = "RESETTING WAYPOINTS A.I";
	if (t.game.gameisexe == 0)  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0, t.screenprompt_s.Get());
	waypoint_reset();
	#endif

	//  setup entities
	t.screenprompt_s="CREATING ENTITY A.I";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	entity_init ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	// Load weapon system (wicked)
	#ifdef WICKEDENGINE
	t.screenprompt_s = "LOADING NEW WEAPONS";
	if (t.game.gameisexe == 0)  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0, t.screenprompt_s.Get());
	gun_activategunsfromentities ();
	gun_setup ();
	gun_loadonlypresent ();
	#endif

	//  create A.I entities for all characters
	t.screenprompt_s="SETTING UP CHARACTERS";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	darkai_setup_characters ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  When all static entities added, complete obstacle map
	if (  g.gskipobstaclecreation == 0 ) 
	{
		t.screenprompt_s="CREATING A.I OBSTACLES";
		#ifndef WICKEDENGINE
		if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
		#endif
		timestampactivity(0,t.screenprompt_s.Get());
	}
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	darkai_completeobstacles ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  setup infinilights
	t.screenprompt_s="PREPARING DYNAMIC LIGHTS";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	lighting_init ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  particles/flak/debris required
	decal_activatedecalsfromentities ( );
	material_activatedecals ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	decal_loadonlyactivedecals ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  load required scripts
	lua_init ( );
	lua_scanandloadactivescripts ( );

	#ifdef VRTECH
	// if still generating LIP file, wait here
	t.screenprompt_s="GENERATING LIP SYNC DATA";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	float fProgressOfGeneration = GetWAVtoLIPProgress();
	while ( fProgressOfGeneration > 0.0f && fProgressOfGeneration < 1.0f )
	{
		fProgressOfGeneration = GetWAVtoLIPProgress();
		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
		Sleep(50);
	}
	#endif

	// load entity sounds and video
	entity_loadactivesoundsandvideo ( );
}

void game_preparelevel_forplayer ( void )
{
	//  Player settings
	physics_player_init ( );

	//  player start position must come from level setuo info
	t.terrain.gameplaycamera=0;
	SetCurrentCamera (  t.terrain.gameplaycamera );
	PositionCamera (  t.terrain.gameplaycamera,t.terrain.playerx_f,t.terrain.playery_f,t.terrain.playerz_f );
	PointCamera (  t.terrain.gameplaycamera,0,100,0 );
	RotateCamera (  t.terrain.gameplaycamera,t.terrain.playerax_f,t.terrain.playeray_f,t.terrain.playeraz_f );

	//  should be done in visual update call now!!
	SetCameraRange (  t.terrain.gameplaycamera, DEFAULT_NEAR_PLANE, DEFAULT_FAR_PLANE );
	SetCameraAspect (  t.terrain.gameplaycamera,1.325f );
	SetCameraFOV (  t.terrain.gameplaycamera,75 );
}

//Dave Performance - setup character entities for shader switching
void game_setup_character_shader_entities ( bool bMode )
{
	#ifdef WICKEDENGINE
	// No need of this
	#else
	//store the ID's of entity and character shaders
	t.entityBasicShaderID=loadinternaleffectunique("effectbank\\reloaded\\character_static.fx", 1); //PE: old effect never deleted. why ?
	//t.entityBasicShaderID = loadinternaleffect("effectbank\\reloaded\\character_static.fx"); //PE: Need to test this more. why would it need to be unique ?.
	t.characterBasicShaderID=loadinternaleffect("effectbank\\reloaded\\character_basic.fx");

	//PE: Bug. reset effect clip , so visible.
	t.tnothing = MakeVector4(g.characterkitvector);
	SetVector4(g.characterkitvector, 500000, 1, 0, 0);
	SetEffectConstantV(t.entityBasicShaderID, "EntityEffectControl", g.characterkitvector);
	SetEffectConstantV(t.characterBasicShaderID, "EntityEffectControl", g.characterkitvector);
	t.tnothing = DeleteVector4(g.characterkitvector);

	t.characterBasicEntityList.clear();
	t.characterBasicEntityListIsSetToCharacter.clear();

	// build up a list of entities that use the character shader
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		t.entobj = g.entitybankoffset + t.entid;
		if ( t.entid > 0 )
		{
			// Dont add CPUANIMS=1 characters
			if ( t.entityprofile[t.entid].cpuanims==0 )
			{
				// Dont add cc characters to this

				//PE: need apbr_basic.fx , apbr_anim.fx
				//PE: pbr restored later so...
				if (strcmp(Lower(Right(t.entityprofile[t.entid].effect_s.Get(), 13)), "apbr_basic.fx") == 0 && t.entityprofile[t.entid].ischaracter == 1 && t.entityprofile[t.entid].ischaractercreator == 0)
				{
					t.characterBasicEntityList.push_back(t.e);
					if (t.entityelement[t.e].active == 1)
						t.characterBasicEntityListIsSetToCharacter.push_back(true);
					else
						t.characterBasicEntityListIsSetToCharacter.push_back(false);

					// set the bank object to freeze also for when they switch to instances
					if (bMode)
						SetObjectEffect(g.entitybankoffset + t.entityelement[t.e].bankindex, t.entityBasicShaderID);
					else
						SetObjectEffect(g.entitybankoffset + t.entityelement[t.e].bankindex, t.characterBasicShaderID);
					SetObjectEffect(t.entityelement[t.e].obj, t.characterBasicShaderID);
				}
				if ( strcmp ( Lower(Right(t.entityprofile[t.entid].effect_s.Get(),18)) , "character_basic.fx" ) == 0 && t.entityprofile[t.entid].ischaracter == 1 && t.entityprofile[t.entid].ischaractercreator == 0 )
				{
					t.characterBasicEntityList.push_back(t.e);
					if ( t.entityelement[t.e].active == 1 )
						t.characterBasicEntityListIsSetToCharacter.push_back(true);
					else
						t.characterBasicEntityListIsSetToCharacter.push_back(false);
					// set the bank object to freeze also for when they switch to instances
					if ( bMode )
						SetObjectEffect( g.entitybankoffset+t.entityelement[t.e].bankindex , t.entityBasicShaderID );
					else
						SetObjectEffect( g.entitybankoffset+t.entityelement[t.e].bankindex , t.characterBasicShaderID );
					SetObjectEffect( t.entityelement[t.e].obj , t.characterBasicShaderID );
				}
			}
		}
	}
	#endif
}

void game_check_character_shader_entities ( void )
{
	#ifdef WICKEDENGINE
	// Nn need of this
	#else
	if ( !t.haveSetupShaderSwitching )
	{
		t.haveSetupShaderSwitching = true;
		game_setup_character_shader_entities(true);
	}

	int tobj;
	float dx;
	float dy;
	float dz;
	float dist;

	if ( g_occluderf9Mode )
	{
		for ( int c = 0 ; c < (int)t.characterBasicEntityList.size() ; c++ )
		{		
			tobj = t.entityelement[t.characterBasicEntityList[c]].obj;
			if ( tobj > 0 )
			{
				if ( ObjectExist ( tobj ) == 1 )
				{

					SetObjectEffect( tobj , t.characterBasicShaderID );
					t.characterBasicEntityListIsSetToCharacter[c] = true;							

				}
			}
		}

		return;
	}

	for ( int c = 0 ; c < (int)t.characterBasicEntityList.size() ; c++ )
	{		
		tobj = t.entityelement[t.characterBasicEntityList[c]].obj;
		if ( tobj > 0 && t.entityelement[t.characterBasicEntityList[c]].active == 1 )
		{
			if ( ObjectExist ( tobj ) == 1 )
			{
				dx = CameraPositionX() - ObjectPositionX( tobj );
				dy = CameraPositionY() - ObjectPositionY( tobj );
				dz = CameraPositionZ() - ObjectPositionZ( tobj );
				dist = sqrtf ( dx*dx + dy*dy + dz*dz );

				if ( t.entityelement[t.characterBasicEntityList[c]].ragdollified )
					dist = 0;

				// 120516 - ensure character shader respects always active
				t.te = t.characterBasicEntityList[c]; entity_getmaxfreezedistance();
				if ( dist < t.maximumnonefreezedistance )
				{
					if ( !t.characterBasicEntityListIsSetToCharacter[c] )
					{
						SetObjectEffect( tobj , t.characterBasicShaderID );
						t.characterBasicEntityListIsSetToCharacter[c] = true;	
					}
				}
				else
				{
					if ( t.characterBasicEntityListIsSetToCharacter[c] )
					{
						SetObjectEffect( tobj , t.entityBasicShaderID );
						t.characterBasicEntityListIsSetToCharacter[c] = false;		
					}
				}
			}
		}
	}
	#endif
}

extern int howManyMarkers;

void game_preparelevel_finally ( void )
{
	// Tell shadow maps to restore previous vis list rather than sort it
	g_bInEditor = false;
	t.performanceCameraDrawDistance = 0;
	t.haveSetupShaderSwitching = false;

	#ifdef WICKEDENGINE
	g.isGameBeingPlayed = true;
	#endif // WICKEDENGINE

	// This is used to record when we have switched lighting modes so we don't do it constantly
	g.inGameLightingMode = 0;
	if (  t.game.runasmultiplayer == 1 ) 
	{
		mp_load_guns ( );
	}

	//Free up spawns sent to lua
	t.entitiesActivatedForLua.clear();

	// Don't switch off guns!
	g.noPlayerGuns = false;
	g.remembergunid = 0;

	//Enable flash light key
	//g.flashLightKeyEnabled = true; this has been moved to the player start marker setup which now controls this

	//  Generate mega texture of terrain paint for VERY LOW shaders
	if (  t.terrain.generatedsupertexture == 0 ) 
	{
		t.screenprompt_s="GENERATING TERRAIN SUPER TEXTURE";
		#ifndef WICKEDENGINE
		if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
		#endif
		timestampactivity(0,t.screenprompt_s.Get());
		terrain_generatesupertexture ( false );
		t.terrain.generatedsupertexture = 1;
	}

	//  Trigger the technique to switch to DISTANT shader is far away
	BT_ForceTerrainTechnique (  0 );

	//  Initiate post process system (or reactivate it)
	t.screenprompt_s="INITIALIZING POSTPROCESS";
	#ifndef WICKEDENGINE
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	#endif
	timestampactivity(0,t.screenprompt_s.Get());
	postprocess_init ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	timestampactivity(0,"postprocessing initialized");

	//  Ensure correct shaders in play
	visuals_shaderlevels_update ( );

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Initialise Construction Kit
	conkit_init ( );

	//  Init physics
	t.screenprompt_s="INITIALIZING PHYSICS";
	if (  t.game.gameisexe == 0  )  printscreenprompt(t.screenprompt_s.Get()); else loadingpageprogress(5);
	timestampactivity(0,t.screenprompt_s.Get());
	physics_init ( );

	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  initialise physics for conkit objects
	//conkit_setupphysics ( );

	//  Activate Occlusion System
	timestampactivity(0,"Activate Occlusion System");
	if (  g.globals.occlusionmode == 1 ) 
	{
		//  Once all assets in place, create occlusion database
		// 110416 - commented out again, turns out when occluder in THREAD is flagged to end, it clears the occluder list
		//CPU3DClear(); // 260316 - dont know why the clear was commented out, it is ESSENTIAL to ensure levels dont mess each other up
		CPU3DSetCameraIndex (  0 );
		//  Occlusion poly list can have a variable size to help performance
		CPU3DSetPolyCount ( t.visuals.occlusionvalue );
		//  Add terrain LOD1s as occluders
		//for ( t.obj = t.terrain.TerrainLODOBJStart ; t.obj<=  t.terrain.TerrainLODOBJFinish; t.obj++ )
		//{
		//	if (  t.obj>0 ) 
		//	{
		//		if ( ObjectExist(t.obj) == 1  )  CPU3DAddOccluder (  t.obj );
		//		if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
		//	}
		//}
		//  Set occludees for all entities in level
		t.toccobj=g.occlusionboxobjectoffset;
		for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
		{
			if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
			t.entid=t.entityelement[t.e].bankindex;
			if (  t.entityprofile[t.entid].ismarker == 0 ) 
			{
				t.obj=t.entityelement[t.e].obj;
				if (  t.obj>0 ) 
				{
					if (  ObjectExist(t.obj) == 1 ) 
					{
						//Dave Performance, if the object is static, make it static for the game
						if ( t.entityelement[t.e].staticflag == 1 )
							SetObjectStatic(t.obj , true );
						else
							SetObjectStatic(t.obj , false );		

						//  reject any objects too small as they won't make good occluders
						if (  t.entityelement[t.e].staticflag == 1 && t.entityprofile[t.entid].notanoccluder == 0 && ((ObjectSizeX(t.obj,1)>MINOCCLUDERSIZE && ObjectSizeY(t.obj,1)>MINOCCLUDERSIZE ) || (ObjectSizeZ(t.obj,1)>MINOCCLUDERSIZE && ObjectSizeY(t.obj,1)>MINOCCLUDERSIZE )) ) 
						{
							//  OCLUDER AND OCLUDEE
							//  OPTIMIZE; we can make this even faster by using a simple math-QUAD in the render part
							//  instead of storing the verts of a 12 polygon box!!
							if ( t.entityelement[t.e].eleprof.isocluder == 1 )
							{
								if (  t.entityprofile[t.entid].physicsobjectcount>0 && t.entityprofile[t.entid].collisionmode == 40 ) 
								{
									t.tocy_f=ObjectSizeY(t.obj)/2.0;
									for ( t.tcount = 0 ; t.tcount<=  t.entityprofile[t.entid].physicsobjectcount-1; t.tcount++ )
									{
										if (  ObjectExist(t.toccobj) == 1  )  DeleteObject (  t.toccobj );
										MakeObjectBox (  t.toccobj, t.entityphysicsbox[t.entid][t.tcount].SizeX * (t.entityprofile[t.entid].scale * 0.01 ), t.entityphysicsbox[t.entid][t.tcount].SizeY * (t.entityprofile[t.entid].scale * 0.01 ), t.entityphysicsbox[t.entid][t.tcount].SizeZ * (t.entityprofile[t.entid].scale * 0.01 ) );
										OffsetLimb (  t.toccobj, 0, t.entityphysicsbox[t.entid][t.tcount].OffX * (t.entityprofile[t.entid].scale * 0.01 ) , t.entityphysicsbox[t.entid][t.tcount].OffY * (t.entityprofile[t.entid].scale * 0.01 ) , t.entityphysicsbox[t.entid][t.tcount].OffZ * (t.entityprofile[t.entid].scale * 0.01 ) );
										RotateLimb (  t.toccobj, 0, t.entityphysicsbox[t.entid][t.tcount].RotX , t.entityphysicsbox[t.entid][t.tcount].RotY , t.entityphysicsbox[t.entid][t.tcount].RotZ );
										MakeMeshFromObject (  g.meshgeneralwork,t.toccobj );
										DeleteObject (  t.toccobj );
										MakeObject (  t.toccobj,g.meshgeneralwork,0 );
										DeleteMesh (  g.meshgeneralwork );
										PositionObject (  t.toccobj,ObjectPositionX(t.obj),ObjectPositionY(t.obj)+t.tocy_f,ObjectPositionZ(t.obj) );
										RotateObject (  t.toccobj,ObjectAngleX(t.obj),ObjectAngleY(t.obj),ObjectAngleZ(t.obj) );
										SetObjectMask (  t.toccobj, 0 );
										SetObjectCollisionProperty (  t.toccobj,1 );
										CPU3DAddOccluder (  t.toccobj );
										++t.toccobj;
									}
								}
								else
								{
									//  polygon occluders TOO EXPENSIVE
									//  NOTE; Suggest a new set of polygons inside each model marked 'occluder'
									//  which when detected are submitted here via the OBJ
									// Dave Performance - adding everything in, even poly stuff
									// Don't add in collisionmode 50-59 (trees) as they make poor occluders
									if ( t.entityprofile[t.entid].collisionmode < 50 || t.entityprofile[t.entid].collisionmode > 59 ) 
										CPU3DAddOccluder (  t.obj );
								}
							}
						}
					}
				}

				// Add as an occludee
				if ( t.entityelement[t.e].eleprof.isocludee == 1 )
				{
					// Also let the occluder know if it is a character or not as characters are shown for longer
					// compared to other objects
					if ( t.entityprofile[t.entid].ischaracter == 1 ) 
					{
						// Also add character creator parts, if this is a cc character
						CPU3DAddOccludee ( t.obj , true );

						/* 100517 - bug fix until figure out why occluder does not restore head bits!
						if ( t.entityprofile[t.entid].ischaractercreator == 1 )
						{
							// Head
							t.tccobj = g.charactercreatorrmodelsoffset+((t.tcce*3)-t.characterkitcontrol.offset);
							if (  ObjectExist(t.tccobj)  ==  1 ) CPU3DAddOccludee (  t.tccobj , false ); // 100517 - fix bug true );

							// Beard
							t.tccobjbeard = g.charactercreatorrmodelsoffset+((t.tcce*3)-t.characterkitcontrol.offset)+1;
							if (  ObjectExist(t.tccobjbeard)  ==  1 ) CPU3DAddOccludee (  t.tccobjbeard , false ); // 100517 - fix bug true );

							// Hat
							t.tccobjhat = g.charactercreatorrmodelsoffset+((t.tcce*3)-t.characterkitcontrol.offset)+2;
							if (  ObjectExist(t.tccobjhat)  ==  1 ) CPU3DAddOccludee (  t.tccobjhat , false ); // 100517 - fix bug true );
						}
						*/
					}
					else
					{
						CPU3DAddOccludee ( t.obj , false );
					}
				}
			}
			else
				howManyMarkers++;
		}
		while (  t.toccobj<g.occlusionboxobjectoffsetfinish ) 
		{
			if (  ObjectExist(t.toccobj) == 1  )  DeleteObject (  t.toccobj );
			++t.toccobj;
		}

		//  also occlude any weapons carried by characters
		if (  g.entityattachmentindex>0 ) 
		{
			for ( t.obj = g.entityattachmentsoffset+1 ; t.obj<=  g.entityattachmentsoffset+g.entityattachmentindex; t.obj++ )
			{
				if (  t.obj>0 ) 
				{
					if (  ObjectExist(t.obj) == 1 ) 
					{
						CPU3DAddOccludee (  t.obj , false );
					}
				}
			}
		}
	}

	//  Final states of entities and call ALL entity script INIT functions
	timestampactivity(0,"Entity Initiations");
	entity_initafterphysics ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
	lua_launchallinitscripts ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Once player start known, fill veg area instantly
	timestampactivity(0,"Fill Veg Areas");
	t.completelyfillvegarea=1;
	grass_loop ( );
	if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );

	//  Force a shader update to ensure correct shadows are used at start
	t.visuals.refreshcountdown=5;

	// Let steam know we have finished loading
	if ( t.game.runasmultiplayer == 1 ) 
	{
		g.mp.finishedLoadingMap = 1;
	}

	// One final prompt, ask user to wait for key press so can read instructions
	#ifdef VRTECH
	 if ( t.game.gameisexe == 0 )
	 {
		#ifndef WICKEDENGINE
		t.screenprompt_s="STARTING LEVEL";
		printscreenprompt(t.screenprompt_s.Get());
		timestampactivity(0,t.screenprompt_s.Get());
		#endif
	 }
	#else
	 // No such wait press for regular GG
	#endif

	// The start marker may have given the play an initial gun, so lets call physics_player_refreshcount just incase it has
	physics_player_refreshcount();
}

void game_stopallsounds ( void )
{
	// stop ALL sounds
	for ( t.s = 1; t.s <= 65535; t.s++ )
	{
		if ( SoundExist(t.s) == 1 ) 
		{
			if ( t.s >= g.musicsoundoffset && t.s <= g.musicsoundoffsetend ) 
			{
				// if from game menu, do not stop dynamic music sound progress
				StopSound (  t.s );
			}
			else
			{
				StopSound ( t.s );
			}
		}
	}
}

void game_freelevel ( void )
{
	#ifdef WICKEDENGINE
	// remove any bulletholes
	bulletholes_free();

	//Reset loading bar percentage
	extern int g_iLastProgressPercentage;
	g_iLastProgressPercentage = 0;
	g.isGameBeingPlayed = false;
	#endif

	//  hide any jetpacks, etc
	hud_free ( );

	//  stop ALL sounds
	game_stopallsounds();

	// Delete occluder thread and close event handles (moved from free game which is too late for multi-level games)
	if ( g_pOccluderThread )
	{
		//Let the occluder know its time to finish
		g_occluderOn = false;

		//Dave Performance - let the occluder thread know it is okay to begin
		if ( g_hOccluderBegin ) SetEvent ( g_hOccluderBegin );

		//Dave Performance - wait for occluder to finish first
		if ( g_hOccluderEnd ) WaitForSingleObject ( g_hOccluderEnd, INFINITE );
	}
	
	//  free any character AI related stuff
	darkai_release_characters ( );
	darkai_destroy_all_characterdata ( );

	// remove bits created by LUA scripts
	lua_freeprompt3d();
	#ifdef VRTECH
	lua_freeallperentity3d();
	#endif

	//  close down game entities
	entity_free ( );

	//  Delete any infinilights
	lighting_free ( );

	//  AI finish
	darkai_free ( );

	//  free physics
	physics_free ( );

	#ifdef WICKEDENGINE
	// Clear the static physics data so we can get the dynamic data.
	BPhys_ClearDebugDrawData();
	#endif

	//  deselect current gun and hide all gun objects
	gun_free ( );
	gun_freeguns ( );
	gun_removempgunsfromlist ( );

	//  remove water and sky effects
	#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
	if( t.game.gameisexe >= 1) //Keep sky for editor.
	#endif
	sky_free ( );

	terrain_water_free ( );

	//  restore terrain from in-game
	terrain_stop_play ( );

	//  free vegetation
	grass_free ( );

	//  free Construction Kit
	conkit_free ( );

	//  free any visual leftovers
	visuals_free ( );

	//  free character sound
	character_sound_free ( );

	//  close script system
	lua_free ( );

	//  free projectiles
	weapon_projectile_free ( );

	//  free particles
	#ifdef WICKEDENGINE
	// this REALLY slows down return from test game!
	#else
	ravey_particles_free ( );
	#endif

	// finally delete entity element objs (only if standalone)
	if ( t.game.gameisexe == 1 )
	{
		#ifdef WICKEDENGINE
		//PE: Need to delete all particle emtters.
		gpup_deleteAllEffects();
		#endif

		// only for standalone as test game needs entities for editor :)
		entity_delete ( );
		//PE: Free any lightmaps, next level might not use lightmaps.
		lm_deleteall();
		ClearAnyLightMapInternalTextures();

		//PE: Delete all entitybank textures used.
		if( g.standalonefreememorybetweenlevels == 1 )
			ClearAnyEntitybankInternalTextures();
	}
}

void game_init ( void )
{
	//  Machine independent speed
	game_timeelapsed_init ( );

	//  Load slider menu resources
	sliders_init ( );

	//  Trigger all visuals to update
	t.visuals.refreshshaders=1;

	//  HideMouse (  and clear deltas )
	game_hidemouse ( );

	//  Last thing before main game loop
	physics_beginsimulation ( );

	//  Just before start, stagger AI processing timers
	darkai_staggerAIprocessing ( );

	//  Reset game checkpoint
	t.playercheckpoint.stored=1;
	t.playercheckpoint.x=CameraPositionX(0);
	t.playercheckpoint.y=CameraPositionY(0);
	t.playercheckpoint.z=CameraPositionZ(0);
	t.playercheckpoint.a=CameraAngleY(0);
	//t.playercheckpoint.soundloopcheckpointcountdown=20;

	//  Reset hardware flags with each new level map
	t.hardwareinfoglobals.noterrain=0;
	t.hardwareinfoglobals.nowater=0;
	t.hardwareinfoglobals.noguns=0;
	t.hardwareinfoglobals.nolmos=0;
	t.hardwareinfoglobals.nosky=0;
	t.hardwareinfoglobals.nophysics=0;
	t.hardwareinfoglobals.noai=0;
	t.hardwareinfoglobals.nograss=0;
	t.hardwareinfoglobals.noentities=0;

	//  initialise panel resources
	panel_init ( );

	//  construction kit f9 mode cursor
	t.characterkitcontrol.oldF9CursorEntid = 0;
}

void game_freegame ( void )
{
	// Ensure we are switched back to full res
	t.bHiResMode = true;
	t.bOldHiResMode = true;
	SetCameraHiRes ( t.bHiResMode );

	//Free up spawns sent to lua
	t.entitiesActivatedForLua.clear();

	// Ensure Steam chat sprite has gone
	if (  SpriteExist(g.steamchatpanelsprite)  )  DeleteSprite (  g.steamchatpanelsprite );

	// Restore fonts
	loadallfonts();

	//Switch back to using an additional sort visibility list in the editor to avoid flickering when adding new entities
	g_bInEditor = true;
	//  Free file map access game uses
	physics_player_free ( );

	//  Free slider resources, not needed for title
	sliders_free ( );

	// Free LUA Sprites and Images
	FreeLUASpritesAndImages ();

	//  Free resources not specific to a single level before returning to title page
	postprocess_free ( );

	if (  t.game.runasmultiplayer == 1 ) 
	{
		mp_free_game ( );
	}

	panel_free ( );

	// free temp bitmap used to redirect 2D drawing to an image
	panel_Free2DDrawing();

	//Dave Performance - switch entities and profile object back to animating
	for ( int c = 0 ; c < (int)t.characterBasicEntityList.size() ; c++ )
	{		
		int tobj = t.entityelement[t.characterBasicEntityList[c]].obj;
		if ( tobj > 0 )
		{
			if ( ObjectExist ( tobj ) == 1 )
			{
				if ( !t.characterBasicEntityListIsSetToCharacter[c] )
				{
					SetObjectEffect( tobj , t.characterBasicShaderID );
					SetObjectEffect( g.entitybankoffset+t.entityelement[t.characterBasicEntityList[c]].bankindex , t.characterBasicShaderID );				
				}
			}
		}
	}

	t.characterBasicEntityList.clear();
	t.characterBasicEntityListIsSetToCharacter.clear();

	// switch all objects back to dynamic before heading back to the editor
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entityprofile[t.entid].ismarker == 0 ) 
		{
			t.obj=t.entityelement[t.e].obj;

			if (  t.obj>0 ) 
			{
				SetObjectStatic(t.obj , false );
			}
		}
	}

	//Dave Performance - restore any ignored objects and get the texture list sortid again to include them
	ClearIgnoredObjects ( );
	DoTextureListSort ( );
}

#ifdef ENABLEIMGUI

void game_hidemouse(void)
{
	if (g.mouseishidden == 0) {
		g.mouseishidden = 1;
		HideMouse();
		t.null = MouseMoveX() + MouseMoveY();
		return;
	}
}

void game_showmouse(void)
{
	if (g.mouseishidden == 1) {
		g.mouseishidden = 0;
		ShowMouse();
		//Tab Tab bug ?
		t.null = MouseMoveX() + MouseMoveY();
		return;
	}
}

#else

void game_hidemouse ( void )
{
	if (  g.mouseishidden == 0 ) 
	{
		g.mouseishidden=1;
		if (  t.game.gameisexe == 1 ) 
		{
			t.tgamemousex_f=MouseX();
			t.tgamemousey_f=MouseY();
			HideMouse (  );
		}
		else
		{
			OpenFileMap (  1, "FPSEXCHANGE" );
			SetEventAndWait (  1 );
			SetFileMapDWORD (  1,974,1 );
			SetEventAndWait (  1 );
			t.tgamemousex_f=GetFileMapDWORD( 1, 0 );
			t.tgamemousey_f=GetFileMapDWORD( 1, 4 );
			t.tgamemousex_f=t.tgamemousex_f/800.0;
			t.tgamemousey_f=t.tgamemousey_f/600.0;
			t.tgamemousex_f=t.tgamemousex_f*(GetDisplayWidth()+0.0);
			t.tgamemousey_f=t.tgamemousey_f*(GetDisplayHeight()+0.0);
		}
		t.null=MouseMoveX()+MouseMoveY();
	}
}

void game_showmouse ( void )
{
	if (  g.mouseishidden == 1 ) 
	{
		g.mouseishidden=0;
		if (  t.game.gameisexe == 1 ) 
		{
			ShowMouse (  );
		}
		else
		{
			OpenFileMap (  1, "FPSEXCHANGE" );
			SetEventAndWait (  1 );
			t.tgamemousex_f=t.inputsys.xmouse+0.0;
			t.tgamemousey_f=t.inputsys.ymouse+0.0;
			SetFileMapDWORD (  1,982,t.tgamemousex_f );
			SetFileMapDWORD (  1,986,t.tgamemousey_f );
			SetFileMapDWORD (  1,974,2 );
			SetEventAndWait (  1 );
		}
		t.null=MouseMoveX()+MouseMoveY();
	}
}
void game_showmouse_restore_mouse(void)
{
	if (g.mouseishidden == 1)
	{
		g.mouseishidden = 0;
		if (t.game.gameisexe == 1)
		{
			ShowMouse();
		}
		else
		{
			OpenFileMap(1, "FPSEXCHANGE");
			SetEventAndWait(1);
			t.tgamemousex_f = t.inputsys.xmouse + 0.0;
			t.tgamemousey_f = t.inputsys.ymouse + 0.0;
			SetFileMapDWORD(1, 982, t.tgamemousex_f);
			SetFileMapDWORD(1, 986, t.tgamemousey_f);
			SetFileMapDWORD(1, 974, 3);
			SetEventAndWait(1);
		}
		t.null = MouseMoveX() + MouseMoveY();
	}
}

#endif

void game_timeelapsed_init ( void )
{
	//  Machine indie speed
	t.TimerFrequency_f=PerformanceFrequency();
	t.StartFrameTime=PerformanceTimer();
	t.ElapsedTime_f=0.0;
	t.LastTimeStamp_f=timeGetSecond();
	g.timeelapsed_f=0;
}

void game_timeelapsed ( void )
{
	// Calculate time between cycles
	float fThisTimeCount = timeGetSecond();
	t.ElapsedTime_f = fThisTimeCount - t.LastTimeStamp_f;
	g.timeelapsed_f = t.ElapsedTime_f * 20.0;
	t.LastTimeStamp_f = fThisTimeCount;

	//  Cap to around 25fps so that leaps in movement/speed not to severe!
	if (  g.timeelapsed_f>0.75f  )  g.timeelapsed_f = 0.75f;
	if (  g.timeelapsed_f<0.00833f  )  g.timeelapsed_f = 0.00833f;

	#ifdef WICKEDENGINE
	void update_per_frame_effects(void);
	update_per_frame_effects();
	#endif
}

#ifdef VRTECH

void game_main_snapshotsoundloopcheckpoint ( void )
{
	//  remember any looping sounds but exclude weapon and rocket sounds
	if (  t.playercontrol.disablemusicreset == 0 ) 
	{
		for ( t.s = g.soundbankoffset ; t.s<=  g.soundbankoffsetfinish; t.s++ )
		{
			if (  t.soundloopcheckpoint[t.s] != 2 ) 
			{
				t.soundloopcheckpoint[t.s]=0;
				if (  SoundExist(t.s) == 1 ) 
				{
					if (  SoundLooping(t.s) == 1 ) 
					{
						t.soundloopcheckpoint[t.s]=1;
					}
				}
			}
		}
	}
}

void game_main_snapshotsoundresume ( void )
{
	if (  t.playercontrol.disablemusicreset == 0 ) 
	{
		for ( t.s = g.soundbankoffset ; t.s<=  g.soundbankoffsetfinish; t.s++ )
		{
			if (  t.soundloopcheckpoint[t.s] != 2 ) 
			{
				if (  t.soundloopcheckpoint[t.s] == 1 ) 
				{
					t.soundloopcheckpoint[t.s]=0;
					if (  SoundExist(t.s) == 1 ) 
					{
						LoopSound (  t.s );
					}
				}
			}
		}
	}
}

#else

void game_main_snapshotsound ( bool bPauseForGameMenu )
{
	// remember any looping sounds but exclude weapon and rocket sounds
	if ( t.playercontrol.disablemusicreset == 0 ) 
	{
		for ( t.s = g.soundbankoffset; t.s <= g.soundbankoffsetfinish; t.s++ )
		{
			if ( t.soundloopgamemenu[t.s] != 2 ) 
			{
				t.soundloopgamemenu[t.s]=0;
				if ( SoundExist(t.s) == 1 ) 
				{
					if (SoundPlaying(t.s) == 1 || SoundLooping(t.s) == 1)
					{
						if ( bPauseForGameMenu == true ) PauseSound(t.s);
						if ( SoundLooping(t.s) == 1 )
							t.soundloopgamemenu[t.s] = 3;
						else
							t.soundloopgamemenu[t.s] = 1;
					}
				}
			}
		}
	}
}

void game_main_snapshotsoundresume ( void )
{
	if ( t.playercontrol.disablemusicreset == 0 ) 
	{
		for ( t.s = g.soundbankoffset; t.s <= g.soundbankoffsetfinish; t.s++ )
		{
			if ( t.soundloopgamemenu[t.s] != 2 ) 
			{
				if (SoundExist(t.s) == 1)
				{				
					if ( t.soundloopgamemenu[t.s] != 0 ) 
					{
						ResumeSound(t.s);
						t.soundloopgamemenu[t.s]=0;
					}
				}
			}
		}
	}
}

#endif

extern bool g_bEarlyExcludeMode;
int oldSpaceKey = 0;

extern int NumberOfObjects;
extern int NumberOfGroupsShown;
extern int NumberOfObjectsShown;

void game_main_loop ( void )
{	
	//  Timer (  based movement )
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_timeelapsed");
	game_timeelapsed ( );

	//  Music processing
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling music_loop");
	music_loop ( );

	//  Character sound update
	//  110315 - 019 - If spawning in, no sound for the player
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling character_sound_update");
	if (  t.game.runasmultiplayer  ==  0 || g.mp.noplayermovement  ==  0 ) 
	{
		character_sound_update ( );
	}

	//  Trigger soundloops to be snapshot (when start level and at checkpoints)
	//if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_main_snapshotsoundloopcheckpoint");
	//if (  t.playercheckpoint.soundloopcheckpointcountdown>0 ) 
	//{
	//	--t.playercheckpoint.soundloopcheckpointcountdown;
	//	if (  t.playercheckpoint.soundloopcheckpointcountdown == 0 ) 
	//	{
	//		game_main_snapshotsoundloopcheckpoint ( );
	//	}
	//}

	//  Force a shader update to ensure correct shadows are used at start
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling visuals_shaderlevels_update");
	if (  t.visuals.refreshcountdown>0 ) 
	{
		--t.visuals.refreshcountdown;
		if (  t.visuals.refreshcountdown == 0 ) 
		{
			visuals_shaderlevels_lighting_update ( );
			visuals_shaderlevels_update ( );
			t.visuals.refreshshaders=1;
		}
	}

	#ifdef VRTECH
	// debug reason - why is my SocialVR completely black!!
	bool bSocialVRDebugTABTAB = true;
	#endif

	// Testgame or Standalone
	// 250316 - when level ends, suspend all logic (including more calls to JumpTolevel or in-game last minute AI stuff)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking levelendingcycle");
	if ( t.game.levelendingcycle == 0 )
	{
		#ifdef VRTECH
		if ( (t.game.gameisexe == 0 || g.gprofileinstandalone == 1) && (t.game.runasmultiplayer == 0 || bSocialVRDebugTABTAB == true)  ) 
		#else
		if ( (t.game.gameisexe == 0 || g.gprofileinstandalone == 1) && (t.game.runasmultiplayer == 0)  ) 
		#endif
		{
			// Test Game Mode
			// Handle light-mapper key
			if ( g.globals.ideinputmode == 1 ) 
			{
				g.lmlightmapnowmode=0;
				#ifdef VRTECH
				 // No lightmapping in VRTECH
				#else
				 if (g.gproducelogfiles == 2) timestampactivity(0, "checking lightmapper handler");
				 if (KeyState(g.keymap[59]) && (KeyState(g.keymap[42]) || KeyState(g.keymap[54])))  g.lmlightmapnowmode = 1;
				 if (KeyState(g.keymap[60]) && (KeyState(g.keymap[42]) || KeyState(g.keymap[54])))  g.lmlightmapnowmode = 2;
				 if (KeyState(g.keymap[61]) && (KeyState(g.keymap[42]) || KeyState(g.keymap[54])))  g.lmlightmapnowmode = 3;
			 	 if (KeyState(g.keymap[62]) && (KeyState(g.keymap[42]) || KeyState(g.keymap[54])))  g.lmlightmapnowmode = 4;
				#endif
				if ( g.lmlightmapnowmode>0 )
				{
					#ifdef FREETRIALVERSION
					 // No lightmapping in free trial version
					 t.visuals.generalpromptstatetimer=Timer()+1000;
					 t.visuals.generalprompt_s="No lightmapper in free trial version";
					#else
					 //  User prompt
					 t.strwork = ""; t.strwork = t.strwork + "Select Lightmapping Mode "+Str(g.lmlightmapnowmode);
					 timestampactivity(0, t.strwork.Get() ) ; t.twas=Timer();
					 t.tdisableLMprogressreading=1;
					 for ( t.n = 0 ; t.n<=  1; t.n++ )
					 {
						t.tonscreenprompt_s="Saving Level Session";
						if ( t.hardwareinfoglobals.noterrain == 0 ) terrain_update ( );
						lm_onscreenprompt ( ) ; Sync ( );
					 }
					 t.tdisableLMprogressreading=0;
					 //  save level (temp)
					 t.gamevisuals=t.visuals;
					 g.gpretestsavemode=1;
					 gridedit_save_test_map ( );
					 g.gpretestsavemode=0;
					 t.visuals=t.gamevisuals;
					 //  first WIPE OUT old lightmap files
					 lm_emptylightmapandttsfilesfolder ( );
					 //  second WIPE OUT old LM objects to ensure FRESH bake
					 for ( t.tlmobj = g.lightmappedobjectoffset; t.tlmobj<= g.lightmappedobjectoffsetlast; t.tlmobj++ )
					 {
						if (  ObjectExist(t.tlmobj) == 1 ) 
						{
							DeleteObject (  t.tlmobj );
						}
					 }
					 g.lightmappedobjectoffsetfinish=92000;
					 g.lightmappedterrainoffset=-1;
					 g.lightmappedterrainoffsetfinish=-1;
					 //  launch external lightmapper
					 //SetDir (  ".." );
					 SetDir ( g.lightmapperexefolder_s.Get() );
					 timestampactivity(0,"launch external lightmapper") ; t.twas=Timer();
					 t.tdisableLMprogressreading=1;
					 for ( t.n = 0 ; t.n<=  1; t.n++ )
					 {
						t.tonscreenprompt_s="Lightmapping in Progress";
						if ( t.hardwareinfoglobals.noterrain == 0 ) terrain_update ( );
						lm_onscreenprompt ( ) ; Sync (  );
					 }
					 t.tdisableLMprogressreading=0;
					 t.strwork = ""; t.strwork = t.strwork + "-"+Str(g.lmlightmapnowmode);
					 ExecuteFile (  "Guru-Lightmapper.exe", t.strwork.Get() ,"",1 );
					 t.strwork = ""; t.strwork = t.strwork + "returned from t.lightmapper - baked in "+Str((Timer()-t.twas)/1000)+" seconds";
					 timestampactivity(0, t.strwork.Get() );
					 SetDir ( g.rootdir_s.Get() );//"Files" );
					 //  Wait for all input to cease
					 t.tdisableLMprogressreading=1;
					 while (  ScanCode() != 0 || MouseClick() != 0 ) 
					 {
						for ( t.n = 0 ; t.n<=  1; t.n++ )
						{
							t.tonscreenprompt_s="Returning from lightmapper";
							if ( t.hardwareinfoglobals.noterrain == 0 ) terrain_update ( );
							lm_onscreenprompt ( ) ; Sync ( ) ;
						}
					 }
					 //  User prompt
					 for ( t.n = 0 ; t.n<=  1; t.n++ )
					 {
						t.tonscreenprompt_s="Loading Lightmaps";
						if ( t.hardwareinfoglobals.noterrain == 0 ) terrain_update ( );
						lm_onscreenprompt ( ) ; Sync ( ) ;
					 }
					 t.tdisableLMprogressreading=0;
					 //  load new lightmap scene
					 t.lightmapper.onlyloadstaticentitiesduringlightmapper=1;
					 lm_loadscene ( );
					 t.lightmapper.onlyloadstaticentitiesduringlightmapper=0;
					 if (  t.tlmloadsuccess == 1 ) 
					 {
						//  Switch to pre-bake mode
						t.visuals.shaderlevels.lighting=1;
						t.gamevisuals.shaderlevels.lighting=1;
						t.slidersmenuindex=t.slidersmenunames.shaderoptions;
						t.slidersmenuvalue[t.slidersmenuindex][4].value=1;
						t.slidersmenuvaluechoice=t.slidersmenuvalue[t.slidersmenuindex][4].gadgettypevalue;
						t.slidersmenuvalueindex=t.slidersmenuvalue[t.slidersmenuindex][4].value;
						sliders_getnamefromvalue ( );
						t.slidersmenuvalue[t.slidersmenuindex][4].value_s=t.slidervaluename_s;
						visuals_shaderlevels_update ( );
					 }
					 //  restore water mask which may be affected
					 terrain_whitewashwatermask ( );
					 //  refresh shaders in any event
					 t.visuals.refreshshaders=1;
					#endif
				}
			 }

			//  Tab Mode (only when not mid-fpswarning)
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking tab key handler");
			if ( t.plrkeySHIFT == 0 && t.plrkeySHIFT2 == 0  )  t.tkeystate15 = KeyState(g.keymap[15]); else t.tkeystate15 = 0;
			#ifdef VRTECH
			if ( t.game.runasmultiplayer == 1 && bSocialVRDebugTABTAB == false ) g.tabmode = 0;
			#else
			if ( t.game.runasmultiplayer == 1 ) g.tabmode = 0;
			#endif
			if ( t.conkit.editmodeactive == 1 )  g.tabmode = 0;
			if ( g.lowfpswarning == 0 || g.lowfpswarning == 3 ) 
			{
				if ( t.tkeystate15 == 0 ) t.tabpress = 0;
				if ( g.globals.riftmode>0 ) 
				{
					if (  t.tkeystate15 == 1 && t.tabpress == 0 ) 
					{
						// 101115 - reset hardware menu if press TAB
						g.tabmodeshowfps = 0;

						if (  g.tabmode == 0 ) 
						{
							game_showmouse ( );
							g.tabmode=2;
						}
						else
						{
							game_hidemouse ( );
							g.tabmode=0;
						}
						t.tabpress=1;
					}
				}
				else
				{
					if (  t.tkeystate15 == 1 && t.tabpress == 0 ) 
					{
						// 101115 - reset hardware menu if press TAB
						g.tabmodeshowfps = 0;

						g.tabmode=g.tabmode+1;
						if (  g.tabmode>2 ) 
						{
							g.tabmode=0;
						}
						if (  g.tabmode<2 ) 
						{
							game_hidemouse ( );
						}
						if (  g.tabmode == 2 ) 
						{
							game_showmouse ( );
						}
						t.tabpress=1;
					}
				}
			}
		}
		else
		{
			//  Standalone Mode
			#ifdef FREETRIALVERSION
			 if ( t.game.gameisexe != 0 )
			 {
				// No lightmapping in free trial version
				t.visuals.generalpromptstatetimer=Timer()+123;
				t.visuals.generalprompt_s="Game Created With Free Trial Version Of GameGuru";
			 }
			#endif
		}

		//  Measure Sync (  to loop start )
		t.game.perf.resttosync += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

		//  Control slider menus (based on tab page)
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sliders_loop");
		sliders_loop ( );

		#ifdef WICKEDENGINE
		// CTRL+H to hide the hud when testing levels. For reason some the key results don't match what they should be from keymap?
		if (t.game.gameisexe == 0)
		{
			static int iKeyRepeatCounter = 0;
			iKeyRepeatCounter++;
			if (iKeyRepeatCounter > 60)
				iKeyRepeatCounter = 60;
			if (KeyState(g.keymap[35]) && KeyState(g.keymap[29]) && iKeyRepeatCounter >= 60)
			{
				g.tabmodehidehuds = !g.tabmodehidehuds;
				iKeyRepeatCounter = 0;
			}
		}
		#endif

		//  Lighting control
		//  Entity control
		//  Particles control
		//  Flak control

		//  update all projectiles
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling weapon_projectile_loop");
		weapon_projectile_loop ( );

//		//  update all particles and emitters
//		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling ravey_particles_update");
//		ravey_particles_update ( );

		//  Decal control
//		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling decalelement_control");
//		decalelement_control ( );

		//  Prompt
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking prompts");
		if (  t.sky.currenthour_f<1.0 || t.sky.currenthour_f >= 13.0 ) 
		{
			t.pm=int(t.sky.currenthour_f);
			if (  t.pm == 0  )  t.pm = 12; else t.pm = t.pm-12;
			t.pm_s = ""; t.pm_s = t.pm_s + Str(t.pm)+"PM";
		}
		else
		{
			t.pm_s = "";t.pm_s = t.pm_s + Str(int(t.sky.currenthour_f))+"AM";
		}
		t.promptextra_s = ""; t.promptextra_s=t.promptextra_s + "FPS:"+Str(GetDisplayFPS())+" TIME:"+t.pm_s;
		t.game.perf.misc += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

		//  any terrain actions
		if (  t.hardwareinfoglobals.noterrain == 0 ) 
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling terrain_loop");
			terrain_loop ( );
		}

		//  loop physics
		if (  t.hardwareinfoglobals.nophysics == 0 ) 
		{
			// Handle physics
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_loop");
			physics_loop ( );

			// read all slider values for player
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sliders readall");
			t.slidersmenuindex=t.slidersmenunames.player; sliders_readall ( );

			//  Do weapon attachments AFTER physics moved objects (and if char killed off)
			for ( g.charanimindex = 1 ; g.charanimindex <= g.charanimindexmax; g.charanimindex++ )
			{
				// update gun position in hand of character
				t.e = t.charanimstates[g.charanimindex].e;
				if ( t.e > 0 ) entity_controlattachments ( );

				// detect collection of dropped guns
				t.e = t.charanimstates[g.charanimindex].originale;
				if ( t.e > 0 ) entity_monitorattachments ( );
			}

			//  But do allow third person protagonist attachment control
			if (  t.playercontrol.thirdperson.enabled == 1 ) 
			{
				t.e=t.playercontrol.thirdperson.charactere;
				if (  t.e>0 && t.player[1].health>0 ) 
				{
					if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling entity_controlattachments");
					entity_controlattachments ( );
				}
			}

			//  Construction Kit control
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling conkit_loop");
			conkit_loop ( );
		}
		t.game.perf.physics += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

		// In-Game Mode (moved from above so LUA is AFTER physics)
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking in-game edit mode");
		if ( t.conkit.editmodeactive == 0 ) 
		{
			// if third person, trick AI by moving camera to protagonist location
			if ( t.playercontrol.thirdperson.enabled == 1 ) 
			{
				t.playercontrol.thirdperson.storecamposx = CameraPositionX(t.terrain.gameplaycamera);
				t.playercontrol.thirdperson.storecamposy = CameraPositionY(t.terrain.gameplaycamera);
				t.playercontrol.thirdperson.storecamposz = CameraPositionZ(t.terrain.gameplaycamera);
				t.tobj = t.aisystem.objectstartindex;
				if ( g.luacameraoverride != 1 && g.luacameraoverride != 3 )
				{
					PositionCamera ( t.terrain.gameplaycamera, ObjectPositionX(t.tobj), ObjectPositionY(t.tobj), ObjectPositionZ(t.tobj) );
				}
			}

			// All Entity logic
			t.ttempoverallaiperftimerstamp=PerformanceTimer();
			if ( t.hardwareinfoglobals.noai == 0 ) 
			{
				// LUA Logic
				#ifdef WICKEDENGINE
				auto range1 = wiProfiler::BeginRangeCPU("Max - General - LUA Logic");
				#endif
				lua_loop ( );
				#ifdef WICKEDENGINE
				wiProfiler::EndRange(range1);
				#endif

				// Entity Logic
				#ifdef WICKEDENGINE
				auto range2 = wiProfiler::BeginRangeCPU("Max - General - Object Logic");
				#endif
				t.game.perf.ai1 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
				entity_loop ( );
				entity_loopanim ( );
				t.game.perf.ai2 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
				#ifdef WICKEDENGINE
				wiProfiler::EndRange(range2);
				#endif

				// Update all AI and Characters and VWeaps
				#ifdef WICKEDENGINE
				auto range3 = wiProfiler::BeginRangeCPU("Max - General - AI Logic");
				#endif
				if ( t.aisystem.processlogic == 1 )
				{
					if ( t.visuals.debugvisualsmode<100 ) 
					{
						darkai_loop ( );
						#ifndef WICKEDENGINE
						t.game.perf.ai3 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
						darkai_update ( );
						t.game.perf.ai4 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
						if (  t.visuals.debugvisualsmode<98 ) 
						{
							darkai_character_loop ( );
						}
						t.game.perf.ai5 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
						t.game.perf.ai6 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
						#endif
					}
				}
				#ifdef WICKEDENGINE
				wiProfiler::EndRange(range3);
				#endif

				// handle any AI stuff related to recastretour
				#ifdef WICKEDENGINE
				#ifdef NEWMAXAISYSTEM
				auto range4 = wiProfiler::BeginRangeCPU("Max - General - Detour Logic");
				game_updatenavmeshsystem();
				wiProfiler::EndRange(range4);
				#endif
				#endif
			}
			t.game.perf.ai += PerformanceTimer()-t.ttempoverallaiperftimerstamp;

			// don't do this when in f9 mode
			if ( !g_occluderf9Mode) game_check_character_shader_entities ( );
		}

		// if third person, restore camera from protag-cam trick
		if ( t.playercontrol.thirdperson.enabled == 1 ) 
		{
			if ( g.luacameraoverride != 1 && g.luacameraoverride != 3 )
			{
				PositionCamera (  t.terrain.gameplaycamera,t.playercontrol.thirdperson.storecamposx,t.playercontrol.thirdperson.storecamposy,t.playercontrol.thirdperson.storecamposz );
			}
		}

		//  Gun control
		if ( t.hardwareinfoglobals.noguns == 0 ) 
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling gun_manager");
			gun_manager ( );
			t.slidersmenuindex=t.slidersmenunames.weapon ; sliders_readall ( );
		}
		t.game.perf.gun += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
	}

	//PE: Moved here for "AmenMoses", issue: https://github.com/TheGameCreators/GameGuruRepo/issues/511

	//  update all particles and emitters
	#ifdef VRTECH
	update_env_particles();
	#endif
	ravey_particles_update();

	//  Decal control
	decalelement_control();

	#ifdef WICKEDENGINE
	// bullethole manegement
	bulletholes_update();
	#endif

	//  Steam call moved here as camera changes need to be BEFORE the shadow update
	if (  t.game.runasmultiplayer == 1 ) 
	{
		// debug tracing
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling mp_gameLoop");

		// run multiplayer logic
		mp_gameLoop ( );

		#ifdef VRTECH
		// mp logic can trigger a new avatar to be loaded and created dynamically
		game_scanfornewavatars ( true );
		#endif
	}

	#ifdef VRTECH
	#else
	//  if we have character creator stuff in, we setup the characters
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling characterkit_updateCharacters");
	if ( t.characterkitcontrol.gameHasCharacterCreatorIn == 1 ) characterkit_updateCharacters ( );
	#endif

	// Handle veg engine, terrain shadow, sky and water
	// Dave Performance, calling update on veg and terrain shadow every 4 frames rather than every frame
	static int terrainshadowdelay = 0;

	// If the camera is spun round quick, redraw shadows immediately
	// 281116 - int tMouseMove = MouseMoveX(); - yep, this killed fluid mousemoveX, thanks for that Lee!
	int tMouseMove = t.cammousemovex_f;
	if ( tMouseMove > 20 || tMouseMove < -20 )
	{
		terrainshadowdelay = 4;		
	}

	if (  t.hardwareinfoglobals.noterrain == 0 ) 
	{
		//Dave Performance, calling update on veg and terrain shadow every 4 frames rather than every frame
		//Grass every other frame
		//Gets me 10fps increase on my machine		
		static bool terrainvegdelay = true;
		if ( terrainvegdelay = !terrainvegdelay )
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling grass_loop");
			grass_loop ( );
			t.game.perf.terrain1 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
		}
		else
			t.game.perf.terrain1 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

		// 111115 - had to add t.visuals.shaderlevels.entities==1 so HIGHEST entities allow self shadow characters and detailed entities which flicker when shadow update is delayed!
		// but also need constant updates for third person (as can see delay!)
		// PE: Its way better to cycle the different cascades so we dont get these "spicks" in FPS. way more smooth. ()
		if ( ++terrainshadowdelay >= 3 || t.visuals.shaderlevels.entities==1 || t.playercontrol.thirdperson.enabled != 0 || g.globals.speedshadows >= 1)
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling terrain_shadowupdate");
			terrainshadowdelay = 0;
			terrain_shadowupdate ( );
			t.game.perf.terrain2 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
		}
		else
		{
			t.game.perf.terrain2 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();
		}
	}
	else // fix to stop shadows going wonky when terrain is off
	{
		if ( ++terrainshadowdelay >= 3 )
		{
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling terrain_shadowupdate");
			terrainshadowdelay = 0;
			terrain_shadowupdate ( );
		}
	}
	if (  t.hardwareinfoglobals.nosky == 0 ) 
	{
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sky_loop");
		sky_loop ( );
	}
	if (  t.hardwareinfoglobals.nowater == 0 ) 
	{
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling terrain_water_loop");
		terrain_water_loop ( );
	}
	t.game.perf.terrain3 += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

	//  Game Debug Prompts
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"assembling debug prompts");
	if (  t.aisystem.showprompts == 1 ) 
	{
		pastebitmapfont("DEBUG PROMPTS",8,20,1,255) ; t.i=1;
		t.strwork = ""; t.strwork = t.strwork + "NUMBER OF CHARACTERS:"+Str(g.charanimindexmax);
		pastebitmapfont( t.strwork.Get() ,8,20+(t.i*25),1,255) ; ++t.i;
		for ( g.charanimindex = 1 ; g.charanimindex<=  g.charanimindexmax; g.charanimindex++ )
		{
			t.tdesc_s = ""; t.tdesc_s = t.tdesc_s + "CHAR:"+Str(g.charanimindex);
			t.tdesc_s=t.tdesc_s+"  AIstate_s="+AIGetEntityState(t.charanimstates[g.charanimindex].obj);
			t.tdesc_s=t.tdesc_s+"  t.moving="+Str(t.charanimcontrols[g.charanimindex].moving);
			t.tdesc_s=t.tdesc_s+"  t.ducking="+Str(t.charanimcontrols[g.charanimindex].ducking);
			pastebitmapfont(t.tdesc_s.Get(),8,20+(t.i*25),1,255) ; ++t.i;
			t.tdesc_s="";
			t.tdesc_s=t.tdesc_s+"  plrvisible="+Str(t.entityelement[t.charanimstates[g.charanimindex].e].plrvisible);
			t.tdesc_s=t.tdesc_s+"  health="+Str(t.entityelement[t.charanimstates[g.charanimindex].e].health);
			t.tdesc_s=t.tdesc_s+"  playcsi="+Str(t.charanimstates[g.charanimindex].playcsi);
			t.tdesc_s=t.tdesc_s+"  t.charseq.mode="+Str(t.charseq[t.charanimstates[g.charanimindex].playcsi].mode);
			pastebitmapfont(t.tdesc_s.Get(),8,20+(t.i*25),1,255) ; ++t.i;
			++t.i;
		}
	}

	// Handle occlusion if active
	if ( g.globals.occlusionmode == 1 ) 
	{
		#ifdef VRTECH
		// VR software cannot use occlusion at the moment
		if ( g.vrqcontrolmode == 0 ) //g.vrglobals.GGVREnabled == 0 )
		#else
		if(1)
		#endif
		{
			// detect velocity of XZ motion of player and advance 'virtual camera' ahead of real camera
			// in order to give occluder time to reveal visible objects in advance of getting there
			if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking occlusionp");
			float plrx = CameraPositionX(0);
			float plrz = CameraPositionZ(0);
			g_fOccluderCamVelX = plrx - g_fOccluderLastCamX;
			g_fOccluderCamVelZ = plrz - g_fOccluderLastCamZ;
			g_fOccluderLastCamX = plrx;
			g_fOccluderLastCamZ = plrz;
			if ( fabs(g_fOccluderCamVelX)>0.01f || fabs(g_fOccluderCamVelZ)>0.01f )
			{
				float fDDMultiplier = 20.0f / sqrt(fabs(g_fOccluderCamVelX*g_fOccluderCamVelX)+fabs(g_fOccluderCamVelZ*g_fOccluderCamVelZ));
				g_fOccluderCamVelX *= fDDMultiplier;
				g_fOccluderCamVelZ *= fDDMultiplier;
				if ( g_fOccluderCamVelX < -20.0f ) g_fOccluderCamVelX = -20.0f;
				if ( g_fOccluderCamVelZ < -20.0f ) g_fOccluderCamVelZ = -20.0f;
				if ( g_fOccluderCamVelX > 20.0f ) g_fOccluderCamVelX = 20.0f;
				if ( g_fOccluderCamVelZ > 20.0f ) g_fOccluderCamVelZ = 20.0f;
			}
			else
			{
				g_fOccluderCamVelX = 0.0f;
				g_fOccluderCamVelZ = 0.0f;
			}
			// show me this
			CPUShiftXZ ( g_fOccluderCamVelX, g_fOccluderCamVelZ );

			CPU3DSetCameraFar ( t.visuals.CameraFAR_f );
			if ( g_pOccluderThread == NULL )
			{
				CPU3DOcclude (  );
				g_hOccluderBegin = CreateEvent ( NULL, FALSE, FALSE, NULL );
				g_hOccluderEnd   = CreateEvent ( NULL, FALSE, FALSE, NULL );
				g_pOccluderThread = new cOccluderThread;
				g_pOccluderThread->Start ( );
			}
			g_occluderOn = true;
		}
	}
	t.game.perf.occlusion += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

	// Final post processing step

	// Render pre-terrain post process cameras (includes lightray rendering)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling postprocess_preterrain");
	postprocess_preterrain ( );

	// Render terrain if flagged
	if ( t.hardwareinfoglobals.noterrain == 0 ) 
	{
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling terrain_update");
		terrain_update ( );
	}

	//  explosions and fire
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling draw_particles");
	#ifdef WICKEDENGINE
	// no debris in wicked - to be GPU particles down the road
	#else
	draw_debris();
	#endif
	draw_particles();

	//  handle fade out for level progression
	if (  t.game.levelendingcycle > 0 ) 
	{
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_end_of_level_check");
		game_end_of_level_check ( );
	}

	//  Post process and visual settings system
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling postprocess_apply");
	postprocess_apply ( );
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling visuals_loop");
	visuals_loop ( );
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling lighting_loop");
	lighting_loop ( );
	t.game.perf.postprocessing += PerformanceTimer()-g.gameperftimestamp ; g.gameperftimestamp=PerformanceTimer();

	// Check for player guns switched off
	if ( g.noPlayerGuns )
	{
		if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling physics_no_gun_zoom");
		physics_no_gun_zoom ( );
		if ( g.autoloadgun != 0 ) { g.autoloadgun=0 ; gun_change ( ); }
	}

	//  Update HUD Layer objects (jetpack)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling hud_updatehudlayerobjects");
	hud_updatehudlayerobjects ( );

	//  Call this at end of game loop to ensure character objects sufficiently overridden
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling darkai_finalsettingofcharacterobjects");
	darkai_finalsettingofcharacterobjects ( );
}

extern int howManyOccluders;
extern int howManyOccludersDrawn;
extern int howManyOccludees;
extern int howManyOccludeesHidden;
extern int howManyMarkers;
extern float trackingSize;

void game_dynamicRes()
{
	// If dynamic res is disabled via setup ini, return out
	if ( t.DisableDynamicRes ) return;

	if ( g_pGlob->dwNumberOfPrimCalls > 500 && GetDisplayFPS() < 60 )
		t.bHiResMode = false;
	else if ( g_pGlob->dwNumberOfPrimCalls < 200 && GetDisplayFPS() > 60 )
		t.bHiResMode = true;

	if ( t.visuals.debugvisualsmode == 20 ) t.bHiResMode = false;
	if ( t.visuals.debugvisualsmode == 21 ) t.bHiResMode = true;

	if ( t.bHiResMode != t.bOldHiResMode )
	{
		SetCameraHiRes ( t.bHiResMode );
		t.bOldHiResMode = t.bHiResMode;
	}
}

extern float smallDistanceMulti;

void game_sync ( void )
{
	//  Work out overall time spent per cycle
	t.game.perf.overall += PerformanceTimer()-g.gameperfoveralltimestamp ; g.gameperfoveralltimestamp=PerformanceTimer();

	//  HUD Damage Display
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling controlblood");
	controlblood();
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling controldamagemarker");
	controldamagemarker();

	//  Slider menus rendered last
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sliders_draw");
	sliders_draw ( );

	//  Detect if FPS drops (only for single player - never for MP games)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking show hide mouse");
	if (  t.game.runasmultiplayer == 0 && g.globals.hidelowfpswarning == 0 && g.tabmode == 0 && g.ghardwareinfomode == 0 && t.visuals.generalpromptstatetimer == 0 ) 
	{
		if ( t.conkit.cooldown>0 )  
		{
			// cooldown ensures this FPS warning only happens when cooled off
			--t.conkit.cooldown;
			g.lowfpstarttimer = Timer();
		}
		if (  t.conkit.editmodeactive == 0 && t.conkit.cooldown == 0 ) 
		{
			if (  g.lowfpswarning == 0 ) 
			{
				if ( (unsigned long)Timer()>g.lowfpstarttimer+2000 ) 
				{
					if ( GetDisplayFPS()<20 ) 
					{
						g.lowfpswarning=1;
						game_showmouse ( );
					}
				}
			}
			else
			{
				if (  g.lowfpswarning == 2 ) 
				{
					game_hidemouse ( );
					while ( MouseClick() != 0 ) { }
					g.lowfpswarning=3;
				}
			}
		}
	}

	//  Only render main and postprocess camera (not paint camera, reflection or lightray cameras)
	//  for globals.riftmode, left and right eyes are rendered in the _postprocess_preterrain step
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"applying sync mask");
	t.tmastersyncmask=0;
	SyncMask (  t.tmastersyncmask+(1<<3)+(1) );

	//  Update RealSense if any
	///realsense_loop ( );

	//  Update screen
	//g.gameperftimestamp=PerformanceTimer();
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sync");
	Sync (  );
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling game_dynamicRes");
	game_dynamicRes();

	//Dave Performance - let the occluder thread know it is okay to begin
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling CPU3DOcclude");
	CPU3DOcclude (  );
	if ( g_hOccluderBegin ) SetEvent ( g_hOccluderBegin );

	t.game.perf.synctime += (PerformanceTimer()-g.gameperftimestamp) ; g.gameperftimestamp=PerformanceTimer();

	//  collect main Sync (  statistics )
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"checking statistics");
	t.mainstatistic1=GetStatistic(1);
	t.mainstatistic5=GetStatistic(5);

	//  detect and slow down action
	if (  t.visuals.debugvisualsmode == 4 ) 
	{
		if (  ReturnKey() == 1 ) 
		{
			t.player[1].health=50000;
			physics_pausephysics ( );
			SleepNow (  200 );
			physics_resumephysics ( );
		}
	}

	#ifdef WICKEDENGINE
	#else
	//  Screen shot feature (redundant on Steam)
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"handling screenshot taking");
	if (  KeyState(g.keymap[68]) == 0  )  t.game.takingsnapshotpress = 0;
	if (  KeyState(g.keymap[68]) == 1 && t.game.takingsnapshotpress == 0 ) 
	{
		//  Grab current screen and save out under unique filename
		if (  ImageExist(g.savescreenshotimage) == 1  )  DeleteImage (  g.savescreenshotimage );
		GrabImage (  g.savescreenshotimage,0,0,GetDisplayWidth(),GetDisplayHeight(),1 );
		file_createmydocsfolder ( );
		t.tfilenum=1;
		while (  FileExist( cstr(g.myownrootdir_s+"\\snapshot"+Str(t.tfilenum)+".jpg").Get() ) == 1 ) 
		{
			++t.tfilenum;

			// GOCTHA!!
			t.strwork = ""; t.strwork = t.strwork + g.myownrootdir_s + "\\snapshot" + Str(t.tfilenum) + ".jpg";
		}
		t.strwork = ""; t.strwork += g.myownrootdir_s+"\\snapshot"+Str(t.tfilenum)+".jpg";
		if ( ImageExist(g.savescreenshotimage) == 1 )
		{
			SaveImage ( t.strwork.Get(), g.savescreenshotimage, 0 );
			DeleteImage (  g.savescreenshotimage );
		}
		t.game.takingsnapshotpress=1;
	}
	#endif

	//  Work out performance metrics
	if ( g.gproducelogfiles == 2 ) timestampactivity(0,"calling sliders readall");
	t.slidersmenuindex=t.slidersmenunames.performance  ; sliders_readall ( );
}

void game_main_stop ( void )
{
	// Rest any ingame variables
	if ( t.conkit.entityeditmode != 0 || t.conkit.editmodeactive == 1 ) 
	{
		conkitedit_switchoff ( );
	}
}

void game_jump_to_level_from_lua ( void )
{
	if (  t.game.gameisexe == 1 ) 
	{
		if (  t.game.gameloop == 1 && t.game.levelendingcycle  ==  0 ) 
		{
			t.game.jumplevel_s = t.tleveltojump_s;
			t.game.levelendingcycle = 4000;
		}
	}
	else
	{
		t.s_s = "" ; t.s_s = t.s_s+"Jump To Level : "+t.tleveltojump_s ; lua_prompt ( );
	}
}

void game_finish_level_from_lua ( void )
{
	if (  t.game.gameisexe == 1 ) 
	{
		if (  t.game.gameloop == 1 && t.game.levelendingcycle  ==  0 ) 
		{
			t.game.levelendingcycle = 4000;
		}
	}
	else
	{
		#ifdef VRTECH
			t.s_s="Game Completed"  ; lua_prompt ( );
		#else
			t.s_s="Level Complete Triggered"  ; lua_prompt ( );
		#endif
	}
}

void game_end_of_level_check ( void )
{
	//  end of level fade out
	t.game.levelendingcycle = t.game.levelendingcycle - (g.timeelapsed_f * 200.0);
	t.huddamage.immunity=1000;
	if (  t.game.levelendingcycle  <=  0 ) 
	{
		t.game.gameloop=0;
		t.game.levelendingcycle = 0;
	}

	//  control fade out of screen
	t.postprocessings.fadeinvalue_f = t.game.levelendingcycle / 4500.0; //PE: fadeinvalue_f is a bit delayed , so just count down faster.
	t.postprocessings.fadeinvalueupdate=1;

	//  fade audio
	if (  t.audioVolume.music > t.postprocessings.fadeinvalue_f * 100.0  )  t.audioVolume.music  =  t.postprocessings.fadeinvalue_f * 100.0;
	if (  t.audioVolume.sound > t.postprocessings.fadeinvalue_f * 100.0  )  t.audioVolume.sound  =  t.postprocessings.fadeinvalue_f * 100.0;
	audio_volume_update ( );
}
