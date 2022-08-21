//----------------------------------------------------
//--- GAMEGURU - M-Entity
//----------------------------------------------------

#include "stdafx.h"
#include "gameguru.h"

#ifdef ENABLEIMGUI
	#include "..\Imgui\imgui_gg_extras.h"
#endif

#ifdef WICKEDENGINE
#include ".\\..\..\\Guru-WickedMAX\\GPUParticles.h"
using namespace GPUParticles;
#include "GGTerrain\GGTerrain.h"
using namespace GGTerrain;
#endif

// Globals for blacklist string array
LPSTR* g_pBlackList = NULL;
int g_iBlackListMax = 0;
bool g_bBlackListRemovedSomeEntities = false;

int g_iWickedEntityId = -1;
int g_iWickedElementId = 0;
int g_iWickedMeshNumber = 0;
bool g_bUseEditorGrideleprof = false;
int g_iAbortedAsEntityIsGroupFileMode = 0;
cstr g_sTempGroupForThumbnail = "";

float g_fFlattenMargin = 100.0f;

// prototypes
void LoadFBX ( LPSTR szFilename, int iID );

//. externs
extern std::vector<int> g_ObjectHighlightList;


// 
//  ENTITY COMMON CODE (not game specific)
// 

void entity_addtoselection_core ( void )
{
	//  ensure ENT$ does not contain duplicate \ symbols
	t.tnewent_s="";
	for ( t.n = 1 ; t.n<=  Len(t.ent_s.Get()); t.n++ )
	{
		t.tnewent_s=t.tnewent_s+Mid(t.ent_s.Get(),t.n);
		if ( cstr(Mid(t.ent_s.Get(),t.n)) == "\\" && cstr(Mid(t.ent_s.Get(),t.n+1)) == "\\" ) 
		{
			++t.n;
		}
	}
	t.ent_s=t.tnewent_s;

	// Load entity from file
	t.entdir_s="entitybank\\";
	t.ent_s=Right(t.ent_s.Get(),1+(Len(t.ent_s.Get())-(Len(g.rootdir_s.Get())+Len(t.entdir_s.Get()))));

	// Check if filename valid
	t.entnewloaded=0 ; t.entid=0;
	if ( cstr(Right(t.ent_s.Get(),4)) == ".fpe" ) 
	{
		// Check entity exists in bank
		t.tokay=1;
		if ( g.entidmaster>0 ) 
		{
			for ( t.entid = 1 ; t.entid<=  g.entidmaster; t.entid++ )
			{
				if ( t.entitybank_s[t.entid] == t.ent_s ) {  t.tokay = 0  ; t.tfoundid = t.entid ; break; }
			}
		}
		if ( t.tokay == 1 ) 
		{
			// Find Free entity Index
			t.freeentid=-1;
			if ( g.entidmaster>0 ) 
			{
				for ( t.entid = 1 ; t.entid <= g.entidmaster; t.entid++ )
				{
					if ( t.entityprofileheader[t.entid].desc_s == "" ) {  t.freeentid = t.entid  ; break; }
				}
			}

			//  New entity or Free One
			if ( t.freeentid == -1 ) 
			{
				++g.entidmaster ; entity_validatearraysize ( );
				t.entid=g.entidmaster;
				t.entnewloaded=1;
			}
			else
			{
				t.entid=t.freeentid;
			}

			// Load entity
			t.entitybank_s[t.entid]=t.ent_s;
			t.entpath_s=getpath(t.ent_s.Get());
			entity_load ( );
		}
		else
		{
			// already got, assign ID from existing
			t.entid=t.tfoundid;
		}
	}
}

void entity_addtoselection ( void )
{
	//  Load entity from file requester
	SetDir (  g.currententitydir_s.Get() );
	t.ent_s=browseropen_s(9);
	g.currententitydir_s=GetDir();
	SetDir (  g.rootdir_s.Get() );
	entity_addtoselection_core ( );
}

void entity_adduniqueentity ( bool bAllowDuplicates )
{
	// Ensure 'entitybank\' is not part of entity filename
	t.entdir_s="entitybank\\";
	if (  cstr(Lower(Left(t.addentityfile_s.Get(),11))) == "entitybank\\" ) 
	{
		t.addentityfile_s=Right(t.addentityfile_s.Get(),Len(t.addentityfile_s.Get())-11);
	}
	if (  cstr(Lower(Left(t.addentityfile_s.Get(),8))) == "ebebank\\" ) 
	{
		t.entdir_s = "";
	}

	//  Check if entity already loaded in
	t.talreadyloaded=0;
	if ( bAllowDuplicates == false )
	{
		for ( t.t = 1 ; t.t<=  g.entidmaster; t.t++ )
		{
			if (  t.entitybank_s[t.t] == t.addentityfile_s ) {  t.talreadyloaded = 1  ; t.entid = t.t; }
		}
	}
	if (t.talreadyloaded == 0)
	{
		//  Allocate one more entity item in array
		if (g.entidmaster > g.entitybankmax - 4)
		{
			Dim (t.tempentitybank_s, g.entitybankmax);
			for (t.t = 0; t.t <= g.entitybankmax; t.t++) t.tempentitybank_s[t.t] = t.entitybank_s[t.t];
			++g.entitybankmax;
			UnDim (t.entitybank_s);
			Dim (t.entitybank_s, g.entitybankmax);
			for (t.t = 0; t.t <= g.entitybankmax - 1; t.t++) t.entitybank_s[t.t] = t.tempentitybank_s[t.t];
		}

		//  Add entity to bank
		++g.entidmaster; entity_validatearraysize ();
		t.entitybank_s[g.entidmaster] = t.addentityfile_s;

		// trigger the creation of a 'group' entity if detected
		if (g_iAbortedAsEntityIsGroupFileMode != 3 )
			g_iAbortedAsEntityIsGroupFileMode = 1;

		//  Load extra entity
		t.entid = g.entidmaster;
		t.ent_s = t.entitybank_s[t.entid];
		t.entpath_s = getpath(t.ent_s.Get());
		entity_load ();

		// 090317 - ignore ebebank new structure to avoid empty EBE icons being added to local library left list
		if ( stricmp ( t.addentityfile_s.Get(), "..\\ebebank\\_builder\\New Site.fpe" ) == NULL )
			t.talreadyloaded = 1;
	}
}

void entity_validatearraysize ( void )
{
	//  ensure enough space in entity profile arrays
	if (  g.entidmaster+32>g.entidmastermax ) 
	{
		g.entidmastermax=g.entidmaster+32;
		Dim2(  t.entitybodypart,g.entidmastermax, 100   );
		Dim2(  t.entityappendanim,g.entidmastermax, 100  );
		Dim2(  t.entityanim,g.entidmastermax, g.animmax   );
		Dim2(  t.entityfootfall,g.entidmastermax, g.footfallmax  );
		Dim (  t.entityprofileheader,g.entidmastermax   );
		Dim (  t.entityprofile,g.entidmastermax  );
		Dim2(  t.entitydecal_s,g.entidmastermax, 100  );
		Dim2(  t.entitydecal,g.entidmastermax, 100   );
#ifndef WICKEDENGINE
		//PE: Not used in wicked. REDUCEMEMUSE
		Dim2(  t.entityblood,g.entidmastermax, BLOODMAX  );
#endif
		g.entitybankmax=g.entidmastermax;
		Dim (  t.entitybank_s,g.entidmastermax  );
	}
}


//PE: GenerateD3D9ForMesh - make sure semantic is stored in old D3D9 format.
//PE: Without get fvf offset can fail , and original skin weight is not used but generated , this can give animation problems.
//PE: This is not a problem when using the importer, as it will save everything in the old D3D9 format into the dbo.
void GenerateD3D9ForMesh(sMesh* pMesh, BOOL bNormals, BOOL bTangents, BOOL bBinormals, BOOL bDiffuse, BOOL bBones)
{
	// get FVF details
	sOffsetMap offsetMap;
	GetFVFValueOffsetMap(pMesh->dwFVF, &offsetMap);

	// deactivate bone flag if no bones in source mesh
	if (pMesh->dwBoneCount == 0) bBones = FALSE;

	// valid mesh (no longer using DXMESH)
	if (pMesh->dwFVF > 0)
	{
		// extract vertex size from mesh
		WORD wNumBytesPerVertex = (WORD)pMesh->dwFVFSize;

		// Starting declaration
		int iDeclarationIndex = 0;
		D3D11_INPUT_ELEMENT_DESC pDeclaration[12];

		// check if mesh already has a component (and build declaration)
		BOOL bHasNormals = FALSE;
		BOOL bHasDiffuse = FALSE;
		BOOL bHasTangents = FALSE;
		BOOL bHasBinormals = FALSE;
		BOOL bHasBlendWeights = FALSE;
		BOOL bHasBlendIndices = FALSE;
		BOOL bHasSecondaryUVs = FALSE;
		if (pMesh->dwFVF & GGFVF_XYZ)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "POSITION";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = 0;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
		}
		if (pMesh->dwFVF & GGFVF_NORMAL)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "NORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasNormals = TRUE;
		}
		if (pMesh->dwFVF & GGFVF_TEX1)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasDiffuse = TRUE;
		}
		if (pMesh->dwFVF & GGFVF_DIFFUSE)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "COLOR";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasDiffuse = TRUE;
		}
		if (pMesh->dwFVF & offsetMap.dwTU[1] > 0)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 1;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			bHasSecondaryUVs = TRUE;
		}

		if (!bHasNormals && bNormals)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "NORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 12;
		}
		if (!bHasDiffuse && bDiffuse)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "COLOR";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 4;
		}
		if (!bHasTangents && bTangents)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TANGENT";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 12;
		}
		if (!bHasBinormals && bBinormals)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "BINORMAL";
			pDeclaration[iDeclarationIndex].SemanticIndex = 0;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 12;
		}
		DWORD dwOffsetToWeights = wNumBytesPerVertex;
		if (!bHasBlendWeights && bBones)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 1;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 16;
		}
		DWORD dwOffsetToIndices = wNumBytesPerVertex;
		if (!bHasBlendIndices && bBones)
		{
			pDeclaration[iDeclarationIndex].SemanticName = "TEXCOORD";
			pDeclaration[iDeclarationIndex].SemanticIndex = 2;
			pDeclaration[iDeclarationIndex].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			pDeclaration[iDeclarationIndex].InputSlot = 0;
			pDeclaration[iDeclarationIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			pDeclaration[iDeclarationIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			pDeclaration[iDeclarationIndex].InstanceDataStepRate = 0;
			iDeclarationIndex++;
			wNumBytesPerVertex += 16;
		}

		// copy declaration into old D3D9 format (as DBO relies on this data in the binary!)
		int iDecIndex = 0;
		int iByteOffset = 0;
		for (; iDecIndex < iDeclarationIndex; iDecIndex++)
		{
			int iEntryByteSize = 0;
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "POSITION") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_POSITION;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "NORMAL") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_NORMAL;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "COLOR") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_COLOR;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT2;
				iEntryByteSize = 4;
			}
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "TANGENT") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_TANGENT;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "BINORMAL") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_BINORMAL;
				pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT3;
				iEntryByteSize = 12;
			}
			if (stricmp(pDeclaration[iDecIndex].SemanticName, "TEXCOORD") == NULL)
			{
				pMesh->pVertexDeclaration[iDecIndex].Usage = GGDECLUSAGE_TEXCOORD;
				if (pDeclaration[iDecIndex].Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
				{
					pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT4;
					iEntryByteSize = 16;
				}
				else
				{
					pMesh->pVertexDeclaration[iDecIndex].Type = GGDECLTYPE_FLOAT2;
					iEntryByteSize = 8;
				}
			}
			pMesh->pVertexDeclaration[iDecIndex].Stream = 0;
			pMesh->pVertexDeclaration[iDecIndex].Method = GGDECLMETHOD_DEFAULT;
			pMesh->pVertexDeclaration[iDecIndex].UsageIndex = pDeclaration[iDecIndex].SemanticIndex;
			pMesh->pVertexDeclaration[iDecIndex].Offset = iByteOffset;
			iByteOffset += iEntryByteSize;
		}
		pMesh->pVertexDeclaration[iDecIndex].Stream = 255;
	}
}

#ifdef WICKEDENGINE
void CreateVectorListFromCPPAssembly ( LPSTR pCCPAssemblyStringLine, std::vector<std::string> & vecOfStrs )
{
	char* pAssemblyString = pCCPAssemblyStringLine;
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
			strlwr(pAssemblyReference);
			if (pAssemblyReference[strlen(pAssemblyReference) - 1] == '\n') pAssemblyReference[strlen(pAssemblyReference) - 1] = 0;
			int iBaseCount = 3;
			for (int iBaseIndex = 0; iBaseIndex < iBaseCount; iBaseIndex++)
			{
				LPSTR pBaseName = "";
				if (iBaseIndex == 0) pBaseName = "adult male";
				if (iBaseIndex == 1) pBaseName = "adult female";
				if (iBaseIndex == 2) pBaseName = "zombie male";
				if (iBaseIndex == 3) pBaseName = "zombie female";
				if (strstr(pAssemblyReference, pBaseName) != NULL)
				{
					// found category
					cstr pPartFolder = "";
					if (iBaseIndex == 0) pPartFolder = "charactercreatorplus\\parts\\adult male\\";
					if (iBaseIndex == 1) pPartFolder = "charactercreatorplus\\parts\\adult female\\";
					if (iBaseIndex == 2) pPartFolder = "charactercreatorplus\\parts\\zombie male\\";
					if (iBaseIndex == 3) pPartFolder = "charactercreatorplus\\parts\\zombie female\\";

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
					std::string str = "";
					str = pRemoveTag; str = str + "_color.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_normal.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_surface.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_mask.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_emissive.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_roughness.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_metalness.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_gloss.dds"; vecOfStrs.push_back(str);
					str = pRemoveTag; str = str + "_ao.dds"; vecOfStrs.push_back(str);
				}
			}
			assemblyString_s = NextToken(",");
		}
	}
}
#endif

#ifdef WICKEDENGINE
bool entity_load_thread_prepare(LPSTR pFpeFile)
{
	// preload thread busy, just ignore
	if (object_preload_files_in_progress() || image_preload_files_in_progress())
		return false;

	// vars to do preloading
	cstr sFpeFile = pFpeFile;
	cstr model_s = ""; //Need this from the fpe. t.entityprofile[t.entid].model_s
	int ischaractercreator = 0; //Need this from fpe. t.entityprofile[t.entid].ischaractercreator
	cstr sFile = "", sDboFile = "";
	std::vector<std::string> fpe_file;

	// work out entity bank path
	cstr sEntityBank = "entitybank\\";
	if (cstr(Lower(Left(sFpeFile.Get(), 11))) == "entitybank\\")
	{
		sFpeFile = Right(sFpeFile.Get(), Len(sFpeFile.Get()) - 11);
	}
	if (cstr(Lower(Left(sFpeFile.Get(), 8))) == "ebebank\\")
	{
		sEntityBank = "";
	}

	// already have this loaded into the level
	for (t.t = 1; t.t <= g.entidmaster; t.t++)
	{
		if (t.entitybank_s[t.t] == sFpeFile)
		{
			// fpe already loaded ready to use
			return true; 
		}
	}

	// find the entity file
	cstr epath_s = getpath(sFpeFile.Get());
	sFpeFile = sEntityBank + sFpeFile;
	if (FileExist(sFpeFile.Get()) == 0)
	{
		return(false); //FPE not found.
	}

	if (getVectorFileContent(sFpeFile.Get(), fpe_file, true) == false)
	{
		return(false); //FPE not found.
	}

	// find model file (DBO)
	std::string model = GetLineParameterFromVectorFile("model=", fpe_file, true);
	if (model.length() <= 0) return(false);

	// determine if character creator object
	std::string sIscharacterCreator = GetLineParameterFromVectorFile("charactercreator=", fpe_file, true);
	model_s = model.c_str();
	if (sIscharacterCreator.length() > 0 && atoi(sIscharacterCreator.c_str()) == 1)
		ischaractercreator = 1;

	// check if we can preload this model
	if (ischaractercreator == 0)
		sFile = sEntityBank + epath_s + model_s;
	else
		sFile = model_s;

	// if .X or .FBX file specified, and DBO exists, load DBO as main model file
	int iSrcFormat = 0;
	if (strcmp(Lower(Right(sFile.Get(), 2)), ".x") == 0) iSrcFormat = 1;
	if (strcmp(Lower(Right(sFile.Get(), 4)), ".fbx") == 0) iSrcFormat = 2;
	if (iSrcFormat > 0)
	{
		if (iSrcFormat == 1)
		{
			// X File Format
			sDboFile = Left(sFile.Get(), Len(sFile.Get()) - 2); sDboFile += ".dbo";
			if (sDboFile != "" && FileExist(sDboFile.Get()) == 1) sFile = sDboFile;
		}
		if (iSrcFormat == 2)
		{
			// FBX File Format
			sDboFile = Left(sFile.Get(), Len(sFile.Get()) - 4); sDboFile += ".dbo";
			if (sDboFile != "" && FileExist(sDboFile.Get()) == 1) sFile = sDboFile;
		}
	}
	else
	{
		// if .X or .FBX file NOT specified
		sDboFile = "";
		if (strcmp(Lower(Right(sFile.Get(), 4)), ".dbo") == 0)
		{
			if (FileExist(sFile.Get()) == 0)
			{
				//We can only preload dbo files.
				return(false);
			}
		}
	}

	// get path to original model
	char pModelPath[10248];
	strcpy(pModelPath, "");
	LPSTR pOrigModelFilename = sFile.Get();
	for (int n = strlen(pOrigModelFilename); n > 0; n--)
	{
		if (pOrigModelFilename[n] == '\\' || pOrigModelFilename[n] == '/')
		{
			strcpy(pModelPath, pOrigModelFilename);
			pModelPath[n + 1] = 0;
			break;
		}
	}
	if (FileExist(sFile.Get()) == 0)
	{
		sFile = model_s;
		if (strcmp(Lower(Right(sFile.Get(), 2)), ".x") == 0) { sDboFile = Left(sFile.Get(), Len(sFile.Get()) - 2); sDboFile += ".dbo"; }
		else sDboFile = "";
		if (sDboFile != "" && FileExist(sDboFile.Get()) == 1)  sFile = sDboFile;
	}

	// if DBO model file exists
	if (FileExist(sFile.Get()) == 1)
	{
		if (FileExist(sDboFile.Get()) == 1)
		{
			sFile = sDboFile;
		}
		if (strcmp(Lower(Right(sFile.Get(), 4)), ".dbo") == 0)
		{
			// DBO we can thread load it
			//PE: Disabled for now until thread safe.
			//object_preload_files_start();
			//object_preload_files_add(sFile.Get());
			//object_preload_files_finish();

			// also scan FPE for all references to textures and preload those
			image_preload_files_start();
			for (int iTexLineScan = 0; iTexLineScan < 100; iTexLineScan++)
			{
				// texture ref field name
				char pTexRefFieldNam[32];
				if ( iTexLineScan > 0 ) 
					sprintf (pTexRefFieldNam, "textureref%d=", iTexLineScan);
				else
					strcpy (pTexRefFieldNam, "textured=");

				// get texture ref data from FPE file
				std::string texturefileref = "";
				texturefileref = GetLineParameterFromVectorFile(pTexRefFieldNam, fpe_file, true);

				// if texture ref valid
				if (texturefileref.length() > 0)
				{
					// certainly preload the one specified
					LPSTR pTextureRef = (LPSTR)texturefileref.c_str();

					// add to image preload
					char pPreloadThisFile[MAX_PATH];
					strcpy (pPreloadThisFile, pModelPath);
					strcat (pPreloadThisFile, pTextureRef);
					image_preload_files_add(pPreloadThisFile);

					// clean tex ref and remove extension if any
					char pCleanTexRef[MAX_PATH];
					strcpy(pCleanTexRef, pTextureRef);
					strlwr(pCleanTexRef);
					LPSTR pExt = NULL;
					pExt = strstr(pCleanTexRef, ".dds"); if (pExt) *pExt = 0;
					pExt = strstr(pCleanTexRef, ".png"); if (pExt) *pExt = 0;
					pExt = strstr(pCleanTexRef, ".tga"); if (pExt) *pExt = 0;
					pExt = strstr(pCleanTexRef, ".jpg"); if (pExt) *pExt = 0;
					pExt = strstr(pCleanTexRef, ".bmp"); if (pExt) *pExt = 0;

					// preload all its PBR set variants
					if ( strstr (pCleanTexRef, "_color") != NULL)
					{
						char pBaseTexRef[MAX_PATH];
						strcpy (pBaseTexRef, pCleanTexRef);
						pBaseTexRef[strlen(pBaseTexRef) - 6] = 0;
						for (int iPBRSet = 1; iPBRSet < 5; iPBRSet++)
						{
							char pThisPBRTex[MAX_PATH];
							strcpy (pThisPBRTex, pBaseTexRef);
							if (iPBRSet == 1) strcat (pThisPBRTex, "_normal.dds");
							if (iPBRSet == 2) strcat (pThisPBRTex, "_surface.dds");
							if (iPBRSet == 3) strcat (pThisPBRTex, "_emissive.dds");
							if (iPBRSet == 4) strcat (pThisPBRTex, "_displacement.dds");
							strcpy (pPreloadThisFile, pModelPath);
							strcat (pPreloadThisFile, pThisPBRTex);
							image_preload_files_add(pPreloadThisFile);
						}
					}
				}
				// also account for character creator assemblies
				std::vector<std::string> ccpAsemblyList;
				std::string ccpAssemblyRef = GetLineParameterFromVectorFile("ccpassembly=", fpe_file, true);
				CreateVectorListFromCPPAssembly((LPSTR)ccpAssemblyRef.c_str(), ccpAsemblyList);
				for ( int i = 0; i < ccpAsemblyList.size(); i++ ) image_preload_files_add((LPSTR)ccpAsemblyList[i].c_str());
				ccpAsemblyList.clear();
			}
			image_preload_files_finish();

			// preload is a go!
			return(true);
		}
	}

	// otherwise not preloading today
	return(false);
}
#endif

bool entity_load (bool bCalledFromLibrary)
{
	// allowed by default on successful load of the model
	bool bSavingDBOAllowed = true;

	//  Activate auto generation of mipmaps for ALL entities
	SetImageAutoMipMap (  1 );

	//  Entity Object Index
	t.entobj=g.entitybankoffset+t.entid;

	//  debug info
	t.mytimer=Timer();

	//  Load Entity profile data
	entity_loaddata ();

	// special group detection
	bool bWeAreAGroup = false;
	#ifdef WICKEDENGINE
	if (g_iAbortedAsEntityIsGroupFileMode == 3)
	{
		// these entities are loaded by the group!
		t.entityprofile[t.entid].ischildofgroup = 1;
	}
	cstr LastGroupFilename_s = "";
	if (g_iAbortedAsEntityIsGroupFileMode == 2)
	{
		// okay we are a group!
		int iStoreObj = t.entobj;
		int iStoreEntID = t.entid;
		extern bool LoadGroup(LPSTR pAbsFilename);
		cstr pGroupFilename_s = t.tFPEName_s;
		LastGroupFilename_s = pGroupFilename_s;
		if (bCalledFromLibrary)
		{
			//PE: check if we already got a thumb.
			//PE: Check if a got the original image for this smart object.
			bool CreateBackBufferCacheName(char *file, int width, int height);
			extern cstr BackBufferCacheName;
			CreateBackBufferCacheName(t.addentityfile_s.Get(), 512, 288);
			if (FileExist(BackBufferCacheName.Get()))
			{
				LoadImage((char *)BackBufferCacheName.Get(), ENTITY_CACHE_ICONS_LARGE + t.entid);
				if (ImageExist(ENTITY_CACHE_ICONS_LARGE + t.entid))
				{
					t.entityprofile[t.entid].iThumbnailLarge = ENTITY_CACHE_ICONS_LARGE + t.entid;
					g_iAbortedAsEntityIsGroupFileMode = 0;
					//PE: No need to read the group and load the files we have the thumb.
					return(false);
				}
			}
		}
		g_iAbortedAsEntityIsGroupFileMode = 3;
		LoadGroup(pGroupFilename_s.Get());
		g_iAbortedAsEntityIsGroupFileMode = 0;
		t.entobj = iStoreObj;
		t.entid = iStoreEntID;
		bWeAreAGroup = true;
		// after group created, need to clear reminants from cursor
		g.thumbentityrubberbandlist = g.entityrubberbandlist;
		g.entityrubberbandlist.clear();

		//PE: Setting current_selected_group = -1 , break the groups, and the t.entityprofile[iGroupEntID].groupreference never get set.
		//PE: @Lee moved this to below after groupreference is set :)
//		extern int current_selected_group; // fixes cellar issue sticky candle and switch escape crash
//		current_selected_group = -1;

		//Make sure any selection are removed
		t.gridentity = 0;
		t.inputsys.constructselection = 0;
		t.inputsys.domodeentity = 1;
		t.grideditselect = 5;
		editor_refresheditmarkers();
	}
	#endif

	//  Only load characters for entity-local-testing
	t.desc_s=t.entityprofileheader[t.entid].desc_s;
	if (  t.scanforentitiescharactersonly == 1 ) 
	{
		if (  t.entityprofile[t.entid].ischaracter == 0 ) 
		{
			t.desc_s="";
		}
	}

	//  Special mode (from lightmapper) which only loads static entities
	//  which will speed up lightmapping process and reduce hit on system memory
	//  not excluding markers as we need some of them for lighting info
	if (  t.lightmapper.onlyloadstaticentitiesduringlightmapper == 1 ) 
	{
		if (  t.entityprofile[t.entid].ischaracter == 1 ) 
		{
			t.desc_s="";
		}
	}

	// Only if profile data exists
	if (t.desc_s != "")
	{
		//Load the thumbnail.
#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
		t.strwork = t.entdir_s + t.ent_s;
		t.tthumbbmpfile_s = "";	t.tthumbbmpfile_s = t.tthumbbmpfile_s + Left(t.strwork.Get(), (Len(t.entdir_s.Get()) + Len(t.ent_s.Get())) - 4) + ".bmp";

		image_setlegacyimageloading(true);
		LoadImage(t.tthumbbmpfile_s.Get(), ENTITY_CACHE_ICONS + t.entid);
		image_setlegacyimageloading(false);
		t.entityprofile[t.entid].iThumbnailSmall = ENTITY_CACHE_ICONS + t.entid;
		if (!ImageExist(t.entityprofile[t.entid].iThumbnailSmall))
			t.entityprofile[t.entid].iThumbnailSmall = TOOL_ENTITY;
		//iThumbnailLarge = 0;
#ifdef WICKEDENGINE
		t.strwork = t.entdir_s + t.ent_s;
		t.entityprofile[t.entid].iThumbnailLarge = 0;
		bool CreateBackBufferCacheName(char *file, int width, int height);
		extern cstr BackBufferCacheName;

		SetMipmapNum(1); //PE: mipmaps not needed.
		image_setlegacyimageloading(true);

		bool bWeGotaThumb = false;
		if (bWeAreAGroup)
		{
			//PE: Check if a got the original image for this smart object.
			CreateBackBufferCacheName(LastGroupFilename_s.Get(), 512, 288);
			if (FileExist(BackBufferCacheName.Get()))
			{
				LoadImage((char *)BackBufferCacheName.Get(), ENTITY_CACHE_ICONS_LARGE + t.entid);
				if (ImageExist(ENTITY_CACHE_ICONS_LARGE + t.entid))
				{
					t.entityprofile[t.entid].iThumbnailLarge = ENTITY_CACHE_ICONS_LARGE + t.entid;
					bWeGotaThumb = true;
				}
			}
		}

		if (!bWeGotaThumb)
		{
			CreateBackBufferCacheName(t.strwork.Get(), 512, 288);
			if (FileExist(BackBufferCacheName.Get()))
			{
				LoadImage((char *)BackBufferCacheName.Get(), ENTITY_CACHE_ICONS_LARGE + t.entid);
				if (ImageExist(ENTITY_CACHE_ICONS_LARGE + t.entid))
					t.entityprofile[t.entid].iThumbnailLarge = ENTITY_CACHE_ICONS_LARGE + t.entid;
			}
		}

		image_setlegacyimageloading(false);
		SetMipmapNum(-1);
#endif
#endif

		// if a group, make a modified entity that references a group
#ifdef WICKEDENGINE
		if (bWeAreAGroup == true)
		{
			// the group entity
			int iGroupEntID = t.entid;
			int iGroupEntObj = t.entobj;

			// if a 'group' entity, set reference to group
			extern int current_selected_group;
			t.entityprofile[iGroupEntID].model_s = "group";

			// record this group into entity parent profile
			t.entityprofile[iGroupEntID].groupreference = current_selected_group;

			// test object!
			g_iWickedEntityId = iGroupEntID;
			MakeObjectCube(iGroupEntObj, 0.0f);
			g_iWickedEntityId = -1;

			// this is ther group object entity ID!
			t.entid = iGroupEntID;
			t.entobj = iGroupEntObj;

			//PE: Moved here so we get the groupreference.
			//PE: current_selected_group is used by thumb system , and could crash without it.
			extern int thumb_selected_group;
			thumb_selected_group = current_selected_group;
			current_selected_group = -1; // fixes cellar issue sticky candle and switch escape crash
			// and go no further
			return false;
		}
#endif

		//  Load the model
		if (t.entityprofile[t.entid].ischaractercreator == 0)
		{
			t.tfile_s = t.entdir_s + t.entpath_s + t.entityprofile[t.entid].model_s;
		}
		else
		{
			t.tfile_s = t.entityprofile[t.entid].model_s;
		}
		deleteOutOfDateDBO(t.tfile_s.Get());
		// if .X or .FBX file specified, and DBO exists, load DBO as main model file
		int iSrcFormat = 0;
		if (strcmp(Lower(Right(t.tfile_s.Get(), 2)), ".x") == 0) iSrcFormat = 1;
		if (strcmp(Lower(Right(t.tfile_s.Get(), 4)), ".fbx") == 0) iSrcFormat = 2;
		if (iSrcFormat > 0)
		{
			if (iSrcFormat == 1)
			{
				// X File Format
				t.tdbofile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 2); t.tdbofile_s += ".dbo";
				if (t.tdbofile_s != "" && FileExist(t.tdbofile_s.Get()) == 1) t.tfile_s = t.tdbofile_s;
			}
			if (iSrcFormat == 2)
			{
				// FBX File Format
				t.tdbofile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 4); t.tdbofile_s += ".dbo";
				if (t.tdbofile_s != "" && FileExist(t.tdbofile_s.Get()) == 1) t.tfile_s = t.tdbofile_s;
			}
		}
		else
		{
			// if .X or .FBX file NOT specified
			t.tdbofile_s = "";
			if (strcmp(Lower(Right(t.tfile_s.Get(), 4)), ".dbo") == 0)
			{
				// and .DBO specified instead, check if .DBO does not exist
				if (FileExist(t.tfile_s.Get()) == 0)
				{
					// if not exist try .X model file (typical of model import entities that use original X file)
					// but do not copy the DBO file with it
					t.tfile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 4); t.tfile_s += ".x";
				}
			}
		}
		// get path to original model
		char pModelPath[10248];
		strcpy(pModelPath, "");
		LPSTR pOrigModelFilename = t.tfile_s.Get();
		for (int n = strlen(pOrigModelFilename); n > 0; n--)
		{
			if (pOrigModelFilename[n] == '\\' || pOrigModelFilename[n] == '/')
			{
				strcpy(pModelPath, pOrigModelFilename);
				pModelPath[n + 1] = 0;
				break;
			}
		}
		// 070718 - if append final file exists, use that
		bool bUsingAppendAnimFileModel = false;
		cstr pAppendFinalModelFilename = t.entityappendanim[t.entid][0].filename;
		if (strlen(pAppendFinalModelFilename.Get()) > 0)
		{
			pAppendFinalModelFilename = cstr(pModelPath) + pAppendFinalModelFilename;
			if (FileExist(pAppendFinalModelFilename.Get()) == 1)
			{
				bUsingAppendAnimFileModel = true;
				t.tfile_s = pAppendFinalModelFilename;
				pAppendFinalModelFilename = "";
				t.tdbofile_s = "";
			}
		}
		if (FileExist(t.tfile_s.Get()) == 0)
		{
			t.tfile_s = t.entityprofile[t.entid].model_s;
			//  V109 BETA6 - 290408 - allow DBO creation/read if full relative path provides (i.e. meshbank\scifi\etc)
			if (strcmp(Lower(Right(t.tfile_s.Get(), 2)), ".x") == 0) { t.tdbofile_s = Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 2); t.tdbofile_s += ".dbo"; }
			else t.tdbofile_s = "";
			if (t.tdbofile_s != "" && FileExist(t.tdbofile_s.Get()) == 1)  t.tfile_s = t.tdbofile_s;
		}
		t.txfile_s = t.tfile_s;
		if (FileExist(t.tfile_s.Get()) == 1)
		{
			//  if DBO version exists, use that (quicker)
			if (FileExist(t.tdbofile_s.Get()) == 1)
			{
				t.tfile_s = t.tdbofile_s;
				t.tdbofile_s = "";
			}
			else
			{
				//  allowed to save DBO (once only)
			}

#ifdef WICKEDENGINE
			bool bNewDecal = false;
#endif
			//  Load entity (compile does not need the dynamic objects)
			if (t.entobj > 0)
			{
				if (ObjectExist(t.entobj) == 0)
				{
					// load entity model
#ifdef WICKEDENGINE
// All handled inside LoadObject for WickedEngine
#else
					if (t.entityprofile[t.entid].fullbounds == 1) SetFastBoundsCalculation(0);
					if (strnicmp(t.tfile_s.Get() + strlen(t.tfile_s.Get()) - 4, ".fbx", 4) == NULL)
					{
						LoadFBX(t.tfile_s.Get(), t.entobj);
					}
					else
#endif

#ifdef WICKEDENGINE
						//if (t.entityprofile[t.entid].WEMaterial.MaterialActive) {
						if (1) //PE: Now always set g_iWickedEntityId so we can detect if we need alpharef on trees...
						{
							g_iWickedEntityId = t.entid;
							char debug[ 1024 ];
							sprintf(debug, "LoadObject( %s )", t.tfile_s.Get());
							timestampactivity(0, debug);
							
							char* tfile_s = t.tfile_s.Get();
							if ( *tfile_s ) LoadObject(t.tfile_s.Get(), t.entobj);
							else MakeObjectBox( t.entobj, 1,1,1 );
							g_iWickedEntityId = -1;
						}
						else
#endif

							LoadObject(t.tfile_s.Get(), t.entobj);
					#ifdef WICKEDENGINE
					if (ObjectExist(t.entobj) == 0)
					{
						// soft error allows failed object loads to continue (message provided in LoadDBO)
						MakeObjectCube(t.entobj, 25);

						// and prevent this temp shape saving as permanent DBO!
						bSavingDBOAllowed = false;
					}
					#endif

					#ifdef WICKEDENGINE
					//LB: default position is not a decal (fixes issue of objects being misaligned in object library auto gen)
					t.entityprofile[t.entid].bIsDecal = false;
					//PE: Old decal support.
					cstr sEffectLower = Lower(t.entityprofile[t.entid].effect_s.Get());
					if (sEffectLower == "effectbank\\reloaded\\decal_animate4.fx")
					{
						t.entityprofile[t.entid].bIsDecal = true;
						t.entityprofile[t.entid].iDecalRows = 4;
						t.entityprofile[t.entid].iDecalColumns = 4;
						t.entityprofile[t.entid].fDecalSpeed = 1.0; // ? later
						bNewDecal = true;
					}
					if (sEffectLower == "effectbank\\reloaded\\decal_animate8.fx")
					{
						t.entityprofile[t.entid].bIsDecal = true;
						t.entityprofile[t.entid].iDecalRows = 8;
						t.entityprofile[t.entid].iDecalColumns = 8;
						t.entityprofile[t.entid].fDecalSpeed = 1.0; // ? later
						bNewDecal = true;
					}
					if (sEffectLower == "effectbank\\reloaded\\decal_animate10.fx" || sEffectLower == "effectbank\\reloaded\\decal_animate10x10.fx")
					{
						t.entityprofile[t.entid].bIsDecal = true;
						t.entityprofile[t.entid].iDecalRows = 10;
						t.entityprofile[t.entid].iDecalColumns = 10;
						t.entityprofile[t.entid].fDecalSpeed = 1.0; // ? later
						bNewDecal = true;
					}
					if (sEffectLower == "effectbank\\reloaded\\decal_animate10x12.fx")
					{
						t.entityprofile[t.entid].bIsDecal = true;
						t.entityprofile[t.entid].iDecalRows = 10;
						t.entityprofile[t.entid].iDecalColumns = 12;
						t.entityprofile[t.entid].fDecalSpeed = 1.0; // ? later
						bNewDecal = true;
					}
					if (bNewDecal)
					{
						float x = ObjectSizeX(t.entobj);
						float y = ObjectSizeY(t.entobj);
						float z = ObjectSizeZ(t.entobj);
						if (ObjectExist(t.entobj)) DeleteObject(t.entobj);
						MakeObjectPlane(t.entobj, x, y);
						PositionObject(t.entobj, 0, y, 0);
						FixObjectPivot(t.entobj);
						SetAlphaMappingOn(t.entobj, 100.0);
						SetObjectTransparency(t.entobj, 6);

						//DisableObjectZWrite(t.tobj);

						LockVertexDataForLimbCore(t.entobj, 0, 1);
						SetVertexDataNormals(0, 0, 1, 0);
						SetVertexDataNormals(1, 0, 1, 0);
						SetVertexDataNormals(2, 0, 1, 0);
						SetVertexDataNormals(3, 0, 1, 0);
						SetVertexDataNormals(4, 0, 1, 0);
						SetVertexDataNormals(5, 0, 1, 0);
						UnlockVertexData();

						SetObjectUVManually(t.entobj, 0, t.entityprofile[t.entid].iDecalRows , t.entityprofile[t.entid].iDecalColumns );

						SetObjectLight(t.entobj, 0);

						void SetupDecalObject(int obj,int elementID);
						SetupDecalObject(t.entobj,0);

					}
					#endif

					// 060718 - append animation data from other DBO files
					if (bUsingAppendAnimFileModel == false)
					{
						if (Len(t.tdbofile_s.Get()) == 0)
						{
							if (t.entityprofile[t.entid].appendanimmax > 0)
							{
								for (int aa = 1; aa <= t.entityprofile[t.entid].appendanimmax; aa++)
								{
									cstr pAppendModelFilename = cstr(pModelPath) + t.entityappendanim[t.entid][aa].filename;
									int iStartFrame = t.entityappendanim[t.entid][aa].startframe;
									AppendObject(pAppendModelFilename.Get(), t.entobj, iStartFrame);
								}
							}
						}
					}

#ifdef WICKEDENGINE
					//PE: Only if not already set by a wicked material.
					if (!t.entityprofile[t.entid].WEMaterial.MaterialActive && t.entityprofile[t.entid].WEMaterial.dwBaseColor[0] == -1)
						SetObjectDiffuse(t.entobj, Rgb(255, 255, 255));
#endif

					// wipe ANY material emission colors
					SetObjectEmissive(t.entobj, 0);

					// prepare properties
					SetFastBoundsCalculation(1);
					SetObjectFilter(t.entobj, 2);
					SetObjectCollisionOff(t.entobj);

					//  if strictly NON-multimaterial, convert now
					if (t.entityprofile[t.entid].onetexture == 1)
					{
						if (GetMeshExist(g.meshgeneralwork) == 1)  DeleteMesh(g.meshgeneralwork);
						MakeMeshFromObject(g.meshgeneralwork, t.entobj);
						DeleteObject(t.entobj);
						MakeObject(t.entobj, g.meshgeneralwork, 0);
					}

					// 011215 - if specified, we can smooth the model before we use it (concrete pipe in TBE level)
					float fSmoothingAngleOrFullGenerate = t.entityprofile[t.entid].smoothangle;
#ifdef VRTECH
					if (fSmoothingAngleOrFullGenerate > 0 && fSmoothingAngleOrFullGenerate <= 200)
#else
					if (fSmoothingAngleOrFullGenerate > 0)
#endif
					{
						// 090217 - this only works on orig X files (not subsequent DBO) as they change 
						// the mesh which is then saved out (below)
						if (Len(t.tdbofile_s.Get()) > 1)
						{
							// 090216 - a special mode of over 101 will flip normals for the object (when normals are bad)
							if (fSmoothingAngleOrFullGenerate >= 101.0f)
							{
								SetObjectNormalsEx(t.entobj, 0); // will generate new smooth normals for object
								fSmoothingAngleOrFullGenerate -= 101.0f;
							}

							// and if smoothing factor required, weld some of them together
							if (fSmoothingAngleOrFullGenerate > 0.0f)
							{
								SetObjectSmoothing(t.entobj, fSmoothingAngleOrFullGenerate);
							}
						}
#ifdef VRTECH
						else
						{
							// some support for direct DBO model smoothing 
							if (fSmoothingAngleOrFullGenerate > 0.0f)
							{
								SetObjectSmoothing(t.entobj, fSmoothingAngleOrFullGenerate);
							}
						}
#endif
					}
#ifdef VRTECH
					else
					{
						// smooth any model, not just X files going to DBO files
						if (fSmoothingAngleOrFullGenerate >= 201)
						{
							SetObjectNormalsEx(t.entobj, 0);
							fSmoothingAngleOrFullGenerate -= 201.0f;
							SetObjectSmoothing(t.entobj, fSmoothingAngleOrFullGenerate);
						}
					}
#endif
				}
			}

			//  loaded okay
			if (ObjectExist(t.entobj) == 1)
			{
				// 070718 - if append final model needs to be created, prefer that
				if (strlen(pAppendFinalModelFilename.Get()) > 0)
					if (FileExist(pAppendFinalModelFilename.Get()) == 0)
						t.tdbofile_s = pAppendFinalModelFilename;

				// Save if DBO not exist for entity (for fast loading)
				if (Len(t.tdbofile_s.Get()) > 1 && bSavingDBOAllowed == true)
				{
					// ensure legacy compatibility (avoids new mapedito crashing build process)
#ifdef WICKEDENGINE
// in wicked, only save if not exist, otherwise existing DBO is not to be touched!
					if (FileExist(t.tdbofile_s.Get()) == 0) SaveObject(t.tdbofile_s.Get(), t.entobj);
#else
					if (FileExist(t.tdbofile_s.Get()) == 1)  DeleteFileA(t.tdbofile_s.Get());
					SaveObject(t.tdbofile_s.Get(), t.entobj);
#endif
					if (FileExist(t.tdbofile_s.Get()) == 1)
					{
						DeleteObject(t.entobj);
						LoadObject(t.tdbofile_s.Get(), t.entobj);
						SetObjectFilter(t.entobj, 2);
						SetObjectCollisionOff(t.entobj);
					}
				}

				// 300817 - if an EBE object with no .EBE file, remove handle from entity
				if (t.entityprofile[t.entid].isebe == 2)
					if (ObjectExist(t.entobj) == 1)
						if (LimbExist(t.entobj, 0) == 1)
							ChangeMesh(t.entobj, 0, 0);

				// Special matrix transform mode for FBX and similar models
				SetObjectRenderMatrixMode(t.entobj, t.entityprofile[t.entid].matrixmode);

#ifdef WICKEDENGINE
				// Do not force native DBO format to olf FVF - we need data preserved for Wicked Loading
#else
				//  XYZ=0x002 and NORMAL=0x010 and 1UV=0x100
				if (t.entityprofile[t.entid].skipfvfconvert == 0)
				{
					// lee - 300714 - seems to screw up Zombie models somehow, does it screw up rest of engine commenting it out?
					// PE: zombie problems could be the missing skin weight like below, did not test this.
					// PE: perhaps we can streamline this now , so skipfvfconvert is not needed :)

					CloneMeshToNewFormat(t.entobj, 0x002 + 0x010 + 0x100);
				}
				else {
					//PE: make sure we use the correct FVF. even when using skipfvfconvert=1
					DWORD dwRequiredFVF = 0x002 + 0x010 + 0x100;
					sObject* pObject = g_ObjectList[t.entobj];
					for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
					{
						sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
						if (pMesh->dwFVF != dwRequiredFVF)
						{
							ConvertToFVF(pMesh, dwRequiredFVF);
						}
					}
				}
#endif

				//PE: We are missing skin weight/others in old DX9 DBO setup. needed for some functions.
				//PE: prevent generation of vertex weight that screw up some animations.
				sObject* pObject = g_ObjectList[t.entobj];
				for (int iMesh = 0; iMesh < pObject->iMeshCount; iMesh++)
				{
					sMesh* pMesh = pObject->ppMeshList[iMesh];
					GenerateD3D9ForMesh(pMesh, true, true, true, true, true);
				}

				// 131115 - fixes issue of some models not being able to detect with intersectall
				SetObjectDefAnim(t.entobj, t.entityprofile[t.entid].ignoredefanim);

				//  the reverse can be used to allow transparent limbs to be rendered last
				if (t.entityprofile[t.entid].reverseframes == 1)
				{
					ReverseObjectFrames(t.entobj);
				}

				//  main profile object adjustments
				#ifdef WICKEDENGINE
				RemoveFixedScale(t.entobj);
				#endif

				if (t.entityprofile[t.entid].scale != 0)
				{
					ScaleObject(t.entobj, t.entityprofile[t.entid].scale, t.entityprofile[t.entid].scale, t.entityprofile[t.entid].scale);
					#ifdef WICKEDENGINE
					//PE: Particle scale bug fix.
					if (t.entityprofile[t.entid].ismarker == 10)
					{
						//PE: Particle use fixed scale.
						SetFixedScale(t.entobj, t.entityprofile[t.entid].scale, t.entityprofile[t.entid].scale, t.entityprofile[t.entid].scale);
					}
					#endif
				}


				//  Apply texture and effect to entity profile obj
				entity_loadtexturesandeffect();

				//  until static bonemodel scales when not animating, loop
				if (t.entityprofile[t.entid].ischaracter == 1)
				{
					//  refresh using loop stop to get correct character pose
					LoopObject(t.entobj); StopObject(t.entobj);
					//  pivot character to face right way
					RotateObject(t.entobj, 0, 180, 0); FixObjectPivot(t.entobj);
				}

#ifdef WICKEDENGINE
				//if (!bNewDecal)
				{
#endif
					//  if entity uses a handle, create and attach it now
					t.entityprofile[t.entid].addhandlelimb = 0;
					if (Len(t.entityprofile[t.entid].addhandle_s.Get()) > 1)
					{
						t.thandlefile_s = t.entdir_s + t.entpath_s + "\\" + t.entityprofile[t.entid].addhandle_s;
						if (FileExist(t.thandlefile_s.Get()) == 1)
						{
							if (ObjectExist(g.entityworkobjectoffset) == 1)  DeleteObject(g.entityworkobjectoffset);
#ifdef WICKEDENGINE
							WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_CURSOROBJECT);
#endif
							MakeObjectPlane(g.entityworkobjectoffset, 25, 25);
#ifdef WICKEDENGINE
							WickedCall_PresetObjectRenderLayer(GGRENDERLAYERS_NORMAL);
#endif
							RotateObject(g.entityworkobjectoffset, 90, 0, 0);
							ScaleObject(g.entityworkobjectoffset, 50, 50, 50);

							if (GetMeshExist(g.entityworkobjectoffset) == 1)  DeleteMesh(g.entityworkobjectoffset);
							MakeMeshFromObject(g.entityworkobjectoffset, g.entityworkobjectoffset);

							#ifdef WICKEDENGINE
							if (bNewDecal)
							{
								//PE: OffsetLimb dont work in wicked , so... (bIsDecal)
								t.tyoffset_f = (t.entityprofile[t.entid].defaultheight / ((t.entityprofile[t.entid].scale + 0.0) / 100.0))*-1;
								LockVertexDataForMesh(g.entityworkobjectoffset);
								for (int i = 0; i < GetVertexDataVertexCount(); i++)
								{
									float vertX = GetVertexDataPositionX(i);
									float vertY = GetVertexDataPositionY(i);
									float vertZ = GetVertexDataPositionZ(i);
									SetVertexDataPosition(i, vertX, vertY + (2 + t.tyoffset_f), vertZ);
								}

								float rc = 4.0;
								SetVertexDataUV(0, 0, 0);
								SetVertexDataUV(1, 1.0 / rc, 0);
								SetVertexDataUV(2, 1.0 / rc, 1.0 / rc);
								SetVertexDataUV(3, 1.0 / rc, 1.0 / rc);
								SetVertexDataUV(4, 0, 1.0 / rc);
								SetVertexDataUV(5, 0, 0);

								UnlockVertexData();
							}
							#endif

							PerformCheckListForLimbs(g.entityworkobjectoffset);
							#ifdef WICKEDENGINE
							if (bNewDecal)
								t.entityprofile[t.entid].addhandlelimb = 1;
							else
							#endif
								t.entityprofile[t.entid].addhandlelimb = 1 + ChecklistQuantity();

							AddLimb(t.entobj, t.entityprofile[t.entid].addhandlelimb, g.entityworkobjectoffset);
							if (ConfirmObjectAndLimb(t.entobj, t.entityprofile[t.entid].addhandlelimb))
							{
								LinkLimb(t.entobj, 0, t.entityprofile[t.entid].addhandlelimb);
								t.tyoffset_f = (t.entityprofile[t.entid].defaultheight / ((t.entityprofile[t.entid].scale + 0.0) / 100.0))*-1;
								#ifdef WICKEDENGINE
								//PE: Offset already done.
								if (!bNewDecal)
								#endif
									OffsetLimb(t.entobj, t.entityprofile[t.entid].addhandlelimb, 0, 2 + t.tyoffset_f, 0);
								t.texhandleid = loadinternaltexture(t.thandlefile_s.Get());
								TextureLimb(t.entobj, t.entityprofile[t.entid].addhandlelimb, t.texhandleid);
								SetLimbEffect(t.entobj, t.entityprofile[t.entid].addhandlelimb, t.entityprofile[t.entid].usingeffect);
								if (ObjectExist(g.entityworkobjectoffset) == 1)  DeleteObject(g.entityworkobjectoffset);
								if (GetMeshExist(g.entityworkobjectoffset) == 1)  DeleteMesh(g.entityworkobjectoffset);
								if (t.game.gameisexe == 1)
									HideLimb(t.entobj, t.entityprofile[t.entid].addhandlelimb);
							}
						}
					}
				#ifdef WICKEDENGINE
				}
				#endif

				//  Parent LOD is not enabled, we use clone and instance
				//  settings as we need per-entity element distance control

				if (t.entityprofile[t.entid].isebe == 0)
				{
					// only set material if not EBE, as EBE objects carry per-mesh material values
					SetObjectArbitaryValue(t.entobj, t.entityprofile[t.entid].materialindex);
				}

				//  if entity has decals, find indexes to decals (which are already preloaded)
				t.entityprofile[t.entid].bloodscorch = 0;
				if (t.entityprofile[t.entid].decalmax > 0)
				{
					for (t.tq = 0; t.tq <= t.entityprofile[t.entid].decalmax - 1; t.tq++)
					{
						t.decal_s = t.entitydecal_s[t.entid][t.tq];
						if (strcmp(Lower(t.decal_s.Get()), "blood") == 0)  t.entityprofile[t.entid].bloodscorch = 1;
						decal_find();
						if (t.decalid < 0)  t.decalid = 0;
						t.entitydecal[t.entid][t.tq] = t.decalid;
					}
				}

				#ifdef VRTECH
				#else
				//  add in character creator objects if needed
				if (t.entityprofile[t.entid].ischaractercreator == 1)
				{
					characterkit_loadEntityProfileObjects();
				}
				#endif

				//  HideObject (  away )
				PositionObject(t.entobj, 100000, 100000, 100000);

				//  Set radius of zero allows parent to animate even if outside of frustrum view
				if (GetNumberOfFrames(t.entobj) > 0)
				{
					//  but ONLY for animating objects, do not need to run parent objects if still
					SetSphereRadius(t.entobj, 0);
				}

			}
		}
		else
		{
			//  prevent crash when model name wrong/geometry file missing/etc
			MakeObjectSphere(t.entobj, 1);
			PositionObject(t.entobj, 100000, 100000, 100000);
		}

		/* bitbob system removed and variables used elsewhere
		//  work out if this is in the bitbob system (1 or -1 automatic (decided based on size) and precalcualte bitbob fade distances
		if (t.entityprofile[t.entid].ischaracter == 1 && t.entityprofile[t.entid].bitbobon == -1)
		{
			//  characters need to remain visible for sniping
			t.entityprofile[t.entid].bitbobon = 0;
		}
		if (t.entityprofile[t.entid].bitbobon != 0 && t.entityprofile[t.entid].ismarker == 0)
		{
			t.tLargestSide_f = ObjectSizeX(t.entobj, 1);
			if (ObjectSizeY(t.entobj, 1) > t.tLargestSide_f)  t.tLargestSide_f = ObjectSizeY(t.entobj, 1);
			if (ObjectSizeZ(t.entobj, 1) > t.tLargestSide_f)  t.tLargestSide_f = ObjectSizeZ(t.entobj, 1);
			if (t.tLargestSide_f < BITBOBS_DEFAULT_MAXSIZE)  t.entityprofile[t.entid].bitbobon = 1;
			if (t.entityprofile[t.entid].bitbobon == 1)
			{
				t.entityprofile[t.entid].bitbobnear_f = t.tLargestSide_f * BITBOBS_DEFAULT_FADEIN * t.entityprofile[t.entid].bitbobdistweight_f;
				t.entityprofile[t.entid].bitbobfar_f = t.tLargestSide_f * BITBOBS_DEFAULT_FADEOUT * t.entityprofile[t.entid].bitbobdistweight_f;
			}
		}
		*/

		//  must hide parent objects
		HideObject(t.entobj);

		//  Resolve default weapon gun ids
		if (t.entityprofile[t.entid].isweapon_s != "")
		{
			t.findgun_s = Lower(t.entityprofile[t.entid].isweapon_s.Get()); gun_findweaponindexbyname();
			t.entityprofile[t.entid].isweapon = t.foundgunid;
			if (t.foundgunid > 0)  t.gun[t.foundgunid].activeingame = 1;
		}
		else
		{
			t.entityprofile[t.entid].isweapon = 0;
		}

		//  Finding hasweapon also in createlemenents (as eleprof may have changed the weapon!)
		if (t.entityprofile[t.entid].hasweapon_s != "")
		{
			t.findgun_s = Lower(t.entityprofile[t.entid].hasweapon_s.Get()); gun_findweaponindexbyname();
			t.entityprofile[t.entid].hasweapon = t.foundgunid;
			if (t.foundgunid > 0 && t.entityprofile[t.entid].isammo == 0)
			{
				// make gun active in game
				t.gun[t.foundgunid].activeingame = 1;

				// 301115 - also populate profile with correct default ammo from clip if default required
				if (t.entityprofile[t.entid].ischaracter == 1)
				{
					if (t.entityprofile[t.entid].quantity == -1)
					{
						t.entityprofile[t.entid].quantity = t.gun[t.foundgunid].settings.reloadqty;
						if (t.entityprofile[t.entid].quantity == 0)
						{
							// discover reload quantity if gun data not loaded in
							t.gunid = t.foundgunid;
							t.gun_s = t.gun[t.gunid].name_s;
							gun_loaddata();
							t.entityprofile[t.entid].quantity = g.firemodes[t.gunid][0].settings.reloadqty;
							t.gunid = 0;
						}
					}
				}
			}
		}
		else
		{
			t.entityprofile[t.entid].hasweapon = 0;
		}

		// see if we can find head automatically
		// 010818 - expanded to find mixamo_xx or bip01_xx or anything_xx
		if (t.entityprofile[t.entid].ischaracter == 1)
		{
			if (t.entityprofile[t.entid].headlimb == -1)
			{
				if (ObjectExist(t.entobj) == 1)
				{
					PerformCheckListForLimbs(t.entobj);
					for (t.tc = 1; t.tc <= ChecklistQuantity(); t.tc++)
					{
						cstr sChecklistFound = Lower(ChecklistString(t.tc));
						LPSTR pChecklistFound = sChecklistFound.Get();
						if (strnicmp(pChecklistFound + strlen(pChecklistFound) - 5, "_head", 5) == NULL)
						{
							t.entityprofile[t.entid].headlimb = t.tc - 1;
							t.tc = ChecklistQuantity() + 1;
						}
					}
				}
			}
			if (t.entityprofile[t.entid].firespotlimb == -1)
			{
				if (ObjectExist(t.entobj) == 1)
				{
					// standard FIRESPOT search
					#ifdef VRTECH
					PerformCheckListForLimbs(t.entobj);
					int iRememberPositionOfRightHand = -1;
					int iLimbCount = ChecklistQuantity();
					for (t.tc = 1; t.tc <= iLimbCount; t.tc++)
					{
						cstr sLimbName = cstr(Lower(ChecklistString(t.tc)));
						if (sLimbName == "firespot")
						{
							t.entityprofile[t.entid].firespotlimb = t.tc - 1;
							t.tc = ChecklistQuantity() + 1;
						}
						else
						{
							if (strstr(sLimbName.Get(),"_r_hand")!=NULL)
							{
								iRememberPositionOfRightHand = t.tc - 1;
							}
						}
					}
					#ifdef WICKEDENGINE
					if (t.entityprofile[t.entid].firespotlimb == -1)
					{
						// could not find a FIRESPOT for this character, so create one if found right hand
						if (iRememberPositionOfRightHand != -1)
						{
							// for now, use right hand
							t.entityprofile[t.entid].firespotlimb = iRememberPositionOfRightHand;

							/*
							// Add firespot limb to character object
							sObject* pObject = GetObjectData(t.entobj);
							WickedCall_RemoveObject(pObject);

							int iLimbIndex = iLimbCount;
							sFrame* pNewFrame = new sFrame;
							strcpy (pNewFrame->szName, "FIRESPOT");
							pNewFrame->iID = iLimbCount;
							pNewFrame->pMesh = NULL;

							// create additional frame in framelist
							sFrame** pNewFrameList = new sFrame*[iLimbCount+1];
							for (int f = 0; f < pObject->iFrameCount; f++) pNewFrameList[f] = pObject->ppFrameList[f];
							delete pObject->ppFrameList;
							pObject->ppFrameList = pNewFrameList;
							pObject->ppFrameList[iLimbCount] = pNewFrame;
							pObject->iFrameCount += 1;

							// Link to right hand
							sFrame* pFrameToMove = pObject->ppFrameList[iLimbIndex];
							sFrame* pFrameToLinkTo = pObject->ppFrameList[iRememberPositionOfRightHand];
							pFrameToMove->pParent = pFrameToLinkTo;
							sFrame* pSybs = pFrameToLinkTo;
							while (pSybs->pSibling) pSybs = pSybs->pSibling;
							pSybs->pSibling = pFrameToMove;

							// Populate FIRESPOT with necessary settings for correct weapon placement
							//pObject->ppFrameList[iLimbIndex]->vecPosition.x = 0;
							//pObject->ppFrameList[iLimbIndex]->vecPosition.y = 0;
							//pObject->ppFrameList[iLimbIndex]->vecPosition.z = 0;

							// finally provide new firespot limb
							t.entityprofile[t.entid].firespotlimb = iLimbIndex;

							// now add object back in (firespot will be part pf wicked hierarchy now)
							WickedCall_AddObject(pObject);
							*/
						}
					}
					#endif
					#else
					PerformCheckListForLimbs(t.entobj);
					for (t.tc = 1; t.tc <= ChecklistQuantity(); t.tc++)
					{
						if (cstr(Lower(ChecklistString(t.tc))) == "firespot")
						{
							// can reconfigure firespot if values specified
							if (t.entityprofile[t.entid].handfirespotoffx != 0
								|| t.entityprofile[t.entid].handfirespotoffy != 0
								|| t.entityprofile[t.entid].handfirespotoffz != 0
								|| t.entityprofile[t.entid].handfirespotrotx != 0
								|| t.entityprofile[t.entid].handfirespotroty != 0
								|| t.entityprofile[t.entid].handfirespotrotz != 0)
							{
								float fFireSpotX = t.entityprofile[t.entid].handfirespotoffx;
								float fFireSpotY = t.entityprofile[t.entid].handfirespotoffy;
								float fFireSpotZ = t.entityprofile[t.entid].handfirespotoffz;
								float fFireSpotRotX = t.entityprofile[t.entid].handfirespotrotx;
								float fFireSpotRotY = t.entityprofile[t.entid].handfirespotroty;
								float fFireSpotRotZ = t.entityprofile[t.entid].handfirespotrotz;
								OffsetLimb(t.entobj, t.tc - 1, fFireSpotX, fFireSpotY, fFireSpotZ);
								RotateLimb(t.entobj, t.tc - 1, fFireSpotRotX, fFireSpotRotY, fFireSpotRotZ);
							}
							t.entityprofile[t.entid].firespotlimb = t.tc - 1;
							t.tc = ChecklistQuantity() + 1;
						}
					}
					#endif
				}
			}
			if (t.entityprofile[t.entid].spine == -1)
			{
				if (ObjectExist(t.entobj) == 1)
				{
					PerformCheckListForLimbs(t.entobj);
					for (t.tc = 1; t.tc <= ChecklistQuantity(); t.tc++)
					{
						cstr sChecklistFound = Lower(ChecklistString(t.tc));
						LPSTR pChecklistFound = sChecklistFound.Get();
						#ifdef WICKEDENGINE
						// LB: as it turns out, it is Bip01 that has the major say in how the model shuffles forward!
						if ( stricmp(pChecklistFound, "Bip1") == NULL
						||   stricmp(pChecklistFound, "Bip01") == NULL
						||   stricmp(pChecklistFound, "Bip001") == NULL)
						#else
						if (strnicmp(pChecklistFound + strlen(pChecklistFound) - 7, "_spine1", 7) == NULL)
						#endif
						{
							t.entityprofile[t.entid].spine = t.tc - 1;
							break;
						}
					}
				}
			}
			if (t.entityprofile[t.entid].spine2 == -1)
			{
				if (ObjectExist(t.entobj) == 1)
				{
					PerformCheckListForLimbs(t.entobj);
					for (t.tc = 1; t.tc <= ChecklistQuantity(); t.tc++)
					{
						cstr sChecklistFound = Lower(ChecklistString(t.tc));
						LPSTR pChecklistFound = sChecklistFound.Get();
						if (strnicmp(pChecklistFound + strlen(pChecklistFound) - 7, "_spine2", 7) == NULL)
						{
							t.entityprofile[t.entid].spine2 = t.tc - 1;
							break;
						}
					}
				}
			}
		}

		// 090217 - new feature for some characters (Fuse FBX) to have perfect foot planting
		if (t.entityprofile[t.entid].ischaracter == 1 && t.entityprofile[t.entid].isspinetracker == 1 && t.entityprofile[t.entid].spine != -1)
		{
			sObject* pObject = GetObjectData(t.entobj);
			if (pObject)
			{
				pObject->bUseSpineCenterSystem = true;
				pObject->dwSpineCenterLimbIndex = t.entityprofile[t.entid].spine;
				pObject->fSpineCenterTravelDeltaX = 0.0f;
				pObject->fSpineCenterTravelDeltaZ = 0.0f;
			}
		}
		else
		{
			// 051115 - if character, ensure the Y offset is applied to the parent object
			if (t.entityprofile[t.entid].ischaracter == 1 || t.entityprofile[t.entid].offyoverride != 0)
			{
				// calc bounds for static entities that have hierarchy that need shifting up when offyoverride used
				int iCalculateBounds = 0;
				if (t.entityprofile[t.entid].offyoverride != 0) iCalculateBounds = 1;
				OffsetLimb(t.entobj, 0, 0.0f, t.entityprofile[t.entid].offy, 0.0f, iCalculateBounds);
			}
		}

#ifdef VRTECH
		// 300819 - reenable offset X and Z (for when geometry is shifted from center)
		if (t.entityprofile[t.entid].defaultstatic == 1 && t.entityprofile[t.entid].isimmobile == 1)
		{
			OffsetLimb(t.entobj, 0, t.entityprofile[t.entid].offx, t.entityprofile[t.entid].offy, t.entityprofile[t.entid].offz, 1);
		}
#endif

		// 010917 - hide any firespot limb meshes
		if (t.entityprofile[t.entid].firespotlimb > 0)
		{
			ExcludeLimbOn(t.entobj, t.entityprofile[t.entid].firespotlimb);
		}

		//  190115 - wipe out limb control on legacy models, not configured for rotation!
		if (t.entityprofile[t.entid].skipfvfconvert == 1)
		{
			//t.entityprofile[t.entid].firespotlimb=0; but can allow firespot to be retained
			t.entityprofile[t.entid].headlimb = 0;
			t.entityprofile[t.entid].spine = 0;
			t.entityprofile[t.entid].spine2 = 0;
		}

#ifdef WICKEDENGINE
		if (t.entityprofile[t.entid].ismarker == 2)
		{
			// Ensure all lights start as points when first loaded as parent
			entity_updatelightobjtype(t.entobj, 0);
		}
#endif

		//PE: Additional settings.
#ifdef WICKEDENGINE
		// For Wicked, cull mode controlled per-mesh with parent default as normal 
		//PE: Prefer WEMaterial over old cullmode
		bool bUseWEMaterial = false;
		if (t.entityprofile[t.entid].WEMaterial.MaterialActive)
		{
			WickedSetEntityId(t.entid);
			WickedSetElementId(0);
			sObject* pObject = g_ObjectList[t.entobj];
			if (pObject)
			{
				bUseWEMaterial = true;
				for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
				{
					sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
					if (pMesh)
					{
						// set properties of mesh
						WickedSetMeshNumber(iMeshIndex);
						bool bDoubleSided = WickedDoubleSided();
						if (bDoubleSided)
						{
							pMesh->bCull = false;
							pMesh->iCullMode = 0;
							WickedCall_SetMeshCullmode(pMesh);
						}
						else
						{
							pMesh->iCullMode = 1;
							pMesh->bCull = true;
							WickedCall_SetMeshCullmode(pMesh);
						}
					}
				}
			}
			WickedSetEntityId(-1);
		}

		if (!bUseWEMaterial)
			SetObjectCull(t.entobj, 1);
		SetObjectLight(t.entobj, 0);
		if (t.entityprofile[t.entid].bIsDecal)
		{
			void SetupDecalObject(int obj, int elementID);
			SetupDecalObject(t.entobj,0);
		}
	
		#else
		if (t.entityprofile[t.entid].cullmode >= 0)
		{
			if (t.entityprofile[t.entid].cullmode != 0)
				SetObjectCull(t.entobj, 0);
			else
				SetObjectCull(t.entobj, 1);
		}
		#endif
		SetObjectCollisionOff(t.entobj);

		#ifdef WICKEDENGINE
		WickedSetEntityId(t.entid);
		#endif
		SetAlphaMappingOn(t.entobj, 100);

		#ifdef WICKEDENGINE
		SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
		WickedSetEntityId(-1);
		#else
		SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
		#endif

		//  debug info and timestamp list (if logging)
		if (  t.entobj>0 ) 
		{
			if (  ObjectExist(t.entobj) == 1 ) 
			{
				t.strwork = "" ; t.strwork = t.strwork + "Loaded "+Str(t.entid)+":"+t.ent_s + " (" + cstr(GetObjectPolygonCount(t.entobj)) + ")";
				timestampactivity(0, t.strwork.Get() );
			}
		}
	}
	else
	{

		//  debug info and timestamp list (if logging)
		t.strwork = ""; t.strwork = t.strwork + "Skipped "+Str(t.entid)+":"+t.ent_s;
		timestampactivity(0, t.strwork.Get() );
	}

	//  Decactivate auto generation of mipmaps when entity load finished
	SetImageAutoMipMap (  0 );

	// normal entity loaded
	return true;
}

int constexpr conststrlen( const char* str )
{
	return *str ? 1 + conststrlen(str + 1) : 0;
}

#define cmpStrConst( str, cmpVal ) \
{ \
	matched = true; \
	if ( strcmp(str, cmpVal) != 0 ) matched = false; \
}

// max length 16
#define cmpNStrConst( str, cmpVal ) \
{ \
	matched = true; \
	int constexpr len = conststrlen( cmpVal ); \
	if ( strncmp(str, cmpVal, len) != 0 ) matched = false; \
}

/*
// max length 32
#define cmpStrConst( str, cmpVal ) \
{ \
	matched = true; \
	int constexpr len = conststrlen( cmpVal ); \
	if ( len != matchlen ) matched = false; \
	else { \
		alignas(16) constexpr char str2[32] = cmpVal; \
		if ( _mm_cmpistrc( *(__m128i*)str, *(__m128i*)str2, _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY ) != 0 ) matched = false; \
		else if ( str[16] != 0 && str2[16] != 0 ) { \
			if ( _mm_cmpistrc( *(__m128i*)&str[16], *(__m128i*)&str2[16], _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY ) != 0 ) matched = false; \
		} \
	} \
}

// max length 16
#define cmpNStrConst( str, cmpVal ) \
{ \
	matched = true; \
	int constexpr len = conststrlen( cmpVal ); \
	alignas(16) constexpr char str2[16] = cmpVal; \
	if ( _mm_cmpestrc( *(__m128i*)str, len, *(__m128i*)str2, len, _SIDD_CMP_EQUAL_EACH | _SIDD_NEGATIVE_POLARITY ) != 0 ) matched = false; \
}
*/

void entity_loaddata ( void )
{
	//  Protectf BIN file if no FPE backup (standalone run)
	t.tprotectBINfile=0;
	if (t.ent_s.Get()[1] == ':') t.entdir_s = "";
	t.tFPEName_s=t.entdir_s+t.ent_s;
	if (  FileExist(t.tFPEName_s.Get()) == 0 ) 
	{
		t.tprotectBINfile=1;
	}

	//  Ensure entity profile still exists
	t.entityprofileheader[t.entid].desc_s="";
	t.tprofile_s=Left(t.tFPEName_s.Get(),Len(t.tFPEName_s.Get())-4); t.tprofile_s += ".bin";
	if (  t.tprotectBINfile == 0 ) 
	{
		//  020715 - to solve BIN issue once and for all, delete them when load entity
		//  and only preserve for final standalone export (actually NO performance difference!)
		if (  FileExist(t.tprofile_s.Get()) == 1  )  DeleteAFile (  t.tprofile_s.Get() );
	}

	t.strwork = t.entdir_s+t.ent_s;
	if (  FileExist(t.strwork.Get()) == 1 || FileExist(t.tprofile_s.Get()) == 1 ) 
	{

	//  Export entity FPE file if flagged
	if (  g.gexportassets == 1 ) 
	{
		t.strwork = t.entdir_s+t.ent_s;
		t.tthumbbmpfile_s = "";	t.tthumbbmpfile_s=t.tthumbbmpfile_s + Left(t.strwork.Get(),(Len(t.entdir_s.Get())+Len(t.ent_s.Get()))-4)+".bmp";
	}

	//  Allowed to loop around if skipBIN flag set
	do {  t.skipBINloadingandtryagain=0;

	//  Check if binary version of entity profile exists (DELETE BIN AT MOMEMENT!)
	//C++ISSUE forcing non binary at the moment due to make memblock from array not implemented for new arrays
	//looks like we are delelting bins anyway, but just incase...
	if ( 1 ) //if (  FileExist(t.tprofile_s.Get()) == 0 ) 
	{
		// 061115 - reset entity anim start value (so can be filled further down)
		for ( int n = 0; n < g.animmax; n++ )
		{
			t.entityanim[t.entid][n].start = 0;
			t.entityanim[t.entid][n].finish = 0;
			t.entityanim[t.entid][n].found = 0;
		}

		//  Must be reset before parse
		t.entityprofile[t.entid].limbmax=0;
		t.entityprofile[t.entid].animmax=0;
		t.entityprofile[t.entid].appendanimmax=0; //PE: sometimes , caused endless loop, was never set anywhere.
		t.entityprofile[t.entid].footfallmax=0;
		t.entityprofile[t.entid].headlimb=-1;
		t.entityprofile[t.entid].firespotlimb=-1;
		t.entityprofile[t.entid].physics=1;
		t.entityprofile[t.entid].phyweight=100;
		t.entityprofile[t.entid].phyfriction=0;
		t.entityprofile[t.entid].hoverfactor=0;
		t.entityprofile[t.entid].phyalways=0;
		t.entityprofile[t.entid].spine=-1;
		t.entityprofile[t.entid].spine2=-1;
		t.entityprofile[t.entid].decaloffsetangle=0;
		t.entityprofile[t.entid].decaloffsetdist=0;
		t.entityprofile[t.entid].decaloffsety=0;
		t.entityprofile[t.entid].ragdoll = 0;
		t.entityprofile[t.entid].nothrowscript=0;
		t.entityprofile[t.entid].canfight=1;
		t.entityprofile[t.entid].rateoffire=85;
		t.entityprofile[t.entid].transparency=0;
		t.entityprofile[t.entid].canseethrough=0;
		t.entityprofile[t.entid].cullmode=0;
		t.entityprofile[t.entid].lod1distance=0;
		t.entityprofile[t.entid].lod2distance=0;
		t.entityprofile[t.entid].characterbasetype = -1;// bitbobon = -1;
		t.entityprofile[t.entid].reservedy1 = 1.0f;// bitbobdistweight_f = 1;
		t.entityprofile[t.entid].reservedy2 = 0.0f;
		t.entityprofile[t.entid].reservedy3 = 0.0f;
		t.entityprofile[t.entid].autoflatten=0;
		t.entityprofile[t.entid].headframestart=-1;
		t.entityprofile[t.entid].headframefinish=-1;
		t.entityprofile[t.entid].hairframestart=-1;
		t.entityprofile[t.entid].hairframefinish=-1;
		t.entityprofile[t.entid].hideframestart=-1;
		t.entityprofile[t.entid].hideframefinish=-1;
		t.entityprofile[t.entid].animspeed=100;
		t.entityprofile[t.entid].animstyle=0;
		t.entityprofile[t.entid].collisionscaling=100;
		t.entityprofile[t.entid].physicsobjectcount = 0;
		t.entityprofile[t.entid].ishudlayer_s="";
		t.entityprofile[t.entid].ishudlayer=0;
		t.entityprofile[t.entid].fatness=50;
		t.entityprofile[t.entid].matrixmode=0;
		t.entityprofile[t.entid].skipfvfconvert=0;
		t.entityprofile[t.entid].resetlimbmatrix=0;
		t.entityprofile[t.entid].onetexture=0;
		t.entityprofile[t.entid].usesweapstyleanims=0;
		t.entityprofile[t.entid].isviolent=1;
		t.entityprofile[t.entid].reverseframes=0;
		t.entityprofile[t.entid].fullbounds=0;
		t.entityprofile[t.entid].cpuanims=0;
		t.entityprofile[t.entid].ignoredefanim=0;
		t.entityprofile[t.entid].teamfield=0;
		t.entityprofile[t.entid].scale=100;
		t.entityprofile[t.entid].addhandle_s="";
		t.entityprofile[t.entid].addhandlelimb=0;
		t.entityprofile[t.entid].ischaractercreator=0;
		t.entityprofile[t.entid].charactercreator_s="";
		t.entityprofile[t.entid].fJumpModifier=1.0f;		
		t.entityprofile[t.entid].jumphold=0;
		t.entityprofile[t.entid].jumpresume=0;
		t.entityprofile[t.entid].jumpvaulttrim=1;
		t.entityprofile[t.entid].meleerange=80;
		t.entityprofile[t.entid].meleehitangle=30;
		t.entityprofile[t.entid].meleestrikest=0;
		t.entityprofile[t.entid].meleestrikefn=0;
		t.entityprofile[t.entid].meleedamagest=20;
		t.entityprofile[t.entid].meleedamagefn=30;
		for ( t.q = 0 ; t.q<=  100 ; t.q++ ) { t.entitybodypart[t.entid][t.q]=0 ;   }
		t.entityprofile[t.entid].usespotlighting=0;
		t.entityprofile[t.entid].lodmodifier=0;
		t.entityprofile[t.entid].isocluder=1; // can be adjusted (if notanoccluder set to 1)
		t.entityprofile[t.entid].isocludee=1;
		t.entityprofile[t.entid].specularperc=100;
		t.entityprofile[t.entid].specular=0;
		t.entityprofile[t.entid].uvscrollu=0;
		t.entityprofile[t.entid].uvscrollv=0;
		t.entityprofile[t.entid].uvscaleu=1.0f;
		t.entityprofile[t.entid].uvscalev=1.0f;
		t.entityprofile[t.entid].invertnormal=0;
		t.entityprofile[t.entid].preservetangents=0;		
		t.entityprofile[t.entid].colondeath=1;
		t.entityprofile[t.entid].parententityindex=0;
		t.entityprofile[t.entid].parentlimbindex=0;
		t.entityprofile[t.entid].quantity=-1; // FPE specifies a value or we use a single weapon clip buy default (below)
		t.entityprofile[t.entid].smoothangle=0;
		t.entityprofile[t.entid].noXZrotation=0;
		t.entityprofile[t.entid].zdepth = 1;
		t.entityprofile[t.entid].isebe = 0;
		t.entityprofile[t.entid].offyoverride = 0;
		t.entityprofile[t.entid].offy = 0; //PE: Was not reset.
		t.entityprofile[t.entid].ismarker = 0;  //PE: Was not reset.
		t.entityprofile[t.entid].ischaracter = 0;
		#ifdef WICKEDENGINE
		t.entityprofile[t.entid].bIsDecal = false; //PE: Was not set.
		t.entityprofile[t.entid].isspinetracker = 1;
		#else
		t.entityprofile[t.entid].isspinetracker = 0;
		#endif
		t.entityprofile[t.entid].phyweight=100;
		t.entityprofile[t.entid].phyfriction=100;
		t.entityprofile[t.entid].phyforcedamage=100;
		t.entityprofile[t.entid].rotatethrow=1;
		t.entityprofile[t.entid].explodedamage=100;
		t.entityprofile[t.entid].forcesimpleobstacle=0;
		#ifdef VRTECH
		t.entityprofile[t.entid].forceobstaclepolysize=20.0f;//30.0f; hagia model
		t.entityprofile[t.entid].forceobstaclesliceheight=20.0f;//14.0f; hagia model
		t.entityprofile[t.entid].forceobstaclesliceminsize=4.0f;//5.0f; hagia model 
		#else
		t.entityprofile[t.entid].forceobstaclepolysize=30.0f;
		t.entityprofile[t.entid].forceobstaclesliceheight=14.0f; 
		t.entityprofile[t.entid].forceobstaclesliceminsize=5.0f;
		#endif
		t.entityprofile[t.entid].effectprofile=0;
		t.entityprofile[t.entid].ignorecsirefs=0;
		#ifdef VRTECH
		t.entityprofile[t.entid].voiceset_s = ""; // when empty, default to first voice
		t.entityprofile[t.entid].voicerate = 0;
		#endif
		// For MAX, these HANDFIRESPOT values may be purposed if different characters needed a hand adjust to fit weapon standard (not in EA)
		t.entityprofile[t.entid].handfirespotoffx = 0.0f;
		t.entityprofile[t.entid].handfirespotoffy = 0.0f;
		t.entityprofile[t.entid].handfirespotoffz = 0.0f;
		t.entityprofile[t.entid].handfirespotrotx = 0.0f;
		t.entityprofile[t.entid].handfirespotroty = 0.0f;
		t.entityprofile[t.entid].handfirespotrotz = 0.0f;
		t.entityprofile[t.entid].handfirespotsize = 100.0f;
		#ifdef WICKEDENGINE
		t.entityprofile[t.entid].coneangle = 100.0f;
		t.entityprofile[t.entid].conerange = 1000.0f;
		#endif

		// reset so characters start with NO WEAPON!
		t.entityprofile[t.entid].isammo = 0;
		t.entityprofile[t.entid].hasweapon = 0;
		t.entityprofile[t.entid].hasweapon_s = "";

		// head and spine tracker detail defaults
		t.entityprofile[t.entid].headspinetracker.headhlimit = 45;
		t.entityprofile[t.entid].headspinetracker.headhoffset = 0;
		t.entityprofile[t.entid].headspinetracker.headvlimit = 45;
		t.entityprofile[t.entid].headspinetracker.headvoffset = 0;
		t.entityprofile[t.entid].headspinetracker.spinehlimit = 45;
		t.entityprofile[t.entid].headspinetracker.spinehoffset = 0;
		t.entityprofile[t.entid].headspinetracker.spinevlimit = 45;
		t.entityprofile[t.entid].headspinetracker.spinevoffset = 0;

		//  Starter animation counts
		t.tnewanimmax=0 ; t.entityprofile[t.entid].animmax=t.tnewanimmax;
		t.tstartofaianim=-1 ; t.entityprofile[t.entid].startofaianim=t.tstartofaianim;

		// other resets
		t.entityappendanim[t.entid][0].filename = "";
		t.entityappendanim[t.entid][0].startframe = 0;

		// wicked custom materials
		#ifdef WICKEDENGINE
		t.entityprofile[t.entid].thumbnailbackdrop = "";
		t.entityprofile[t.entid].WEMaterial.MaterialActive = false;
		for (int i = 0; i < MAXMESHMATERIALS; i++)
		{
			// per mesh textures and control values
			t.entityprofile[t.entid].WEMaterial.baseColorMapName[i] = "";
			t.entityprofile[t.entid].WEMaterial.normalMapName[i] = "";
			t.entityprofile[t.entid].WEMaterial.emissiveMapName[i] = "";
			t.entityprofile[t.entid].WEMaterial.surfaceMapName[i] = "";
			#ifndef DISABLEOCCLUSIONMAP
			t.entityprofile[t.entid].WEMaterial.occlusionMapName[i] = "";
			#endif
			t.entityprofile[t.entid].WEMaterial.displacementMapName[i] = "";
			t.entityprofile[t.entid].WEMaterial.fAlphaRef[i] = 1.0;
			t.entityprofile[t.entid].WEMaterial.fNormal[i] = 1.0;
			t.entityprofile[t.entid].WEMaterial.fEmissive[i] = 0.0;
			t.entityprofile[t.entid].WEMaterial.fRoughness[i] = 1.0;
			t.entityprofile[t.entid].WEMaterial.fMetallness[i] = 1.0;

			// per mesh colors
			t.entityprofile[t.entid].WEMaterial.dwBaseColor[i] = -1;
			t.entityprofile[t.entid].WEMaterial.dwEmmisiveColor[i] = -1;

			// per mesh settings and flags
			t.entityprofile[t.entid].WEMaterial.bCastShadows[i] = true;
			t.entityprofile[t.entid].WEMaterial.bDoubleSided[i] = false;
			t.entityprofile[t.entid].WEMaterial.fRenderOrderBias[i] = 0;
			t.entityprofile[t.entid].WEMaterial.bPlanerReflection[i] = false;
			t.entityprofile[t.entid].WEMaterial.bTransparency[i] = false;
			t.entityprofile[t.entid].WEMaterial.fReflectance[i] = 0.04f;// 0.002f;
		}

		//Resolve conflicts.
		//t.entityprofile[t.entid].WEMaterial.MaterialActive = false;
		//t.entityprofile[t.entid].WEMaterial.bCastShadows = true;
		//t.entityprofile[t.entid].WEMaterial.bFlipNormalMap = false;
		//t.entityprofile[t.entid].WEMaterial.bPlanerReflection = false;
		//t.entityprofile[t.entid].WEMaterial.bTransparency = false;
		//t.entityprofile[t.entid].WEMaterial.dwEmmisiveColor = -1;
		//t.entityprofile[t.entid].WEMaterial.dwBaseColor = -1;
		//t.entityprofile[t.entid].WEMaterial.fReflectance = 0.002f; //PE: Turn down reflectance on default materials.

		t.entityprofile[t.entid].thumbnailbackdrop = "";
		t.entityprofile[t.entid].BackBufferZoom = -1.0f;
		t.entityprofile[t.entid].BackBufferCamLeft = -1.0f;
		t.entityprofile[t.entid].BackBufferCamUp = -1.0f;
		t.entityprofile[t.entid].BackBufferRotateX = -1.0f;
		t.entityprofile[t.entid].BackBufferRotateY = -1.0f;
		t.entityprofile[t.entid].iThumbnailAnimset = -1.0f;
		t.entityprofile[t.entid].keywords_s = "";
		
		t.entityprofile[t.entid].effect_s = ""; //bIsDecal , needed reset.
		#endif

		//  temp variable to hold which physics object we are on from the importer
		t.tPhysObjCount = 0;

		//  Load entity Data from file
		// ZJ: Users have been reporting their imported models don't load correctly, caused by the FPEs having more than 400 lines.
		Dim(t.data_s, 500); //Dim (  t.data_s,400  );
		t.strwork = t.entdir_s + t.ent_s;
		LoadArray(t.strwork.Get(), t.data_s);
		for (t.l = 0; t.l <= 499; t.l++) //for ( t.l = 0 ; t.l<=  399; t.l++ )*/
		{
			t.line_s=t.data_s[t.l];
			if (  Len(t.line_s.Get())>0 ) 
			{
				if (  t.line_s.Get()[0] != ';' ) 
				{
					//  take fieldname and value
					for ( t.c = 0 ; t.c < Len(t.line_s.Get()); t.c++ )
					{
						if ( t.line_s.Get()[t.c] == '=' ) 
						{ 
							t.mid = t.c+1  ; break;
						}
					}
					t.field_s=cstr(Lower(removeedgespaces(Left(t.line_s.Get(),t.mid-1))));
					t.value_s=cstr(removeedgespaces(Right(t.line_s.Get(),Len(t.line_s.Get())-t.mid)));

					//  take value 1 and 2 from value
					for ( t.c = 0 ; t.c < Len(t.value_s.Get()); t.c++ )
					{
						if (  t.value_s.Get()[t.c] == ',' ) 
						{ 
							t.mid = t.c+1 ; break; 
						}
					}
					t.value1=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value1_f=ValF(removeedgespaces(Left(t.value_s.Get(),t.mid-1)));
					t.value2_s=cstr(removeedgespaces(Right(t.value_s.Get(),Len(t.value_s.Get())-t.mid)));
					if (  Len(t.value2_s.Get())>0  )  t.value2 = ValF(t.value2_s.Get()); else t.value2 = -1;

					// string comparison optimization
					alignas(16) char t_field_s[32];
					const char* src = Lower(t.field_s.Get());
					int index = 0;
					while( src[index] && index < 32 )
					{
						t_field_s[ index ] = src[ index ];
						index++;
					}

					int matchlen = index;

					while( index < 32 )
					{
						t_field_s[ index++ ] = 0;
					}

					bool matched = false;

					//  entity header
					cmpStrConst( t_field_s, "desc" );
					if ( matched )  t.entityprofileheader[t.entid].desc_s = t.value_s;
					
					// check if group early, and abort (we use LoadGroup for these type of FPE files)
					#ifdef WICKEDENGINE
					cmpStrConst( t_field_s, "isgroupobject" );
					if (matched && t.value1 == 1)
					{
						// if looking for the group flag, mark it as found
						extern int g_iAbortedAsEntityIsGroupFileMode;
						if (g_iAbortedAsEntityIsGroupFileMode == 1)
						{
							// move mode to show we have determined it IS a group file!
							g_iAbortedAsEntityIsGroupFileMode = 2;
						}
					}
					#endif

					//  entity AI
					//cmpStrConst( t_field_s, "aiinit" ); //PE: Not used anymore.
					//if (  matched  )  t.entityprofile[t.entid].aiinit_s = Lower(t.value_s.Get());
					cmpStrConst( t_field_s, "aimain" );
					if (matched)
					{
						t.entityprofile[t.entid].aimain_s = Lower(t.value_s.Get());
						#ifdef WICKEDENGINE
						if (strnicmp (t.entityprofile[t.entid].aimain_s.Get(), "default.lua", 11) == NULL)
						{
							t.entityprofile[t.entid].aimain_s = "no_behavior_selected.lua";
						}
						else
						{
							if (strlen(t.entityprofile[t.entid].aimain_s.Get()) < 4 ) 
							{
								t.entityprofile[t.entid].aimain_s = "no_behavior_selected.lua";
							}
						}
						if (strnicmp (t.entityprofile[t.entid].aimain_s.Get(), "markers\\togglelight.lua", 23) == NULL)
						{
							// ensure this particular behavior uses case (eventually allow all objects to use case (carefully))
							// doing so right now might introduce new issues!
							t.entityprofile[t.entid].aimain_s = "markers\\ToggleLight.lua";
						}
						#endif
					}
					//cmpStrConst( t_field_s, "aidestroy" );
					//if (  matched  )  t.entityprofile[t.entid].aidestroy_s = Lower(t.value_s.Get());
					//cmpStrConst( t_field_s, "aishoot" ); //PE: Not used anymore.
					//if (  matched  )  t.entityprofile[t.entid].aishoot_s = Lower(t.value_s.Get());

					#ifdef WICKEDENGINE
					// determine CCP base type and store
					cmpStrConst( t_field_s, "ccpassembly" );
					if (matched)
					{
						t.entityprofile[t.entid].characterbasetype = -1;
						if (strstr (t.value_s.Get(), "adult male") != NULL) t.entityprofile[t.entid].characterbasetype = 0;
						if (strstr (t.value_s.Get(), "adult female") != NULL) t.entityprofile[t.entid].characterbasetype = 1;
						if (strstr (t.value_s.Get(), "zombie male") != NULL) t.entityprofile[t.entid].characterbasetype = 2;
						if (strstr (t.value_s.Get(), "zombie female") != NULL) t.entityprofile[t.entid].characterbasetype = 3;
					}
					#endif

					#ifdef VRTECH
					cmpStrConst( t_field_s, "voice" );
					if (  matched  )  t.entityprofile[t.entid].voiceset_s = t.value_s;
					cmpStrConst( t_field_s, "speakrate" );
					if (  matched  )  t.entityprofile[t.entid].voicerate = t.value1;
					#endif
					cmpStrConst( t_field_s, "soundset" );
					if (  matched  )  t.entityprofile[t.entid].soundset_s = t.value_s;
					cmpStrConst( t_field_s, "soundset1" );
					if (  matched  )  t.entityprofile[t.entid].soundset1_s = t.value_s;
					cmpStrConst( t_field_s, "soundset2" );
					if (  matched  )  t.entityprofile[t.entid].soundset2_s = t.value_s;
					cmpStrConst( t_field_s, "soundset3" );
					if (  matched  )  t.entityprofile[t.entid].soundset3_s = t.value_s;
					cmpStrConst( t_field_s, "soundset4" );
					if (  matched  )  t.entityprofile[t.entid].soundset5_s = t.value_s;
					cmpStrConst( t_field_s, "soundset5");
					if (  matched  )  t.entityprofile[t.entid].soundset6_s = t.value_s;
					//  entity AI related vars
					cmpStrConst( t_field_s, "usekey" );
					if (  matched  )  t.entityprofile[t.entid].usekey_s = t.value_s;
					cmpStrConst( t_field_s, "ifused" );
					if (  matched  )  t.entityprofile[t.entid].ifused_s = t.value_s;
					//cmpStrConst( t_field_s, "ifusednear" );
					//if (  matched  )  t.entityprofile[t.entid].ifusednear_s = t.value_s;

					//  entity SPAWN
					cmpStrConst( t_field_s, "spawnmax" );
					if (  matched  )  t.entityprofile[t.entid].spawnmax = t.value1;
					cmpStrConst( t_field_s, "spawndelay" );
					if (  matched  )  t.entityprofile[t.entid].spawndelay = t.value1;
					cmpStrConst( t_field_s, "spawnqty" );
					if (  matched  )  t.entityprofile[t.entid].spawnqty = t.value1;
					//  entity orientation
					cmpStrConst( t_field_s, "model" );
					//  if it is a character creator model, ignore this
					if (  t.entityprofile[t.entid].ischaractercreator == 0 ) 
					{
						if (  matched  ) 
							t.entityprofile[t.entid].model_s = t.value_s;
					}

					cmpStrConst( t_field_s, "offsetx" );
					if (  matched  )  t.entityprofile[t.entid].offx = t.value1;
					cmpStrConst( t_field_s, "offsety" );
					if (  matched  )  t.entityprofile[t.entid].offy = t.value1;
					cmpStrConst( t_field_s, "offyoverride" );
					if (  matched  )  
					{
						t.entityprofile[t.entid].offyoverride = 1;
						t.entityprofile[t.entid].offy = t.value1;
					}
					cmpStrConst( t_field_s, "offsetz" );
					if (  matched  )  t.entityprofile[t.entid].offz = t.value1;
					cmpStrConst( t_field_s, "rotx" );
					if (  matched  )  t.entityprofile[t.entid].rotx = t.value1;
					cmpStrConst( t_field_s, "roty" );
					if (  matched  )  t.entityprofile[t.entid].roty = t.value1;
					cmpStrConst( t_field_s, "rotz" );
					if (  matched  )  t.entityprofile[t.entid].rotz = t.value1;
					cmpStrConst( t_field_s, "scale" );
					if (  matched  )  t.entityprofile[t.entid].scale = t.value1;
					cmpStrConst( t_field_s, "fixnewy" );
					if (  matched  )  t.entityprofile[t.entid].fixnewy = t.value1;
					cmpStrConst( t_field_s, "hoverfactor" );
					if (  matched ) 
					{
						//  FPGC - V116 - some FPE characters use a range 0.1-0.9, must be accounted!
						if (  t.value1_f>-1.0 && t.value1_f<1.0  )  t.value1 = 1;
						t.entityprofile[t.entid].hoverfactor=t.value1;
						t.entityprofile[t.entid].hoverfactor=t.value1;
					}
					cmpStrConst( t_field_s, "forwardfacing" );
					if (  matched  )  t.entityprofile[t.entid].forwardfacing = t.value1;
					cmpStrConst( t_field_s, "dontfindfloor" );
					if (  matched  )  t.entityprofile[t.entid].dontfindfloor = t.value1;
					cmpStrConst( t_field_s, "defaultheight" );
					if (  matched  )  t.entityprofile[t.entid].defaultheight = t.value1;
					cmpStrConst( t_field_s, "defaultstatic" );
					if (  matched  )  t.entityprofile[t.entid].defaultstatic = t.value1;

					//  autoflatten
					//  0  ; none (default)
					//  1  ; rectangle area
					//  2  ; circle area
					cmpStrConst( t_field_s, "autoflatten" );
					if (  matched  )  t.entityprofile[t.entid].autoflatten = t.value1;

					//  collisionmode (see GameGuru\Docs\collisionmodevalues.txt)
					//  0  ; box shape (default)
					//  1  ; polygon shape
					//  2  ; sphere shape
					//  3  ; cylinder shape
					//  8  ; polygon shape using OBJ file
					//  9  ; convex hull reduction shape
					//  10 ; hull decomposition - multiple convex hulls
					//  11 ; no physics
					//  12 ; no physics but can still be detected with IntersectAll command
					//  21 ; player repel feature (for characters and other beasts/zombies)
					//  22 ; no repel (for animals that player can pass through)
					//  40 ; collision boxes (defined in Import Model feature)
					//  41-49 ; reserved (collision polylist, sphere list, cylinder list)
					//  50 ; generate obstacle and cylinder from 1/64th up from base of model
					//  51 ; generate obstacle and cylinder from 1/32th down from base of model
					//  52 ; generate obstacle and cylinder from 8/16th up from base of model
					//  53 ; generate obstacle and cylinder from 7/16th up from base of model
					//  54 ; generate obstacle and cylinder from 6/16th up from base of model
					//  55 ; generate obstacle and cylinder from 5/16th up from base of model
					//  56 ; generate obstacle and cylinder from 4/16th up from base of model
					//  57 ; generate obstacle and cylinder from 3/16th up from base of model
					//  58 ; generate obstacle and cylinder from 2/16th up from base of model
					//  59 ; generate obstacle and cylinder from 1/16th up from base of model
					//  1000-2000 ; only one limb has collision Box Shape (1000=limb zero,1001=limb one,etc)
					//  2000-3000 ; only one limb has collision Polygons Shape (2000=limb zero,2001=limb one,etc)					
					cmpStrConst( t_field_s, "collisionmode" );
					if (  matched  )  t.entityprofile[t.entid].collisionmode = t.value1;
					cmpStrConst( t_field_s, "collisionscaling" );
					if (  matched  )  t.entityprofile[t.entid].collisionscaling = t.value1;
					cmpStrConst( t_field_s, "collisionoverride" );
					if (  matched  )  t.entityprofile[t.entid].collisionoverride = t.value1;

					// endcollision:
					// 0 - no collision for ragdoll
					// 1 - collision for ragdoll
					cmpStrConst( t_field_s, "endcollision" );
					if (  matched  )  t.entityprofile[t.entid].colondeath = t.value1;

					//  forcesimpleobstacle
					//  -1 ; absolutely no obstacle
					//  0 ; default
					//  1 ; Box (  )
					//  2 ; contour
					//  3 ; full poly scan
					cmpStrConst( t_field_s, "forcesimpleobstacle" );
					if (  matched  )  t.entityprofile[t.entid].forcesimpleobstacle = t.value1;
					cmpStrConst( t_field_s, "forceobstaclepolysize" );
					if (  matched  )  t.entityprofile[t.entid].forceobstaclepolysize = t.value1;
					cmpStrConst( t_field_s, "forceobstaclesliceheight" );
					if (  matched  )  t.entityprofile[t.entid].forceobstaclesliceheight = t.value1;
					cmpStrConst( t_field_s, "forceobstaclesliceminsize" );
					if (  matched  )  t.entityprofile[t.entid].forceobstaclesliceminsize = t.value1;

					cmpStrConst( t_field_s, "notanoccluder" );
					if (  matched  )  
					{
						t.entityprofile[t.entid].notanoccluder = t.value1;
						if ( t.entityprofile[t.entid].notanoccluder == 1 ) t.entityprofile[t.entid].isocluder = 0;
					}
					cmpStrConst( t_field_s, "notanoccludee" );
					if (  matched  )  
					{
						int notanoccludee = t.value1;
						if ( notanoccludee == 1 ) t.entityprofile[t.entid].isocludee = 0;
					}

					cmpStrConst( t_field_s, "skipfvfconvert" );
					if (  matched  )  t.entityprofile[t.entid].skipfvfconvert = t.value1;
					cmpStrConst( t_field_s, "matrixmode" );
					if (  matched  )  t.entityprofile[t.entid].matrixmode = t.value1;

					// 040116 - when lightmapper scales entities incorrectly, need this flag to correct!
					cmpStrConst( t_field_s, "resetlimbmatrix" );
					if (  matched  )  t.entityprofile[t.entid].resetlimbmatrix = t.value1;

					cmpStrConst( t_field_s, "reverseframes" );
					if (  matched  )  t.entityprofile[t.entid].reverseframes = t.value1;
					cmpStrConst( t_field_s, "fullbounds" );
					if (  matched  )  t.entityprofile[t.entid].fullbounds = t.value1;

					//  cpuanims
					//  0 ; GPU animation
					//  1 ; CPU animation (for wide scope animations that need accurate ray detection)
					//  2 ; Same as [1] but will hide any meshes that do not have animations
					cmpStrConst( t_field_s, "cpuanims" );
					if (  matched  )  t.entityprofile[t.entid].cpuanims = t.value1;
				
					// 131115 - some legacy models hold an OLD nasty frame in matCombined for each frame, and can mess up collision detection if CPUANIMS=1 also
					cmpStrConst( t_field_s, "ignoredefanim" );
					if (  matched  )  t.entityprofile[t.entid].ignoredefanim = t.value1;

					//  materialindex
					//  0    = - GenericSoft
					//  1    = S Stone
					//  2    = M Metal
					//  3    = W Wood
					//  4    = G Glass
					//  5    = L Liquid Splashy Wet
					//  6    = F Flesh (Bloody Organic)
					//  7    = H Hollow Drum Metal
					//  8    = T Small High Pitch Tin
					//  9    = V Small Low Pitch Tin
					//  10   = I Silly Material
					//  11   = A Marble
					//  12   = C Cobble
					//  13   = R Gravel
					//  14   = E Soft Metal
					//  15   = O Old Stone
					//  16   = D Old Wood
					//  17   = W Shallow Water
					//  18   = U Underwater
					cmpStrConst( t_field_s, "materialindex" );
					if (  matched  )  t.entityprofile[t.entid].materialindex = t.value1;
					//cmpStrConst( t_field_s, "debrisshape" );
					//if (  matched  )  t.entityprofile[t.entid].debrisshapeindex = t.value1;

					//  LOD and BITBOB system
					cmpStrConst( t_field_s, "disablebatch" );
					if (  matched  )  t.entityprofile[t.entid].disablebatch = t.value1;
					cmpStrConst( t_field_s, "lod1distance" );
					if (  matched  )  t.entityprofile[t.entid].lod1distance = t.value1;
					cmpStrConst( t_field_s, "lod2distance" );
					if (  matched  )  t.entityprofile[t.entid].lod2distance = t.value1;

					//  physics setup
					cmpStrConst( t_field_s, "physics" );
					if (  matched  )  t.entityprofile[t.entid].physics = t.value1;
					cmpStrConst( t_field_s, "phyweight" );
					if (  matched  )  t.entityprofile[t.entid].phyweight = t.value1;
					cmpStrConst( t_field_s, "phyfriction" );
					if (  matched  )  t.entityprofile[t.entid].phyfriction = t.value1;
					cmpStrConst( t_field_s, "explodable" );
					if (  matched  )  t.entityprofile[t.entid].explodable = t.value1;
					cmpStrConst( t_field_s, "explodedamage" );
					if (  matched  )  t.entityprofile[t.entid].explodedamage = t.value1;
					cmpStrConst( t_field_s, "ragdoll" );
					if (matched)
					{
						t.entityprofile[t.entid].ragdoll = t.value1;
					}

					//  FPGC - 160511 - added NOTHROWSCRIPT to entity profile
					cmpStrConst( t_field_s, "nothrowscript" );
					if (  matched  )  t.entityprofile[t.entid].nothrowscript = t.value1;

					//  cone of sight
					cmpStrConst( t_field_s, "coneheight" );
					if (  matched  )  t.entityprofile[t.entid].coneheight = t.value1;
					cmpStrConst( t_field_s, "coneangle" );
					if (  matched  )  t.entityprofile[t.entid].coneangle = t.value1;
					cmpStrConst( t_field_s, "conerange" );
					if (  matched  )  t.entityprofile[t.entid].conerange = t.value1;

					//  visual info
					cmpStrConst( t_field_s, "onetexture" );
					if (  matched  )  t.entityprofile[t.entid].onetexture = t.value1;
					if (t.entityprofile[t.entid].ischaractercreator == 0)
					{
						cmpStrConst( t_field_s, "texturepath" );
						if (matched)  t.entityprofile[t.entid].texpath_s = t.value_s;
						cmpStrConst( t_field_s, "textured" );
						if (matched)  t.entityprofile[t.entid].texd_s = t.value_s;
						cmpStrConst( t_field_s, "texturealtd" );
						if (matched)  t.entityprofile[t.entid].texaltd_s = t.value_s;

						#ifdef WICKEDENGINE

						// store thumbnail backdrop image name
						cmpStrConst( t_field_s, "thumbnailbackdrop" ); if (matched) { t.entityprofile[t.entid].thumbnailbackdrop = t.value_s; }
						cmpStrConst( t_field_s, "thumbnailzoom" ); if (matched) { t.entityprofile[t.entid].BackBufferZoom = t.value1_f; }
						cmpStrConst( t_field_s, "thumbnailcamleft" ); if (matched) { t.entityprofile[t.entid].BackBufferCamLeft = t.value1_f; }
						cmpStrConst( t_field_s, "thumbnailcamup" ); if (matched) { t.entityprofile[t.entid].BackBufferCamUp = t.value1_f; }
						cmpStrConst( t_field_s, "thumbnailrotatex" ); if (matched) { t.entityprofile[t.entid].BackBufferRotateX = t.value1_f; }
						cmpStrConst( t_field_s, "thumbnailrotatey" ); if (matched) { t.entityprofile[t.entid].BackBufferRotateY = t.value1_f; }
						cmpStrConst( t_field_s, "thumbnailanimset" ); if (matched) { t.entityprofile[t.entid].iThumbnailAnimset = (int) t.value1_f; }
						cmpStrConst( t_field_s, "keywords" ); if (matched) { t.entityprofile[t.entid].keywords_s = t.value_s; }

						/*
						// custom materials for single material objects
						cmpStrConst( t_field_s, "basecolormap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.baseColorMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "alpharef" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fAlphaRef[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "normalmap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.normalMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "normalstrength" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fNormal[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "surfacemap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.surfaceMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "roughnessstrength" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fRoughness[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "metalnessstrength" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fMetallness[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "displacementmap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.displacementMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "emissivemap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.emissiveMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "emissivestrength" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fEmissive[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						#ifndef DISABLEOCCLUSIONMAP
						cmpStrConst( t_field_s, "occlusionmap" ); if (matched) { t.entityprofile[t.entid].WEMaterial.occlusionMapName[0] = t.value_s; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						#endif

						// global material colors
						cmpStrConst( t_field_s, "emissivecolor" ); 
						if (matched)
						{
							unsigned long ulValue = 0;
							sscanf(t.value_s.Get(), "%lu", &ulValue);
							t.entityprofile[t.entid].WEMaterial.dwEmmisiveColor[0] = ulValue;
							t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
						}
						cmpStrConst( t_field_s, "basecolor" ); 
						if (matched)
						{
							unsigned long ulValue = 0;
							sscanf(t.value_s.Get(), "%lu", &ulValue);
							t.entityprofile[t.entid].WEMaterial.dwBaseColor[0] = ulValue;
							t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
						}

						// global transparency setting
						cmpStrConst( t_field_s, "transparency" ); 
						if (matched)
						{
							t.entityprofile[t.entid].WEMaterial.bTransparency[0] = t.value1_f;
							if (t.entityprofile[t.entid].WEMaterial.bTransparency[0] < 0)
								t.entityprofile[t.entid].WEMaterial.bTransparency[0] = 0;
						}

						// global material values
						cmpStrConst( t_field_s, "doublesided" ); if (matched) { t.entityprofile[t.entid].WEMaterial.bDoubleSided[0] = t.value1_f; }
						cmpStrConst( t_field_s, "renderorderbias" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fRenderOrderBias[0] = t.value1_f; }
						cmpStrConst( t_field_s, "castshadow" ); if (matched) 
						{ 
							if ( t.value1_f == -1 )
								t.entityprofile[t.entid].WEMaterial.bCastShadows[0] = false; 
							else
								t.entityprofile[t.entid].WEMaterial.bCastShadows[0] = true; 
						}
						cmpStrConst( t_field_s, "planerreflection" ); if (matched) { t.entityprofile[t.entid].WEMaterial.bPlanerReflection[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						cmpStrConst( t_field_s, "reflectance" ); if (matched) { t.entityprofile[t.entid].WEMaterial.fReflectance[0] = t.value1_f; t.entityprofile[t.entid].WEMaterial.MaterialActive = true; }
						*/

						cmpNStrConst( t_field_s, "basecolormap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 12 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.baseColorMapName[index] = t.value_s; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "alpharef" );
						if ( matched )
						{
							int index = atoi( t_field_s + 8 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fAlphaRef[index] = t.value1_f; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "normalmap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 9 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.normalMapName[index] = t.value_s; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "normalstrength" );
						if ( matched )
						{
							int index = atoi( t_field_s + 14 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fNormal[index] = t.value1_f; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "surfacemap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 10 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.surfaceMapName[index] = t.value_s;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "roughnessstreng" );
						if ( matched )
						{
							int index = atoi( t_field_s + 17 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fRoughness[index] = t.value1_f;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "metalnessstreng" );
						if ( matched )
						{
							int index = atoi( t_field_s + 17 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fMetallness[index] = t.value1_f;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "displacementmap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 15 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.displacementMapName[index] = t.value_s;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "emissivemap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 11 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.emissiveMapName[index] = t.value_s;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "emissivestrengt" );
						if ( matched )
						{
							int index = atoi( t_field_s + 16 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fEmissive[index] = t.value1_f;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						#ifndef DISABLEOCCLUSIONMAP
						cmpNStrConst( t_field_s, "occlusionmap" );
						if ( matched )
						{
							int index = atoi( t_field_s + 12 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.occlusionMapName[index] = t.value_s;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}
						#endif

						cmpNStrConst( t_field_s, "emissivecolor" );
						if ( matched )
						{
							int index = atoi( t_field_s + 13 );
							if ( index < MAXMESHMATERIALS )
							{
								unsigned long ulValue = 0;
								sscanf(t.value_s.Get(), "%lu", &ulValue);
								t.entityprofile[t.entid].WEMaterial.dwEmmisiveColor[index] = ulValue;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "basecolor" );
						if ( matched && t_field_s[9] != 'm' ) // filter out "basecolormap"
						{
							int index = atoi( t_field_s + 9 );
							if ( index < MAXMESHMATERIALS )
							{
								unsigned long ulValue = 0;
								sscanf(t.value_s.Get(), "%lu", &ulValue);
								t.entityprofile[t.entid].WEMaterial.dwBaseColor[index] = ulValue;
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "transparency" );
						if ( matched )
						{
							int index = atoi( t_field_s + 12 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.bTransparency[index] = t.value1_f;
								if (t.entityprofile[t.entid].WEMaterial.bTransparency[index] < 0)
									t.entityprofile[t.entid].WEMaterial.bTransparency[index] = 0;
							}

							//  transparency modes;
							//  0 - first-phase no alpha
							//  1 - first-phase with alpha masking
							//  2 and 3 - second-phase which overlaps solid geometry
							//  4 - alpha test (only render beyond 0x000000CF alpha values)
							//  5 - water Line (  object (seperates depth sort automatically) )
							//  6 - combination of 3 and 4 (second phase render with alpha blend AND alpha test, used for fading LOD leaves)
							//  7 - very early draw phase no alpha
							if ( t_field_s[12] == 0 )
							{
								if ( t.value1 == 5 ) t.value1 = 6; // 021215 - can only ben one water line
								t.entityprofile[t.entid].transparency = t.value1;
								if (t.entityprofile[t.entid].transparency < 0)
									t.entityprofile[t.entid].transparency = 0;
							}
						}

						cmpNStrConst( t_field_s, "doublesided" );
						if ( matched )
						{
							int index = atoi( t_field_s + 11 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.bDoubleSided[index] = t.value1_f;
							}
						}

						cmpNStrConst( t_field_s, "renderorderbias" );
						if ( matched )
						{
							int index = atoi( t_field_s + 15 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fRenderOrderBias[index] = t.value1_f;
							}
						}

						cmpNStrConst( t_field_s, "castshadow" );
						if ( matched )
						{
							int index = atoi( t_field_s + 10 );
							if ( index < MAXMESHMATERIALS )
							{
								if ( t.value1_f == - 1 )
									t.entityprofile[t.entid].WEMaterial.bCastShadows[index] = false; 
								else
									t.entityprofile[t.entid].WEMaterial.bCastShadows[index] = true; 
							}
						}

						cmpNStrConst( t_field_s, "planerreflectio" );
						if ( matched )
						{
							int index = atoi( t_field_s + 16 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.bPlanerReflection[index] = t.value1_f; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}

						cmpNStrConst( t_field_s, "reflectance" );
						if ( matched )
						{
							int index = atoi( t_field_s + 11 );
							if ( index < MAXMESHMATERIALS )
							{
								t.entityprofile[t.entid].WEMaterial.fReflectance[index] = t.value1_f; 
								t.entityprofile[t.entid].WEMaterial.MaterialActive = true;
							}
						}
						#endif
					}

					cmpStrConst( t_field_s, "effect" );
					if (  matched  )  t.entityprofile[t.entid].effect_s = t.value_s;

					// effectprofile:
					// 0 - default non-PBR
					// 1 - new PBR texture arrangement
					cmpStrConst( t_field_s, "effectprofile" );
					if ( matched ) t.entityprofile[t.entid].effectprofile = t.value1;

					cmpStrConst( t_field_s, "canseethrough" );
					if (  matched  )  t.entityprofile[t.entid].canseethrough = t.value1;

					// specular:
					// 0 - uses _S texture
					// 1 - uses effectbank\\reloaded\\media\\blank_none_S.dds
					// 2 - uses effectbank\\reloaded\\media\\blank_low_S.dds
					// 3 - uses effectbank\\reloaded\\media\\blank_medium_S.dds
					// 4 - uses effectbank\\reloaded\\media\\blank_high_S.dds
					cmpStrConst( t_field_s, "specular" );
					if (  matched  )  t.entityprofile[t.entid].specular = t.value1;
					// specularperc:
					// percentage 0 to 100 to modulate how much global specular gets to individual entity
					cmpStrConst( t_field_s, "specularperc" );
					if (  matched  )  t.entityprofile[t.entid].specularperc = t.value1;

					// can scale the uv data inside the shader
					cmpStrConst( t_field_s, "uvscroll" );
					if (  matched  ) { t.entityprofile[t.entid].uvscrollu = t.value1/100.0f; t.entityprofile[t.entid].uvscrollv = t.value2/100.0f; }
					cmpStrConst( t_field_s, "uvscale" );
					if (  matched  ) { t.entityprofile[t.entid].uvscaleu = t.value1/100.0f; t.entityprofile[t.entid].uvscalev = t.value2/100.0f; }

					// can invert the normal, or set to zero to not invert (not inverted by default)
					cmpStrConst( t_field_s, "invertnormal" );
					if (  matched  )  t.entityprofile[t.entid].invertnormal = t.value1;

					// can choose whether to generate tangent/binormal in the shader
					cmpStrConst( t_field_s, "preservetangents" );
					if (  matched  )  t.entityprofile[t.entid].preservetangents = t.value1;
					
					cmpStrConst( t_field_s, "zdepth" );
					if (  matched  )  t.entityprofile[t.entid].zdepth = t.value1;

					cmpStrConst( t_field_s, "cullmode" );
					if (  matched  )  t.entityprofile[t.entid].cullmode = t.value1;
					cmpStrConst( t_field_s, "reducetexture" );
					if (  matched  )  t.entityprofile[t.entid].reducetexture = t.value1;

					// castshadow:
					//  0 = default shadow caster mode
					// -1 = do not cast shadows or lightmap shadows
					cmpStrConst( t_field_s, "castshadow" );
					if (  matched  )  t.entityprofile[t.entid].castshadow = t.value1;
					cmpStrConst( t_field_s, "smoothangle" );
					if (  matched  )  t.entityprofile[t.entid].smoothangle = t.value1;

					//  entity identity details
					cmpStrConst( t_field_s, "strength" );
					if (  matched  )  t.entityprofile[t.entid].strength = t.value1;
					cmpStrConst( t_field_s, "lives" );
					if (  matched  )  t.entityprofile[t.entid].lives = t.value1;
					cmpStrConst( t_field_s, "speed" );
					if (  matched  )  t.entityprofile[t.entid].speed = t.value1;
					cmpStrConst( t_field_s, "animspeed" );
					if (  matched  )  t.entityprofile[t.entid].animspeed = t.value1;
					cmpStrConst( t_field_s, "hurtfall" );
					if (  matched  )  t.entityprofile[t.entid].hurtfall = t.value1;

					cmpStrConst( t_field_s, "isimmobile" );
					if (  matched  )  t.entityprofile[t.entid].isimmobile = t.value1;
					cmpStrConst( t_field_s, "isviolent" );
					if (  matched  )  t.entityprofile[t.entid].isviolent = t.value1;
					cmpStrConst( t_field_s, "isobjective" );
					if (  matched  )  t.entityprofile[t.entid].isobjective = t.value1;
					cmpStrConst( t_field_s, "alwaysactive" );
					if (  matched  )  t.entityprofile[t.entid].phyalways = t.value1;

					cmpStrConst( t_field_s, "ischaracter" );
					if (  matched  )  t.entityprofile[t.entid].ischaracter = t.value1;
					cmpStrConst( t_field_s, "isspinetracker" );
					if (  matched  )  t.entityprofile[t.entid].isspinetracker = t.value1;
					
					cmpStrConst( t_field_s, "noxzrotation" );
					if (  matched  )  t.entityprofile[t.entid].noXZrotation = t.value1;				
					cmpStrConst( t_field_s, "canfight" );
					if (  matched  )  t.entityprofile[t.entid].canfight = t.value1;

					cmpStrConst( t_field_s, "jumpmodifier" );
					if (  matched  )  t.entityprofile[t.entid].fJumpModifier = (float)t.value1 / 100.0f;
					cmpStrConst( t_field_s, "jumphold" );
					if (  matched  )  t.entityprofile[t.entid].jumphold = t.value1;
					cmpStrConst( t_field_s, "jumpresume" );
					if (  matched  )  t.entityprofile[t.entid].jumpresume = t.value1;
					cmpStrConst( t_field_s, "jumpvaulttrim" );
					if (  matched  )  t.entityprofile[t.entid].jumpvaulttrim = t.value1;

					cmpStrConst( t_field_s, "meleerange" );
					if (  matched  )  t.entityprofile[t.entid].meleerange = t.value1;
					cmpStrConst( t_field_s, "meleehitangle" );
					if (  matched  )  t.entityprofile[t.entid].meleehitangle = t.value1;
					cmpStrConst( t_field_s, "meleestrikest" );
					if (  matched  )  t.entityprofile[t.entid].meleestrikest = t.value1;
					cmpStrConst( t_field_s, "meleestrikefn" );
					if (  matched  )  t.entityprofile[t.entid].meleestrikefn = t.value1;
					cmpStrConst( t_field_s, "meleedamagest" );
					if (  matched  )  t.entityprofile[t.entid].meleedamagest = t.value1;
					cmpStrConst( t_field_s, "meleedamagefn" );
					if (  matched  )  t.entityprofile[t.entid].meleedamagefn = t.value1;

					cmpStrConst( t_field_s, "custombiped" );
					if (  matched  )  t.entityprofile[t.entid].custombiped = t.value1;

					cmpStrConst( t_field_s, "cantakeweapon" );
					if (  matched  )  t.entityprofile[t.entid].cantakeweapon = t.value1;
					cmpStrConst( t_field_s, "isweapon" );
					if (  matched  )  t.entityprofile[t.entid].isweapon_s = t.value_s;
					cmpStrConst( t_field_s, "rateoffire" );
					if (  matched  )  t.entityprofile[t.entid].rateoffire = t.value1;
					cmpStrConst( t_field_s, "ishudlayer" );
					if (  matched  )  t.entityprofile[t.entid].ishudlayer_s = t.value_s;

					//  fpgc - same internals just sanitized  -NEED TO GO IN EDITOR TOO!
					cmpStrConst( t_field_s, "isequipment" );
					if (  matched ) 
					{
						t.entityprofile[t.entid].isweapon_s=t.value_s;
						if (  Len(t.value_s.Get())>2 ) 
						{
							//  FPGC - 280809 - if equipment specified, this entity is ALWAYS ACTIVE (so can pickup AND DROP the item)
							t.entityprofile[t.entid].phyalways=1;
						}
					}

					cmpStrConst( t_field_s, "isammo" );
					if (  matched  )  t.entityprofile[t.entid].isammo = t.value1;
					cmpStrConst( t_field_s, "hasweapon" );
					if (  matched  )  t.entityprofile[t.entid].hasweapon_s = t.value_s;
					cmpStrConst(t_field_s, "ammopool");
					if (matched)  
						t.entityprofile[t.entid].ammopool_s = t.value_s;
					#ifdef VRTECH
					#else
					cmpStrConst( t_field_s, "handfirespotoffx" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotoffx = t.value1;
					cmpStrConst( t_field_s, "handfirespotoffy" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotoffy = t.value1;
					cmpStrConst( t_field_s, "handfirespotoffz" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotoffz = t.value1;
					cmpStrConst( t_field_s, "handfirespotrotx" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotrotx = t.value1;
					cmpStrConst( t_field_s, "handfirespotroty" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotroty = t.value1;
					cmpStrConst( t_field_s, "handfirespotrotz" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotrotz = t.value1;
					cmpStrConst( t_field_s, "handfirespotsize" );
					if (  matched  )  t.entityprofile[t.entid].handfirespotsize = t.value1;
					#endif
					cmpStrConst( t_field_s, "hasequipment" );
					if (  matched  )  t.entityprofile[t.entid].hasweapon_s = t.value_s;
					cmpStrConst( t_field_s, "ishealth" );
					if (  matched  )  t.entityprofile[t.entid].ishealth = t.value1;
					cmpStrConst( t_field_s, "isflak" );
					if (  matched  )  t.entityprofile[t.entid].isflak = t.value1;
					cmpStrConst( t_field_s, "fatness" );
					if (  matched  )  t.entityprofile[t.entid].fatness = t.value1;

					//  marker extras
					//  1=player
					//  2=lights
					//  3=trigger zone
					//  4=decal particle emitter
					//  5=entity lights
					//  6=checkpoint zone
					//  7=multiplayer start
					//  8=floor zone
					//  9=cover zone
					//  10=new particle emitter (particle editor export)
					cmpStrConst( t_field_s, "ismarker" );
					if (  matched  )  
					{
						t.entityprofile[t.entid].ismarker = t.value1;
						if ( t.entityprofile[t.entid].ismarker > 0 && t.entityprofile[t.entid].ismarker != 2 )
						{
							// force zone and arrow markers to rise above terrain if planted there
							t.entityprofile[t.entid].offyoverride = 1;
							t.entityprofile[t.entid].offy = 2;
						}
					}
					cmpStrConst( t_field_s, "markerindex" );
					if (  matched  )  t.entityprofile[t.entid].markerindex = t.value1;
					cmpStrConst( t_field_s, "addhandle" );
					if (  matched  )  t.entityprofile[t.entid].addhandle_s = t.value_s;

					// ebe builder extras
					cmpStrConst( t_field_s, "isebe" );
					if (  matched  )  t.entityprofile[t.entid].isebe = t.value1;

					//  light extras
					cmpStrConst( t_field_s, "lightcolor" );
					if (  matched  )  t.entityprofile[t.entid].light.color = t.value1;
					cmpStrConst( t_field_s, "lightrange" );
					if (  matched  )  t.entityprofile[t.entid].light.range = t.value1;
					cmpStrConst( t_field_s, "lightoffsetup" );
					if (  matched  )  t.entityprofile[t.entid].light.offsetup = t.value1;
					cmpStrConst( t_field_s, "lightoffsetz" );
					if (  matched  )  t.entityprofile[t.entid].light.offsetz = t.value1;
					#ifdef WICKEDENGINE
					cmpStrConst( t_field_s, "lightprobescale" );
					if (matched)  t.entityprofile[t.entid].light.fLightHasProbe = t.value1;
					#endif

					// light type flags
					cmpStrConst( t_field_s, "usespotlighting" );
					if (  matched  )  t.entityprofile[t.entid].usespotlighting = t.value1;

					//  trigger extras
					cmpStrConst( t_field_s, "stylecolor" );
					if (  matched  )  t.entityprofile[t.entid].trigger.stylecolor = t.value1;

					//  extra decal offset (ideal for placing flames in torches, etc)
					cmpStrConst( t_field_s, "decalangle" );
					if (  matched  )  t.entityprofile[t.entid].decaloffsetangle = t.value1;
					cmpStrConst( t_field_s, "decaldist" );
					if (  matched  )  t.entityprofile[t.entid].decaloffsetdist = t.value1/10.0;
					cmpStrConst( t_field_s, "decaly" );
					if (  matched  )  t.entityprofile[t.entid].decaloffsety = t.value1/10.0;

					//  entity body part list (20/01/11 - refeatured for V118)
					cmpStrConst( t_field_s, "limbmax" );
					if (  matched  )  
					{
						t.entityprofile[t.entid].limbmax = t.value1;
						if (  t.entityprofile[t.entid].limbmax>100  )  t.entityprofile[t.entid].limbmax = 100;
					}

					if (  t.entityprofile[t.entid].limbmax>0 ) 
					{
						cmpNStrConst( t_field_s, "limb" );
						if (  matched  )  
						{
							int limbNum = atoi( t_field_s+4 );
							if ( limbNum < t.entityprofile[t.entid].limbmax )
							{
								if ( limbNum == 0 )
								{
									if ( t_field_s[4] == '0' && t_field_s[5] == 0 ) t.entitybodypart[t.entid][ limbNum ] = t.value1;
								}
								else if ( limbNum < 10 )
								{
									if ( t_field_s[5] == 0 ) t.entitybodypart[t.entid][ limbNum ] = t.value1;
								}
								else if ( limbNum < 100 )
								{
									if ( t_field_s[6] == 0 ) t.entitybodypart[t.entid][ limbNum ] = t.value1;
								}
								else if ( limbNum < 1000 )
								{
									if ( t_field_s[7] == 0 ) t.entitybodypart[t.entid][ limbNum ] = t.value1;
								}
							}
						}
						matched = false; // prevent skipping other strings that start with "limb"
					}

					// read in head and spine tracker settings for this character (if any)
					cmpStrConst( t_field_s, "headhlimit" ); if (matched) { t.entityprofile[t.entid].headspinetracker.headhlimit = t.value1; }
					cmpStrConst( t_field_s, "headhoffset" ); if (matched) { t.entityprofile[t.entid].headspinetracker.headhoffset = t.value1; }
					cmpStrConst( t_field_s, "headvlimit" ); if (matched) { t.entityprofile[t.entid].headspinetracker.headvlimit = t.value1; }
					cmpStrConst( t_field_s, "headvoffset" ); if (matched) { t.entityprofile[t.entid].headspinetracker.headvoffset = t.value1; }
					cmpStrConst( t_field_s, "spinehlimit" ); if (matched) { t.entityprofile[t.entid].headspinetracker.spinehlimit = t.value1; }
					cmpStrConst( t_field_s, "spinehoffset" ); if (matched) { t.entityprofile[t.entid].headspinetracker.spinehoffset = t.value1; }
					cmpStrConst( t_field_s, "spinevlimit" ); if (matched) { t.entityprofile[t.entid].headspinetracker.spinevlimit = t.value1; }
					cmpStrConst( t_field_s, "spinevoffset" ); if (matched) { t.entityprofile[t.entid].headspinetracker.spinevoffset = t.value1; }

					//  determine if entity has a head, and which limbs represent it
					cmpStrConst( t_field_s, "headlimbs" );
					if (matched) { t.entityprofile[t.entid].headframestart = t.value1; t.entityprofile[t.entid].headframefinish = t.value2; }

					//  determine if entity has hair, and which limbs represent it/them
					cmpStrConst( t_field_s, "hairlimbs" );
					if (  matched ) { t.entityprofile[t.entid].hairframestart = t.value1; t.entityprofile[t.entid].hairframefinish = t.value2; }

					//  determine if entity has limbs to hide
					cmpStrConst( t_field_s, "hidelimbs" );
					if (  matched ) { t.entityprofile[t.entid].hideframestart = t.value1; t.entityprofile[t.entid].hideframefinish = t.value2; }

					//  entity decal refs
					cmpStrConst( t_field_s, "decalmax" );
					if (  matched  )  t.entityprofile[t.entid].decalmax = t.value1;
					if (  t.entityprofile[t.entid].decalmax>0 ) 
					{
						cmpNStrConst( t_field_s, "decal" );
						if (  matched  )  
						{
							const int rootindex = 5;
							bool bValid = false;
							int index = atoi( t_field_s+rootindex );
							if ( index == 0 )
							{
								if ( t_field_s[rootindex] == '0' && t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 10 )
							{
								if ( t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 100 )
							{
								if ( t_field_s[rootindex+2] == 0 ) bValid = true;
							}
							else if ( index < 1000 )
							{
								if ( t_field_s[rootindex+3] == 0 ) bValid = true;
							}
							else if ( index < 10000 )
							{
								if ( t_field_s[rootindex+4] == 0 ) bValid = true;
							}

							if ( bValid && index < t.entityprofile[t.entid].decalmax )
							{
								t.entitydecal_s[t.entid][index] = t.value_s;
							}
						}
					}

					// 060718 - entity append anim system
					cmpStrConst( t_field_s, "appendanimfinal" );
					if (  matched )
					{ 
						t.entityappendanim[t.entid][0].filename = t.value_s; 
						t.entityappendanim[t.entid][0].startframe = 0;
					}
					cmpStrConst( t_field_s, "appendanimmax" );
					if ( matched ) 
					{
						t.entityprofile[t.entid].appendanimmax = t.value1; 
						if ( t.entityprofile[t.entid].appendanimmax > 99 ) 
							t.entityprofile[t.entid].appendanimmax = 99;
					}

					//PE: Hanging, in my case: appendanimmax=573444874 value_s=road_straight01.x
					//PE: Hang if you are unlucky and get mem that "appendanimmax" are not already set to zero.
					if ( t.entityprofile[t.entid].appendanimmax > 0 && t.entityprofile[t.entid].appendanimmax <= 99 )
					{
						cmpNStrConst( t_field_s, "appendanimframe" );
						if (  matched  )  
						{
							int index = atoi( t_field_s+15 );
							if ( index != 0 && index < t.entityprofile[t.entid].appendanimmax )
							{
								if ( index < 10 )
								{
									if ( t_field_s[16] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
								}
								else if ( index < 100 )
								{
									if ( t_field_s[17] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
								}
								else if ( index < 1000 )
								{
									if ( t_field_s[18] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
								}
							}
						}
						else
						{
							cmpNStrConst( t_field_s, "appendanim" );
							if (  matched  )  
							{
								int index = atoi( t_field_s+10 );
								if ( index != 0 && index < t.entityprofile[t.entid].appendanimmax )
								{
									if ( index < 10 )
									{
										if ( t_field_s[11] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
									}
									else if ( index < 100 )
									{
										if ( t_field_s[12] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
									}
									else if ( index < 1000 )
									{
										if ( t_field_s[13] == 0 ) t.entityappendanim[t.entid][index].filename = t.value_s;
									}
								}
							}
						}
					}

					cmpStrConst( t_field_s, "drawcalloptimizer" );
					if (matched)  t.entityprofile[t.entid].drawcalloptimizer = t.value1;
					cmpStrConst( t_field_s, "drawcalloptimizeroff" );
					if (matched)  t.entityprofile[t.entid].drawcalloptimizeroff = t.value1;
					cmpStrConst( t_field_s, "drawcallscaleadjust" );
					if (matched)  t.entityprofile[t.entid].drawcallscaleadjust = t.value1;

					#ifdef VRTECH
					#else
					cmpStrConst( t_field_s, "startanimingame" );
					if (matched)  t.entityprofile[t.entid].startanimingame = t.value1;
					#endif

					//  entity animation sets
					cmpStrConst( t_field_s, "ignorecsirefs" );
					if (  matched  )  t.entityprofile[t.entid].ignorecsirefs = t.value1;
					cmpStrConst( t_field_s, "playanimineditor" );
					if (matched)
					{
						// work out correct anim to play in editor
						int iRealPlayAnimInEditorIndex = t.value1;
						LPSTR pNumString = t.value_s.Get();
						int iIsAPureNumeric = 1;
						for (int n = 0; n < strlen(pNumString); n++)
						{
							if (pNumString[n] >= '0' && pNumString[n] <= '9')
							{
								// can have numbers in a name string
							}
							else
							{
								// found a non number, cannot be a numeric
								iIsAPureNumeric = 0;
								break;
							}
						}
						if (iIsAPureNumeric == 0 && strlen(t.value_s.Get())>0)
						{
							// not a numeric, is an anim name we can search for	
							t.entityprofile[t.entid].playanimineditor_name = t.value_s;
							t.entityprofile[t.entid].playanimineditor = -1;
							t.entityprofile[t.entid].startanimingame = 0;
						}
						else
						{
							// editor uses this to play
							t.entityprofile[t.entid].playanimineditor = iRealPlayAnimInEditorIndex;
							t.entityprofile[t.entid].playanimineditor_name = "";
							// startanimingame is still used in standalone.
							t.entityprofile[t.entid].startanimingame = iRealPlayAnimInEditorIndex;
						}
					}
					cmpStrConst( t_field_s, "animstyle" );
					if (  matched  )  
						t.entityprofile[t.entid].animstyle = t.value1;
					cmpStrConst( t_field_s, "animmax" );
					if (  matched ) 
					{
						t.tnewanimmax=t.value1 ; t.tstartofaianim=t.tnewanimmax;
					}
					if (  t.tnewanimmax>0 ) 
					{
						cmpNStrConst( t_field_s, "anim" );
						if (  matched  )  
						{
							const int rootindex = 4;
							int index = atoi( t_field_s+rootindex );
							bool bValid = false;
							if ( index == 0 )
							{
								if ( t_field_s[rootindex] == '0' && t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 10 )
							{
								if ( t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 100 )
							{
								if ( t_field_s[rootindex+2] == 0 ) bValid = true;
							}
							else if ( index < 1000 )
							{
								if ( t_field_s[rootindex+3] == 0 ) bValid = true;
							}
							else if ( index < 10000 )
							{
								if ( t_field_s[rootindex+4] == 0 ) bValid = true;
							}

							if ( bValid && index < t.tstartofaianim )
							{
								t.entityanim[t.entid][index].start = t.value1 ; 
								t.entityanim[t.entid][index].finish = t.value2 ; 
								t.entityanim[t.entid][index].found = 1;
							}
						}
						matched = false; // prevent skipping other strings that start with "anim"
					}

					// 291014 - AI system animation sets (takes field$ and value1/value2)
					if ( 1 ) // t.entityprofile[t.entid].ignorecsirefs == 0 ) // for now, still need these for THIRD PERSON which uses old CSI system
					{
						// 200918 - externalised internal AI system into scripts, but keeping for legacy support
						darkai_assignanimtofield ( );
					}

					// get foot fall data (optional)
					cmpStrConst( t_field_s, "footfallmax" );
					if (  matched  )  t.entityprofile[t.entid].footfallmax = t.value1;
					if (  t.entityprofile[t.entid].footfallmax>0 ) 
					{
						cmpNStrConst( t_field_s, "footfall" );
						if (  matched  )  
						{
							const int rootindex = 8;
							int index = atoi( t_field_s+rootindex );
							bool bValid = false;
							if ( index == 0 )
							{
								if ( t_field_s[rootindex] == '0' && t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 10 )
							{
								if ( t_field_s[rootindex+1] == 0 ) bValid = true;
							}
							else if ( index < 100 )
							{
								if ( t_field_s[rootindex+2] == 0 ) bValid = true;
							}
							else if ( index < 1000 )
							{
								if ( t_field_s[rootindex+3] == 0 ) bValid = true;
							}
							else if ( index < 10000 )
							{
								if ( t_field_s[rootindex+4] == 0 ) bValid = true;
							}

							if ( bValid && index < t.entityprofile[t.entid].footfallmax )
							{
								t.entityfootfall[t.entid][index].leftfootkeyframe = t.value1; 
								t.entityfootfall[t.entid][index].rightfootkeyframe = t.value2;
							}
						}
					}

					//  more data
					cmpStrConst( t_field_s, "quantity" );
					if (  matched  )  t.entityprofile[t.entid].quantity = t.value1;

					//  character creator
					#ifdef VRTECH
					#else
					cmpStrConst( t_field_s, "charactercreator" );
					if (  matched ) 
					{
						t.entityprofile[t.entid].ischaractercreator=1;
						t.entityprofile[t.entid].charactercreator_s=t.value_s;
						t.tstring_s = t.value_s;
						//  "v"
						t.tnothing_s = FirstToken(t.tstring_s.Get(),":");
						//  "version number"
						t.tnothing_s = NextToken(":");
						//  body mesh
						t.tbody_s = NextToken(":");

						//  read in cci
						t.tcciloadname_s = g.fpscrootdir_s+"\\Files\\characterkit\\bodyandhead\\" + t.tbody_s + ".cci";
						t.tpath_s = "characterkit\\bodyandhead\\";
						t.tccquick = 1;
						characterkit_loadCCI ( );
						t.entityprofile[t.entid].model_s = t.tccimesh_s;
						t.entityprofile[t.entid].texpath_s = t.tpath_s;
						t.entityprofile[t.entid].texd_s = t.tccidiffuse_s;
					}
					#endif

					//  physics objects from the importer
					cmpStrConst( t_field_s, "physicscount" );
					if (  matched  )  t.entityprofile[t.entid].physicsobjectcount = t.value1;

					if (  cstr(Left(t.field_s.Get(),7)) == "physics" && t.field_s != "physicscount" && t.tPhysObjCount < MAX_ENTITY_PHYSICS_BOXES ) 
					{

						Dim (  t.tArray,10 );

							//  get rid of the quotation marks
							t.tStrip_s = t.value_s;
							t.tStrip_s = Left(t.tStrip_s.Get(), Len(t.tStrip_s.Get())-1);
							t.tStrip_s = Right(t.tStrip_s.Get(), Len(t.tStrip_s.Get())-1);

							t.tArrayMarker = 0;
							t.ttToken_s=FirstToken(t.tStrip_s.Get(),",");
							//PE: Make sure we only run it if we find a token.
							//PE: https://github.com/TheGameCreators/GameGuruRepo/issues/979

							if (t.ttToken_s != "")
							{
								t.tArray[t.tArrayMarker] = t.ttToken_s;
								++t.tArrayMarker;

								do
								{
									t.ttToken_s = NextToken(",");
									if (t.ttToken_s != "")
									{
										t.tArray[t.tArrayMarker] = t.ttToken_s;
										++t.tArrayMarker;
									}
								} while (!(t.ttToken_s == ""));

								//  Format; shapetype, sizex, sizey, sizez, offx, offy, offz, rotx, roty, rotz
								t.tPShapeType = ValF(t.tArray[0].Get());
								//  is it a box?
								if (t.tPShapeType == 0)
								{
									//Dave Crash fix - check we are not going out of bounds
									if (t.entid < MAX_ENTITY_PHYSICS_BOXES * 2)
									{
										t.entityphysicsbox[t.entid][t.tPhysObjCount].SizeX = ValF(t.tArray[1].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].SizeY = ValF(t.tArray[2].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].SizeZ = ValF(t.tArray[3].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].OffX = ValF(t.tArray[4].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].OffY = ValF(t.tArray[5].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].OffZ = ValF(t.tArray[6].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].RotX = ValF(t.tArray[7].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].RotY = ValF(t.tArray[8].Get());
										t.entityphysicsbox[t.entid][t.tPhysObjCount].RotZ = ValF(t.tArray[9].Get());
									}
									++t.tPhysObjCount;
								}

							}

						UnDim (  t.tArray );

					}

				}
			}
		}
		UnDim (  t.data_s );

		for ( int i = 0 ; i <= t.tstartofaianim-1; i++ )
		{
			if ( t.entityanim[t.entid][i].found == 0 ) 
			{ 
				t.entityanim[t.entid][i].start = -1; 
				t.entityanim[t.entid][i].finish = -1; 
			}
		}

		#ifdef WICKEDENGINE
		if (t.entityprofile[t.entid].ismarker == 2) 
		{
			//PE: LMFIX - Light , default to 100.
			if (t.entityprofile[t.entid].defaultheight == 155)
				t.entityprofile[t.entid].defaultheight = 80;
		}
		#endif

		#ifdef WICKEDENGINE
		// new MAX system does not use hard coded or CSI stuff!
		#else
		// 200918 - no longer use internal AI system, but keep for legacy compatibility
		if ( 1 ) //t.entityprofile[t.entid].ignorecsirefs == 0 ) // for now, still need these for THIRD PERSON which uses old CSI system
		{
			// if No AI anim sets, fill with hard defaults from official template character
			if (  t.entityprofile[t.entid].ischaracter == 1 ) 
			{
				// 010917 - but only if a character, so as not to force non-anim entities to use entity_anim
				for ( t.n = 1 ; t.n<=  90; t.n++ )
				{
					if (  t.n == 1 ) { t.field_s = "csi_relaxed1"  ; t.value1 = 900 ; t.value2 = 999; }
					if (  t.n == 2 ) { t.field_s = "csi_relaxed2"  ; t.value1 = 1000 ; t.value2 = 1282; }
					if (  t.n == 3 ) { t.field_s = "csi_relaxedmovefore"  ; t.value1 = 1290 ; t.value2 = 1419; }
					if (  t.n == 4 ) { t.field_s = "csi_cautious"  ; t.value1 = 900 ; t.value2 = 999; }
					if (  t.n == 5 ) { t.field_s = "csi_cautiousmovefore"  ; t.value1 = 1325 ; t.value2 = 1419; }
					if (  t.n == 6 ) { t.field_s = "csi_unarmed1"  ; t.value1 = 3000 ; t.value2 = 3100; }
					if (  t.n == 7 ) { t.field_s = "csi_unarmed2"  ; t.value1 = 3430 ; t.value2 = 3697; }
					if (  t.n == 8 ) { t.field_s = "csi_unarmedconversation"  ; t.value1 = 3110 ; t.value2 = 3420; }
					if (  t.n == 10 ) { t.field_s = "csi_unarmedexplain"  ; t.value1 = 4260 ; t.value2 = 4464; }
					if (  t.n == 11 ) { t.field_s = "csi_unarmedpointfore"  ; t.value1 = 4470 ; t.value2 = 4535; }
					if (  t.n == 12 ) { t.field_s = "csi_unarmedpointback"  ; t.value1 = 4680 ; t.value2 = 4745; }
					if (  t.n == 13 ) { t.field_s = "csi_unarmedpointleft"  ; t.value1 = 4610 ; t.value2 = 4675; }
					if (  t.n == 14 ) { t.field_s = "csi_unarmedpointright"  ; t.value1 = 4540 ; t.value2 = 4605; }
					if (  t.n == 15 ) { t.field_s = "csi_unarmedmovefore"  ; t.value1 = 3870 ; t.value2 = 3900; }
					if (  t.n == 16 ) { t.field_s = "csi_unarmedmoverun"  ; t.value1 = 3905 ; t.value2 = 3925; }
					if (  t.n == 17 ) { t.field_s = "csi_unarmedstairascend"  ; t.value1 = 5600 ; t.value2 = 5768; }
					if (  t.n == 18 ) { t.field_s = "csi_unarmedstairdecend"  ; t.value1 = 5800 ; t.value2 = 5965; }
					if (  t.n == 19 ) { t.field_s = "csi_unarmedladderascend1"  ; t.value1 = 4148 ; t.value2 = 4110; }
					if (  t.n == 20 ) { t.field_s = "csi_unarmedladderascend2"  ; t.value1 = 4148 ; t.value2 = 4255; }
					if (  t.n == 21 ) { t.field_s = "csi_unarmedladderascend3"  ; t.value1 = 4225 ; t.value2 = 4255; }
					if (  t.n == 22 ) { t.field_s = "csi_unarmedladderdecend1"  ; t.value1 = 4255 ; t.value2 = 4225; }
					if (  t.n == 23 ) { t.field_s = "csi_unarmedladderdecend2"  ; t.value1 = 4225 ; t.value2 = 4148; }
					if (  t.n == 24 ) { t.field_s = "csi_unarmeddeath"  ; t.value1 = 4800 ; t.value2 = 4958; }
					if (  t.n == 25 ) { t.field_s = "csi_unarmedimpactfore"  ; t.value1 = 4971 ; t.value2 = 5021; }
					if (  t.n == 26 ) { t.field_s = "csi_unarmedimpactback"  ; t.value1 = 5031 ; t.value2 = 5090; }
					if (  t.n == 27 ) { t.field_s = "csi_unarmedimpactleft"  ; t.value1 = 5171 ; t.value2 = 5229; }
					if (  t.n == 28 ) { t.field_s = "csi_unarmedimpactright"  ; t.value1 = 5101 ; t.value2 = 5160; }
					if (  t.n == 29 ) { t.field_s = "csi_inchair"  ; t.value1 = 3744 ; t.value2 = 3828; }
					if (  t.n == 30 ) { t.field_s = "csi_inchairsit"  ; t.value1 = 3710 ; t.value2 = 3744; }
					if (  t.n == 31 ) { t.field_s = "csi_inchairgetup"  ; t.value1 = 3828 ; t.value2 = 3862; }
					if (  t.n == 32 ) { t.field_s = "csi_swim"  ; t.value1 = 3930 ; t.value2 = 4015; }
					if (  t.n == 33 ) { t.field_s = "csi_swimmovefore"  ; t.value1 = 4030 ; t.value2 = 4072; }
					if (  t.n == 34 ) { t.field_s = "csi_stoodnormal"  ; t.value1 = 100 ; t.value2 = 205; }
					if (  t.n == 35 ) { t.field_s = "csi_stoodrocket"  ; t.value1 = 6133 ; t.value2 = 6206; }
					if (  t.n == 36 ) { t.field_s = "csi_stoodfidget1"  ; t.value1 = 100 ; t.value2 = 205; }
					if (  t.n == 37 ) { t.field_s = "csi_stoodfidget2"  ; t.value1 = 210 ; t.value2 = 318; }
					if (  t.n == 38 ) { t.field_s = "csi_stoodfidget3"  ; t.value1 = 325 ; t.value2 = 431; }
					if (  t.n == 39 ) { t.field_s = "csi_stoodfidget4"  ; t.value1 = 440 ; t.value2 = 511; }
					if (  t.n == 40 ) { t.field_s = "csi_stoodstartled"  ; t.value1 = 1425 ; t.value2 = 1465; }
					if (  t.n == 41 ) { t.field_s = "csi_stoodpunch"  ; t.value1 = 0 ; t.value2 = 0; }
					if (  t.n == 42 ) { t.field_s = "csi_stoodkick"  ; t.value1 = 5511 ; t.value2 = 5553; }
					if (  t.n == 43 ) { t.field_s = "csi_stoodmovefore"  ; t.value1 = 685 ; t.value2 = 707; }
					if (  t.n == 44 ) { t.field_s = "csi_stoodmoveback"  ; t.value1 = 710 ; t.value2 = 735; }
					if (  t.n == 45 ) { t.field_s = "csi_stoodmoveleft"  ; t.value1 = 740 ; t.value2 = 762; }
					if (  t.n == 46 ) { t.field_s = "csi_stoodmoveright"  ; t.value1 = 765 ; t.value2 = 789; }
					if (  t.n == 47 ) { t.field_s = "csi_stoodstepleft"  ; t.value1 = 610 ; t.value2 = 640; }
					if (  t.n == 48 ) { t.field_s = "csi_stoodstepright"  ; t.value1 = 645 ; t.value2 = 676; }
					if (  t.n == 49 ) { t.field_s = "csi_stoodstrafeleft"  ; t.value1 = 855 ; t.value2 = 871; }
					if (  t.n == 50 ) { t.field_s = "csi_stoodstraferight"  ; t.value1 = 875 ; t.value2 = 892; }
					//  51 see below
					//  reserved 52
					if (  t.n == 53 ) { t.field_s = "csi_stoodvault"  ; t.value1 = 0 ; t.value2 = 0; } // 220217 - these now need to come from FPE
					if (  t.n == 54 ) { t.field_s = "csi_stoodmoverun"  ; t.value1 = 795 ; t.value2 = 811; }
					if (  t.n == 55 ) { t.field_s = "csi_stoodmoverunleft"  ; t.value1 = 815 ; t.value2 = 830; }
					if (  t.n == 56 ) { t.field_s = "csi_stoodmoverunright"  ; t.value1 = 835 ; t.value2 = 850; }
					if (  t.n == 57 ) { t.field_s = "csi_stoodreload"  ; t.value1 = 515 ; t.value2 = 605; }
					if (  t.n == 58 ) { t.field_s = "csi_stoodreloadrocket"  ; t.value1 = 6233 ; t.value2 = 6315; }
					if (  t.n == 59 ) { t.field_s = "csi_stoodwave"  ; t.value1 = 1470 ; t.value2 = 1520; }
					if (  t.n == 60 ) { t.field_s = "csi_stoodtoss"  ; t.value1 = 2390 ; t.value2 = 2444; }
					if (  t.n == 61 ) { t.field_s = "csi_stoodfirerocket"  ; t.value1 = 6207 ; t.value2 = 6232; }
					if (  t.n == 62 ) { t.field_s = "csi_stoodincoverleft"  ; t.value1 = 1580 ; t.value2 = 1580; }
					if (  t.n == 63 ) { t.field_s = "csi_stoodincoverpeekleft"  ; t.value1 = 1581 ; t.value2 = 1581; }
					if (  t.n == 64 ) { t.field_s = "csi_stoodincoverthrowleft"  ; t.value1 = 2680 ; t.value2 = 2680; }
					if (  t.n == 65 ) { t.field_s = "csi_stoodincoverright"  ; t.value1 = 1525 ; t.value2 = 1525; }
					if (  t.n == 66 ) { t.field_s = "csi_stoodincoverpeekright"  ; t.value1 = 1526 ; t.value2 = 1526; }
					if (  t.n == 67 ) { t.field_s = "csi_stoodincoverthrowright"  ; t.value1 = 2570 ; t.value2 = 2570; }
					if (  t.n == 51 ) { t.field_s = "csi_stoodandturn"  ; t.value1 = 0 ; t.value2 = 0; }
					if (  t.n == 68 ) { t.field_s = "csi_crouchidlenormal1"  ; t.value1 = 1670 ; t.value2 = 1819; }
					if (  t.n == 69 ) { t.field_s = "csi_crouchidlenormal2"  ; t.value1 = 1825 ; t.value2 = 1914; }
					if (  t.n == 70 ) { t.field_s = "csi_crouchidlerocket"  ; t.value1 = 6472 ; t.value2 = 6545; }
					if (  t.n == 71 ) { t.field_s = "csi_crouchdown"  ; t.value1 = 1630 ; t.value2 = 1646; }
					if (  t.n == 72 ) { t.field_s = "csi_crouchdownrocket"  ; t.value1 = 6316 ; t.value2 = 6356; }
					if (  t.n == 73 ) { t.field_s = "csi_crouchrolldown"  ; t.value1 = 2160 ; t.value2 = 2216; }
					if (  t.n == 74 ) { t.field_s = "csi_crouchrollup"  ; t.value1 = 2225 ; t.value2 = 2281; }
					if (  t.n == 75 ) { t.field_s = "csi_crouchmovefore"  ; t.value1 = 2075 ; t.value2 = 2102; }
					if (  t.n == 76 ) { t.field_s = "csi_crouchmoveback"  ; t.value1 = 2102 ; t.value2 = 2131; }
					if (  t.n == 77 ) { t.field_s = "csi_crouchmoveleft"  ; t.value1 = 2015 ; t.value2 = 2043; }
					if (  t.n == 78 ) { t.field_s = "csi_crouchmoveright"  ; t.value1 = 2043 ; t.value2 = 2072; }
					if (  t.n == 79 ) { t.field_s = "csi_crouchmoverun"  ; t.value1 = 2135 ; t.value2 = 2153; }
					if (  t.n == 80 ) { t.field_s = "csi_crouchreload"  ; t.value1 = 1920 ; t.value2 = 2010; }
					if (  t.n == 81 ) { t.field_s = "csi_crouchreloadrocket"  ; t.value1 = 6380 ; t.value2 = 6471; }
					if (  t.n == 82 ) { t.field_s = "csi_crouchwave"  ; t.value1 = 2460 ; t.value2 = 2510; }
					if (  t.n == 83 ) { t.field_s = "csi_crouchtoss"  ; t.value1 = 2520 ; t.value2 = 2555; }
					if (  t.n == 84 ) { t.field_s = "csi_crouchfirerocket"  ; t.value1 = 6357 ; t.value2 = 6379; }
					if (  t.n == 85 ) { t.field_s = "csi_crouchimpactfore"  ; t.value1 = 5240 ; t.value2 = 5277; }
					if (  t.n == 86 ) { t.field_s = "csi_crouchimpactback"  ; t.value1 = 5290 ; t.value2 = 5339; }
					if (  t.n == 87 ) { t.field_s = "csi_crouchimpactleft"  ; t.value1 = 5409 ; t.value2 = 5466; }
					if (  t.n == 88 ) { t.field_s = "csi_crouchimpactright"  ; t.value1 = 5350 ; t.value2 = 5395; }
					if (  t.n == 89 ) { t.field_s = "csi_crouchgetup"  ; t.value1 = 1646 ; t.value2 = 1663; }
					if (  t.n == 90 ) { t.field_s = "csi_crouchgetuprocket"  ; t.value1 = 6573 ; t.value2 = 6607; }
					darkai_assignanimtofield ( );
				}
			}
		}
		#endif

		//  Finish animation quantities
		t.entityprofile[t.entid].animmax=t.tnewanimmax;
		t.entityprofile[t.entid].startofaianim=t.tstartofaianim;

		//  Localisation must change desc to local name
		if (  t.entityprofileheader[t.entid].desc_s != "" ) 
		{
			if (  cstr(Left(t.entityprofileheader[t.entid].desc_s.Get(),1)) != "%" ) 
			{
				#ifdef WICKEDENGINE
				// no LOC files in wicked for now
				#else
				t.tflocalfilename_s=cstr("languagebank\\")+g.language_s+"\\textfiles\\library\\"+t.entdir_s+t.ent_s;
				t.tflocalfilename_s=cstr(Left(t.tflocalfilename_s.Get(),Len(t.tflocalfilename_s.Get())-4))+cstr(".loc");
				if (  FileExist(t.tflocalfilename_s.Get()) == 1 ) 
				{
					Dim (  t.tflocal_s,1  );
					LoadArray (  t.tflocalfilename_s.Get() ,t.tflocal_s );
					t.entityprofileheader[t.entid].desc_s=t.tflocal_s[0];
					UnDim (  t.tflocal_s );
				}
				#endif
			}
		}

		//  Translate entity references inside entity profile (token translations)
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%key" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[472];
		}
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%light" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[473];
		}
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%remote door" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[474];
		}
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%teleporter in" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[615];
		}
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%teleporter out" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[616];
		}
		if (  cstr(Lower(t.entityprofileheader[t.entid].desc_s.Get())) == "%lift" ) 
		{
			t.entityprofileheader[t.entid].desc_s=t.strarr_s[617];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].usekey_s.Get())) == "%key" ) 
		{
			t.entityprofile[t.entid].usekey_s=t.strarr_s[472];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].ifused_s.Get())) == "%light" ) 
		{
			t.entityprofile[t.entid].ifused_s=t.strarr_s[473];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].ifused_s.Get())) == "%remote door" ) 
		{
			t.entityprofile[t.entid].ifused_s=t.strarr_s[474];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].ifused_s.Get())) == "%teleporter in" ) 
		{
			t.entityprofile[t.entid].ifused_s=t.strarr_s[615];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].ifused_s.Get())) == "%teleporter out" ) 
		{
			t.entityprofile[t.entid].ifused_s=t.strarr_s[616];
		}
		if (  cstr(Lower(t.entityprofile[t.entid].ifused_s.Get())) == "%lift" ) 
		{
			t.entityprofile[t.entid].ifused_s=t.strarr_s[617];
		}

		//  All profile defaults
		if (  t.entityprofile[t.entid].ismarker != 1 ) 
		{
			if (  t.entityprofile[t.entid].lives<1  )  t.entityprofile[t.entid].lives = 1;
		}
		if (  t.entityprofile[t.entid].speed == 0  )  t.entityprofile[t.entid].speed = 100;
		if (  t.entityprofile[t.entid].hurtfall == 0  )  t.entityprofile[t.entid].hurtfall = 100;

		//  Physics Data Defaults
		if (  t.entityprofile[t.entid].ismarker == 0 ) 
		{
			//  default physics settings (weight and friction done during object load (we need the obj size!)
			//  health packs have no physics by default for A compatibility
			if (  t.entityprofile[t.entid].ishealth != 0 ) 
			{
				t.entityprofile[t.entid].physics=0;
			}
		}
		else
		{
			t.entityprofile[t.entid].physics=0;
		}

		//LB: Additional assumption that objects with no collision should not have physics
		#ifdef WICKEDENGINE
		if (t.entityprofile[t.entid].collisionmode >= 11 && t.entityprofile[t.entid].collisionmode <= 12)
		{
			t.entityprofile[t.entid].physics = 0;
		}
		#endif

		//  LOD System Defaults
		if ( t.entityprofile[t.entid].lod1distance > 0 && t.entityprofile[t.entid].lod2distance == 0 )
		{
			t.entityprofile[t.entid].lod2distance = t.entityprofile[t.entid].lod1distance;
		}

		//  Spawn defaults
		t.entityprofile[t.entid].spawnatstart=1;
		t.entityprofile[t.entid].spawndelayrandom=0;
		t.entityprofile[t.entid].spawnqtyrandom=0;
		t.entityprofile[t.entid].spawnvel=0;
		t.entityprofile[t.entid].spawnvelrandom=0;
		t.entityprofile[t.entid].spawnangle=0;
		t.entityprofile[t.entid].spawnanglerandom=0;
		t.entityprofile[t.entid].spawnlife=0;
		if (  t.entityprofile[t.entid].spawnmax>0 ) 
		{
			t.entityprofile[t.entid].spawnupto=t.entityprofile[t.entid].spawnmax;
			t.entityprofile[t.entid].spawnafterdelay=1;
			if (  t.entityprofile[t.entid].ischaracter == 1 ) 
			{
				t.entityprofile[t.entid].spawnwhendead=1;
			}
			else
			{
				t.entityprofile[t.entid].spawnwhendead=0;
			}
		}
		else
		{
			t.entityprofile[t.entid].spawnupto=0;
			t.entityprofile[t.entid].spawnafterdelay=0;
			t.entityprofile[t.entid].spawnwhendead=0;
		}

		//  Fix scale for FPE
		if (  t.entityprofile[t.entid].scale == 0 ) 
		{
			t.entityprofile[t.entid].scale=100;
		}

		// 010917 - if shader effect is a decal, auto switch zdepth flag (shader no longer does this in DX11)
		LPSTR pEffectMatch = "effectbank\\reloaded\\decal";
		if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pEffectMatch, strlen(pEffectMatch) ) == NULL ) 
		{
			t.entityprofile[t.entid].zdepth = 0;
		}

		// 261117 - intercept and replace any legacy shaders with new PBR ones if game visuals using RealtimePBR (3) mode
		if ( g.gpbroverride == 1 )
		{
			int iReplaceMode = 0;
			LPSTR pTryMatch = "effectbank\\reloaded\\entity_basic.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 1;
			pTryMatch = "effectbank\\reloaded\\character_static.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 1;
			pTryMatch = "effectbank\\reloaded\\entity_anim.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 2;
			pTryMatch = "effectbank\\reloaded\\character_basic.fx";
			#ifdef VRTECH
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 5;
			#else
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 2;
			#endif
			pTryMatch = "effectbank\\reloaded\\character_transparency.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 2;
			pTryMatch = "effectbank\\reloaded\\tree_basic.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 3;			
			pTryMatch = "effectbank\\reloaded\\treea_basic.fx";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 4;	
			#ifdef VRTECH
			if ( strlen ( t.entityprofile[t.entid].effect_s.Get() ) == 0 ) iReplaceMode = 1;
			#endif
			if ( iReplaceMode > 0 )
			{
				if ( iReplaceMode == 1 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\apbr_basic.fx";
				if ( iReplaceMode == 2 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\apbr_anim.fx";
				if ( iReplaceMode == 3 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\apbr_tree.fx";
				if ( iReplaceMode == 4 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\apbr_treea.fx";
				#ifdef VRTECH
				if ( iReplaceMode == 5 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\apbr_animwithtran.fx";
				#endif
			}
		}
		else
		{
			// 120418 - conversely, if PBR override not active, and have new PBR asset entities that still 
			// have old DNS textures, switch them back to classic non-PBR (this allows new PBR assets to 
			// replace older legacy assets but still allow backwards compatibility for users who want the
			// old shaders and old textures to remain in effect using PBR override of zero)
			char pEntityItemPath[1024];
			strcpy ( pEntityItemPath, t.ent_s.Get() );
			int n = 0;
			for ( n = strlen(pEntityItemPath)-1; n > 0; n-- )
			{
				if ( pEntityItemPath[n] == '\\' || pEntityItemPath[n] == '/' )
				{
					pEntityItemPath[n+1] = 0;
					break;
				}
			}
			if ( n <= 0 ) strcpy ( pEntityItemPath, "" );
			char pJustTextureName[1024];
			strcpy ( pJustTextureName, t.entityprofile[t.entid].texd_s.Get() );
			if ( strlen ( pJustTextureName ) > 4 )
			{
				pJustTextureName[strlen(pJustTextureName)-4]=0;
				if ( stricmp ( pJustTextureName+strlen(pJustTextureName)-6, "_color" ) == NULL )
				{
					pJustTextureName[strlen(pJustTextureName)-6]=0;
					strcat ( pJustTextureName, "_D" );
				}
				strcat ( pJustTextureName, ".png" );
			}
			char pReplaceWithDNS[1024];
			strcpy ( pReplaceWithDNS, pEntityItemPath );
			strcat ( pReplaceWithDNS, pJustTextureName );
			bool bReplacePBRWithNonPBRDNS = false;
			LPSTR pPBREffectMatch = "effectbank\\reloaded\\apbr";
			if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pPBREffectMatch, strlen(pPBREffectMatch) ) == NULL ) 
			{
				// entity effect specifies PBR, do we have the DNS files available
				if ( strlen ( pJustTextureName ) > 4 )
				{
					cstr pFindDNSFile = t.entdir_s + pReplaceWithDNS;
					if ( FileExist ( pFindDNSFile.Get() ) == 0 )
					{
						pReplaceWithDNS[strlen(pReplaceWithDNS)-4]=0;
						strcat ( pReplaceWithDNS, ".dds" );
						pFindDNSFile = t.entdir_s + pReplaceWithDNS;
						if ( FileExist ( pFindDNSFile.Get() ) == 0 )
						{
							pReplaceWithDNS[strlen(pReplaceWithDNS)-4]=0;
							strcat ( pReplaceWithDNS, ".jpg" );
							pFindDNSFile = t.entdir_s + pReplaceWithDNS;
							if ( FileExist ( pFindDNSFile.Get() ) == 1 )
							{
								bReplacePBRWithNonPBRDNS = true;
							}
						}
						else
						{
							bReplacePBRWithNonPBRDNS = true;
						}
					}
					else
					{
						bReplacePBRWithNonPBRDNS = true;
					}
				}
				else
				{
					// no texture specified, but can still switch to classic shaders (legacy behavior)
					bReplacePBRWithNonPBRDNS = true;
				}
			}
			if ( bReplacePBRWithNonPBRDNS == true )
			{
				// replace the shader used
				int iReplaceMode = 0;
				LPSTR pTryMatch = "effectbank\\reloaded\\apbr_basic.fx";
				if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 1;
				pTryMatch = "effectbank\\reloaded\\apbr_anim.fx";
				if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 2;
				pTryMatch = "effectbank\\reloaded\\apbr_tree.fx";
				if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 3;
				pTryMatch = "effectbank\\reloaded\\apbr_treea.fx";
				if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pTryMatch, strlen(pTryMatch) ) == NULL ) iReplaceMode = 4;
				if ( iReplaceMode > 0 )
				{
					if ( iReplaceMode == 1 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\entity_basic.fx";
					if ( iReplaceMode == 2 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\character_basic.fx";
					if ( iReplaceMode == 3 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\tree_basic.fx";
					if ( iReplaceMode == 4 ) t.entityprofile[t.entid].effect_s = "effectbank\\reloaded\\treea_basic.fx";
				}

				// replace the texture specified (from _color to _D)
				t.entityprofile[t.entid].texd_s = pJustTextureName;
			}
		}

		// if effect shader starts with APBR, auto shift effectprofile from zero to one
		LPSTR pPBREffectMatch = "effectbank\\reloaded\\apbr";
		if ( strnicmp ( t.entityprofile[t.entid].effect_s.Get(), pPBREffectMatch, strlen(pPBREffectMatch) ) == NULL ) 
		{
			if ( t.entityprofile[t.entid].effectprofile == 0 )
				t.entityprofile[t.entid].effectprofile = 1;
		}
	}

	//  Can loop back if skipBIN flag set
	if (  t.skipBINloadingandtryagain == 1 ) 
	{
		timestampactivity(0,cstr(cstr(Str(t.tprotectBINfile))+" Entity BIN File Out Of Date: "+t.tprofile_s).Get());
		if (  t.game.gameisexe == 1 ) 
		{
			t.skipBINloadingandtryagain=0;
		}
		else
		{
			if (  t.tprotectBINfile == 0 ) 
			{
				if (  FileExist(t.tprofile_s.Get()) == 1  )  DeleteAFile (  t.tprofile_s.Get() );
			}
			else
			{
				t.skipBINloadingandtryagain=0;
			}
		}
	}

	} while ( !(  t.skipBINloadingandtryagain == 0 ) );

	//  new field as we now have pure lights and entity lights
	if (  t.entityprofile[t.entid].ismarker == 2 || t.entityprofile[t.entid].ismarker == 5 ) 
	{
		if (  t.entityprofile[t.entid].ismarker == 5  )  t.entityprofile[t.entid].ismarker = 0;
		t.entityprofile[t.entid].islightmarker=1;
		//  FPGC - 300310 - entitylights always active as they may control a dynamic light and possibly decal-particle(mode7)
		t.entityprofile[t.entid].phyalways=1;
	}
	else
	{
		t.entityprofile[t.entid].islightmarker=0;
	}

	//  FPGC - 100610 - all FPGC characters are ALWAYS ACTIVE for full speed logic (more predictable)
	if (  t.entityprofile[t.entid].ischaracter == 1 && g.fpgcgenre == 0 ) 
	{
		t.entityprofile[t.entid].phyalways=1;
		//  FPGC - 110610 - and ALL are invincible
		t.entityprofile[t.entid].strength=0;
	}

	#ifdef WICKEDENGINE
	if (t.entityprofile[t.entid].ischaracter == 0)
	{
		// only characters can use isspinetracker mode (now on by default for wicked)
		t.entityprofile[t.entid].isspinetracker = 0;
	}
	#endif

	//  fileexistelse
	}
	else
	{
		//  File not exist, provide debug information (only if file specified (old entities can be renamed and still hang around inside FPMs)
		if (  Len( cstr(t.entdir_s+t.ent_s).Get() )>Len("entitybank\\") ) 
		{
			debugfilename( cstr(t.entdir_s+t.ent_s).Get(),t.tprofile_s.Get() );
		}
	}

	//  V109 BETA5 - 250408 - flag material is being used
	if (  t.entityprofile[t.entid].materialindex>0 ) 
	{
		t.mi=t.entityprofile[t.entid].materialindex-1;
		t.material[t.mi].usedinlevel=1;
	}
	//if (  t.entityprofile[t.entid].debrisshapeindex>0 ) 
	//{
	//	t.di=t.entityprofile[t.entid].debrisshapeindex;
	//	t.debrisshapeindexused[t.di]=1;
	//}

	// if flagged as EBE, attempt to load any EBE cube data
	if ( t.entityprofile[t.entid].isebe != 0 )
	{
		cstr sEBEFile = cstr(Left(t.tFPEName_s.Get(),strlen(t.tFPEName_s.Get())-4)) + cstr(".ebe");
		if ( FileExist ( sEBEFile.Get() ) ) 
		{
			// load EBE data into entityID
			ebe_load_ebefile ( sEBEFile, t.entid );
		}
		else
		{
			// 300817 - EBE has had .ebe file removed to make it regular entity, so remove handle limb
			if ( t.entityprofile[t.entid].isebe == 1 ) 
			{
				t.entityprofile[t.entid].isebe = 2;
			}
		}
	}

	#ifdef WICKEDENGINE
	// set transparency for all markers
	if (t.entityprofile[t.entid].ismarker > 0 && t.entityprofile[t.entid].ismarker != 11)
		t.entityprofile[t.entid].transparency = 6;
	#endif
}

void entity_loadvideoid ( void )
{
	t.tvideoid=0;
	t.text_s = Lower(Right(t.tvideofile_s.Get(),4));
	if ( t.text_s == ".ogv" || t.text_s == ".mp4" ) 
	{
		t.tvideoid=32;
		for ( t.tt = 1 ; t.tt<=  32; t.tt++ )
		{
			if ( AnimationExist(t.tt) == 0 ) { t.tvideoid = t.tt  ; break; }
		}
		#ifdef VRTECH
		if ( LoadAnimation(t.tvideofile_s.Get(), t.tvideoid, g.videoprecacheframes, g.videodelayedload, 1) == false )
		{
			t.tvideoid = -999;
		}
		#else
		LoadAnimation(t.tvideofile_s.Get(), t.tvideoid, g.videoprecacheframes, g.videodelayedload, 1);
		#endif
	}
}

void entity_loadactivesoundsandvideo ( void )
{
	// sounds in each entity
	char pSoundSet0[MAX_PATH];
	char pSoundSet1[MAX_PATH];
	char pSoundSet2[MAX_PATH];
	char pSoundSet3[MAX_PATH];

	// go through all entities in level
	for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if ( t.entid>0 ) 
		{
			if ( t.entityelement[t.e].active == 1 ) 
			{
				// original base filenames for sound
				strcpy(pSoundSet0, t.entityelement[t.e].eleprof.soundset_s.Get());
				strcpy(pSoundSet1, t.entityelement[t.e].eleprof.soundset1_s.Get());
				strcpy(pSoundSet2, t.entityelement[t.e].eleprof.soundset2_s.Get());
				strcpy(pSoundSet3, t.entityelement[t.e].eleprof.soundset3_s.Get());

				#ifdef WICKEDENGINE
				// new system can adjust sound at load time to provide automatic variances
				if (t.entityprofile[t.entid].ischaracter != 0 && t.entityelement[t.e].eleprof.iUseSoundVariants)
				{
					// only apply variant system to characters (for now to limit additional setup time)
					for (int allfour = 0; allfour < 4; allfour++)
					{
						bool bMightHaveVariant = false;
						LPSTR pThisStr = NULL;
						if (allfour == 0) pThisStr = pSoundSet0;
						if (allfour == 1) pThisStr = pSoundSet1;
						if (allfour == 2) pThisStr = pSoundSet2;
						if (allfour == 3) pThisStr = pSoundSet3;
						if (bMightHaveVariant == false && strnicmp (pThisStr + strlen(pThisStr) - 5, "1.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && strnicmp (pThisStr + strlen(pThisStr) - 5, "2.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && strnicmp (pThisStr + strlen(pThisStr) - 5, "3.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && strnicmp (pThisStr + strlen(pThisStr) - 5, "4.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == false && strnicmp (pThisStr + strlen(pThisStr) - 5, "5.wav", 5) == NULL) bMightHaveVariant = true;
						if (bMightHaveVariant == true)
						{
							int iOriginal = pThisStr[strlen(pThisStr) - 5] - '1';
							int iRnd = rand() % 5;
							int iAttempts = 3;
							while (iAttempts > 0)
							{
								iRnd = rand() % 5;
								pThisStr[strlen(pThisStr) - 5] = 0;
								char pNewStr[MAX_PATH];
								sprintf(pNewStr, "%s%d.wav", pThisStr, 1 + iRnd);
								strcpy(pThisStr, pNewStr);
								if (FileExist(pThisStr) == 1)
								{
									// found variant, use this!
									if (iRnd != iOriginal)
										break;
								}
								else
								{
									// not exist
									pThisStr[strlen(pThisStr) - 5] = 0;
									sprintf(pNewStr, "%s%d.wav", pThisStr, 1 + iOriginal);
									strcpy(pThisStr, pNewStr);
									iRnd = iOriginal;
								}
								iAttempts--;
							}
						}
					}
				}
				#endif

				#ifdef VRTECH
				// sounds or videos
				if ( t.entityelement[t.e].soundset == 0 ) 
				{
					t.tvideofile_s = pSoundSet0; entity_loadvideoid ( );
					if ( t.tvideoid == -999 )
					{
						t.entityelement[t.e].soundset = 0;
					}
					else
					{
						if ( t.tvideoid > 0 ) 
							t.entityelement[t.e].soundset=t.tvideoid*-1;
						else
							t.entityelement[t.e].soundset=loadinternalsoundcore(pSoundSet0,1);
					}
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (  t.entityelement[t.e].soundset1 == 0 ) 
				{
					t.tvideofile_s = pSoundSet1; entity_loadvideoid ( );
					if ( t.tvideoid == -999 )
					{
						t.entityelement[t.e].soundset1 = 0;
					}
					else
					{
						if (  t.tvideoid>0 ) 
							t.entityelement[t.e].soundset1=t.tvideoid*-1;
						else
							t.entityelement[t.e].soundset1=loadinternalsoundcore(pSoundSet1,1);
					}
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				#else
				if (  t.entityelement[t.e].soundset == 0 ) 
				{
					t.tvideofile_s=t.entityelement[t.e].eleprof.soundset_s ; entity_loadvideoid ( );
					if (  t.tvideoid>0 ) 
						t.entityelement[t.e].soundset=t.tvideoid*-1;
					else
						t.entityelement[t.e].soundset=loadinternalsoundcore(t.entityelement[t.e].eleprof.soundset_s.Get(),1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (  t.entityelement[t.e].soundset1 == 0 ) 
				{
					t.tvideofile_s=t.entityelement[t.e].eleprof.soundset1_s ; entity_loadvideoid ( );
					if (  t.tvideoid>0 ) 
						t.entityelement[t.e].soundset1=t.tvideoid*-1;
					else
						t.entityelement[t.e].soundset1=loadinternalsoundcore(t.entityelement[t.e].eleprof.soundset1_s.Get(),1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				#endif
				if (  t.entityelement[t.e].soundset2 == 0 ) 
				{
					t.entityelement[t.e].soundset2 = loadinternalsoundcore(pSoundSet2,1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				if (  t.entityelement[t.e].soundset3 == 0 ) 
				{
					t.entityelement[t.e].soundset3 = loadinternalsoundcore(pSoundSet3,1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}

				#ifdef VRTECH
				// lipsync LIP data (associated with a sound file)
				if ( t.entityprofile[t.entid].ischaracter != 0 )
				{
					for (int s = 0; s < 4; s++)
					{
						bool bFoundSound = false;
						if (s == 0 && t.entityelement[t.e].soundset != 0) bFoundSound = true;
						if (s == 1 && t.entityelement[t.e].soundset1 != 0) bFoundSound = true;
						if (s == 2 && t.entityelement[t.e].soundset2 != 0) bFoundSound = true;
						if (s == 3 && t.entityelement[t.e].soundset3 != 0) bFoundSound = true;
						if (bFoundSound == true)
						{
							// construct LIP filename for this sound
							char pWAVFile[2048];
							char pLIPFile[2048];
							if (s == 0) strcpy(pLIPFile, pSoundSet0);
							if (s == 1) strcpy(pLIPFile, pSoundSet1);
							if (s == 2) strcpy(pLIPFile, pSoundSet2);
							if (s == 3) strcpy(pLIPFile, pSoundSet3);
							strcpy(pWAVFile, pLIPFile);
							pLIPFile[strlen(pLIPFile) - 4] = 0;
							strcat(pLIPFile, ".lip");

							// and then get real location
							char pRealLIPFile[MAX_PATH];
							strcpy(pRealLIPFile, pLIPFile);
							GG_GetRealPath(pRealLIPFile, 0);

							// load in mouth shape data from LIP file
							if (s == 0) t.entityelement[t.e].lipset.clear();
							if (s == 1) t.entityelement[t.e].lipset1.clear();
							if (s == 2) t.entityelement[t.e].lipset2.clear();
							if (s == 3) t.entityelement[t.e].lipset3.clear();
							Dim(t.data_s, 9999);
							LoadArray(pRealLIPFile, t.data_s);
							for (t.l = 0; t.l <= 9999; t.l++)
							{
								t.line_s = t.data_s[t.l];
								LPSTR pLine = t.line_s.Get();
								if (Len(pLine) > 0)
								{
									char pTimeStr[32];
									char pMouthShapeStr[32];
									strcpy(pTimeStr, pLine);
									strcpy(pMouthShapeStr, "X");
									for (int n = 0; n < strlen(pTimeStr); n++)
									{
										if (pTimeStr[n] == 9)
										{
											pTimeStr[n] = 0;
											strcpy(pMouthShapeStr, pLine + n + 1);
											break;
										}
									}
									sCharacterCreatorPlusMouthData mouthDataShape;
									mouthDataShape.fTimeStamp = atof(pTimeStr);
									int iMouthShapeFrameIndex = 0;
									switch (pMouthShapeStr[0])
									{
									case 'A': iMouthShapeFrameIndex = 6;  break; // M
									case 'B': iMouthShapeFrameIndex = 2;  break; // K, S, T, EE
									case 'C': iMouthShapeFrameIndex = 1;  break; // AE 
									case 'D': iMouthShapeFrameIndex = 11; break; // AA
									case 'E': iMouthShapeFrameIndex = 7;  break; // AO
									case 'F': iMouthShapeFrameIndex = 9;  break; // W
									case 'G': iMouthShapeFrameIndex = 3;  break; // F V
									case 'H': iMouthShapeFrameIndex = 5;  break; // L
									case 'X': iMouthShapeFrameIndex = 0;  break; // CLOSED RELAXED MOUTH
									}
									mouthDataShape.iMouthShape = iMouthShapeFrameIndex;
									if (s == 0) t.entityelement[t.e].lipset.push_back(mouthDataShape);
									if (s == 1) t.entityelement[t.e].lipset1.push_back(mouthDataShape);
									if (s == 2) t.entityelement[t.e].lipset2.push_back(mouthDataShape);
									if (s == 3) t.entityelement[t.e].lipset3.push_back(mouthDataShape);
								}
							}
							UnDim(t.data_s);
						}
					}
				}
				#else
				if (  t.entityelement[t.e].soundset4 == 0 ) 
				{
					t.entityelement[t.e].soundset4=loadinternalsoundcore(t.entityelement[t.e].eleprof.soundset4_s.Get(),1);
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
				}
				#endif
			}
		}
	}
}

#ifdef WICKEDENGINE
void entity_cleargrideleprofrelationshipdata (void)
{
	// wipe out relational data when adding new object
	t.grideleprof.iObjectLinkID = 0;
	for (int i = 0; i < 10; i++)
	{
		t.grideleprof.iObjectRelationships[i] = 0;
		t.grideleprof.iObjectRelationshipsData[i] = 0;
		t.grideleprof.iObjectRelationshipsType[i] = 0;
	}
}
#endif

void entity_fillgrideleproffromprofile ( void )
{
	// Name
	t.grideleprof.name_s=t.entityprofileheader[t.entid].desc_s;

	#ifdef WICKEDENGINE
	t.grideleprof.iFlattenID = -1; // never carries ID of individual elements
	if (!g_bEnableAutoFlattenSystem) //PE: If disabled always disable autoflatten.
		t.grideleprof.bAutoFlatten = false;
	//g_bEnableAutoFlattenSystem
	#endif

	#ifdef WICKEDENGINE
	// smart system to auto assign whether character is an ally or enemy
	t.grideleprof.iCharAlliance = 0;
	if ( strstr(t.grideleprof.name_s.Lower().Get(), "ally" ) != NULL )
	{
		t.grideleprof.iCharAlliance = 1;
	}
	#endif

	// Group reference
	t.grideleprof.groupreference = t.entityprofile[t.entid].groupreference;

	// AI values
	t.grideleprof.aimain_s=t.entityprofile[t.entid].aimain_s;

	// AI use vars
	t.grideleprof.usekey_s=t.entityprofile[t.entid].usekey_s;
	t.grideleprof.ifused_s=t.entityprofile[t.entid].ifused_s;

	//  Spawn
	t.grideleprof.spawnatstart=t.entityprofile[t.entid].spawnatstart;
	t.grideleprof.spawnmax=t.entityprofile[t.entid].spawnmax;
	t.grideleprof.spawndelay=t.entityprofile[t.entid].spawndelay;
	t.grideleprof.spawnqty=t.entityprofile[t.entid].spawnqty;
	t.grideleprof.spawnupto=t.entityprofile[t.entid].spawnupto;
	t.grideleprof.spawnafterdelay=t.entityprofile[t.entid].spawnafterdelay;
	t.grideleprof.spawnwhendead=t.entityprofile[t.entid].spawnwhendead;
	t.grideleprof.spawndelayrandom=t.entityprofile[t.entid].spawndelayrandom;
	t.grideleprof.spawnqtyrandom=t.entityprofile[t.entid].spawnqtyrandom;
	t.grideleprof.spawnvel=t.entityprofile[t.entid].spawnvel;
	t.grideleprof.spawnvelrandom=t.entityprofile[t.entid].spawnvelrandom;
	t.grideleprof.spawnangle=t.entityprofile[t.entid].spawnangle;
	t.grideleprof.spawnanglerandom=t.entityprofile[t.entid].spawnanglerandom;
	t.grideleprof.spawnlife=t.entityprofile[t.entid].spawnlife;

	//  Scale, Cone
	t.grideleprof.scale=t.entityprofile[t.entid].scale;
	t.grideleprof.coneheight=t.entityprofile[t.entid].coneheight;
	t.grideleprof.coneangle=t.entityprofile[t.entid].coneangle;
	t.grideleprof.conerange=t.entityprofile[t.entid].conerange;

	//  Texture and Effect Data
	t.grideleprof.uniqueelement=0;
	t.grideleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.grideleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.grideleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.grideleprof.transparency=t.entityprofile[t.entid].transparency;
	t.grideleprof.castshadow=t.entityprofile[t.entid].castshadow;
	t.grideleprof.reducetexture=t.entityprofile[t.entid].reducetexture;

	//  Strength and Quantity
	t.grideleprof.strength=t.entityprofile[t.entid].strength;
	t.grideleprof.lives=t.entityprofile[t.entid].lives;
	t.grideleprof.isimmobile=t.entityprofile[t.entid].isimmobile;
	t.grideleprof.lodmodifier=t.entityprofile[t.entid].lodmodifier;
	t.grideleprof.isocluder=t.entityprofile[t.entid].isocluder;
	t.grideleprof.isocludee=t.entityprofile[t.entid].isocludee;
	t.grideleprof.specularperc=t.entityprofile[t.entid].specularperc;
	t.grideleprof.colondeath=t.entityprofile[t.entid].colondeath;
	t.grideleprof.parententityindex=t.entityprofile[t.entid].parententityindex;
	t.grideleprof.parentlimbindex=t.entityprofile[t.entid].parentlimbindex;
	t.grideleprof.isviolent=t.entityprofile[t.entid].isviolent;
	t.grideleprof.cantakeweapon=t.entityprofile[t.entid].cantakeweapon;
	t.grideleprof.hasweapon_s=t.entityprofile[t.entid].hasweapon_s;
	t.grideleprof.quantity=t.entityprofile[t.entid].quantity;
	t.grideleprof.isobjective=t.entityprofile[t.entid].isobjective;
	t.grideleprof.hurtfall=t.entityprofile[t.entid].hurtfall;
	t.grideleprof.speed=t.entityprofile[t.entid].speed;
	t.grideleprof.animspeed=t.entityprofile[t.entid].animspeed;

	//  Decal and Sound Name
	//t.grideleprof.basedecal_s=t.entitydecal_s[t.entid][0]; //PE: Not used anymore.
	#ifdef VRTECH
	t.grideleprof.voiceset_s=t.entityprofile[t.entid].voiceset_s;
	t.grideleprof.voicerate=t.entityprofile[t.entid].voicerate;
	#endif
	t.grideleprof.soundset_s=t.entityprofile[t.entid].soundset_s;
	t.grideleprof.soundset1_s=t.entityprofile[t.entid].soundset1_s;
	t.grideleprof.soundset2_s=t.entityprofile[t.entid].soundset2_s;
	t.grideleprof.soundset3_s=t.entityprofile[t.entid].soundset3_s;
	t.grideleprof.soundset4_s=t.entityprofile[t.entid].soundset4_s;
	#ifdef WICKEDENGINE
	t.grideleprof.soundset5_s = t.entityprofile[t.entid].soundset5_s;
	t.grideleprof.soundset6_s = t.entityprofile[t.entid].soundset6_s;
	t.grideleprof.overrideanimset_s = "";
	#endif

	//  FPGC - 310710 - decal particle settings
	t.particlefile_s = ""; //t.grideleprof.basedecal_s; //PE: Not used anymore.
	decal_getparticlefile ( );
	t.grideleprof.particleoverride=1;
	t.grideleprof.particle=g.gotparticle;

	//  Marker Data
	t.grideleprof.markerindex=t.entityprofile[t.entid].markerindex;
	t.grideleprof.light=t.entityprofile[t.entid].light;
	t.grideleprof.trigger=t.entityprofile[t.entid].trigger;
	t.grideleprof.usespotlighting=t.entityprofile[t.entid].usespotlighting;

	//  Data Extracted From GUN and FLAK
	t.tgunid_s=t.entityprofile[t.entid].isweapon_s;
	entity_getgunidandflakid ( );
	t.grideleprof.rateoffire=t.entityprofile[t.entid].rateoffire;
	t.grideleprof.weaponisammo=0;
	if (  t.tgunid>0 ) 
	{
		t.grideleprof.accuracy=g.firemodes[t.tgunid][0].settings.accuracy;
		t.grideleprof.reloadqty=g.firemodes[t.tgunid][0].settings.reloadqty;
		t.grideleprof.fireiterations=g.firemodes[t.tgunid][0].settings.iterate;
		t.grideleprof.usespotlighting=g.firemodes[t.tgunid][0].settings.usespotlighting;
		if (  t.tflakid == 0 ) 
		{
			t.grideleprof.damage=g.firemodes[t.tgunid][0].settings.damage;
			t.grideleprof.range=g.firemodes[t.tgunid][0].settings.range;
			t.grideleprof.dropoff=g.firemodes[t.tgunid][0].settings.dropoff;
		}
		else
		{
			t.grideleprof.damage=0;
			t.grideleprof.lifespan=0;
			t.grideleprof.throwspeed=0;
			t.grideleprof.throwangle=0;
			t.grideleprof.bounceqty=0;
			t.grideleprof.explodeonhit=0;
			t.grideleprof.weaponisammo=t.tflakid;
		}
	}

	#ifdef WICKEDENGINE
	// Collision Data Overide
	t.grideleprof.iOverrideCollisionMode = -1;
	#endif

	// Physics Data
	t.grideleprof.physics=t.entityprofile[t.entid].physics;
	t.grideleprof.phyalways=t.entityprofile[t.entid].phyalways;
	t.grideleprof.phyweight=t.entityprofile[t.entid].phyweight;
	t.grideleprof.phyfriction=t.entityprofile[t.entid].phyfriction;
	t.grideleprof.phyforcedamage=t.entityprofile[t.entid].phyforcedamage;
	t.grideleprof.rotatethrow=t.entityprofile[t.entid].rotatethrow;
	t.grideleprof.explodable=t.entityprofile[t.entid].explodable;
	t.grideleprof.explodedamage=t.entityprofile[t.entid].explodedamage;
	t.grideleprof.teamfield=t.entityprofile[t.entid].teamfield;

	// 301115 - data extracted from neighbors (LOD Modifiers are shared across all parent copies)
	int iThisBankIndex = t.entid;
	if ( t.entityprofile[iThisBankIndex].addhandlelimb==0 )
	{
		for ( int e=1; e<=g.entityelementlist; e++ )
		{
			if ( t.entityelement[e].bankindex==iThisBankIndex )
			{
				t.grideleprof.lodmodifier = t.entityelement[e].eleprof.lodmodifier;
				break;
			}
		}
	}

	#ifdef WICKEDENGINE
	//PE: Make sure when we create we use default variables in eleprof.
	if (t.entityprofile[t.entid].WEMaterial.MaterialActive)
		t.grideleprof.bCustomWickedMaterialActive = true;
	else
		t.grideleprof.bCustomWickedMaterialActive = false;
	t.grideleprof.WEMaterial = t.entityprofile[t.entid].WEMaterial;

	//Need default particle setup here. or if will use the last inside "t.grideleprof".
	t.grideleprof.newparticle.emitterid = -1;
	t.grideleprof.newparticle.emittername = "particlesbank/default";
	#endif

	#ifdef WICKEDENGINE
	// wipe out relational data when adding new object
	entity_cleargrideleprofrelationshipdata();
	#endif

	#ifdef WICKEDENGINE
	// when first load an object, need to populate sound4 so _properties is ALWAYS called!
	cstr script_name = "scriptbank\\";
	script_name += t.grideleprof.aimain_s;
	extern void ParseLuaScript(entityeleproftype *tmpeleprof, char * script);
	ParseLuaScript(&t.grideleprof, script_name.Get());
	#endif
}

void entity_updatetextureandeffectfromeleprof ( void )
{

	//  Texture and Effect (use entityprofile loader)
	//t.storeentdefaults as entityprofiletype;
	t.storeentdefaults=t.entityprofile[t.entid];
	t.entityprofile[t.entid].texd_s=t.entityelement[t.e].eleprof.texd_s;
	t.entityprofile[t.entid].texaltd_s=t.entityelement[t.e].eleprof.texaltd_s;
	t.entityprofile[t.entid].texdid=t.entityelement[t.e].eleprof.texdid;
	t.entityprofile[t.entid].texaltdid=t.entityelement[t.e].eleprof.texaltdid;
	t.entityprofile[t.entid].effect_s=t.entityelement[t.e].eleprof.effect_s;
	t.entityprofile[t.entid].usingeffect=t.entityelement[t.e].eleprof.usingeffect;
	t.entityprofile[t.entid].texnid=t.entityelement[t.e].eleprof.texnid;
	t.entityprofile[t.entid].texsid=t.entityelement[t.e].eleprof.texsid;
	t.entityprofile[t.entid].texidmax=t.entityelement[t.e].eleprof.texidmax;
	t.entityprofile[t.entid].transparency=t.entityelement[t.e].eleprof.transparency;
	t.entityprofile[t.entid].reducetexture=t.entityelement[t.e].eleprof.reducetexture;
	entity_loadtexturesandeffect ( );
	t.entityelement[t.e].eleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.entityelement[t.e].eleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.entityelement[t.e].eleprof.texdid=t.entityprofile[t.entid].texdid;
	t.entityelement[t.e].eleprof.texaltdid=t.entityprofile[t.entid].texaltdid;
	t.entityelement[t.e].eleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.entityelement[t.e].eleprof.usingeffect=t.entityprofile[t.entid].usingeffect;
	t.entityelement[t.e].eleprof.texnid=t.entityprofile[t.entid].texnid;
	t.entityelement[t.e].eleprof.texsid=t.entityprofile[t.entid].texsid;
	t.entityelement[t.e].eleprof.texidmax=t.entityprofile[t.entid].texidmax;
	t.entityelement[t.e].eleprof.transparency=t.entityprofile[t.entid].transparency;
	t.entityelement[t.e].eleprof.reducetexture=t.entityprofile[t.entid].reducetexture;
	t.entityprofile[t.entid]=t.storeentdefaults;
}

void entity_updatetextureandeffectfromgrideleprof ( void )
{
	//  Texture and Effect (use entityprofile loader)
	//t.storeentdefaults as entityprofiletype;
	t.storeentdefaults=t.entityprofile[t.entid];
	t.entityprofile[t.entid].texd_s=t.grideleprof.texd_s;
	t.entityprofile[t.entid].texaltd_s=t.grideleprof.texaltd_s;
	t.entityprofile[t.entid].texdid=t.grideleprof.texdid;
	t.entityprofile[t.entid].texaltdid=t.grideleprof.texaltdid;
	t.entityprofile[t.entid].effect_s=t.grideleprof.effect_s;
	t.entityprofile[t.entid].usingeffect=t.grideleprof.usingeffect;
	t.entityprofile[t.entid].texnid=t.grideleprof.texnid;
	t.entityprofile[t.entid].texsid=t.grideleprof.texsid;
	t.entityprofile[t.entid].texidmax=t.grideleprof.texidmax;
	t.entityprofile[t.entid].transparency=t.grideleprof.transparency;
	t.entityprofile[t.entid].reducetexture=t.grideleprof.reducetexture;
	entity_loadtexturesandeffect ( );
	t.grideleprof.texd_s=t.entityprofile[t.entid].texd_s;
	t.grideleprof.texaltd_s=t.entityprofile[t.entid].texaltd_s;
	t.grideleprof.texdid=t.entityprofile[t.entid].texdid;
	t.grideleprof.texaltdid=t.entityprofile[t.entid].texaltdid;
	t.grideleprof.effect_s=t.entityprofile[t.entid].effect_s;
	t.grideleprof.usingeffect=t.entityprofile[t.entid].usingeffect;
	t.grideleprof.texnid=t.entityprofile[t.entid].texnid;
	t.grideleprof.texsid=t.entityprofile[t.entid].texsid;
	t.grideleprof.texidmax=t.entityprofile[t.entid].texidmax;
	t.grideleprof.transparency=t.entityprofile[t.entid].transparency;
	t.grideleprof.reducetexture=t.entityprofile[t.entid].reducetexture;
	t.entityprofile[t.entid]=t.storeentdefaults;
}

void entity_getgunidandflakid ( void )
{
	//  Use Weapon Name to get GUNID and FLAKID
	if (  t.tgunid_s != "" ) 
	{
		//  get gun
		t.findgun_s=Lower(t.tgunid_s.Get());
		gun_findweaponindexbyname ( );
		t.tgunid=t.foundgunid;
		//  no flak - old system
		t.tflakid=0;
	}
	else
	{
		t.tgunid=0 ; t.tflakid=0;
	}
}

void entity_loadtexturesandeffect ( void )
{
	//  If entity object not exist, reset var
	if (  t.entobj>0 ) 
	{
		if (  ObjectExist(t.entobj) == 0  )  t.entobj = 0;
	}

	//  Only characters need a higher quality texture, rest use divide standard settings
	t.tfullorhalfdivide=0;
	if (  t.segobjusedformapeditor == 0 ) 
	{
		if (  t.entityprofile[t.entid].ischaracter == 1 ) 
		{
			t.tfullorhalfdivide=2;
		}
		else
		{
			if (  t.entityprofile[t.entid].reducetexture != 0 ) 
			{
				if (  t.entityprofile[t.entid].reducetexture == -1 ) 
				{
					t.tfullorhalfdivide=1;
				}
				else
				{
					t.tfullorhalfdivide=2;
				}
			}
		}
	}

	//  Apply TEXTURE to entity object
	bool bMultiMaterialObject = false;
	t.tuseeffecttexture=0;
	t.texdir_s = "";
	t.texaltdir_s = "";
	t.tfile_s=t.entityprofile[t.entid].texd_s;
	t.tfilealt_s=t.entityprofile[t.entid].texaltd_s;
	if (t.tfile_s != "")
	{
		if (t.entityprofile[t.entid].texpath_s != "")
		{
			t.texdir_s = t.entityprofile[t.entid].texpath_s + t.tfile_s;
			t.texaltdir_s = t.entityprofile[t.entid].texpath_s + t.tfilealt_s;
		}
		else
		{
			#ifdef WICKEDENGINE
			// wicked relies on accurate texture path and name entries, and a Classic bug caused an error
			// when a full texture path was already provided, i.e. gamecore\guns\modern, etc
			bool bPathInsideFilename = false;
			LPSTR pFilenamePart = t.tfile_s.Get();
			for (int n = 0; n < strlen(pFilenamePart); n++)
				if (pFilenamePart[n] == '\\' || pFilenamePart[n] == '/')
					bPathInsideFilename = true;
			if ( bPathInsideFilename == true )
			{
				// path is already provided in t.tfile_s!
				t.texdir_s = t.tfile_s;
				t.texaltdir_s = t.tfilealt_s;
			}
			else
			{
				// regular assembly of entity folder, entity path and entity filename only
				t.texdir_s = t.entdir_s + t.entpath_s + t.tfile_s;
				t.texaltdir_s = t.entdir_s + t.entpath_s + t.tfilealt_s;
			}
			#else
			t.texdir_s = t.entdir_s + t.entpath_s + t.tfile_s;
			t.texaltdir_s = t.entdir_s + t.entpath_s + t.tfilealt_s;
			#endif
		}
	}
	#ifdef WICKEDENGINE
	// Wicked does not support old method of loading images/effects
	t.entityprofile[t.entid].usingeffect=0;
	t.entityprofile[t.entid].texaltdid=0;
	#else
	if ( t.tfile_s != "" ) 
	{
		t.tthistexdir_s=t.texdir_s;
		if (  g.gdividetexturesize == 0  )  t.tthistexdir_s = "effectbank\\reloaded\\media\\white_D.dds";
		if (  t.entityprofile[t.entid].transparency == 0 ) 
		{
			t.texuseid=loadinternaltextureex(t.tthistexdir_s.Get(),1,t.tfullorhalfdivide);
		}
		else
		{
			t.texuseid=loadinternaltextureex(t.tthistexdir_s.Get(),5,t.tfullorhalfdivide);
		}
		if (  t.texuseid == 0 ) 
		{
			t.texdir_s=t.entityprofile[t.entid].texd_s;
			t.texaltdir_s=t.entityprofile[t.entid].texaltd_s;
			t.texuseid=loadinternaltextureex(t.texdir_s.Get(),1,t.tfullorhalfdivide);
		}
		if (  t.texuseid == 0 ) 
		{
			//  if still no texture, maybe FPE specifies it WRONG and correct is inside model
			for ( t.tlimbindex = 0 ; t.tlimbindex<=  999; t.tlimbindex++ )
			{
				if (  LimbExist(t.entobj,t.tlimbindex) == 1 ) 
				{
					LPSTR sTmp = LimbTextureName(t.entobj, t.tlimbindex);
					t.tlimbtex_s=t.entdir_s+t.entpath_s+ sTmp;
					if(sTmp) delete[] sTmp;
					t.tispresent=FileExist(t.tlimbtex_s.Get());
					if (  t.tispresent == 1 ) 
					{
						break;
					}
				}
			}
			t.texdir_s=t.tlimbtex_s;
			t.texaltdir_s=t.entityprofile[t.entid].texaltd_s;
			t.texuseid=loadinternaltextureex(t.texdir_s.Get(),1,t.tfullorhalfdivide);
		}

		//  Load ALT texture if available
		t.texaltdid=loadinternalimagecompressquality(t.texaltdir_s.Get(),1,t.tfullorhalfdivide);
		t.entityprofile[t.entid].texaltdid=t.texaltdid;
		bMultiMaterialObject = false;
	}
	else
	{
		// textured field left blank to indicate multi-material has filled in and loaded texture stages already
		t.texdir_s="" ; t.texuseid=0;
		t.texaltdid=0 ; t.entityprofile[t.entid].texaltdid=t.texaltdid;
		bMultiMaterialObject = true;
	}
	
	// 261117 - intercept and replace any legacy shaders with new PBR ones if game visuals using RealtimePBR (3) mode
	cstr EffectFile_s = t.entityprofile[t.entid].effect_s;
	int iEffectProfile = t.entityprofile[t.entid].effectprofile;

	//  Load entity effect
	t.entityprofile[t.entid].usingeffect=0;
	if (  t.entityprofile[t.entid].ismarker == 0 ) 
	{
		t.tfile_s=EffectFile_s;
		common_wipeeffectifnotexist ( );
		if (  t.tfile_s != "" ) 
		{
			t.teffectid=loadinternaleffect(t.tfile_s.Get());
			if (  t.teffectid>0 ) 
			{
				t.entityprofile[t.entid].usingeffect=t.teffectid;
			}
		}
	}
	else
	{
		// 110517 - no fixed function any more
		t.entityprofile[t.entid].usingeffect = g.guishadereffectindex;
	}
	#endif

	// Texture and apply effect
	#ifdef WICKEDENGINE
	// remove '.dds' from texture filename
	char pNoExtFilename[1024];
	strcpy ( pNoExtFilename, t.texdir_s.Get() );
	if (strlen(pNoExtFilename) > 4)
	{
		// strip .DDS/.JPG/etc from filename
		pNoExtFilename[strlen(pNoExtFilename) - 4] = 0;

		// for valid objects
		sObject* pObject = GetObjectData(t.entobj);
		if (pObject)
		{
			// Leave texture filename alone with .DDS/.PNG intact (not a WickedPBR texture set designation)
			t.texdirnoext_s = t.texdir_s;

			// go through all meshes in object and texture them
			bool bApplyTexture = false;
			for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
			{
				sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
				if (pMesh)
				{
					if (pMesh->dwTextureCount > 0)
					{
						// if not enough textures create required number
						if (pMesh->dwTextureCount < GG_MESH_TEXTURE_SURFACE)
						{
							extern bool EnsureTextureStageValid (sMesh* pMesh, int iTextureStage);
							EnsureTextureStageValid (pMesh, GG_MESH_TEXTURE_SURFACE);
						}

						// determine if base texture is a _color, in which case we can organize the texture array properly
						if (strnicmp (pNoExtFilename + strlen(pNoExtFilename) - 6, "_color", 6) == NULL)
						{
							// strip _color from pNoExtFilename
							pNoExtFilename[strlen(pNoExtFilename) - 6] = 0;

							// construct cousin texture references
							if (pMesh->dwTextureCount >= GG_MESH_TEXTURE_SURFACE)
							{
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_DIFFUSE].pName, "_color.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_NORMAL].pName, "_normal.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_ROUGHNESS].channelMask = (15) + (1 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_METALNESS].channelMask = (15) + (2 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].pName, "_surface.dds");
								pMesh->pTextures[GG_MESH_TEXTURE_OCCLUSION].channelMask = (15) + (0 << 4);
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_EMISSIVE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_EMISSIVE].pName, "_emissive.dds");
								strcpy(pMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName, pNoExtFilename);
								strcat(pMesh->pTextures[GG_MESH_TEXTURE_SURFACE].pName, "_surface.dds");
							}
						}
						else
						{
							// simple base texture
							strcpy(pMesh->pTextures[0].pName, t.texdirnoext_s.Get());
						}
						bApplyTexture = true;
					}
				}
			}
			if (bApplyTexture == true)
			{
				WickedSetEntityId(t.entid);
				WickedCall_TextureObject(pObject, NULL);
				WickedSetEntityId(-1);
			}
		}
	}
	#else
	if ( t.entobj>0 ) 
	{
		// lee - 300518 - added extra code in LoadObject to detect DNS and PBR texture file sets and set the mesh, so 
		// skip the override code below if the object has a good texture in place
		//bool bGotAO = false - replaced this with a later scan to add AO only when missing
		#ifdef VRTECH
		bool bGotNormal = false, bGotMetalness = false, bGotGloss = false, bGotMask = false, bGotAltAlbedo = false;
		#else
		bool bGotNormal = false, bGotMetalness = false, bGotGloss = false;
		#endif
		sObject* pObject = GetObjectData ( t.entobj );
		if ( pObject )
		{
			for ( int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++ )
			{
				sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
				if ( pMesh )
				{
					for ( int iTextureIndex = 2; iTextureIndex < pMesh->dwTextureCount; iTextureIndex++ )
					{
						if ( pMesh->pTextures[iTextureIndex].iImageID > 0 )
						{
							#ifdef VRTECH
							if ( iTextureIndex == 2 ) bGotNormal = true;
							if ( iTextureIndex == 3 ) bGotMetalness = true;
							if ( iTextureIndex == 4 ) bGotGloss = true;
							if ( iTextureIndex == 5 ) bGotMask = true;
							if ( iTextureIndex == 7 ) bGotAltAlbedo = true;
							#else
							//if ( iTextureIndex == 1 ) bGotAO = true;
							if ( iTextureIndex == 2 ) bGotNormal = true;
							if ( iTextureIndex == 3 ) bGotMetalness = true;
							if ( iTextureIndex == 4 ) bGotGloss = true;
							#endif
						}
					}
				}
			}
		}

		// detect if using an effect or not
		int use_illumination = false;
		#ifdef WICKEDENGINE
		// Shaders always provided by Wicked Engine
		#else
		if ( t.entityprofile[t.entid].usingeffect == 0 ) 
		{
			//  No effect
			t.entityprofile[t.entid].texdid=t.texuseid;
			t.entityprofile[t.entid].texnid=0;
			t.entityprofile[t.entid].texsid=0;
			TextureObject ( t.entobj, t.texuseid );
		}
		else
		#endif
		{
			if ( bMultiMaterialObject == false )
			{
				// Strip out _D.dds or COLOR.dds 
				char pNoExtFilename[1024];
				strcpy ( pNoExtFilename, t.texdir_s.Get() );
				pNoExtFilename[strlen(pNoExtFilename)-4] = 0;
				//PE: Some textures do not have _d,_color,_albedo , so always reset.
				t.texdirnoext_s = "";
				if ( strnicmp ( pNoExtFilename+strlen(pNoExtFilename)-2, "_d", 2 ) == NULL )
				{
					t.texdirnoext_s=Left(pNoExtFilename,Len(pNoExtFilename)-Len("_d"));
				}
				else
				{
					if ( strnicmp ( pNoExtFilename+strlen(pNoExtFilename)-6, "_color", 6 ) == NULL )
						t.texdirnoext_s=Left(pNoExtFilename,Len(pNoExtFilename)-Len("_color"));
					else
						if ( strnicmp ( pNoExtFilename+strlen(pNoExtFilename)-7, "_albedo", 7 ) == NULL )
							t.texdirnoext_s=Left(pNoExtFilename,Len(pNoExtFilename)-Len("_albedo"));
				}

				//  Assign DIFFUSE
				t.entityprofile[t.entid].texdid = t.texuseid;

				// Assign NORMAL
				if ( iEffectProfile == 1 )
					t.texdirN_s = t.texdirnoext_s+"_normal.dds";
				else
					t.texdirN_s = t.texdirnoext_s+"_n.dds";
				t.texuseid = loadinternaltextureex(t.texdirN_s.Get(),1,t.tfullorhalfdivide);
				if ( t.texuseid == 0 ) 
				{
					if ( iEffectProfile == 1 )
						t.texdirN_s = t.texdirnoext_s+"_n.dds";
					else
						t.texdirN_s = t.texdirnoext_s+"_normal.dds";
					t.texuseid = loadinternaltextureex(t.texdirN_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 ) 
					{
						t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_N.dds",1,t.tfullorhalfdivide);
					}
				}
				t.entityprofile[t.entid].texnid = t.texuseid;

				// Assign SPECULAR (PBR Metalness)
				if ( t.entityprofile[t.entid].specular == 0 ) 
				{
					if ( iEffectProfile == 1 )
					{
						t.texdirS_s = t.texdirnoext_s+"_metalness.dds";
						t.texuseid = loadinternaltextureex(t.texdirS_s.Get(),1,t.tfullorhalfdivide);
						if ( t.texuseid == 0 ) 
						{
							//PE: We dont use _s.dds ?
							t.texdirS_s = t.texdirnoext_s+"_specular.dds";
							t.texuseid = loadinternaltextureex(t.texdirS_s.Get(),1,t.tfullorhalfdivide);
							if ( t.texuseid == 0 ) 
							{
								// 261117 - search material for stock metalness
								char pFullFilename[1024];
								int iMaterialIndex = t.entityprofile[t.entid].materialindex;
								sprintf ( pFullFilename, "effectbank\\reloaded\\media\\materials\\%d_Metalness.dds", iMaterialIndex );
								t.texdirS_s = pFullFilename;
								t.texuseid = loadinternaltextureex(t.texdirS_s.Get(),1,t.tfullorhalfdivide);
								if ( t.texuseid == 0 ) 
								{
									t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds",1,t.tfullorhalfdivide);
								}
							}
						}
					}
					else
					{
						t.texdirS_s = t.texdirnoext_s+"_s.dds";
						t.texuseid = loadinternaltextureex(t.texdirS_s.Get(),1,t.tfullorhalfdivide);
					}
				}
				else
				{
					if (  t.entityprofile[t.entid].specular == 1  )  t.texdirS_s = "effectbank\\reloaded\\media\\blank_none_S.dds";
					if (  t.entityprofile[t.entid].specular == 2  )  t.texdirS_s = "effectbank\\reloaded\\media\\blank_low_S.dds";
					if (  t.entityprofile[t.entid].specular == 3  )  t.texdirS_s = "effectbank\\reloaded\\media\\blank_medium_S.dds";
					if (  t.entityprofile[t.entid].specular == 4  )  t.texdirS_s = "effectbank\\reloaded\\media\\blank_high_S.dds";
					t.texuseid = loadinternaltextureex(t.texdirS_s.Get(),1,t.tfullorhalfdivide);
				}
				t.entityprofile[t.entid].texsid = t.texuseid;

				// Assign ILLUMINATION or CUBE (or real-time 'ENVCUBE for PBR' later)
				if ( iEffectProfile == 0 )
				{
					// non-PBR legacy behaviour
					t.texdirI_s = t.texdirnoext_s+"_i.dds";
					t.texuseid = loadinternaltextureex(t.texdirI_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 )
					{
						// if no _I file, try to find and load _CUBE file (load mode 2 = cube)
						t.texdirI_s = t.texdirnoext_s+"_cube.dds";
						t.texuseid = loadinternaltexturemode(t.texdirI_s.Get(),2);
						if ( t.texuseid == 0 )
						{
							// if no local CUBE, see if the level has generated one (matches sky and terrain)
							t.texuseid = t.terrain.imagestartindex+31;
						}
					}
					t.entityprofiletexiid = t.texuseid;
				}
				else
				{
					// PBR behaviour only allow _CUBE to override PBR reflection
					t.texdirI_s = t.texdirnoext_s+"_cube.dds";
					t.texuseid = loadinternaltexturemode(t.texdirI_s.Get(),2);
					if ( t.texuseid == 0 )
					{
						// if no local CUBE, see if the level has generated one (matches sky and terrain)
						t.texuseid = t.terrain.imagestartindex+31;
					}
					t.entityprofiletexiid = t.texuseid;
				}
				t.entityprofile[t.entid].texiid = t.entityprofiletexiid;

				// Assign AMBIENT OCCLUSION MAP
				t.texdirO_s = t.texdirnoext_s+"_o.dds";
				t.texuseid = loadinternaltextureex(t.texdirO_s.Get(),1,t.tfullorhalfdivide);
				if ( t.texuseid == 0 ) 
				{
					t.texdirO_s = t.texdirnoext_s+"_ao.dds";
					t.texuseid = loadinternaltextureex(t.texdirO_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 ) 
					{
						t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_O.dds",1,t.tfullorhalfdivide);
					}
					else
					{
						// disable override to AO that exists can be used
						if ( strlen ( t.entityprofile[t.entid].texd_s.Get() ) > 0 )
						{
							// but only if texture was specified in FPE, not if we assume model based textures
							//bGotAO = false; // see replacement solution below
						}
					}
				}
				t.entityprofiletexoid = t.texuseid;

				//PE: IBR old t7 is now t8 , detail/illum t8 is now t7. Done so we can skip t8, we still need the correct order of textures.
				//PE: IBR was not large but generate tons of stage changes.

				// Assign textures for PBR
				if ( iEffectProfile == 1 )
				{
					// gloss texture
					cstr pGlosstex_s = t.texdirnoext_s+"_gloss.dds";
					t.texuseid = loadinternaltextureex(pGlosstex_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 ) 
					{
						// 261117 - search material for stock metalness
						char pFullFilename[1024];
						int iMaterialIndex = t.entityprofile[t.entid].materialindex;
						sprintf ( pFullFilename, "effectbank\\reloaded\\media\\materials\\%d_Gloss.dds", iMaterialIndex );
						pGlosstex_s = pFullFilename;
						t.texuseid = loadinternaltextureex(pGlosstex_s.Get(),1,t.tfullorhalfdivide);
						if ( t.texuseid == 0 ) 
						{
							t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\white_D.dds",1,t.tfullorhalfdivide);
						}
					}
					t.entityprofile[t.entid].texgid = t.texuseid;

					// mask or height texture
					#ifdef VRTECH
					cstr pHeighttex_s = t.texdirnoext_s+"_mask.dds";
					t.texuseid = loadinternaltextureex(pHeighttex_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 ) 
					{
						pHeighttex_s = t.texdirnoext_s+"_height.dds";
						t.texuseid = loadinternaltextureex(pHeighttex_s.Get(),1,t.tfullorhalfdivide);
						if (t.texuseid == 0)
						{
							t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds", 1, t.tfullorhalfdivide);
						}
					}
					t.entityprofile[t.entid].texhid = t.texuseid;
					#else
					cstr pHeighttex_s = t.texdirnoext_s+"_height.dds";
					if( g.skipunusedtextures == 0 ) t.texuseid = loadinternaltextureex(pHeighttex_s.Get(),1,t.tfullorhalfdivide);
					if ( t.texuseid == 0 || g.skipunusedtextures == 1 )
					{
						t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds",1,t.tfullorhalfdivide);
					}
					t.entityprofile[t.entid].texhid = t.texuseid;
					#endif

					// IBR texture
					if (g.memskipibr == 0) 
					{
						t.entityprofiletexibrid = t.terrain.imagestartindex + 32;
					}

					//PE: Use illumination instead of detail if found.
					//PE: Illumination overwrite detail.
					use_illumination = true;
					cstr pDetailtex_s = t.texdirnoext_s + "_illumination.dds";
					t.entityprofile[t.entid].texlid = loadinternaltextureex(pDetailtex_s.Get(), 1, t.tfullorhalfdivide);
					if (t.entityprofile[t.entid].texlid == 0)
					{
						cstr pDetailtex_s = t.texdirnoext_s + "_emissive.dds"; // _emissive
						t.entityprofile[t.entid].texlid = loadinternaltextureex(pDetailtex_s.Get(), 1, t.tfullorhalfdivide);
						if (t.entityprofile[t.entid].texlid == 0)
						{
							if (g.gpbroverride == 1) 
							{
								// PE: Also support _i when using gpbroverride == 1.
								t.texdirI_s = t.texdirnoext_s + "_i.dds";
								t.entityprofile[t.entid].texlid = loadinternaltextureex(t.texdirI_s.Get(), 1, t.tfullorhalfdivide);
							}
							if (t.entityprofile[t.entid].texlid == 0) 
							{
								// Detail texture
								cstr pDetailtex_s = t.texdirnoext_s + "_detail.dds";
								#ifdef VRTECH
								t.entityprofile[t.entid].texlid = loadinternaltextureex(pDetailtex_s.Get(), 1, t.tfullorhalfdivide);
								if (t.entityprofile[t.entid].texlid == 0)
								#else
								if( g.skipunusedtextures == 0 ) t.entityprofile[t.entid].texlid = loadinternaltextureex(pDetailtex_s.Get(), 1, t.tfullorhalfdivide);
								if (t.entityprofile[t.entid].texlid == 0 || g.skipunusedtextures == 1)
								#endif
								{
									t.entityprofile[t.entid].texlid = loadinternaltextureex("effectbank\\reloaded\\media\\detail_default.dds", 1, t.tfullorhalfdivide);
								}
								use_illumination = false;
							}
						}
					}
				}

				// Apply all textures to NON-MULTIMATERIAL entity parent object (D O N S)
				if ( t.entityprofile[t.entid].texdid > 0 ) 
				{
					// but only if diffuse specified, else use texture already loaded for model
					TextureObject ( t.entobj, 0, t.entityprofile[t.entid].texdid );
				}
				TextureObject ( t.entobj, 2, t.entityprofile[t.entid].texnid );
				TextureObject ( t.entobj, 3, t.entityprofile[t.entid].texsid );

				// Additional texture assignments required for PBR mode
				if ( iEffectProfile == 1 )
				{
					if (g.memskipibr == 0) TextureObject ( t.entobj, 8, t.entityprofiletexibrid );
					#ifdef VRTECH
					if ( bGotAltAlbedo == false ) TextureObject ( t.entobj, 7, t.entityprofile[t.entid].texlid );
					TextureObject ( t.entobj, 4, t.entityprofile[t.entid].texgid );
					if ( bGotMask == false ) TextureObject ( t.entobj, 5, t.entityprofile[t.entid].texhid );
					#else
					TextureObject ( t.entobj, 7, t.entityprofile[t.entid].texlid );
					TextureObject ( t.entobj, 4, t.entityprofile[t.entid].texgid );
					TextureObject ( t.entobj, 5, t.entityprofile[t.entid].texhid );
					#endif
				}
			}
			else
			{
				// object entity uses multi-material (Fuse character)
				// and already loaded D N and S into 0, 2 and 3
				t.entityprofile[t.entid].texiid = 0;

				// PE 240118: not always if missing texture= in fpe , normal/spec is not always in the objects texture lists. ( we must have normal maps on all objects ).
				// Fix: https://github.com/LeeBamberTGC/GameGuruRepo/issues/10
				// lee - 300518 - added extra code in LoadObject to detect DNS and PBR texture file sets and set the mesh, so 
				// skip the override code below if the object has a good texture in place

				if (t.entityprofile[t.entid].texnid == 0 && bGotNormal == false )
				{
					t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_N.dds", 1, t.tfullorhalfdivide);
					t.entityprofile[t.entid].texnid = t.texuseid;
					TextureObject(t.entobj, 2, t.entityprofile[t.entid].texnid);
				}
				if (t.entityprofile[t.entid].texsid == 0 && bGotMetalness == false )
				{
					t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds", 1, t.tfullorhalfdivide);
					t.entityprofile[t.entid].texsid = t.texuseid;
					TextureObject(t.entobj, 3, t.entityprofile[t.entid].texsid);
				}
				// PE: iEffectProfile != 1 for this type of objects at this point in code.
				if (t.entityprofile[t.entid].texiid == 0)
				{
					if (g.gpbroverride == 1) // iEffectProfile == 1
					{
						// if no local CUBE, see if the level has generated one (matches sky and terrain)
						t.entityprofile[t.entid].texiid = t.terrain.imagestartindex + 31;
					}
				}

				// Not set anywhere, so just use blank_o.
				t.texuseid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_O.dds", 1, t.tfullorhalfdivide);
				t.entityprofiletexoid = t.texuseid; // must always be set.

				// PE: PBR shader support for old media with no texture in fpe.
				if (g.gpbroverride == 1) 
				{
					// None of the below will be in old media , so setup using fallback textures.
					t.entityprofile[t.entid].texlid = loadinternaltextureex("effectbank\\reloaded\\media\\detail_default.dds", 1, t.tfullorhalfdivide);
					t.entityprofile[t.entid].texgid = loadinternaltextureex("effectbank\\reloaded\\media\\white_D.dds", 1, t.tfullorhalfdivide);
					t.entityprofile[t.entid].texhid = loadinternaltextureex("effectbank\\reloaded\\media\\blank_black.dds", 1, t.tfullorhalfdivide);
					if (g.memskipibr == 0) TextureObject(t.entobj, 8, t.entityprofiletexibrid);
					#ifdef VRTECH
					if ( bGotAltAlbedo == false ) TextureObject(t.entobj, 7, t.entityprofile[t.entid].texlid);
					if ( bGotGloss == false ) TextureObject(t.entobj, 4, t.entityprofile[t.entid].texgid);
					if ( bGotMask == false ) TextureObject(t.entobj, 5, t.entityprofile[t.entid].texhid);
					#else
					TextureObject(t.entobj, 7, t.entityprofile[t.entid].texlid);
					if ( bGotGloss == false ) TextureObject(t.entobj, 4, t.entityprofile[t.entid].texgid);
					TextureObject(t.entobj, 5, t.entityprofile[t.entid].texhid);
					#endif
				}
			}

			// 230618 - apply AO texture ONLY when missing
			// if ( bGotAO == false ) TextureObject ( t.entobj, 1, t.entityprofiletexoid );
			sObject* pObject = GetObjectData ( t.entobj );
			if ( pObject )
			{
				for ( int iFrameIndex = 0; iFrameIndex < pObject->iFrameCount; iFrameIndex++ )
				{
					sFrame* pFrame = pObject->ppFrameList[iFrameIndex];
					if ( pFrame )
					{
						sMesh* pMesh = pFrame->pMesh;
						if ( pMesh )
						{
							for ( int iTextureIndex = 1; iTextureIndex < pMesh->dwTextureCount; iTextureIndex++ )
							{
								if ( pMesh->pTextures[iTextureIndex].iImageID == 0 )
								{
									if ( iTextureIndex == 1 ) TextureLimbStage ( t.entobj, iFrameIndex, iTextureIndex, t.entityprofiletexoid );
								}
							}
						}
					}
				}
			}

			// Apply all textures to REMAINING entity parent object (V C I)
			TextureObject ( t.entobj, 6, t.entityprofile[t.entid].texiid );

			// PBR or non-PBR modes
			LPSTR pEntityBasic = "effectbank\\reloaded\\entity_basic.fx";
			LPSTR pEntityAnim = "effectbank\\reloaded\\entity_anim.fx";
			LPSTR pCharacterBasic = "effectbank\\reloaded\\character_basic.fx";
			LPSTR pEntityBasicIllum = "effectbank\\reloaded\\apbr_illum.fx";
			LPSTR pCharacterBasicIllum = "effectbank\\reloaded\\apbr_illum_anim.fx";

			if ( g.gpbroverride == 1 )
			{
				pEntityBasic = "effectbank\\reloaded\\apbr_basic.fx";
				pEntityAnim = "effectbank\\reloaded\\apbr_anim.fx";
				//PE: The other way around :)
				#ifdef VRTECH
				pCharacterBasic = "effectbank\\reloaded\\apbr_animwithtran.fx";
				#else
				pCharacterBasic = "effectbank\\reloaded\\apbr_anim.fx";
				#endif
			}
			
			// 100718 - fix issue where old effect (non-illum) is retained for non-bone shader
			if ( stricmp ( EffectFile_s.Get(), pEntityBasic)==NULL ) 
			{
				if ( g.gpbroverride == 1 && use_illumination )
				{
					t.entityprofile[t.entid].usingeffect = loadinternaleffect(pEntityBasicIllum);
				}
			}

			//PE: Same problem with pEntityAnim if no bones. (large door.dbo)
			t.teffectid2 = 0;
			#ifdef VRTECH
			if (stricmp(EffectFile_s.Get(), pEntityAnim) == NULL)
			{
				if (g.gpbroverride == 1 && use_illumination)
					t.teffectid2 = loadinternaleffect(pEntityBasicIllum);
				else
					t.teffectid2 = loadinternaleffect(pEntityBasic);
			}
			#endif

			// Special case for character_basic shader, when has meshes with no bones, use entity_basic instead
			if ( stricmp(EffectFile_s.Get(), pCharacterBasic) == NULL )  
			{
				if ( g.gpbroverride == 1 && use_illumination)
					t.teffectid2 = loadinternaleffect(pEntityBasicIllum);
				else
					t.teffectid2 = loadinternaleffect(pEntityBasic);
			}

			// 010917 - or if using entity_basic shader, and HAS anim meshes with bones, use entity_anim instead
			if ( stricmp ( EffectFile_s.Get(), pEntityBasic)==NULL ) 
			{
				if ( t.entityprofile[t.entid].animmax > 0 )
				{
					t.teffectid2 = t.entityprofile[t.entid].usingeffect;
					if (g.gpbroverride == 1 && use_illumination)
						t.entityprofile[t.entid].usingeffect = loadinternaleffect(pCharacterBasicIllum);
					else
						t.entityprofile[t.entid].usingeffect = loadinternaleffect(pEntityAnim);
				}
				else 
				{
					// PE: Change basic effect to use illumination
					if ( g.gpbroverride == 1 && use_illumination )
						t.entityprofile[t.entid].usingeffect = loadinternaleffect(pEntityBasicIllum);
				}
			}

			if (stricmp(EffectFile_s.Get(), pCharacterBasic) == NULL) 
			{
				// PE: Change character effect to use illumination
				if (g.gpbroverride == 1 && use_illumination) 
				{
					t.entityprofile[t.entid].usingeffect = loadinternaleffect(pCharacterBasicIllum);
				}
			}

			// Apply effect and textures
			if ( t.lightmapper.onlyloadstaticentitiesduringlightmapper == 0 )
			{
				// don't use shader effects when lightmapping
				SetObjectEffectCore ( t.entobj, t.entityprofile[t.entid].usingeffect, t.teffectid2, t.entityprofile[t.entid].cpuanims );
			}
		}
	}
	#endif

	// Set any entity transparenct
	if ( t.entobj>0 ) 
	{
		#ifdef VRTECH
		if (t.entityprofile[t.entid].transparency >= 0)
		{
			#ifdef WICKEDENGINE
			WickedSetEntityId(t.entid);
			SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
			WickedSetEntityId(-1);
			#else
			SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
			#endif
		}
		#else
		SetObjectTransparency(t.entobj, t.entityprofile[t.entid].transparency);
		#endif
	}

	// Set entity culling (added COLLMODE 300114)
	#ifdef VRTECH
	if (t.entityprofile[t.entid].cullmode >= 0)
	#else
	if(1)
	#endif
	{
		#ifdef WICKEDENGINE
		// For Wicked, cull mode controlled per-mesh with parent default as normal 
		
		//PE: Prefer WEMaterial over old cullmode
		bool bUseWEMaterial = false;
		if (t.entityprofile[t.entid].WEMaterial.MaterialActive)
		{
			WickedSetEntityId(t.entid);
			WickedSetElementId(0);
			sObject* pObject = g_ObjectList[t.entobj];
			if (pObject)
			{
				bUseWEMaterial = true;
				for (int iMeshIndex = 0; iMeshIndex < pObject->iMeshCount; iMeshIndex++)
				{
					sMesh* pMesh = pObject->ppMeshList[iMeshIndex];
					if (pMesh)
					{
						// set properties of mesh
						WickedSetMeshNumber(iMeshIndex);
						bool bDoubleSided = WickedDoubleSided();
						if (bDoubleSided)
						{
							pMesh->bCull = false;
							pMesh->iCullMode = 0;
							WickedCall_SetMeshCullmode(pMesh);
						}
						else
						{
							pMesh->iCullMode = 1;
							pMesh->bCull = true;
							WickedCall_SetMeshCullmode(pMesh);
						}
					}
				}
			}
			WickedSetEntityId(-1);
		}

		if(!bUseWEMaterial)
		{
			SetObjectCull(t.entobj, 1);
		}
		#else
		// but only if not negative (as negative allows object to retain its own cull values as in CCP characters saved (for hair))
		if (t.entityprofile[t.entid].cullmode != 0)
		{
			// cull mode OFF used for single sided polygon models
			SetObjectCull(t.entobj, 0);
		}
		else
		{
			SetObjectCull(t.entobj, 1);
		}
		#endif
	}

	// Set cull mode for limbs (if hair specified)
	if ( t.entityprofile[t.entid].hairframestart != -1 )
	{
		for ( int tlmb = t.entityprofile[t.entid].hairframestart; tlmb <= t.entityprofile[t.entid].hairframefinish; tlmb++ )
		{
			if ( LimbExist ( t.entobj, tlmb ) == 1 )
			{
				SetLimbCull ( t.entobj, tlmb, 0 );
			}
		}
	}

	// hide specified limbs
	if ( t.entityprofile[t.entid].hideframestart != -1 )
	{
		for ( int tlmb = t.entityprofile[t.entid].hideframestart; tlmb <= t.entityprofile[t.entid].hideframefinish; tlmb++ )
		{
			if ( LimbExist ( t.entobj, tlmb ) == 1 )
			{
				ExcludeLimbOn ( t.entobj, tlmb );
			}
		}
	}

	//PE: Below will enable additive i wicked.
	#ifndef WICKEDENGINE
	if ( t.entobj > 0 )
	{
		//  If transparent, no need to Z write
		if (  t.entityprofile[t.entid].transparency>0 ) 
		{
			if ( t.entityprofile[t.entid].transparency >= 2 ) 
			{
				DisableObjectZWrite ( t.entobj );
			}
		}
	}
	#endif

}

//PE: Faster loading
#ifdef WICKEDENGINE
#define USEFASTLOADING
#endif

#ifdef USEFASTLOADING
char * c_data = NULL;
char * c_data_pointer = NULL;
int c_data_size = 0;
HANDLE c_hFile;
extern DBPRO_GLOBAL char m_pWorkString[_MAX_PATH];
DARKSDK LPSTR GetReturnStringFromWorkString(char* WorkString = m_pWorkString);

void c_OpenToRead(int f, LPSTR pFilename)
{
	//Read everything.

	HANDLE hreadfile = GG_CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hreadfile != INVALID_HANDLE_VALUE)
	{
		int filebuffersize = GetFileSize(hreadfile, NULL);
		c_data = new char[filebuffersize+256];
		// Read file into memory
		DWORD bytesread;
		ReadFile(hreadfile, c_data, filebuffersize, &bytesread, NULL);
		CloseHandle(hreadfile);
		c_data_pointer = c_data;
		c_data_size = filebuffersize;
	}
	else
	{
		c_data = NULL;
		c_data_pointer = c_data;
		c_data_size = 0;
	}

}
//delete(c_data);

bool c_ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	if ( ((c_data_pointer-c_data) + nNumberOfBytesToRead) > c_data_size)
		return false;
	memcpy(lpBuffer, c_data_pointer, nNumberOfBytesToRead);
	c_data_pointer += nNumberOfBytesToRead;
	*lpNumberOfBytesRead = nNumberOfBytesToRead;
	return true;
}

int c_ReadLong(int f)
{
	if (!c_data) return 0;
	int iResult = 0;
	DWORD bytes;
	// Read from file
	DWORD data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); // RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
	iResult = data;
	return iResult;
}


void c_CloseFile(int f)
{
	if(c_data) delete(c_data);
	c_data = NULL;
	c_data_pointer = c_data;
	c_data_size = 0;
}

int c_ReadByte(int f)
{
	if (!c_data) return 0;

	int iResult = 0;
	DWORD bytes;
	// Read from file
	unsigned char data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	iResult = data;
	return iResult;
}

int c_ReadWord(int f)
{
	if (!c_data) return 0;

	int iResult = 0;
	DWORD bytes;
	// Read from file
	WORD data;
			
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	iResult = data;
	return iResult;
}


float c_ReadFloat(int f)
{
	if (!c_data) return 0;

	float fResult = 0.0f;
	DWORD bytes;
	// Read from file
	float data;
	if (c_ReadFile(c_hFile, &data, sizeof(data), &bytes, NULL) == 0)
		return(0); //RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);

	fResult = data;

	return fResult;
}

char c_filerror[2] = "";

LPSTR c_ReadString(int f)
{
	if (!c_data) return 0;

	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	pReturnString = GetReturnStringFromWorkString(&WorkString[0]);

	return pReturnString;
}

//PE: We can have 0x0a in soundset4 (entered text) so always use 13 to stop.
LPSTR c_ReadStringIncl0xA(int f)
{
	if (!c_data) return 0;


	LPSTR pReturnString = 0;

	unsigned char c = 0;
	DWORD bytes;
	std::vector<char> WorkString;

	bool eof = false;
	do
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
		if (bytes == 0)
		{
			eof = true;
		}
		else if (c >= 32 || c == 9 || c == 10)
		{
			WorkString.push_back(c);
		}
	} while ((c >= 32 || c == 9 || c == 10) && !eof);

	WorkString.push_back(0);

	if (c == 13)
	{
		if (c_ReadFile(c_hFile, &c, 1, &bytes, NULL) == 0)
		{
			//RunTimeWarning(RUNTIMEERROR_CANNOTREADFROMFILE);
			return(&c_filerror[0]);
		}
	}

	// Create and return string
	pReturnString = GetReturnStringFromWorkString(&WorkString[0]);
	return pReturnString;
}


void c_entity_loadelementsdata ( void )
{
	//  Free any old elements
	entity_deleteelementsdata ( );

	//  Uses elementfilename$ (element data from test game quick build - not from universe.ele created during full build)
	if (  t.elementsfilename_s == ""  )  t.elementsfilename_s = g.mysystem.levelBankTestMap_s+"map.ele";

	// load entity element list
	t.failedtoload=0;
	#ifdef VRTECH
	t.versionnumbersupported = 313;
	#else
	t.versionnumbersupported = 312;
	#endif
	#ifdef WICKEDENGINE
	t.versionnumbersupported = 331; //329;//327;// 323; // 322; //319;
	#endif

	if ( FileExist(t.elementsfilename_s.Get()) == 1 ) 
	{
		c_OpenToRead(1, t.elementsfilename_s.Get());
		t.versionnumberload = c_ReadLong ( 1 );
		if (  t.versionnumberload<100 ) 
		{
			//  Pre-version data - development only
			g.entityelementlist=t.versionnumberload;
			t.versionnumberload=100;
		}
		else
		{
			g.entityelementlist = c_ReadLong ( 1 );
		}
		if (  t.versionnumberload <= t.versionnumbersupported ) 
		{
			if (  g.entityelementlist>0 ) 
			{
				UnDim (  t.entityelement );
				#ifdef VRTECH
				UnDim2 (  t.entityshadervar );
				UnDim (  t.entitydebug_s );
				#endif
				g.entityelementmax=g.entityelementlist;
				Dim (  t.entityelement,g.entityelementmax );
				#ifdef VRTECH
				Dim2(  t.entityshadervar,g.entityelementmax, g.globalselectedshadermax  );
				Dim (  t.entitydebug_s,g.entityelementmax  );
				#endif
				for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
				{
					#ifdef VRTECH
					if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
					#endif
					//  actual file data
					if (  t.versionnumberload >= 101 ) 
					{
						//  Version 1.01
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].maintype=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].bankindex=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].staticflag=t.a;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].x=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].y=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].z=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].rx=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].ry=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].rz=t.a_f;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.name_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); // t.entityelement[t.e].eleprof.aiinit_s=t.a_s; //PE: Not used anymore.
						#ifdef WICKEDENGINE
						t.a_s = c_ReadString (1);
						if (strnicmp (t.a_s.Get(), "default.lua", 11) == NULL)
						{
							t.entityelement[t.e].eleprof.aimain_s = "no_behavior_selected.lua";
						}
						else
						{
							if (strlen(t.a_s.Get()) < 4)
							{
								t.entityelement[t.e].eleprof.aimain_s = "no_behavior_selected.lua";
							}
							else
							{
								t.entityelement[t.e].eleprof.aimain_s = t.a_s;
							}
						}
						#else
						t.a_s = c_ReadString (1); t.entityelement[t.e].eleprof.aimain_s = t.a_s;
						#endif
						t.a_s = c_ReadString ( 1 ); // t.entityelement[t.e].eleprof.aidestroy_s=t.a_s;  //PE: Not used anymore.
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.isobjective=t.a;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.usekey_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.ifused_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); //t.entityelement[t.e].eleprof.ifusednear_s=t.a_s;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.uniqueelement=t.a;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.texd_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.texaltd_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.effect_s=t.a_s;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.transparency=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].editorfixed=t.a;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset1_s=t.a_s;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnmax=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawndelay=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnqty=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.hurtfall=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.castshadow=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.reducetexture=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.speed=t.a;
						t.a_s = c_ReadString ( 1 ); // t.entityelement[t.e].eleprof.aishoot_s=t.a_s;  //PE: Not used anymore.
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.hasweapon_s=t.a_s;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.lives=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].spawn.max=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].spawn.delay=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].spawn.qty=t.a;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.scale=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.coneheight=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.coneangle=t.a_f;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.strength=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.isimmobile=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.cantakeweapon=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.quantity=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.markerindex=t.a;
						t.a = c_ReadLong ( 1 ); t.dw=t.a ; t.dw=t.dw+0xFF000000 ; t.entityelement[t.e].eleprof.light.color=t.dw;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.light.range=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.trigger.stylecolor=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.trigger.waypointzoneindex=t.a;
						t.a_s = c_ReadString ( 1 ); // t.entityelement[t.e].eleprof.basedecal_s=t.a_s;  //PE: Not used anymore.
					}
					if (  t.versionnumberload >= 102 ) 
					{
						//  Version 1.02
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.rateoffire=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.damage=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.accuracy=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.reloadqty=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.fireiterations=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.lifespan=t.a;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.throwspeed=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.throwangle=t.a_f;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.bounceqty=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.explodeonhit=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.weaponisammo=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnupto=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnafterdelay=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnwhendead=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.perentityflags =t.a;
					}
					if (  t.versionnumberload >= 103 ) 
					{
						//  Version 1.03 - V1 draft physics
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.physics=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.phyweight=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.phyfriction=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.phyforcedamage=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.rotatethrow=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.explodable=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.explodedamage=t.a;
						t.a = c_ReadLong ( 1 ); //t.entityelement[t.e].eleprof.phydw4=t.a;
						t.a = c_ReadLong ( 1 ); //t.entityelement[t.e].eleprof.phydw5=t.a;
					}
					if (  t.versionnumberload >= 104 ) 
					{
						//  Version 1.04 - BETA4 extra field
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.phyalways=t.a;
					}
					if (  t.versionnumberload >= 105 ) 
					{
						//  Version 1.05 - BETA8 extra fields
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawndelayrandom=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnqtyrandom=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnvel=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnvelrandom=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnangle=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnanglerandom=t.a;
					}
					if (  t.versionnumberload >= 106 ) 
					{
						//  Version 1.06 - BETA10 extra fields
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnatstart=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.spawnlife=t.a;
					}
					if (  t.versionnumberload >= 107 ) 
					{
						//  FPSCV104RC8 - forgot to save infinilight index (dynamic lights in final build never worked)
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.light.index=t.a;
					}
					if (  t.versionnumberload >= 199 ) 
					{
						//  X10 EXTRAS - Ignored in X9
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
					}
					if (  t.versionnumberload >= 200 ) 
					{
						//  X10 EXTRAS 190707 - Ignored in X9
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
						t.a = c_ReadLong ( 1 );
					}
					if (  t.versionnumberload >= 217 ) 
					{
						//  FPGC - 300710 - save new entity element data
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particleoverride=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.offsety=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.scale=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randomstartx=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randomstarty=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randomstartz=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.linearmotionx=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.linearmotiony=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.linearmotionz=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randommotionx=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randommotiony=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.randommotionz=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.mirrormode=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.camerazshift=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.scaleonlyx=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.lifeincrement=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.alphaintensity=t.a;
					}
					if (  t.versionnumberload >= 218 ) 
					{
						//  V118 - 060810 - knxrb - Decal animation setting (Added animation choice setting).
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.particle.animated=t.a;
					}
					if (  t.versionnumberload >= 301 ) 
					{
						//  Reloaded ALPHA 1.0045
						t.a_s = c_ReadString ( 1 ); //t.entityelement[t.e].eleprof.aiinitname_s=t.a_s; //PE: Not used anymore.
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.aimainname_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); //t.entityelement[t.e].eleprof.aidestroyname_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); //t.entityelement[t.e].eleprof.aishootname_s=t.a_s;
					}
					if (  t.versionnumberload >= 302 ) 
					{
						//  Reloaded BETA 1.005
					}
					if (  t.versionnumberload >= 303 ) 
					{
						//  Reloaded BETA 1.007
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.animspeed=t.a;
					}
					if (  t.versionnumberload >= 304 ) 
					{
						//  Reloaded BETA 1.007-200514
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.conerange=t.a_f;
					}
					if (  t.versionnumberload >= 305 ) 
					{
						//  Reloaded BETA 1.0085
						t.a_f = c_ReadFloat ( 1 ); 
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scalex=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); 
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scaley=t.a_f;
						t.a_f = c_ReadFloat ( 1 ); 
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scalez=t.a_f;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.range=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.dropoff=t.a;
					}
					if (  t.versionnumberload >= 306 ) 
					{
						//  GameGuru 1.00.010
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.isviolent=t.a;
					}
					if (  t.versionnumberload >= 307 ) 
					{
						//  GameGuru 1.00.020
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.teamfield=t.a;
					}
					if (  t.versionnumberload >= 308 ) 
					{
						//  GameGuru 1.01.001
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.usespotlighting=t.a;
					}
					if (  t.versionnumberload >= 309 ) 
					{
						//  GameGuru 1.01.002
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.lodmodifier=t.a;
					}
					if (  t.versionnumberload >= 310 ) 
					{
						//  GameGuru 1.133
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.isocluder=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.isocludee=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.colondeath=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.parententityindex=t.a;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.parentlimbindex=t.a;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset2_s=t.a_s;
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset3_s=t.a_s;
						//PE: This can be done on both versions.
						//#ifdef VRTECH
						////PE: Crash - map load failed, as we can have 0xa inside here. (when entering text) only 0x0d terminate string. (secoatan.fpm version 313)
						t.a_s = c_ReadStringIncl0xA( 1 ); t.entityelement[t.e].eleprof.soundset4_s=t.a_s;
						//#else
						//t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset4_s=t.a_s;
						//#endif
					}
					if (  t.versionnumberload >= 311 ) 
					{
						//  GameGuru 1.133B
						t.a_f = c_ReadFloat ( 1 ); t.entityelement[t.e].eleprof.specularperc=t.a_f;
					}
					if (  t.versionnumberload >= 312 ) 
					{
						//  GameGuru 1.14 EBE
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].iHasParentIndex = t.a;
					}
					#ifdef VRTECH
					if (  t.versionnumberload >= 313 ) 
					{
						// VRQ V3
						t.a_s = c_ReadString ( 1 ); t.entityelement[t.e].eleprof.voiceset_s=t.a_s;
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.voicerate = t.a;
					}
					#endif
					#ifdef WICKEDENGINE
					if (t.versionnumberload >= 314)
					{
						//PE: we need to copy t.entityprofile[t.ttentid].WEMaterial before customizing.
						int tmaster = t.entityelement[t.e].bankindex;
						if (tmaster < t.entityprofile.size())
						{
							t.entityelement[t.e].eleprof.WEMaterial = t.entityprofile[tmaster].WEMaterial;
						}

						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.bCustomWickedMaterialActive = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.MaterialActive = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bCastShadows[0] = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bDoubleSided[0] = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bPlanerReflection[0] = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bTransparency[0] = t.a;

						unsigned long ulValue = 0;
						t.a_s = c_ReadString(1);
						sscanf(t.a_s.Get(), "%lu", &ulValue);
						t.entityelement[t.e].eleprof.WEMaterial.dwBaseColor[0] = ulValue;

						t.a_s = c_ReadString(1);
						sscanf(t.a_s.Get(), "%lu", &ulValue);
						t.entityelement[t.e].eleprof.WEMaterial.dwEmmisiveColor[0] = ulValue;

						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fReflectance[0] = t.a_f;

						// LB: in previous builds, only one mesh material details where stored, we can retain this for single material objects in slot 0
						t.a = c_ReadLong(1);
						int iFrameAndWEMaterialSlotIndex = 0;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = c_ReadString(1);
						#ifndef DISABLEOCCLUSIONMAP
						t.entityelement[t.e].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						#endif
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] = t.a_f;
					}
					if (t.versionnumberload >= 315)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.light.fLightHasProbe = t.a;
					}

					if (t.versionnumberload >= 316)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectLinkID = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iCharAlliance = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iCharFaction = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved1 = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved2 = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved3 = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iCharPatrolMode = t.a;
						t.a = c_ReadFloat(1); t.entityelement[t.e].eleprof.fCharRange[0] = t.a;
						t.a = c_ReadFloat(1); t.entityelement[t.e].eleprof.fCharRange[1] = t.a;
						for (int i = 0;i < 10;i++)
						{
							t.a = c_ReadFloat(1); t.entityelement[t.e].eleprof.fObjectDataReserved[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationships[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationshipsType[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationshipsData[i] = t.a;
						}
					}
					if (t.versionnumberload >= 317)
					{
						for (int i = 1; i < MAXMESHMATERIALS; i++)
						{
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bCastShadows[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bDoubleSided[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bPlanerReflection[i] = t.a;
							t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bTransparency[i] = t.a;

							unsigned long ulValue = 0;
							t.a_s = c_ReadString(1);
							sscanf(t.a_s.Get(), "%lu", &ulValue);
							t.entityelement[t.e].eleprof.WEMaterial.dwBaseColor[i] = ulValue;

							t.a_s = c_ReadString(1);
							sscanf(t.a_s.Get(), "%lu", &ulValue);
							t.entityelement[t.e].eleprof.WEMaterial.dwEmmisiveColor[i] = ulValue;

							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fReflectance[i] = t.a_f;

							int iFrameAndWEMaterialSlotIndex = i;
							t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = c_ReadString(1);
							#ifndef DISABLEOCCLUSIONMAP
							t.entityelement[t.e].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							#endif
							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] = t.a_f;
						}
					}
					if (t.versionnumberload >= 318)
					{
						t.a = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRenderOrderBias[0] = t.a;
						for (int i = 1; i < MAXMESHMATERIALS; i++)
						{
							t.a = c_ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRenderOrderBias[i] = t.a;
						}
					}
					if (t.versionnumberload >= 319)
					{
						//PE: The same group data is stored under ALL t.e and get reset each time.
						//PE: So level with 1000 objects , WILL setup groups and load ALL group images 1000 times ?
						//PE: We cant change old level now, so use this hack.
						//PE: Also this was leaking mem , not sure where. anyway this hack fix it. New maps will only save it under t.e == 1

						bool bFirstTime = false;
						if (t.e == 1) bFirstTime = true;

						extern int g_iUniqueGroupID;
						t.a = c_ReadLong(1); g_iUniqueGroupID = t.a;
						#define MAXGROUPSLISTS 100 // duplicated in GridEdit.cpp (would replace this when the group list is dynamic)
						extern std::vector<sRubberBandType> vEntityGroupList[MAXGROUPSLISTS];
						int iNumberOfGroups = 0;
						t.a = c_ReadLong(1); iNumberOfGroups = t.a;
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							int iItemsInThisGroup = 0;
							t.a = c_ReadLong(1); iItemsInThisGroup = t.a;
							if(bFirstTime) vEntityGroupList[gi].clear();
							for (int i = 0; i < iItemsInThisGroup; i++)
							{
								sRubberBandType item;
								t.a = c_ReadLong(1); item.iGroupID = t.a;
								t.a = c_ReadLong(1); item.iParentGroupID = t.a;
								t.a = c_ReadLong(1); item.e = t.a;
								t.a = c_ReadFloat(1); item.x = t.a;
								t.a = c_ReadFloat(1); item.y = t.a;
								t.a = c_ReadFloat(1); item.z = t.a;
								t.a = c_ReadFloat(1); item.quatAngle.x = t.a;
								t.a = c_ReadFloat(1); item.quatAngle.y = t.a;
								t.a = c_ReadFloat(1); item.quatAngle.z = t.a;
								t.a = c_ReadFloat(1); item.quatAngle.w = t.a;
								if (bFirstTime) vEntityGroupList[gi].push_back(item);
							}
						}
						// and load in group thumb images, and load them into the iEntityGroupListImage image list (so can see them in groups tab)
						extern int iEntityGroupListImage[MAXGROUPSLISTS];
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							if (bFirstTime) iEntityGroupListImage[gi] = 0;
							t.a = c_ReadLong(1); int iHasImage = t.a;
							if ( iHasImage == 1 && bFirstTime )
							{
								char pGroupImgFilename[MAX_PATH];
								sprintf(pGroupImgFilename, "%sgroupimg%d.png", g.mysystem.levelBankTestMap_s.Get(), gi);
								if (FileExist(pGroupImgFilename) == 1)
								{
									//Find free image id.
									int iImageID = 0;
									for (int i = 0; i < MAXGROUPSLISTS; i++)
									{
										bool bAlreadyUsed = false;
										//#define BACKBUFFERIMAGE (g.perentitypromptimageoffset+9000) // duplicated in GridEdit.cpp
										//int iNewImageID = BACKBUFFERIMAGE + i;
										//g.perentitypromptimageoffset = 110000; // allow 10,000 slots (found in Common.cpp)
										int iNewImageID = (110000 + 9000) + i;// (g.perentitypromptimageoffset + 9000) + i;
										for (int l = MAXGROUPSLISTS; l > 0; l--)
										{
											if (iEntityGroupListImage[l] == iNewImageID)
											{
												bAlreadyUsed = true;
												break;
											}
										}
										if (!bAlreadyUsed) 
										{
											iImageID = iNewImageID;
											break;
										}
									}
									if (iImageID != 0)
									{
										image_setlegacyimageloading(true);
										LoadImage(pGroupImgFilename, iImageID);
										image_setlegacyimageloading(false);
										iEntityGroupListImage[gi] = iImageID;
									}
								}
							}
						}
					}

					if (t.versionnumberload >= 320)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Preview = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Show_At_Start = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Looping_Animation = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Full_Screen = t.a;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Duration = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadein = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadeout = t.a_f;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.newparticle.Particle_Fullscreen_Transition = t.a_s;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Speed = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Opacity = t.a_f;
					}
					if (t.versionnumberload >= 321)
					{
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.newparticle.emittername= t.a_s;
					}
					if (t.versionnumberload >= 322)
					{
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].fDecalSpeed = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].fDecalOpacity = t.a_f;
					}
					if (t.versionnumberload >= 323)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iOverrideCollisionMode = t.a;
					}
					if (t.versionnumberload >= 324)
					{
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.weapondamagemultiplier = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].eleprof.meleedamagemultiplier = t.a_f;
					}
					if (t.versionnumberload >= 325)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iAffectedByGravity = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iMoveSpeed = t.a;
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iTurnSpeed = t.a;
					}

					if (t.versionnumberload >= 326)
					{
						t.a = c_ReadLong ( 1 ); t.entityelement[t.e].eleprof.light.offsetup =t.a; //Store spot radius.
					}
					if (t.versionnumberload >= 327)
					{
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.soundset5_s = t.a_s;
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.soundset6_s = t.a_s;
					}
					if (t.versionnumberload >= 328)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.iUseSoundVariants = t.a;
					}
					if (t.versionnumberload >= 329)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].quatmode = t.a;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].quatx = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].quaty = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].quatz = t.a_f;
						t.a_f = c_ReadFloat(1); t.entityelement[t.e].quatw = t.a_f;
					}
					if (t.versionnumberload >= 330)
					{
						t.a = c_ReadLong(1); t.entityelement[t.e].eleprof.bAutoFlatten = t.a;
					}
					if (t.versionnumberload >= 331)
					{
						t.a_s = c_ReadString(1); t.entityelement[t.e].eleprof.overrideanimset_s = t.a_s;
					}

					#endif

					// get the index of the entity profile
					t.ttentid=t.entityelement[t.e].bankindex;
					#ifdef VRTECH
					if (t.ttentid >= t.entityprofile.size())
					{
						// somehow, the entity profile bank index was corrupted
						t.ttentid = 0;
						t.entityelement[t.e].bankindex = 0;
						continue;
					}
					#endif

					// fill in the blanks if load older version
					if (  t.versionnumberload<103 ) 
					{
						//  Version 1.03 - V1 draft physics (-1 means calculate at entobj-loadtime)
						t.entityelement[t.e].eleprof.physics=t.entityprofile[t.ttentid].physics;
						t.entityelement[t.e].eleprof.phyweight=t.entityprofile[t.ttentid].phyweight;
						t.entityelement[t.e].eleprof.phyfriction=t.entityprofile[t.ttentid].phyfriction;
						t.entityelement[t.e].eleprof.phyforcedamage=t.entityprofile[t.ttentid].phyforcedamage;
						t.entityelement[t.e].eleprof.rotatethrow=t.entityprofile[t.ttentid].rotatethrow;
						t.entityelement[t.e].eleprof.explodable=t.entityprofile[t.ttentid].explodable;
						//t.entityelement[t.e].eleprof.phydw3=0;
						//t.entityelement[t.e].eleprof.phydw4=0;
						//t.entityelement[t.e].eleprof.phydw5=0;
					}
					if (  t.versionnumberload<104 ) 
					{
						//  Version 1.04 - BETA4 extra field
						t.entityelement[t.e].eleprof.phyalways=t.entityprofile[t.ttentid].phyalways;
					}
					if (  t.versionnumberload<105 ) 
					{
						//  Version 1.05 - BETA8
						t.entityelement[t.e].eleprof.spawndelayrandom=t.entityprofile[t.ttentid].spawndelayrandom;
						t.entityelement[t.e].eleprof.spawnqtyrandom=t.entityprofile[t.ttentid].spawnqtyrandom;
						t.entityelement[t.e].eleprof.spawnvel=t.entityprofile[t.ttentid].spawnvel;
						t.entityelement[t.e].eleprof.spawnvelrandom=t.entityprofile[t.ttentid].spawnvelrandom;
						t.entityelement[t.e].eleprof.spawnangle=t.entityprofile[t.ttentid].spawnangle;
						t.entityelement[t.e].eleprof.spawnanglerandom=t.entityprofile[t.ttentid].spawnanglerandom;
					}
					if (  t.versionnumberload<106 ) 
					{
						//  Version 1.06 - BETA10
						t.entityelement[t.e].eleprof.spawnatstart=t.entityprofile[t.ttentid].spawnatstart;
						t.entityelement[t.e].eleprof.spawnlife=t.entityprofile[t.ttentid].spawnlife;
					}
					if (  t.versionnumberload<217 ) 
					{
						//  FPGC - 300710 - older levels dont use particle override
						t.entityelement[t.e].eleprof.particleoverride=0;
					}
					if (  t.versionnumberload<303 ) 
					{
						//  Reloaded BETA 1.007
						t.entityelement[t.e].eleprof.animspeed=t.entityprofile[t.ttentid].animspeed;
					}
					if (  t.versionnumberload<304 ) 
					{
						//  Reloaded BETA 1.007-200514
						t.entityelement[t.e].eleprof.conerange=t.entityprofile[t.ttentid].conerange;
					}
					if (  t.versionnumberload<306 ) 
					{
						//  GameGuru 1.00.010
						t.entityelement[t.e].eleprof.isviolent=t.entityprofile[t.ttentid].isviolent;
					}
					if (  t.versionnumberload<307 ) 
					{
						//  GameGuru 1.00.020
						t.entityelement[t.e].eleprof.teamfield=t.entityprofile[t.ttentid].teamfield;
					}
					if (  t.versionnumberload<310 ) 
					{
						//  GameGuru 1.133
						t.entityelement[t.e].eleprof.isocluder=t.entityprofile[t.ttentid].isocluder;
						t.entityelement[t.e].eleprof.isocludee=t.entityprofile[t.ttentid].isocludee;
						t.entityelement[t.e].eleprof.colondeath=t.entityprofile[t.ttentid].colondeath;
						t.entityelement[t.e].eleprof.parententityindex=t.entityprofile[t.ttentid].parententityindex;
						t.entityelement[t.e].eleprof.parentlimbindex=t.entityprofile[t.ttentid].parentlimbindex;
						t.entityelement[t.e].eleprof.soundset2_s=t.entityprofile[t.ttentid].soundset2_s;
						t.entityelement[t.e].eleprof.soundset3_s=t.entityprofile[t.ttentid].soundset3_s;
						t.entityelement[t.e].eleprof.soundset4_s=t.entityprofile[t.ttentid].soundset4_s;
					}
					if (  t.versionnumberload<311 ) 
					{
						//  GameGuru 1.133B
						t.entityelement[t.e].eleprof.specularperc=t.entityprofile[t.ttentid].specularperc;
					}
					if (  t.versionnumberload<312 ) 
					{
						//  GameGuru 1.14 EBE
						t.entityelement[t.e].iHasParentIndex = 0;
					}
					#ifdef VRTECH
					if (  t.versionnumberload < 313 ) 
					{
						// VRQ V3
						t.entityelement[t.e].eleprof.voiceset_s=t.entityprofile[t.ttentid].voiceset_s;
						t.entityelement[t.e].eleprof.voicerate=t.entityprofile[t.ttentid].voicerate;
					}
					#endif

					t.entityelement[t.e].entitydammult_f=1.0;
					t.entityelement[t.e].entityacc=1.0;

					// 131115 - transparency control was removed from GG properties IDE, so ensure
					// it reflects the latest entity profile information (until we allow this value back in)
					t.entityelement[t.e].eleprof.transparency = t.entityprofile[t.ttentid].transparency;
				}
			}
		}
		else
		{
			t.failedtoload=1;
		}
		c_CloseFile (  1 );

		#ifndef WICKEDENGINE
		// 050416 - remove any weapons from start marker if parental control, no weapon, no violence
		if ( g.quickparentalcontrolmode == 2 )
		{
			for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
			{
				t.entid = t.entityelement[t.e].bankindex;
				if (  t.entityprofile[t.entid].ismarker == 1 ) 
				{
					//  Player Start Marker Settings
					t.entityelement[t.e].eleprof.hasweapon_s = "";
					t.entityelement[t.e].eleprof.hasweapon = 0;
					t.entityelement[t.e].eleprof.quantity = 0;
					t.entityelement[t.e].eleprof.isviolent = 0;
				}
			}
		}
		#endif

		//  If replacement file active, can swap in new SCRIPT and SOUND references
		if (  Len(t.editor.replacefilepresent_s.Get())>1 ) 
		{
			//  now go through ELEPROF enrties to update any SCRIPTBANK references and SOUNDSET references
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				#ifdef WICKEDENGINE
				for (t.tcheck = 1; t.tcheck <= 8; t.tcheck++)
				#else
				for (t.tcheck = 1; t.tcheck <= 6; t.tcheck++)
				#endif
				{
					if (  t.tcheck == 1  )  t.tcheck_s = t.entityelement[t.e].eleprof.aimain_s;
					if (  t.tcheck == 2  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset_s;
					if (  t.tcheck == 3  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset1_s;
					if (  t.tcheck == 4  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset2_s;
					if (  t.tcheck == 5  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset3_s;
					if (  t.tcheck == 6  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset4_s;
					#ifdef WICKEDENGINE
					if (  t.tcheck == 7  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset5_s;
					if (  t.tcheck == 8  )  t.tcheck_s = t.entityelement[t.e].eleprof.soundset6_s;
					#endif
					t.ttry_s="";
					for ( t.nn = 1 ; t.nn<=  Len(t.tcheck_s.Get()); t.nn++ )
					{
						t.ttry_s=t.ttry_s+Mid(t.tcheck_s.Get(),t.nn);
						if (  (cstr(Mid(t.tcheck_s.Get(),t.nn)) == "\\" && cstr(Mid(t.tcheck_s.Get(),t.nn+1)) == "\\") || (cstr(Mid(t.tcheck_s.Get(),t.nn)) == "/" && cstr(Mid(t.tcheck_s.Get(),t.nn+1)) == "/") ) 
						{
							++t.nn;
						}
					}
					t.ttry_s=Lower(t.ttry_s.Get());
					for ( t.tt = 1 ; t.tt<=  t.treplacementmax; t.tt++ )
					{
						if (  t.replacements_s[t.tt][0] == t.ttry_s ) 
						{
							//  found entry we can replace
							if (  t.tcheck == 1 ) { t.entityelement[t.e].eleprof.aimain_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							if (  t.tcheck == 2 ) { t.entityelement[t.e].eleprof.soundset_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							if (  t.tcheck == 3 ) { t.entityelement[t.e].eleprof.soundset1_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							if (  t.tcheck == 4 ) { t.entityelement[t.e].eleprof.soundset2_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							if (  t.tcheck == 5 ) { t.entityelement[t.e].eleprof.soundset3_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							if (  t.tcheck == 6 ) { t.entityelement[t.e].eleprof.soundset4_s = t.replacements_s[t.tt][1]  ; t.tt = t.treplacementmax+1; }
							#ifdef WICKEDENGINE
							if (  t.tcheck == 7) { t.entityelement[t.e].eleprof.soundset5_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 8) { t.entityelement[t.e].eleprof.soundset6_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							#endif
						}
					}
				}
			}
			//  free usages
			UnDim (  t.replacements_s );
		}
	}

	// and erase any elements that DO NOT have a valid profile (file moved/deleted)
	if ( t.failedtoload == 1 ) 
	{
		//  FPGC - 270410 - if entity binary from X10 (or just not supported), ensure NO entities!
		g.entityelementlist=0;
		g.entityelementmax=0;
	}
	else
	{
		for ( t.e = 1 ; t.e <= g.entityelementlist; t.e++ )
		{
			t.entid=t.entityelement[t.e].bankindex;
			if (  t.entid>0 ) 
			{
				if (  t.entid>ArrayCount(t.entitybank_s) ) 
				{
					t.entityelement[t.e].bankindex=0;
				}
				else
				{
					if (  Len(t.entitybank_s[t.entid].Get()) == 0 ) 
					{
						//  030715 - but only erase if entity not a marker
						if (  t.entityprofile[t.entid].ismarker == 0 ) 
						{
							t.entityelement[t.e].bankindex=0;
						}
					}
				}
			}
		}
	}
}
#endif

void entity_loadelementsdata(void)
{
	#ifdef USEFASTLOADING
	c_entity_loadelementsdata();
	return;
	#endif
	//  Free any old elements
	entity_deleteelementsdata();

	//  Uses elementfilename$ (element data from test game quick build - not from universe.ele created during full build)
	if (t.elementsfilename_s == "")  t.elementsfilename_s = g.mysystem.levelBankTestMap_s + "map.ele";

	// load entity element list
	t.failedtoload = 0;
	#ifdef VRTECH
	t.versionnumbersupported = 313;
	#else
	t.versionnumbersupported = 312;
	#endif
	#ifdef WICKEDENGINE
	t.versionnumbersupported = 331;//327;// 323; // 322; //319;
	#endif

	if (FileExist(t.elementsfilename_s.Get()) == 1)
	{
		OpenToRead(1, t.elementsfilename_s.Get());
		t.versionnumberload = ReadLong(1);
		if (t.versionnumberload < 100)
		{
			//  Pre-version data - development only
			g.entityelementlist = t.versionnumberload;
			t.versionnumberload = 100;
		}
		else
		{
			g.entityelementlist = ReadLong(1);
		}
		if (t.versionnumberload <= t.versionnumbersupported)
		{
			if (g.entityelementlist > 0)
			{
				UnDim(t.entityelement);
				#ifdef VRTECH
				UnDim2(t.entityshadervar);
				UnDim(t.entitydebug_s);
				#endif
				g.entityelementmax = g.entityelementlist;
				Dim(t.entityelement, g.entityelementmax);
				#ifdef VRTECH
				Dim2(t.entityshadervar, g.entityelementmax, g.globalselectedshadermax);
				Dim(t.entitydebug_s, g.entityelementmax);
				#endif
				for (t.e = 1; t.e <= g.entityelementlist; t.e++)
				{
					#ifdef VRTECH
					if (t.game.runasmultiplayer == 1) mp_refresh();
					#endif
					//  actual file data
					if (t.versionnumberload >= 101)
					{
						//  Version 1.01
						t.a = ReadLong(1); t.entityelement[t.e].maintype = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].bankindex = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].staticflag = t.a;
						t.a_f = ReadFloat(1); t.entityelement[t.e].x = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].y = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].z = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].rx = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].ry = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].rz = t.a_f;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.name_s = t.a_s;
						t.a_s = ReadString(1); // t.entityelement[t.e].eleprof.aiinit_s=t.a_s; //PE: Not used anymore.
						#ifdef WICKEDENGINE
						t.a_s = ReadString(1);
						if (strnicmp(t.a_s.Get(), "default.lua", 11) == NULL)
						{
							t.entityelement[t.e].eleprof.aimain_s = "no_behavior_selected.lua";
						}
						else
						{
							if (strlen(t.a_s.Get()) < 4)
							{
								t.entityelement[t.e].eleprof.aimain_s = "no_behavior_selected.lua";
							}
							else
							{
								t.entityelement[t.e].eleprof.aimain_s = t.a_s;
							}
						}
						#else
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.aimain_s = t.a_s;
						#endif
						t.a_s = ReadString(1); // t.entityelement[t.e].eleprof.aidestroy_s=t.a_s;  //PE: Not used anymore.
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.isobjective = t.a;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.usekey_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.ifused_s = t.a_s;
						t.a_s = ReadString(1); //t.entityelement[t.e].eleprof.ifusednear_s=t.a_s;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.uniqueelement = t.a;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.texd_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.texaltd_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.effect_s = t.a_s;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.transparency = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].editorfixed = t.a;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset1_s = t.a_s;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnmax = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawndelay = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnqty = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.hurtfall = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.castshadow = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.reducetexture = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.speed = t.a;
						t.a_s = ReadString(1); // t.entityelement[t.e].eleprof.aishoot_s=t.a_s;  //PE: Not used anymore.
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.hasweapon_s = t.a_s;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.lives = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].spawn.max = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].spawn.delay = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].spawn.qty = t.a;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.scale = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.coneheight = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.coneangle = t.a_f;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.strength = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.isimmobile = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.cantakeweapon = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.quantity = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.markerindex = t.a;
						t.a = ReadLong(1); t.dw = t.a; t.dw = t.dw + 0xFF000000; t.entityelement[t.e].eleprof.light.color = t.dw;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.light.range = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.trigger.stylecolor = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.trigger.waypointzoneindex = t.a;
						t.a_s = ReadString(1); // t.entityelement[t.e].eleprof.basedecal_s=t.a_s;  //PE: Not used anymore.
					}
					if (t.versionnumberload >= 102)
					{
						//  Version 1.02
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.rateoffire = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.damage = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.accuracy = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.reloadqty = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.fireiterations = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.lifespan = t.a;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.throwspeed = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.throwangle = t.a_f;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.bounceqty = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.explodeonhit = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.weaponisammo = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnupto = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnafterdelay = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnwhendead = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.perentityflags = t.a;
					}
					if (t.versionnumberload >= 103)
					{
						//  Version 1.03 - V1 draft physics
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.physics = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.phyweight = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.phyfriction = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.phyforcedamage = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.rotatethrow = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.explodable = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.explodedamage = t.a;
						t.a = ReadLong(1); //t.entityelement[t.e].eleprof.phydw4=t.a;
						t.a = ReadLong(1); //t.entityelement[t.e].eleprof.phydw5=t.a;
					}
					if (t.versionnumberload >= 104)
					{
						//  Version 1.04 - BETA4 extra field
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.phyalways = t.a;
					}
					if (t.versionnumberload >= 105)
					{
						//  Version 1.05 - BETA8 extra fields
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawndelayrandom = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnqtyrandom = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnvel = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnvelrandom = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnangle = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnanglerandom = t.a;
					}
					if (t.versionnumberload >= 106)
					{
						//  Version 1.06 - BETA10 extra fields
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnatstart = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.spawnlife = t.a;
					}
					if (t.versionnumberload >= 107)
					{
						//  FPSCV104RC8 - forgot to save infinilight index (dynamic lights in final build never worked)
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.light.index = t.a;
					}
					if (t.versionnumberload >= 199)
					{
						//  X10 EXTRAS - Ignored in X9
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
					}
					if (t.versionnumberload >= 200)
					{
						//  X10 EXTRAS 190707 - Ignored in X9
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
						t.a = ReadLong(1);
					}
					if (t.versionnumberload >= 217)
					{
						//  FPGC - 300710 - save new entity element data
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particleoverride = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.offsety = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.scale = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randomstartx = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randomstarty = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randomstartz = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.linearmotionx = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.linearmotiony = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.linearmotionz = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randommotionx = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randommotiony = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.randommotionz = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.mirrormode = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.camerazshift = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.scaleonlyx = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.lifeincrement = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.alphaintensity = t.a;
					}
					if (t.versionnumberload >= 218)
					{
						//  V118 - 060810 - knxrb - Decal animation setting (Added animation choice setting).
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.particle.animated = t.a;
					}
					if (t.versionnumberload >= 301)
					{
						//  Reloaded ALPHA 1.0045
						t.a_s = ReadString(1); //t.entityelement[t.e].eleprof.aiinitname_s=t.a_s; //PE: Not used anymore.
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.aimainname_s = t.a_s;
						t.a_s = ReadString(1); //t.entityelement[t.e].eleprof.aidestroyname_s=t.a_s;
						t.a_s = ReadString(1); //t.entityelement[t.e].eleprof.aishootname_s=t.a_s;
					}
					if (t.versionnumberload >= 302)
					{
						//  Reloaded BETA 1.005
					}
					if (t.versionnumberload >= 303)
					{
						//  Reloaded BETA 1.007
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.animspeed = t.a;
					}
					if (t.versionnumberload >= 304)
					{
						//  Reloaded BETA 1.007-200514
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.conerange = t.a_f;
					}
					if (t.versionnumberload >= 305)
					{
						//  Reloaded BETA 1.0085
						t.a_f = ReadFloat(1);
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scalex = t.a_f;
						t.a_f = ReadFloat(1); 
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scaley = t.a_f;
						t.a_f = ReadFloat(1); 
						if ( t.a_f > 1e8 ) t.a_f = 0;
						t.entityelement[t.e].scalez = t.a_f;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.range = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.dropoff = t.a;
					}
					if (t.versionnumberload >= 306)
					{
						//  GameGuru 1.00.010
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.isviolent = t.a;
					}
					if (t.versionnumberload >= 307)
					{
						//  GameGuru 1.00.020
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.teamfield = t.a;
					}
					if (t.versionnumberload >= 308)
					{
						//  GameGuru 1.01.001
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.usespotlighting = t.a;
					}
					if (t.versionnumberload >= 309)
					{
						//  GameGuru 1.01.002
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.lodmodifier = t.a;
					}
					if (t.versionnumberload >= 310)
					{
						//  GameGuru 1.133
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.isocluder = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.isocludee = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.colondeath = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.parententityindex = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.parentlimbindex = t.a;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset2_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset3_s = t.a_s;
						//PE: This can be done on both versions.
						//#ifdef VRTECH
						////PE: Crash - map load failed, as we can have 0xa inside here. (when entering text) only 0x0d terminate string. (secoatan.fpm version 313)
						t.a_s = ReadStringIncl0xA(1); t.entityelement[t.e].eleprof.soundset4_s = t.a_s;
						//#else
						//t.a_s = ReadString ( 1 ); t.entityelement[t.e].eleprof.soundset4_s=t.a_s;
						//#endif
					}
					if (t.versionnumberload >= 311)
					{
						//  GameGuru 1.133B
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.specularperc = t.a_f;
					}
					if (t.versionnumberload >= 312)
					{
						//  GameGuru 1.14 EBE
						t.a = ReadLong(1); t.entityelement[t.e].iHasParentIndex = t.a;
					}
					#ifdef VRTECH
					if (t.versionnumberload >= 313)
					{
						// VRQ V3
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.voiceset_s = t.a_s;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.voicerate = t.a;
					}
					#endif
					#ifdef WICKEDENGINE
					if (t.versionnumberload >= 314)
					{
						//PE: we need to copy t.entityprofile[t.ttentid].WEMaterial before customizing.
						int tmaster = t.entityelement[t.e].bankindex;
						if (tmaster < t.entityprofile.size())
						{
							t.entityelement[t.e].eleprof.WEMaterial = t.entityprofile[tmaster].WEMaterial;
						}

						t.a = ReadLong(1); t.entityelement[t.e].eleprof.bCustomWickedMaterialActive = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.MaterialActive = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bCastShadows[0] = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bDoubleSided[0] = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bPlanerReflection[0] = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bTransparency[0] = t.a;

						unsigned long ulValue = 0;
						t.a_s = ReadString(1);
						sscanf(t.a_s.Get(), "%lu", &ulValue);
						t.entityelement[t.e].eleprof.WEMaterial.dwBaseColor[0] = ulValue;

						t.a_s = ReadString(1);
						sscanf(t.a_s.Get(), "%lu", &ulValue);
						t.entityelement[t.e].eleprof.WEMaterial.dwEmmisiveColor[0] = ulValue;

						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fReflectance[0] = t.a_f;

						// LB: in previous builds, only one mesh material details where stored, we can retain this for single material objects in slot 0
						t.a = ReadLong(1);
						int iFrameAndWEMaterialSlotIndex = 0;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						t.a_s = ReadString(1);
						#ifndef DISABLEOCCLUSIONMAP
						t.entityelement[t.e].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
						#endif
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] = t.a_f;
					}
					if (t.versionnumberload >= 315)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.light.fLightHasProbe = t.a;
					}

					if (t.versionnumberload >= 316)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectLinkID = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iCharAlliance = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iCharFaction = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved1 = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved2 = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectReserved3 = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iCharPatrolMode = t.a;
						t.a = ReadFloat(1); t.entityelement[t.e].eleprof.fCharRange[0] = t.a;
						t.a = ReadFloat(1); t.entityelement[t.e].eleprof.fCharRange[1] = t.a;
						for (int i = 0;i < 10;i++)
						{
							t.a = ReadFloat(1); t.entityelement[t.e].eleprof.fObjectDataReserved[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationships[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationshipsType[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.iObjectRelationshipsData[i] = t.a;
						}
					}
					if (t.versionnumberload >= 317)
					{
						for (int i = 1; i < MAXMESHMATERIALS; i++)
						{
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bCastShadows[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bDoubleSided[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bPlanerReflection[i] = t.a;
							t.a = ReadLong(1); t.entityelement[t.e].eleprof.WEMaterial.bTransparency[i] = t.a;

							unsigned long ulValue = 0;
							t.a_s = ReadString(1);
							sscanf(t.a_s.Get(), "%lu", &ulValue);
							t.entityelement[t.e].eleprof.WEMaterial.dwBaseColor[i] = ulValue;

							t.a_s = ReadString(1);
							sscanf(t.a_s.Get(), "%lu", &ulValue);
							t.entityelement[t.e].eleprof.WEMaterial.dwEmmisiveColor[i] = ulValue;

							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fReflectance[i] = t.a_f;

							int iFrameAndWEMaterialSlotIndex = i;
							t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = ReadString(1); t.entityelement[t.e].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							t.a_s = ReadString(1);
							#ifndef DISABLEOCCLUSIONMAP
							t.entityelement[t.e].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex] = t.a_s;
							#endif
							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] = t.a_f;
							t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] = t.a_f;
						}
					}
					if (t.versionnumberload >= 318)
					{
						t.a = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRenderOrderBias[0] = t.a;
						for (int i = 1; i < MAXMESHMATERIALS; i++)
						{
							t.a = ReadFloat(1); t.entityelement[t.e].eleprof.WEMaterial.fRenderOrderBias[i] = t.a;
						}
					}
					if (t.versionnumberload >= 319)
					{
						//PE: The same group data is stored under ALL t.e and get reset each time.
						//PE: So level with 1000 objects , WILL setup groups and load ALL group images 1000 times ?
						//PE: We cant change old level now, so use this hack.
						//PE: Also this was leaking mem , not sure where. anyway this hack fix it. New maps will only save it under t.e == 1

						bool bFirstTime = false;
						if (t.e == 1) bFirstTime = true;

						extern int g_iUniqueGroupID;
						t.a = ReadLong(1); g_iUniqueGroupID = t.a;
						#define MAXGROUPSLISTS 100 // duplicated in GridEdit.cpp (would replace this when the group list is dynamic)
						extern std::vector<sRubberBandType> vEntityGroupList[MAXGROUPSLISTS];
						int iNumberOfGroups = 0;
						t.a = ReadLong(1); iNumberOfGroups = t.a;
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							int iItemsInThisGroup = 0;
							t.a = ReadLong(1); iItemsInThisGroup = t.a;
							if (bFirstTime) vEntityGroupList[gi].clear();
							for (int i = 0; i < iItemsInThisGroup; i++)
							{
								sRubberBandType item;
								t.a = ReadLong(1); item.iGroupID = t.a;
								t.a = ReadLong(1); item.iParentGroupID = t.a;
								t.a = ReadLong(1); item.e = t.a;
								t.a = ReadFloat(1); item.x = t.a;
								t.a = ReadFloat(1); item.y = t.a;
								t.a = ReadFloat(1); item.z = t.a;
								t.a = ReadFloat(1); item.quatAngle.x = t.a;
								t.a = ReadFloat(1); item.quatAngle.y = t.a;
								t.a = ReadFloat(1); item.quatAngle.z = t.a;
								t.a = ReadFloat(1); item.quatAngle.w = t.a;
								if (bFirstTime) vEntityGroupList[gi].push_back(item);
							}
						}
						// and load in group thumb images, and load them into the iEntityGroupListImage image list (so can see them in groups tab)
						extern int iEntityGroupListImage[MAXGROUPSLISTS];
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							if (bFirstTime) iEntityGroupListImage[gi] = 0;
							t.a = ReadLong(1); int iHasImage = t.a;
							if (iHasImage == 1 && bFirstTime)
							{
								char pGroupImgFilename[MAX_PATH];
								sprintf(pGroupImgFilename, "%sgroupimg%d.png", g.mysystem.levelBankTestMap_s.Get(), gi);
								if (FileExist(pGroupImgFilename) == 1)
								{
									//Find free image id.
									int iImageID = 0;
									for (int i = 0; i < MAXGROUPSLISTS; i++)
									{
										bool bAlreadyUsed = false;
										//#define BACKBUFFERIMAGE (g.perentitypromptimageoffset+9000) // duplicated in GridEdit.cpp
										//int iNewImageID = BACKBUFFERIMAGE + i;
										//g.perentitypromptimageoffset = 110000; // allow 10,000 slots (found in Common.cpp)
										int iNewImageID = (110000 + 9000) + i;// (g.perentitypromptimageoffset + 9000) + i;
										for (int l = MAXGROUPSLISTS; l > 0; l--)
										{
											if (iEntityGroupListImage[l] == iNewImageID)
											{
												bAlreadyUsed = true;
												break;
											}
										}
										if (!bAlreadyUsed)
										{
											iImageID = iNewImageID;
											break;
										}
									}
									if (iImageID != 0)
									{
										image_setlegacyimageloading(true);
										LoadImage(pGroupImgFilename, iImageID);
										image_setlegacyimageloading(false);
										iEntityGroupListImage[gi] = iImageID;
									}
								}
							}
						}
					}

					if (t.versionnumberload >= 320)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Preview = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Show_At_Start = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Looping_Animation = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.newparticle.bParticle_Full_Screen = t.a;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Duration = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadein = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Fullscreen_Fadeout = t.a_f;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.newparticle.Particle_Fullscreen_Transition = t.a_s;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Speed = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.newparticle.fParticle_Opacity = t.a_f;
					}
					if (t.versionnumberload >= 321)
					{
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.newparticle.emittername = t.a_s;
					}
					if (t.versionnumberload >= 322)
					{
						t.a_f = ReadFloat(1); t.entityelement[t.e].fDecalSpeed = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].fDecalOpacity = t.a_f;
					}
					if (t.versionnumberload >= 323)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iOverrideCollisionMode = t.a;
					}
					if (t.versionnumberload >= 324)
					{
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.weapondamagemultiplier = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].eleprof.meleedamagemultiplier = t.a_f;
					}
					if (t.versionnumberload >= 325)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iAffectedByGravity = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iMoveSpeed = t.a;
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iTurnSpeed = t.a;
					}

					if (t.versionnumberload >= 326)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.light.offsetup = t.a; //Store spot radius.
					}
					if (t.versionnumberload >= 327)
					{
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset5_s = t.a_s;
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.soundset6_s = t.a_s;
					}
					if (t.versionnumberload >= 328)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.iUseSoundVariants = t.a;
					}
					if (t.versionnumberload >= 329)
					{
						t.a = ReadLong(1); t.entityelement[t.e].quatmode = t.a;
						t.a_f = ReadFloat(1); t.entityelement[t.e].quatx = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].quaty = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].quatz = t.a_f;
						t.a_f = ReadFloat(1); t.entityelement[t.e].quatw = t.a_f;
					}
					if (t.versionnumberload >= 330)
					{
						t.a = ReadLong(1); t.entityelement[t.e].eleprof.bAutoFlatten = t.a;
					}
					if (t.versionnumberload >= 331)
					{
						t.a_s = ReadString(1); t.entityelement[t.e].eleprof.overrideanimset_s = t.a_s;
					}					
					#endif

					// get the index of the entity profile
					t.ttentid = t.entityelement[t.e].bankindex;
					#ifdef VRTECH
					if (t.ttentid >= t.entityprofile.size())
					{
						// somehow, the entity profile bank index was corrupted
						t.ttentid = 0;
						t.entityelement[t.e].bankindex = 0;
						continue;
					}
					#endif

					// fill in the blanks if load older version
					if (t.versionnumberload < 103)
					{
						//  Version 1.03 - V1 draft physics (-1 means calculate at entobj-loadtime)
						t.entityelement[t.e].eleprof.physics = t.entityprofile[t.ttentid].physics;
						t.entityelement[t.e].eleprof.phyweight = t.entityprofile[t.ttentid].phyweight;
						t.entityelement[t.e].eleprof.phyfriction = t.entityprofile[t.ttentid].phyfriction;
						t.entityelement[t.e].eleprof.phyforcedamage = t.entityprofile[t.ttentid].phyforcedamage;
						t.entityelement[t.e].eleprof.rotatethrow = t.entityprofile[t.ttentid].rotatethrow;
						t.entityelement[t.e].eleprof.explodable = t.entityprofile[t.ttentid].explodable;
						//t.entityelement[t.e].eleprof.phydw3=0;
						//t.entityelement[t.e].eleprof.phydw4=0;
						//t.entityelement[t.e].eleprof.phydw5=0;
					}
					if (t.versionnumberload < 104)
					{
						//  Version 1.04 - BETA4 extra field
						t.entityelement[t.e].eleprof.phyalways = t.entityprofile[t.ttentid].phyalways;
					}
					if (t.versionnumberload < 105)
					{
						//  Version 1.05 - BETA8
						t.entityelement[t.e].eleprof.spawndelayrandom = t.entityprofile[t.ttentid].spawndelayrandom;
						t.entityelement[t.e].eleprof.spawnqtyrandom = t.entityprofile[t.ttentid].spawnqtyrandom;
						t.entityelement[t.e].eleprof.spawnvel = t.entityprofile[t.ttentid].spawnvel;
						t.entityelement[t.e].eleprof.spawnvelrandom = t.entityprofile[t.ttentid].spawnvelrandom;
						t.entityelement[t.e].eleprof.spawnangle = t.entityprofile[t.ttentid].spawnangle;
						t.entityelement[t.e].eleprof.spawnanglerandom = t.entityprofile[t.ttentid].spawnanglerandom;
					}
					if (t.versionnumberload < 106)
					{
						//  Version 1.06 - BETA10
						t.entityelement[t.e].eleprof.spawnatstart = t.entityprofile[t.ttentid].spawnatstart;
						t.entityelement[t.e].eleprof.spawnlife = t.entityprofile[t.ttentid].spawnlife;
					}
					if (t.versionnumberload < 217)
					{
						//  FPGC - 300710 - older levels dont use particle override
						t.entityelement[t.e].eleprof.particleoverride = 0;
					}
					if (t.versionnumberload < 303)
					{
						//  Reloaded BETA 1.007
						t.entityelement[t.e].eleprof.animspeed = t.entityprofile[t.ttentid].animspeed;
					}
					if (t.versionnumberload < 304)
					{
						//  Reloaded BETA 1.007-200514
						t.entityelement[t.e].eleprof.conerange = t.entityprofile[t.ttentid].conerange;
					}
					if (t.versionnumberload < 306)
					{
						//  GameGuru 1.00.010
						t.entityelement[t.e].eleprof.isviolent = t.entityprofile[t.ttentid].isviolent;
					}
					if (t.versionnumberload < 307)
					{
						//  GameGuru 1.00.020
						t.entityelement[t.e].eleprof.teamfield = t.entityprofile[t.ttentid].teamfield;
					}
					if (t.versionnumberload < 310)
					{
						//  GameGuru 1.133
						t.entityelement[t.e].eleprof.isocluder = t.entityprofile[t.ttentid].isocluder;
						t.entityelement[t.e].eleprof.isocludee = t.entityprofile[t.ttentid].isocludee;
						t.entityelement[t.e].eleprof.colondeath = t.entityprofile[t.ttentid].colondeath;
						t.entityelement[t.e].eleprof.parententityindex = t.entityprofile[t.ttentid].parententityindex;
						t.entityelement[t.e].eleprof.parentlimbindex = t.entityprofile[t.ttentid].parentlimbindex;
						t.entityelement[t.e].eleprof.soundset2_s = t.entityprofile[t.ttentid].soundset2_s;
						t.entityelement[t.e].eleprof.soundset3_s = t.entityprofile[t.ttentid].soundset3_s;
						t.entityelement[t.e].eleprof.soundset4_s = t.entityprofile[t.ttentid].soundset4_s;
					}
					if (t.versionnumberload < 311)
					{
						//  GameGuru 1.133B
						t.entityelement[t.e].eleprof.specularperc = t.entityprofile[t.ttentid].specularperc;
					}
					if (t.versionnumberload < 312)
					{
						//  GameGuru 1.14 EBE
						t.entityelement[t.e].iHasParentIndex = 0;
					}
					#ifdef VRTECH
					if (t.versionnumberload < 313)
					{
						// VRQ V3
						t.entityelement[t.e].eleprof.voiceset_s = t.entityprofile[t.ttentid].voiceset_s;
						t.entityelement[t.e].eleprof.voicerate = t.entityprofile[t.ttentid].voicerate;
					}
					#endif

					t.entityelement[t.e].entitydammult_f = 1.0;
					t.entityelement[t.e].entityacc = 1.0;

					// 131115 - transparency control was removed from GG properties IDE, so ensure
					// it reflects the latest entity profile information (until we allow this value back in)
					t.entityelement[t.e].eleprof.transparency = t.entityprofile[t.ttentid].transparency;
				}
			}
		}
		else
		{
			t.failedtoload = 1;
		}
		CloseFile(1);

		// 050416 - remove any weapons from start marker if parental control, no weapon, no violence
		if (g.quickparentalcontrolmode == 2)
		{
			for (t.e = 1; t.e <= g.entityelementlist; t.e++)
			{
				t.entid = t.entityelement[t.e].bankindex;
				if (t.entityprofile[t.entid].ismarker == 1)
				{
					//  Player Start Marker Settings
					t.entityelement[t.e].eleprof.hasweapon_s = "";
					t.entityelement[t.e].eleprof.hasweapon = 0;
					t.entityelement[t.e].eleprof.quantity = 0;
					t.entityelement[t.e].eleprof.isviolent = 0;
				}
			}
		}

		//  If replacement file active, can swap in new SCRIPT and SOUND references
		if (Len(t.editor.replacefilepresent_s.Get()) > 1)
		{
			//  now go through ELEPROF enrties to update any SCRIPTBANK references and SOUNDSET references
			for (t.e = 1; t.e <= g.entityelementlist; t.e++)
			{
				#ifdef WICKEDENGINE
				for (t.tcheck = 1; t.tcheck <= 8; t.tcheck++)
				#else
				for (t.tcheck = 1; t.tcheck <= 6; t.tcheck++)
				#endif
				{
					if (t.tcheck == 1)  t.tcheck_s = t.entityelement[t.e].eleprof.aimain_s;
					if (t.tcheck == 2)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset_s;
					if (t.tcheck == 3)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset1_s;
					if (t.tcheck == 4)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset2_s;
					if (t.tcheck == 5)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset3_s;
					if (t.tcheck == 6)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset4_s;
					#ifdef WICKEDENGINE
					if (t.tcheck == 7)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset5_s;
					if (t.tcheck == 8)  t.tcheck_s = t.entityelement[t.e].eleprof.soundset6_s;
					#endif
					t.ttry_s = "";
					for (t.nn = 1; t.nn <= Len(t.tcheck_s.Get()); t.nn++)
					{
						t.ttry_s = t.ttry_s + Mid(t.tcheck_s.Get(), t.nn);
						if ((cstr(Mid(t.tcheck_s.Get(), t.nn)) == "\\" && cstr(Mid(t.tcheck_s.Get(), t.nn + 1)) == "\\") || (cstr(Mid(t.tcheck_s.Get(), t.nn)) == "/" && cstr(Mid(t.tcheck_s.Get(), t.nn + 1)) == "/"))
						{
							++t.nn;
						}
					}
					t.ttry_s = Lower(t.ttry_s.Get());
					for (t.tt = 1; t.tt <= t.treplacementmax; t.tt++)
					{
						if (t.replacements_s[t.tt][0] == t.ttry_s)
						{
							//  found entry we can replace
							if (t.tcheck == 1) { t.entityelement[t.e].eleprof.aimain_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 2) { t.entityelement[t.e].eleprof.soundset_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 3) { t.entityelement[t.e].eleprof.soundset1_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 4) { t.entityelement[t.e].eleprof.soundset2_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 5) { t.entityelement[t.e].eleprof.soundset3_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 6) { t.entityelement[t.e].eleprof.soundset4_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							#ifdef WICKEDENGINE
							if (t.tcheck == 7) { t.entityelement[t.e].eleprof.soundset5_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							if (t.tcheck == 8) { t.entityelement[t.e].eleprof.soundset6_s = t.replacements_s[t.tt][1]; t.tt = t.treplacementmax + 1; }
							#endif
						}
					}
				}
			}
			//  free usages
			UnDim(t.replacements_s);
		}
	}

	// and erase any elements that DO NOT have a valid profile (file moved/deleted)
	if (t.failedtoload == 1)
	{
		//  FPGC - 270410 - if entity binary from X10 (or just not supported), ensure NO entities!
		g.entityelementlist = 0;
		g.entityelementmax = 0;
	}
	else
	{
		for (t.e = 1; t.e <= g.entityelementlist; t.e++)
		{
			t.entid = t.entityelement[t.e].bankindex;
			if (t.entid > 0)
			{
				if (t.entid > ArrayCount(t.entitybank_s))
				{
					t.entityelement[t.e].bankindex = 0;
				}
				else
				{
					if (Len(t.entitybank_s[t.entid].Get()) == 0)
					{
						//  030715 - but only erase if entity not a marker
						if (t.entityprofile[t.entid].ismarker == 0)
						{
							t.entityelement[t.e].bankindex = 0;
						}
					}
				}
			}
		}
	}
}

// class to write in two passes, first adds up total size, second creates and fills the data buffer
class EntityWriter
{
protected:

	unsigned char* pData = 0;
	unsigned int iDataSize = 0;
	unsigned int iMaxDataSize = 0;
	unsigned int doWrite = 0; // first pass = 0, second pass = 1

public:

	EntityWriter() {}
	~EntityWriter() { if ( pData ) delete [] pData; }

	unsigned char* GetData() { return pData; }
	unsigned int GetDataSize() { return iDataSize; }

	// call between passes
	void AllocateDataForWrite()
	{
		assert( !pData ); 
		if ( pData ) delete [] pData; // should't be called more than once, but prevent memory leak if it is
		pData = new unsigned char[ iDataSize ];
		iMaxDataSize = iDataSize;
		iDataSize = 0;
		doWrite = 1;
	}

	void WriteLong( int num ) 
	{
		const unsigned int elementSize = sizeof(int);
		if ( !doWrite ) iDataSize += elementSize;
		else
		{
			assert( (iDataSize + elementSize) <= iMaxDataSize );
			memcpy( pData + iDataSize, &num, elementSize );
			iDataSize += elementSize;
		}
	}
	
	void WriteFloat( float num ) 
	{
		const unsigned int elementSize = sizeof(float);
		if ( !doWrite ) iDataSize += elementSize;
		else
		{
			assert( (iDataSize + elementSize) <= iMaxDataSize );
			memcpy( pData + iDataSize, &num, elementSize );
			iDataSize += elementSize;
		}
	}

	void WriteString( const char* str ) 
	{
		unsigned int elementSize = strlen(str);
		if ( !doWrite ) iDataSize += elementSize + 2;
		else
		{
			if ( elementSize )
			{
				assert( (iDataSize + elementSize) <= iMaxDataSize );
				memcpy( pData + iDataSize, str, elementSize );
				iDataSize += elementSize;
			}

			pData[ iDataSize ] = 13;
			pData[ iDataSize + 1 ] = 10;
			iDataSize += 2;
		}
	}
};

void entity_saveelementsdata ( void )
{
	//  Uses elementfilename$
	if ( t.elementsfilename_s == ""  )  t.elementsfilename_s = g.mysystem.levelBankTestMap_s+"map.ele";

	//  Reduce list size if later elements blank
	int temp = g.entityelementlist;
	while ( temp > 0 ) 
	{
		if ( t.entityelement[temp].maintype == 0  )  --temp; else break;
	}
	g.entityelementlist = temp;

	//  Save entity element list
	#ifdef VRTECH
	t.versionnumbersave = 313;
	#else
	t.versionnumbersave = 312;
	#endif
	#ifdef WICKEDENGINE
	t.versionnumbersave = 331; //329; //327;// 323; //322; //319;
	#endif

	EntityWriter writer;

	// write in two passes, first adds up total data size, second pass allocates and writes to the buffer
	for( int pass = 0; pass < 2; pass++ )
	{
		writer.WriteLong ( t.versionnumbersave );
		writer.WriteLong ( g.entityelementlist );
		if ( g.entityelementlist>0 ) 
		{
			for ( int ent = 1 ; ent<=  g.entityelementlist; ent++ )
			{
				if ( t.versionnumbersave >= 101 ) 
				{
					//  Version 1.01 - EA
					writer.WriteLong( t.entityelement[ent].maintype );
					writer.WriteLong( t.entityelement[ent].bankindex );
					writer.WriteLong( t.entityelement[ent].staticflag );
					writer.WriteFloat( t.entityelement[ent].x );
					writer.WriteFloat( t.entityelement[ent].y );
					writer.WriteFloat( t.entityelement[ent].z );
					writer.WriteFloat( t.entityelement[ent].rx );
					writer.WriteFloat( t.entityelement[ent].ry );
					writer.WriteFloat( t.entityelement[ent].rz );
					writer.WriteString( t.entityelement[ent].eleprof.name_s.Get() );
					//t.a_s=t.entityelement[ent].eleprof.aiinit_s ; //PE: Not used anymore.
					writer.WriteString ( "" );
					writer.WriteString( t.entityelement[ent].eleprof.aimain_s.Get() );
					// t.a_s=t.entityelement[ent].eleprof.aidestroy_s ; //PE: Not used anymore.
					writer.WriteString ( "" );
					writer.WriteLong( t.entityelement[ent].eleprof.isobjective );
					writer.WriteString( t.entityelement[ent].eleprof.usekey_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.ifused_s.Get() );
					//t.a_s=t.entityelement[ent].eleprof.ifusednear_s ;
					writer.WriteString ( "" );
					writer.WriteLong( t.entityelement[ent].eleprof.uniqueelement );
					writer.WriteString( t.entityelement[ent].eleprof.texd_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.texaltd_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.effect_s.Get() );
					writer.WriteLong( t.entityelement[ent].eleprof.transparency );
					writer.WriteLong( t.entityelement[ent].editorfixed );
					writer.WriteString( t.entityelement[ent].eleprof.soundset_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.soundset1_s.Get() );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnmax );
					writer.WriteLong( t.entityelement[ent].eleprof.spawndelay );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnqty );
					writer.WriteLong( t.entityelement[ent].eleprof.hurtfall );
					writer.WriteLong( t.entityelement[ent].eleprof.castshadow );
					writer.WriteLong( t.entityelement[ent].eleprof.reducetexture );
					writer.WriteLong( t.entityelement[ent].eleprof.speed );
					// t.a_s=t.entityelement[ent].eleprof.aishoot_s ; //  //PE: Not used anymore.
					writer.WriteString ( "" );
					writer.WriteString( t.entityelement[ent].eleprof.hasweapon_s.Get() );
					writer.WriteLong( t.entityelement[ent].eleprof.lives );
					writer.WriteLong( t.entityelement[ent].spawn.max );
					writer.WriteLong( t.entityelement[ent].spawn.delay );
					writer.WriteLong( t.entityelement[ent].spawn.qty );
					writer.WriteFloat( t.entityelement[ent].eleprof.scale );
					writer.WriteFloat( t.entityelement[ent].eleprof.coneheight );
					writer.WriteFloat( t.entityelement[ent].eleprof.coneangle );
					writer.WriteLong( t.entityelement[ent].eleprof.strength );
					writer.WriteLong( t.entityelement[ent].eleprof.isimmobile );
					writer.WriteLong( t.entityelement[ent].eleprof.cantakeweapon );
					writer.WriteLong( t.entityelement[ent].eleprof.quantity );
					writer.WriteLong( t.entityelement[ent].eleprof.markerindex );
					writer.WriteLong( t.entityelement[ent].eleprof.light.color & 0x00FFFFFF );
					writer.WriteLong( t.entityelement[ent].eleprof.light.range );
					writer.WriteLong( t.entityelement[ent].eleprof.trigger.stylecolor );
					writer.WriteLong( t.entityelement[ent].eleprof.trigger.waypointzoneindex );
					//t.a_s=t.entityelement[ent].eleprof.basedecal_s ;  //PE: Not used anymore.
					writer.WriteString ( "" );
				}
				if ( t.versionnumbersave >= 102 ) 
				{
					writer.WriteLong( t.entityelement[ent].eleprof.rateoffire );
					writer.WriteLong( t.entityelement[ent].eleprof.damage );
					writer.WriteLong( t.entityelement[ent].eleprof.accuracy );
					writer.WriteLong( t.entityelement[ent].eleprof.reloadqty );
					writer.WriteLong( t.entityelement[ent].eleprof.fireiterations );
					writer.WriteLong( t.entityelement[ent].eleprof.lifespan );
					writer.WriteFloat( t.entityelement[ent].eleprof.throwspeed );
					writer.WriteFloat( t.entityelement[ent].eleprof.throwangle );
					writer.WriteLong( t.entityelement[ent].eleprof.bounceqty );
					writer.WriteLong( t.entityelement[ent].eleprof.explodeonhit );
					writer.WriteLong( t.entityelement[ent].eleprof.weaponisammo );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnupto );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnafterdelay );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnwhendead );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
					writer.WriteLong( t.entityelement[ent].eleprof.perentityflags );
				}
				if ( t.versionnumbersave >= 103 ) 
				{
					//  V1 first draft - physics
					writer.WriteLong( t.entityelement[ent].eleprof.physics );
					writer.WriteLong( t.entityelement[ent].eleprof.phyweight );
					writer.WriteLong( t.entityelement[ent].eleprof.phyfriction );
					writer.WriteLong( t.entityelement[ent].eleprof.phyforcedamage );
					writer.WriteLong( t.entityelement[ent].eleprof.rotatethrow );
					writer.WriteLong( t.entityelement[ent].eleprof.explodable );
					writer.WriteLong( t.entityelement[ent].eleprof.explodedamage );
					//t.a=t.entityelement[ent].eleprof.phydw4 ;
					writer.WriteLong( 0 );
					//t.a=t.entityelement[ent].eleprof.phydw5 ;
					writer.WriteLong( 0 );
				}
				if ( t.versionnumbersave >= 104 ) 
				{
					//  Addition of new physics field for BETA4
					writer.WriteLong( t.entityelement[ent].eleprof.phyalways );
				}
				if ( t.versionnumbersave >= 105 ) 
				{
					//  Addition of new spawn fields for BETA8
					writer.WriteLong( t.entityelement[ent].eleprof.spawndelayrandom );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnqtyrandom );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnvel );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnvelrandom );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnangle );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnanglerandom );
				}
				if ( t.versionnumbersave >= 106 ) 
				{
					//  Addition of new fields for BETA10
					writer.WriteLong( t.entityelement[ent].eleprof.spawnatstart );
					writer.WriteLong( t.entityelement[ent].eleprof.spawnlife );
				}
				if ( t.versionnumbersave >= 107 ) 
				{
					//  FPSCV104RC8 - forgot to save infinilight index (dynamic lights in final build never worked)
					writer.WriteLong( t.entityelement[ent].eleprof.light.index );
				}
				if ( t.versionnumbersave >= 199 ) 
				{
					//  X10 specific version - any new save data must be higher than 200
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
				}
				if ( t.versionnumbersave >= 200 ) 
				{
					//  X10 specific version - any new save data must be higher than 200
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
					writer.WriteLong ( 0 );
				}
				if ( t.versionnumbersave >= 217 ) 
				{
					//  FPGC - 300710 - save new entity element data
					writer.WriteLong( t.entityelement[ent].eleprof.particleoverride );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.offsety );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.scale );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randomstartx );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randomstarty );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randomstartz );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.linearmotionx );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.linearmotiony );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.linearmotionz );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randommotionx );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randommotiony );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.randommotionz );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.mirrormode );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.camerazshift );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.scaleonlyx );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.lifeincrement );
					writer.WriteLong( t.entityelement[ent].eleprof.particle.alphaintensity );
				}
				if ( t.versionnumbersave >= 218 ) 
				{
					//  V118 - 060810 - knxrb - Decal animation setting (Added animation choice setting).
					writer.WriteLong( t.entityelement[ent].eleprof.particle.animated );
				}
				if ( t.versionnumbersave >= 301 ) 
				{
					//  Reloaded ALPHA 1.0045
					//t.a_s=t.entityelement[ent].eleprof.aiinitname_s ; //PE: Not used anymore.
					writer.WriteString ( "" );
					writer.WriteString( t.entityelement[ent].eleprof.aimainname_s.Get() );
					//t.a_s=t.entityelement[ent].eleprof.aidestroyname_s ;
					writer.WriteString ( "" );
					//t.a_s=t.entityelement[ent].eleprof.aishootname_s ;
					writer.WriteString ( "" );
				}
				if ( t.versionnumbersave >= 302 ) 
				{
					//  Reloaded BETA 1.005
				}
				if ( t.versionnumbersave >= 303 ) 
				{
					//  Reloaded BETA 1.007
					writer.WriteLong( t.entityelement[ent].eleprof.animspeed );
				}
				if ( t.versionnumbersave >= 304 ) 
				{
					//  Reloaded BETA 1.007-200514
					writer.WriteFloat( t.entityelement[ent].eleprof.conerange );
				}
				if ( t.versionnumbersave >= 305 ) 
				{
					//  Reloaded BETA 1.009
					writer.WriteFloat( t.entityelement[ent].scalex );
					writer.WriteFloat( t.entityelement[ent].scaley );
					writer.WriteFloat( t.entityelement[ent].scalez );
					writer.WriteLong( t.entityelement[ent].eleprof.range );
					writer.WriteLong( t.entityelement[ent].eleprof.dropoff );
				}
				if ( t.versionnumbersave >= 306 ) 
				{
					//  Guru 1.00.010
					writer.WriteLong( t.entityelement[ent].eleprof.isviolent );
				}
				if ( t.versionnumbersave >= 307 ) 
				{
					//  Guru 1.00.020
					writer.WriteLong( t.entityelement[ent].eleprof.teamfield );
				}
				if ( t.versionnumbersave >= 308 ) 
				{
					//  Guru 1.01.001
					writer.WriteLong( t.entityelement[ent].eleprof.usespotlighting );
				}
				if ( t.versionnumbersave >= 309 )
				{
					//  Guru 1.01.002
					writer.WriteLong( t.entityelement[ent].eleprof.lodmodifier );
				}
				if ( t.versionnumbersave >= 310 )
				{
					//  Guru 1.133
					writer.WriteLong( t.entityelement[ent].eleprof.isocluder );
					writer.WriteLong( t.entityelement[ent].eleprof.isocludee );
					writer.WriteLong( t.entityelement[ent].eleprof.colondeath );
					writer.WriteLong( t.entityelement[ent].eleprof.parententityindex );
					writer.WriteLong( t.entityelement[ent].eleprof.parentlimbindex );
					writer.WriteString( t.entityelement[ent].eleprof.soundset2_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.soundset3_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.soundset4_s.Get() );
				}
				if ( t.versionnumbersave >= 311 )
				{
					//  Guru 1.133B
					writer.WriteFloat( t.entityelement[ent].eleprof.specularperc );
				}
				if ( t.versionnumbersave >= 312 )
				{
					//  Guru 1.14 EBE
					writer.WriteLong( t.entityelement[ent].iHasParentIndex );
				}			
				#ifdef VRTECH
				if ( t.versionnumbersave >= 313 )
				{
					// VRQ V3
					writer.WriteString( t.entityelement[ent].eleprof.voiceset_s.Get() );
					writer.WriteLong( t.entityelement[ent].eleprof.voicerate );
				}
				#endif
				#ifdef WICKEDENGINE
				if (t.versionnumbersave >= 314)
				{
					//PE: We must save custom "Materials" here.

					writer.WriteLong( t.entityelement[ent].eleprof.bCustomWickedMaterialActive );
					writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.MaterialActive );
					writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bCastShadows[0] );
					writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bDoubleSided[0] );
					writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bPlanerReflection[0] );
					writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bTransparency[0] );

					char tas[256];
					sprintf(tas, "%lu", (unsigned long) t.entityelement[ent].eleprof.WEMaterial.dwBaseColor[0] );
					writer.WriteString( tas );
					sprintf(tas, "%lu", (unsigned long)t.entityelement[ent].eleprof.WEMaterial.dwEmmisiveColor[0] );
					writer.WriteString( tas );

					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fReflectance[0] );

					// header 9*4 = 36 bytes for each entry.
					// per mesh 11 * 4 = 44 bytes for a empty material.
					// LB: in previous builds, only one mesh material details where stored, we can retain this for single material objects in slot 0
					writer.WriteLong( 0 );
					int iFrameAndWEMaterialSlotIndex = 0;
					writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex].Get() );
					writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex].Get() );
					writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex].Get() );
					writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex].Get() );
					writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex].Get() );
					#ifndef DISABLEOCCLUSIONMAP
					writer.WriteString ( t.entityelement[ent].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex].Get() );
					#else
					writer.WriteString ( "" );
					#endif
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] );
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] );
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] );
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] );
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] );
				}
				if (t.versionnumbersave >= 315)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.light.fLightHasProbe );
				}
				if (t.versionnumbersave >= 316)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.iObjectLinkID );
					writer.WriteLong( t.entityelement[ent].eleprof.iCharAlliance );
					writer.WriteLong( t.entityelement[ent].eleprof.iCharFaction );
					writer.WriteLong( t.entityelement[ent].eleprof.iObjectReserved1 );
					writer.WriteLong( t.entityelement[ent].eleprof.iObjectReserved2 );
					writer.WriteLong( t.entityelement[ent].eleprof.iObjectReserved3 );
					writer.WriteLong( t.entityelement[ent].eleprof.iCharPatrolMode );
					writer.WriteFloat( t.entityelement[ent].eleprof.fCharRange[0] );
					writer.WriteFloat( t.entityelement[ent].eleprof.fCharRange[1] );
					for (int i = 0; i < 10; i++)
					{
						writer.WriteFloat( t.entityelement[ent].eleprof.fObjectDataReserved[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.iObjectRelationships[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.iObjectRelationshipsType[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.iObjectRelationshipsData[i] );
					}
				}
				if (t.versionnumbersave >= 317)
				{
					//PE: Save per mesh material settings.
					for (int i = 1; i < MAXMESHMATERIALS; i++)
					{
						writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bCastShadows[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bDoubleSided[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bPlanerReflection[i] );
						writer.WriteLong( t.entityelement[ent].eleprof.WEMaterial.bTransparency[i] );

						char tas[256];
						sprintf(tas, "%lu", (unsigned long)t.entityelement[ent].eleprof.WEMaterial.dwBaseColor[i] );
						writer.WriteString( tas );
						sprintf(tas, "%lu", (unsigned long)t.entityelement[ent].eleprof.WEMaterial.dwEmmisiveColor[i] );
						writer.WriteString( tas );

						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fReflectance[i] );

						int iFrameAndWEMaterialSlotIndex = i;
						writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.baseColorMapName[iFrameAndWEMaterialSlotIndex].Get() );
						writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.normalMapName[iFrameAndWEMaterialSlotIndex].Get() );
						writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.surfaceMapName[iFrameAndWEMaterialSlotIndex].Get() );
						writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.displacementMapName[iFrameAndWEMaterialSlotIndex].Get() );
						writer.WriteString( t.entityelement[ent].eleprof.WEMaterial.emissiveMapName[iFrameAndWEMaterialSlotIndex].Get() );
						#ifndef DISABLEOCCLUSIONMAP
						writer.WriteString ( t.entityelement[ent].eleprof.WEMaterial.occlusionMapName[iFrameAndWEMaterialSlotIndex].Get() );
						#else
						writer.WriteString ( "" );
						#endif
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fNormal[iFrameAndWEMaterialSlotIndex] );
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fRoughness[iFrameAndWEMaterialSlotIndex] );
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fMetallness[iFrameAndWEMaterialSlotIndex] );
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fEmissive[iFrameAndWEMaterialSlotIndex] );
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fAlphaRef[iFrameAndWEMaterialSlotIndex] );
					}
				}
				if (t.versionnumbersave >= 318)
				{
					writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fRenderOrderBias[0] );
					for (int i = 1; i < MAXMESHMATERIALS; i++)
					{
						writer.WriteFloat( t.entityelement[ent].eleprof.WEMaterial.fRenderOrderBias[i] );
					}
				}
				if (t.versionnumbersave >= 319)
				{
					//PE: No relation between ent and all groups information, so only store it under ent = 1
					extern int g_iUniqueGroupID;
					if (ent > 1)
					{
						writer.WriteLong( 0 );
						writer.WriteLong( 0 ); //PE: zero groups on other ent entrys.
					}
					else
					{
						writer.WriteLong( g_iUniqueGroupID );
	#define MAXGROUPSLISTS 100 // duplicated in GridEdit.cpp (would replace this when the group list is dynamic)
						extern std::vector<sRubberBandType> vEntityGroupList[MAXGROUPSLISTS];
						int iNumberOfGroups = MAXGROUPSLISTS;
						writer.WriteLong( iNumberOfGroups );
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							int iItemsInThisGroup = vEntityGroupList[gi].size( );
							writer.WriteLong( iItemsInThisGroup );
							for (int i = 0; i < iItemsInThisGroup; i++)
							{
								writer.WriteLong( vEntityGroupList[gi][i].iGroupID );
								writer.WriteLong( vEntityGroupList[gi][i].iParentGroupID );
								writer.WriteLong( vEntityGroupList[gi][i].e );
								writer.WriteFloat( vEntityGroupList[gi][i].x );
								writer.WriteFloat( vEntityGroupList[gi][i].y );
								writer.WriteFloat( vEntityGroupList[gi][i].z );
								writer.WriteFloat( vEntityGroupList[gi][i].quatAngle.x );
								writer.WriteFloat( vEntityGroupList[gi][i].quatAngle.y );
								writer.WriteFloat( vEntityGroupList[gi][i].quatAngle.z );
								writer.WriteFloat( vEntityGroupList[gi][i].quatAngle.w );
							}
						}
						// and save out group thumb images, as need these to identify parent groups
						extern int iEntityGroupListImage[MAXGROUPSLISTS];
						for (int gi = 0; gi < iNumberOfGroups; gi++)
						{
							int iImgIndex = iEntityGroupListImage[gi];
							char pGroupImgFilename[MAX_PATH];
							sprintf(pGroupImgFilename, "%sgroupimg%d.png", g.mysystem.levelBankTestMap_s.Get(), gi );
							if (FileExist(pGroupImgFilename) == 1) DeleteAFile(pGroupImgFilename );
							if (iEntityGroupListImage[gi] > 0)
							{
								if (ImageExist(iImgIndex) == 1)
								{
									// img value not important, only as reference to image file creating now for the loader
									SaveImage(pGroupImgFilename, iImgIndex );
								}
							}
							writer.WriteLong( iImgIndex > 0 ? 1 : 0 );
						}
					}
				}

				if (t.versionnumbersave >= 320)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Preview );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Show_At_Start );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Looping_Animation );
					writer.WriteLong( t.entityelement[ent].eleprof.newparticle.bParticle_Full_Screen );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Duration );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Fadein );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Fullscreen_Fadeout );
					writer.WriteString( t.entityelement[ent].eleprof.newparticle.Particle_Fullscreen_Transition.Get() );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Speed );
					writer.WriteFloat( t.entityelement[ent].eleprof.newparticle.fParticle_Opacity );

				}
				if (t.versionnumbersave >= 321)
				{
					writer.WriteString( t.entityelement[ent].eleprof.newparticle.emittername.Get() );
				}

				if (t.versionnumbersave >= 322)
				{
					writer.WriteFloat( t.entityelement[ent].fDecalSpeed );
					writer.WriteFloat( t.entityelement[ent].fDecalOpacity );
				}
				if (t.versionnumbersave >= 323)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.iOverrideCollisionMode );
				}
				if (t.versionnumbersave >= 324)
				{
					writer.WriteFloat( t.entityelement[ent].eleprof.weapondamagemultiplier );
					writer.WriteFloat( t.entityelement[ent].eleprof.meleedamagemultiplier );
				}
				if (t.versionnumbersave >= 325)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.iAffectedByGravity );
					writer.WriteLong( t.entityelement[ent].eleprof.iMoveSpeed );
					writer.WriteLong( t.entityelement[ent].eleprof.iTurnSpeed );
				}
				if (t.versionnumbersave >= 326)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.light.offsetup );  //Store spot radius.
				}

				if (t.versionnumbersave >= 327)
				{
					writer.WriteString( t.entityelement[ent].eleprof.soundset5_s.Get() );
					writer.WriteString( t.entityelement[ent].eleprof.soundset6_s.Get() );
				}

				if (t.versionnumbersave >= 328)
				{
					writer.WriteLong( t.entityelement[ent].eleprof.iUseSoundVariants );
				}
				if (t.versionnumbersave >= 329)
				{
					writer.WriteFloat(t.entityelement[ent].quatmode);
					writer.WriteFloat(t.entityelement[ent].quatx);
					writer.WriteFloat(t.entityelement[ent].quaty);
					writer.WriteFloat(t.entityelement[ent].quatz);
					writer.WriteFloat(t.entityelement[ent].quatw);
				}
				if (t.versionnumbersave >= 330)
				{
					writer.WriteFloat(t.entityelement[ent].eleprof.bAutoFlatten);
				}
				if (t.versionnumbersave >= 331)
				{
					writer.WriteString(t.entityelement[ent].eleprof.overrideanimset_s.Get());
				}
				#endif
			}
		} 

		if ( pass == 0 ) writer.AllocateDataForWrite();
	} // pass loop

	// write data buffer to file, EntityWriter will clean itself up
	if ( FileExist(t.elementsfilename_s.Get()) == 1  )  DeleteAFile ( t.elementsfilename_s.Get() );
	OpenToWrite ( 1, t.elementsfilename_s.Get() );
	WriteData( 1, writer.GetData(), writer.GetDataSize() );
	CloseFile ( 1 );
}

void entity_savebank ( void )
{
	//  Scan entire entityelement, delete all entitybank entries not used
	if (  g.gcompilestandaloneexe == 0 && g.gpretestsavemode == 0 ) 
	{
		if (  g.entidmaster>0 ) 
		{
			Dim ( t.entitybankused,g.entidmaster  );
			for ( t.tttentid = 1 ; t.tttentid<= g.entidmaster; t.tttentid++ )
			{
				t.entitybankused[t.tttentid]=0;
			}
			for ( t.ttte = 1 ; t.ttte <= g.entityelementlist; t.ttte++ )
			{
				t.tttentid = t.entityelement[t.ttte].bankindex;
				if (  t.tttentid > 0 && t.tttentid <= g.entidmaster ) 
				{
					t.entitybankused[t.tttentid]=1;
				}
			}
			for ( t.tttentid = 1 ; t.tttentid <= g.entidmaster; t.tttentid++ )
			{
				if (  t.entitybankused[t.tttentid] == 0 ) 
				{
					// free RLE data in profile
					ebe_freecubedata ( t.tttentid );
			
					//  remove entity entry if not used when save FPM
					t.entitybank_s[t.tttentid]="";
				}
			}
			//  shuffle to remove empty entries
			for ( t.tttentid = 1 ; t.tttentid <= g.entidmaster; t.tttentid++ )
			{
				//  not used to record where entities have been moved to
				t.entitybankused[t.tttentid]=0;
			}
			t.treadentid=1 ; t.tlargest=0;
			for ( t.tttentid = 1 ; t.tttentid<=  g.entidmaster; t.tttentid++ )
			{
				if (  t.treadentid <= g.entidmaster ) 
				{
					while (  t.entitybank_s[t.treadentid] == "" ) 
					{
						++t.treadentid ; if (  t.treadentid>g.entidmaster  )  break;
					}
					if (  t.treadentid <= g.entidmaster ) 
					{
						t.entitybank_s[t.tttentid] = t.entitybank_s[t.treadentid];
						t.entityprofileheader[t.tttentid]=t.entityprofileheader[t.treadentid];
						t.entityprofile[t.tttentid]=t.entityprofile[t.treadentid];
						if ( t.entityprofile[t.treadentid].ebe.pRLEData != NULL && t.tttentid < t.treadentid )
						{
							// if we are shifting an EBE entity into place
							// EBE entity parents can be saved shortly after, so ensure object is copied over
							t.sourceobj = g.entitybankoffset + t.tttentid;
							if ( ObjectExist ( t.sourceobj ) == 1 ) DeleteObject ( t.sourceobj );
							int iSourceObjBeingMoved = g.entitybankoffset + t.treadentid;
							if ( ObjectExist ( iSourceObjBeingMoved ) == 1 ) CloneObject ( t.sourceobj, iSourceObjBeingMoved );
							
							// this overrites regular saved EBE entities
							t.entitybank_s[t.tttentid] = cstr("EBE") + cstr(t.tttentid);
						}
						t.entityprofile[t.tttentid].ebe.dwRLESize=t.entityprofile[t.treadentid].ebe.dwRLESize;
						t.entityprofile[t.tttentid].ebe.pRLEData=t.entityprofile[t.treadentid].ebe.pRLEData;
						t.entityprofile[t.tttentid].ebe.dwMatRefCount=t.entityprofile[t.treadentid].ebe.dwMatRefCount;
						t.entityprofile[t.tttentid].ebe.iMatRef=t.entityprofile[t.treadentid].ebe.iMatRef;
						t.entityprofile[t.tttentid].ebe.dwTexRefCount=t.entityprofile[t.treadentid].ebe.dwTexRefCount;
						t.entityprofile[t.tttentid].ebe.pTexRef=t.entityprofile[t.treadentid].ebe.pTexRef;
						for ( t.tt = 0 ; t.tt <=  100 ; t.tt++ ) t.entitybodypart[t.tttentid][t.tt]=t.entitybodypart[t.treadentid][t.tt] ;
						for ( t.tt = 0 ; t.tt <=  g.animmax ; t.tt++ ) t.entityanim[t.tttentid][t.tt]=t.entityanim[t.treadentid][t.tt] ;
						for ( t.tt = 0 ; t.tt <=  g.footfallmax ; t.tt++ ) t.entityfootfall[t.tttentid][t.tt]=t.entityfootfall[t.treadentid][t.tt] ; 
						for ( t.tt = 0 ; t.tt <=  100 ; t.tt++ ) t.entitydecal_s[t.tttentid][t.tt]=t.entitydecal_s[t.treadentid][t.tt] ;
						for ( t.tt = 0 ; t.tt <=  100 ; t.tt++ ) t.entitydecal[t.tttentid][t.tt]=t.entitydecal[t.treadentid][t.tt] ;
						t.entitybankused[t.treadentid]=t.tttentid;
						t.tlargest=t.tttentid;
					}
					else
					{
						// wipe after end of shuffle
						t.entitybank_s[t.tttentid] = "";
						t.entityprofile[t.tttentid].ebe.dwRLESize = 0;
						t.entityprofile[t.tttentid].ebe.pRLEData = NULL;
						t.entityprofile[t.tttentid].ebe.dwMatRefCount = 0;
						t.entityprofile[t.tttentid].ebe.iMatRef = NULL;
						t.entityprofile[t.tttentid].ebe.dwTexRefCount = 0;
						t.entityprofile[t.tttentid].ebe.pTexRef = NULL;
					}
				}
				else
				{
					// wipe after end of shuffle
					t.entitybank_s[t.tttentid]="";
					t.entityprofile[t.tttentid].ebe.dwRLESize = 0;
					t.entityprofile[t.tttentid].ebe.pRLEData = NULL;
					t.entityprofile[t.tttentid].ebe.dwMatRefCount = 0;
					t.entityprofile[t.tttentid].ebe.iMatRef = NULL;
					t.entityprofile[t.tttentid].ebe.dwTexRefCount = 0;
					t.entityprofile[t.tttentid].ebe.pTexRef = NULL;
				}
				++t.treadentid;
			}
			//  update bank index numbers in entityelements
			for ( t.ttte = 1 ; t.ttte<=  g.entityelementlist; t.ttte++ )
			{
				t.tttentid=t.entityelement[t.ttte].bankindex;
				if (  t.tttentid>0 && t.tttentid <= g.entidmaster ) 
				{
					//  new entity entry place index
					t.entityelement[t.ttte].bankindex=t.entitybankused[t.tttentid];
				}
			}
			UnDim (  t.entitybankused );

			//  new list size
			if (  g.entidmaster != t.tlargest ) 
			{
				g.entidmaster=t.tlargest;
				t.entityorsegmententrieschanged=1;
			}
		}
	}

	//  Save entity bank
	cstr entitybank_s = g.mysystem.levelBankTestMap_s+"map.ent";
	if ( FileExist(entitybank_s.Get()) == 1 ) DeleteAFile ( entitybank_s.Get() );
	OpenToWrite ( 1, entitybank_s.Get() );
	WriteLong ( 1, g.entidmaster );
	if ( g.entidmaster>0 ) 
	{
		for ( t.entid = 1 ; t.entid <= g.entidmaster; t.entid++ )
		{
			WriteString ( 1, t.entitybank_s[t.entid].Get() );
		}
	}
	CloseFile ( 1 );
}

void entity_savebank_ebe ( void )
{
	// Empty EBEs from testmap folder
	// 190417 - oops, this is deleting perfectly needed textures from testmap folder
	// when it should leave textures alone that may belong to the level EBEs
	// so set a flag to protect textures when save
	cstr pStoreOld = GetDir(); SetDir ( g.mysystem.levelBankTestMap_s.Get() ); //"levelbank\\testmap\\" );
	mapfile_emptyebesfromtestmapfolder(true);
	SetDir ( pStoreOld.Get() );

	// now save all EBE to testmap folder
	for ( t.tttentid = 1 ; t.tttentid <= g.entidmaster; t.tttentid++ )
	{
		if ( strlen(t.entitybank_s[t.tttentid].Get()) > 0 )
		{
			if ( t.entityprofile[t.tttentid].ebe.dwRLESize > 0 )
			{
				// Save EBE to represent this creation in the level
				//cStr tSaveFile = cstr("levelbank\\testmap\\ebe") + cstr(t.tttentid) + cstr(".ebe");
				cStr tSaveFile = g.mysystem.levelBankTestMap_s + cstr("ebe") + cstr(t.tttentid) + cstr(".ebe");
				
				ebe_save_ebefile ( tSaveFile, t.tttentid );
			}
		}
	}
}

void entity_loadbank ( void )
{
	// 050416 - build a black list for parental control (used below)
	#ifdef VRTECH
	 // No blacklist as all content is pre-vetted and clean
	#else
	 if ( g_pBlackList == NULL && g.quickparentalcontrolmode == 2 )
	 {
		// first pass count, second one writes into array
		for ( int iPass = 0; iPass < 2; iPass++ )
		{
			// reset count
			g_iBlackListMax = 0;

			// open blacklist file and go through all contents
			FILE* fp = GG_fopen ( ".\\..\\parentalcontrolblacklist.ini", "rt" );
			if ( fp )
			{
				char c;
				fread ( &c, sizeof ( char ), 1, fp );
				while ( !feof ( fp ) )
				{
					// get string from file
					char szEntNameFromFile [ MAX_PATH ] = "";
					int iOffset = 0;
					while ( !feof ( fp ) && c!=13 && c!=10 )
					{
						szEntNameFromFile [ iOffset++ ] = c;
						fread ( &c, sizeof ( char ), 1, fp );
					}
					szEntNameFromFile [ iOffset ] = 0;

					// skip beyond CR
					while ( !feof ( fp ) && (c==13 || c==10) )
						fread ( &c, sizeof ( char ), 1, fp );

					// count or write
					if ( iPass==0 )
					{
						// count
						g_iBlackListMax++;
					}
					else
					{
						// write into array
						g_pBlackList[g_iBlackListMax] = new char[512];
						strlwr ( szEntNameFromFile );
						strcpy ( g_pBlackList[g_iBlackListMax], szEntNameFromFile );
						g_iBlackListMax++;
					}
				}
				fclose ( fp );
			}
			
			// at end, create dynamic string array
			if ( iPass==0 ) g_pBlackList = new LPSTR[g_iBlackListMax];
		}
	 }
	#endif

	//  If ent file exists
	t.filename_s=t.levelmapptah_s+"map.ent";
	if (  FileExist(t.filename_s.Get()) == 1 ) 
	{
		//  Destroy old entities
		entity_deletebank ( );

		//  Load entity bank
		OpenToRead (  1, cstr(t.levelmapptah_s+"map.ent").Get() );
		g.entidmaster = ReadLong ( 1 );
		if (  g.entidmaster>0 ) 
		{
			entity_validatearraysize ( );
			for ( t.entid = 1 ; t.entid<=  g.entidmaster; t.entid++ )
			{
				t.entitybank_s[t.entid] = ReadString ( 1 );
			}
		}
		CloseFile (  1 );

		// 050416 - blank out any entities which are blacklisted
		g_bBlackListRemovedSomeEntities = false;
		if ( g_pBlackList != NULL )
		{
			char pThisEntityFilename[512];
			for ( t.entid = 1 ; t.entid <= g.entidmaster; t.entid++ )
			{
				strcpy ( pThisEntityFilename, t.entitybank_s[t.entid].Get() );
				strlwr ( pThisEntityFilename );
				for ( int iBlackListIndex=0; iBlackListIndex<g_iBlackListMax; iBlackListIndex++ )
				{
					int iBlacklistEntityNameLength = strlen ( g_pBlackList[iBlackListIndex] );
					int iCompareEnd = strlen ( pThisEntityFilename ) - iBlacklistEntityNameLength;
					if ( strnicmp ( g_pBlackList[iBlackListIndex], pThisEntityFilename + iCompareEnd, iBlacklistEntityNameLength )==NULL )
					{
						// this entity has been banned by parents
						g_bBlackListRemovedSomeEntities = true;
						t.entitybank_s[t.entid] = "";
					}
				}
			}
		}

		// 260215 - Do a pre-scan to determine if any entities are missing
		if ( Len(t.editor.replacefilepresent_s.Get())>1 ) 
		{
			// clear replacement output array
			Dim2( t.replacements_s, 1000, 1 );
			for ( int n=0; n < 1000; n++ )
			{
				t.replacements_s[n][0]="";
				t.replacements_s[n][1]="";
			}

			// load all replacements in a table
			Dim( t.replacementsinput_s, 1000 );
			t.treplacementmax=0;
			if ( FileExist(t.editor.replacefilepresent_s.Get()) == 1 ) 
			{
				LoadArray ( t.editor.replacefilepresent_s.Get(), t.replacementsinput_s );
				for ( t.l = 0 ; t.l <= ArrayCount(t.replacementsinput_s); t.l++ )
				{
					t.tline_s = t.replacementsinput_s[t.l];
					if ( Len(t.tline_s.Get()) > 0 ) 
					{
						for ( t.n = 1 ; t.n<=  Len(t.tline_s.Get()); t.n++ )
						{
							if (  cstr(Mid(t.tline_s.Get(),t.n)) == "=" ) 
							{
								t.told_s=Left(t.tline_s.Get(),t.n-1);
								t.told2_s="";
								for ( t.nn = 1 ; t.nn<=  Len(t.told_s.Get()); t.nn++ )
								{
									t.told2_s=t.told2_s+Mid(t.told_s.Get(),t.nn);
									if ( (cstr(Mid(t.told_s.Get(),t.nn)) == "\\" && cstr(Mid(t.told_s.Get(),t.nn+1)) == "\\") || (cstr(Mid(t.told_s.Get(),t.nn)) == "/" && cstr(Mid(t.told_s.Get(),t.nn+1)) == "/" ) )
									{
										++t.nn;
									}
								}
								t.tnew_s=Right(t.tline_s.Get(),Len(t.tline_s.Get())-t.n);
								++t.treplacementmax;
								t.replacements_s[t.treplacementmax][0]=Lower(t.told2_s.Get());
								t.replacements_s[t.treplacementmax][1]=Lower(t.tnew_s.Get());
							}
						}
					}
				}
			}
			//  go through all entities FPM is about to use
			for ( t.entid = 1 ; t.entid <= g.entidmaster; t.entid++ )
			{
				if ( strlen ( t.entitybank_s[t.entid].Get() ) > 0 )
				{
					LPSTR pEntityRef = t.entitybank_s[t.entid].Get();
					if ( pEntityRef[1] == ':')
					{
						// corrects for an ugly bug which exported entity bank references in absolute paths!
						t.tfile_s = t.entitybank_s[t.entid];
						for (t.tt = 1; t.tt <= t.treplacementmax; t.tt++)
						{
							LPSTR pReplaceSearch = t.replacements_s[t.tt][0].Get();
							LPSTR pThisEntFile = t.tfile_s.Get();
							if ( stricmp ( pReplaceSearch, pThisEntFile ) == NULL )
							{
								// found a match with an entry in the .REPLACE file
								// so replace master entry (entity instances will continue to reference by index)
								t.tnewent_s = t.replacements_s[t.tt][1];
								t.entitybank_s[t.entid] = Right(t.tnewent_s.Get(), Len(t.tnewent_s.Get()) - Len("entitybank\\"));
								break;
							}
						}
					}
					else
					{
						t.tfile_s = g.fpscrootdir_s + "\\Files\\entitybank\\" + t.entitybank_s[t.entid];
						if (FileExist(t.tfile_s.Get()) == 0)
						{
							t.tbinfile_s = cstr(Left(t.tfile_s.Get(), Len(t.tfile_s.Get()) - 4)) + cstr(".bin");
							if (FileExist(t.tbinfile_s.Get()) == 0)
							{
								//  cannot find FPE or BIN, so search replace file if we have a substitute
								t.tent_s = cstr("entitybank\\") + t.entitybank_s[t.entid];
								t.ttry2_s = "";
								for (t.nn = 1; t.nn <= Len(t.tent_s.Get()); t.nn++)
								{
									t.ttry2_s = t.ttry2_s + Mid(t.tent_s.Get(), t.nn);
									if ((cstr(Mid(t.tent_s.Get(), t.nn)) == "\\" && cstr(Mid(t.tent_s.Get(), t.nn + 1)) == "\\") || (cstr(Mid(t.tent_s.Get(), t.nn)) == "/" && cstr(Mid(t.tent_s.Get(), t.nn + 1)) == "/"))
									{
										++t.nn;
									}
								}
								t.tent_s = Lower(t.ttry2_s.Get());
								t.tfoundmatch = 0;
								for (t.tt = 1; t.tt <= t.treplacementmax; t.tt++)
								{
									if (t.replacements_s[t.tt][0] == t.tent_s)
									{
										//  found a match with an entry in the .REPLACE file
										//  so replace master entry (entity instances will continue to reference by index)
										t.tnewent_s = t.replacements_s[t.tt][1];
										t.entitybank_s[t.entid] = Right(t.tnewent_s.Get(), Len(t.tnewent_s.Get()) - Len("entitybank\\"));
										t.tfoundmatch = 1;
									}
								}
							}
						}
					}
				}
			}
			//  retain replacements$() for later in loadelementsdata
		}

		//  Load in all entity objects and data
		entity_loadentitiesnow ( );
	}
}

void entity_loadentitiesnow ( void )
{
	// Load entities specified by bank
	if ( g.entidmaster>0 ) 
	{
		char debug[MAX_PATH];
		sprintf(debug, "Loading master objects: %ld", g.entidmaster);
		extern int total_mem_from_load;
		total_mem_from_load = 0;
		timestampactivity(0, debug);
		for ( t.entid = 1 ; t.entid <= g.entidmaster; t.entid++ )
		{
			/* PE: debug huge levels.
			if (t.entid == 120)
			{
				timestampactivity(0, "Memory Dump:");
				DumpImageList(); // PE: Dump image usage after level.
				char debug[MAX_PATH];
				sprintf(debug,"total_mem_from_load: %.2f gb. (%.2f mb)", (float) total_mem_from_load / 1024.0 / 1024.0, (float) total_mem_from_load / 1024.0);
				timestampactivity(0, debug);
				//dump list.
				extern std::vector<sImageList> g_imageList;
				for (int i = 0; i < g_imageList.size(); i++)
				{
					if (g_imageList[i].pName != NULL)
					{
						sprintf(debug, "%s (%d kb.)", g_imageList[i].pName, g_imageList[i].iMemUsedKB);
						timestampactivity(0, debug);
					}
				}
				printf("tmp");
			}
			*/

			// set entity name and load it in
			t.entdir_s = "entitybank\\";
			t.ent_s = t.entitybank_s[t.entid];
			t.entpath_s = getpath(t.ent_s.Get());
			if (  t.lightmapper.onlyloadstaticentitiesduringlightmapper == 1 ) 
			{
				t.tonscreenprompt_s=cstr("Load ")+Str(t.entid)+" : "+Right(t.ent_s.Get(),Len(t.ent_s.Get())-1)  ; lm_flashprompt ( );
			}
			// if not an FPE, load FPE from EBE source
			if ( strcmp ( Lower(Right(t.ent_s.Get(),4)), ".fpe" ) != NULL )
			{
				// special EBE entity
				//ebe_load_ebefile ( cstr("levelbank\\testmap\\ebe") + cstr(t.entid) + cstr(".ebe"), t.entid );
				ebe_load_ebefile ( g.mysystem.levelBankTestMap_s + cstr("ebe") + cstr(t.entid) + cstr(".ebe"), t.entid );
				
				t.entityprofileheader[t.entid].desc_s = cstr("EBE") + cstr(t.entid);
				t.entdir_s = g.mysystem.levelBankTestMap_s; //"levelbank\\testmap\\";
				t.ent_s = cstr("ebe") + cstr(t.entid) + cstr(".fpe");
				t.entpath_s = "";
			}

			//LB: massive ommission - was this deleted, totally needed to detect and load 'groups'
			extern int g_iAbortedAsEntityIsGroupFileMode;
			g_iAbortedAsEntityIsGroupFileMode = 1;

			// regular FPE entity
			entity_load ( );
			if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
			if (  t.desc_s == "" ) 
			{
				// free RLE data in profile
				ebe_freecubedata ( t.entid );
			
				//  where entities have been lost, delete from list
				t.entitybank_s[t.entid]="";
			}

			#ifdef VRTECH
			// keep multiplayer alive
			if ( t.game.runasmultiplayer == 1 ) mp_refresh ( );
			#endif
		}
		timestampactivity(0, "End Loading master objects:");
	}
}

void entity_deletebank ( void )
{
	//  Destroy old entities
	if ( g.entidmastermax>0 ) 
	{
		for ( t.entid = 1 ; t.entid<=  g.entidmastermax; t.entid++ )
		{
			// delete parent entity object
			t.entobj = g.entitybankoffset+t.entid;
			if ( ObjectExist(t.entobj) == 1  ) DeleteObject (  t.entobj );

			#ifdef VRTECH
			#else
			characterkit_deleteBankObject ( );
			#endif

			// free RLE data in profile
			ebe_freecubedata ( t.entid );

			// wipe from table
			t.entitybank_s[t.entid]="";
		}
	}

	//  reset character creator bankoffset
	t.characterkitcontrol.bankOffset = 0;
	t.characterkitcontrol.count = 0; // 150216 - Dave needs to learn how to clean up after himself!

	//C++ISSUE ADDED THIS IN - but commented it out, left it here tho
	//deleteallinternalimages();

	//  Destroy profile data
	UnDim (  t.entityprofile );
	#ifdef DEFAULTMASTERENTITY
	Dim(t.entityprofile, DEFAULTMASTERENTITY);
	g.entidmastermax = DEFAULTMASTERENTITY;
	#else
	Dim(t.entityprofile, 100);
	g.entidmastermax = 100;
	#endif
	g.entidmaster=0;
}

void entity_deleteelementsdata ( void )
{
	//  Free any old elements
	entity_deleteelements ( );

	//  Clear counter for new load
	g.entityelementlist=0;
	g.entityelementmax=0;
}

void entity_deleteelements ( void )
{
	//  Quick deletes
	if ( g.entityelementlist > 0 ) 
	{
		DeleteObjects (  g.entityviewstartobj+1, g.entityviewstartobj+g.entityelementlist );
	}
	if ( g.entityattachmentindex > 0 ) 
	{
		DeleteObjects (  g.entityattachmentsoffset+1, g.entityattachmentsoffset+g.entityattachmentindex );
	}

	#ifdef VRTECH
	#else
	//  clear any character creator objects associated with this entity
	for ( t.ccobjToDelete = 1 ; t.ccobjToDelete<=  g.entityelementlist; t.ccobjToDelete++ )
	{
		characterkit_deleteEntity ( );
	}
	#endif

	//  set character creator offset back to 0 (which is a nice indicator that it is not in use also)
	t.characterkitcontrol.offset = 0;
}

void entity_assignentityparticletodecalelement ( void )
{
	if (  t.originatore>0 ) 
	{
		if (  t.entityelement[t.originatore].eleprof.particleoverride == 1 ) 
		{
			t.decalelement[t.d].particle=t.entityelement[t.originatore].eleprof.particle;
		}
	}
}

void entity_addentitytomap_core ( void )
{
	// called from _entity_addentitytomap and also _game_masterroot
	if ( t.gridentityoverwritemode == 0 )
	{
		// First see if we have a prfeference (when click object to cursor, ideally want it dropped back in same t.e
		t.tokay = 0;
		if (t.gridentitypreferelementindex > 0)
		{
			if (t.entityelement[t.gridentitypreferelementindex].maintype == 0)
			{
				t.tokay = t.gridentitypreferelementindex;
			}
			t.gridentitypreferelementindex = 0;
		}

		// Create new or use free entity element
		if ( t.tokay == 0 && g.entityelementlist > 0 ) 
		{
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				if ( t.entityelement[t.e].maintype == 0 ) { t.tokay = t.e; break; }
			}
		}
		if ( t.tokay == 0 ) 
		{
			++g.entityelementlist;
			t.e=g.entityelementlist;
			if ( g.entityelementlist>g.entityelementmax ) 
			{
				Dim ( t.storeentityelement,g.entityelementmax );
				for ( t.e = 1 ; t.e<=  g.entityelementmax; t.e++ )
				{
					t.storeentityelement[t.e]=t.entityelement[t.e];
				}
				UnDim (  t.entityelement );
				#ifdef VRTECH
				UnDim (  t.entityshadervar );
				#endif
				g.entityelementmax +=10;
				Dim (  t.entityelement,g.entityelementmax );
				#ifdef VRTECH
				Dim2(  t.entityshadervar,g.entityelementmax, g.globalselectedshadermax  );
				#endif
				for ( t.e = 1 ; t.e<=  g.entityelementmax-10; t.e++ )
				{
					t.entityelement[t.e]=t.storeentityelement[t.e];
				}
			}
		}
		else
		{
			t.e=t.tokay;
		}
	}
	else
	{
		// can force new entity into a specific slot (when undo entiy group delete)
		t.e = t.gridentityoverwritemode;
	}

	//  Fill entity element details
	t.entityelement[t.e].editorfixed=t.gridentityeditorfixed;
	t.entityelement[t.e].maintype=t.entitymaintype;
	t.entityelement[t.e].bankindex=t.entitybankindex;
	t.entityelement[t.e].staticflag=t.gridentitystaticmode;
	t.entityelement[t.e].iHasParentIndex=t.gridentityhasparent;
	t.entityelement[t.e].x=t.gridentityposx_f;
	t.entityelement[t.e].z=t.gridentityposz_f;
	t.entityelement[t.e].y=t.gridentityposy_f;
	t.entityelement[t.e].rx=t.gridentityrotatex_f;
	t.entityelement[t.e].ry=t.gridentityrotatey_f;
	t.entityelement[t.e].rz=t.gridentityrotatez_f;
	t.entityelement[t.e].quatmode = t.gridentityrotatequatmode;
	t.entityelement[t.e].quatx = t.gridentityrotatequatx_f;
	t.entityelement[t.e].quaty = t.gridentityrotatequaty_f;
	t.entityelement[t.e].quatz = t.gridentityrotatequatz_f;
	t.entityelement[t.e].quatw = t.gridentityrotatequatw_f;
	t.entityelement[t.e].scalex=t.gridentityscalex_f-100;
	t.entityelement[t.e].scaley=t.gridentityscaley_f-100;
	t.entityelement[t.e].scalez=t.gridentityscalez_f-100;
	t.entityelement[t.e].eleprof=t.grideleprof;
	t.entityelement[t.e].eleprof.light.index=0;
	t.entityelement[t.e].soundset=0;
	t.entityelement[t.e].soundset1=0;
	t.entityelement[t.e].soundset2=0;
	t.entityelement[t.e].soundset3=0;
	t.entityelement[t.e].soundset4 = 0;
	t.entityelement[t.e].soundset5 = 0;
	t.entityelement[t.e].soundset6 = 0;
	t.entityelement[t.e].underground=0;
	t.entityelement[t.e].beenmoved=1;
	#ifdef WICKEDENGINE
	t.entityelement[t.e].soundset5 = 0;
	t.entityelement[t.e].soundset6 = 0;
	t.entityelement[t.e].eleprof.iFlattenID = -1; // cannot carry this ID over
	if (!g_bEnableAutoFlattenSystem) //PE: If disabled always disable autoflatten.
		t.entityelement[t.e].eleprof.bAutoFlatten = false;

	t.entityelement[t.e].lua.outofrangefreeze = 0;
	#endif
}

void entity_addentitytomap ( void )
{
	// Entity To Add
	t.entitymaintype=1;
	t.entitybankindex=t.gridentity;
	entity_addentitytomap_core ( );

	//Debug mem use per object added.
	//void timestampactivity(int i, char* desc_s);
	//char timestampMsg[1024];
	//sprintf(timestampMsg, "Add Entity: (%ld) %s", t.e,t.entitybank_s[t.entid].Get());
	//timestampactivity(0, timestampMsg);

	// transfer waypoint zone index to entityelement
	t.waypointindex=t.grideleprof.trigger.waypointzoneindex;
	t.entityelement[t.e].eleprof.trigger.waypointzoneindex=t.waypointindex;
	t.waypoint[t.waypointindex].linkedtoentityindex=t.e;

	//  as create entity, apply any texture change required
	t.stentid=t.entid ; t.entid=t.entitybankindex;
	t.entdir_s="entitybank\\" ; t.ent_s=t.entitybank_s[t.entid] ; t.entpath_s=getpath(t.ent_s.Get());

	//  GRIDELEPROF might contain GUN+FLAK Data
	t.entid=t.entityelement[t.e].bankindex;
	t.tgunid_s=t.entityprofile[t.entid].isweapon_s;
	entity_getgunidandflakid ( );
	if (  t.tgunid>0 ) 
	{
		// populate the actual gun and flak settings (for further weapon entity creations)
		int firemode = 0; // 110718 - entity properties should only edit first primary gun settings (so we dont mess up enhanced weapons)
		//for ( int firemode = 0; firemode < 2; firemode++ )
		g.firemodes[t.tgunid][firemode].settings.damage=t.grideleprof.damage;
		g.firemodes[t.tgunid][firemode].settings.accuracy=t.grideleprof.accuracy;
		g.firemodes[t.tgunid][firemode].settings.reloadqty=t.grideleprof.reloadqty;
		g.firemodes[t.tgunid][firemode].settings.iterate=t.grideleprof.fireiterations;
		g.firemodes[t.tgunid][firemode].settings.range=t.grideleprof.range;
		g.firemodes[t.tgunid][firemode].settings.dropoff=t.grideleprof.dropoff;
		g.firemodes[t.tgunid][firemode].settings.usespotlighting=t.grideleprof.usespotlighting;
		if (  t.tflakid>0 ) 
		{
			// flak to follow
		}
		//  which must also populate ALL other entities of same weapon
		t.tgunidchanged=t.tgunid;
		for ( t.te = 1 ; t.te<=  g.entityelementlist; t.te++ )
		{
			t.tentid=t.entityelement[t.te].bankindex;
			t.tgunid_s=t.entityprofile[t.tentid].isweapon_s;
			entity_getgunidandflakid ( );
			if (  t.tgunid == t.tgunidchanged ) 
			{
				t.entityelement[t.te].eleprof.damage=t.grideleprof.damage;
				t.entityelement[t.te].eleprof.accuracy=t.grideleprof.accuracy;
				t.entityelement[t.te].eleprof.reloadqty=t.grideleprof.reloadqty;
				t.entityelement[t.te].eleprof.fireiterations=t.grideleprof.fireiterations;
				t.entityelement[t.te].eleprof.range=t.grideleprof.range;
				t.entityelement[t.te].eleprof.dropoff=t.grideleprof.dropoff;
				t.entityelement[t.te].eleprof.usespotlighting=t.grideleprof.usespotlighting;
				t.entityelement[t.te].eleprof.lifespan=t.grideleprof.lifespan;
				t.entityelement[t.te].eleprof.throwspeed=t.grideleprof.throwspeed;
				t.entityelement[t.te].eleprof.throwangle=t.grideleprof.throwangle;
				t.entityelement[t.te].eleprof.bounceqty=t.grideleprof.bounceqty;
				t.entityelement[t.te].eleprof.explodeonhit=t.grideleprof.explodeonhit;
			}
		}
	}

	//  If multiplayer start marker, must propogate any script change to all others
	if (  t.entityprofile[t.entid].ismarker == 7 ) 
	{
		for ( t.te = 1 ; t.te<=  g.entityelementlist; t.te++ )
		{
			if (  t.te != t.e ) 
			{
				t.tentid=t.entityelement[t.te].bankindex;
				if (  t.entityprofile[t.tentid].ismarker == 7 ) 
				{
					//t.entityelement[t.te].eleprof.aiinit_s=t.entityelement[t.e].eleprof.aiinit_s; //PE: Not used anymore.
					t.entityelement[t.te].eleprof.aimain_s=t.entityelement[t.e].eleprof.aimain_s;
					//t.entityelement[t.te].eleprof.aidestroy_s=t.entityelement[t.e].eleprof.aidestroy_s;
					//t.entityelement[t.te].eleprof.aishoot_s=t.entityelement[t.e].eleprof.aishoot_s; //PE: Not used anymore.
					t.entityelement[t.te].eleprof.soundset_s=t.entityelement[t.e].eleprof.soundset_s;
					t.entityelement[t.te].eleprof.soundset1_s=t.entityelement[t.e].eleprof.soundset1_s;
					t.entityelement[t.te].eleprof.soundset2_s=t.entityelement[t.e].eleprof.soundset2_s;
					t.entityelement[t.te].eleprof.soundset3_s=t.entityelement[t.e].eleprof.soundset3_s;
					t.entityelement[t.te].eleprof.soundset4_s=t.entityelement[t.e].eleprof.soundset4_s;
					#ifdef WICKEDENGINE
					t.entityelement[t.te].eleprof.soundset5_s = t.entityelement[t.e].eleprof.soundset5_s;
					t.entityelement[t.te].eleprof.soundset6_s = t.entityelement[t.e].eleprof.soundset6_s;
					t.entityelement[t.te].eleprof.overrideanimset_s = t.entityelement[t.e].eleprof.overrideanimset_s;
					#endif
					t.entityelement[t.te].eleprof.strength=t.entityelement[t.e].eleprof.strength;
					t.entityelement[t.te].eleprof.speed=t.entityelement[t.e].eleprof.speed;
					t.entityelement[t.te].eleprof.animspeed=t.entityelement[t.e].eleprof.animspeed;
				}
			}
		}
	}

	//  Add entity reference into map
	t.tupdatee=t.e; entity_updateentityobj ( );

	// mark as static if it was
	if ( t.entityelement[t.tupdatee].staticflag == 1 ) g.projectmodifiedstatic = 1;

	// 160616 - just added EBE Builder New Site Entity 
	if ( t.entityprofile[t.gridentity].isebe > 0 )
	{
		if ( stricmp ( t.entitybank_s[t.gridentity].Get(), "..\\ebebank\\_builder\\New Site.fpe" ) == NULL )
		{
			// NEW FPE and EBE SITE
			t.entitybankindex = t.gridentity;

			// add new entity to entity library
			t.entityprofileheader[t.entitybankindex].desc_s = cstr("EBE") + cstr(t.entitybankindex);

			// update entityelement with new parent entity details
			t.entityelement[t.e].bankindex = t.entitybankindex;

			// change to site name after above creation from orig FPE template
			ebe_newsite ( t.e );

			// update entity FPE parent object from above newsite entity element obj
			t.entitybank_s[t.entitybankindex] = t.entityprofileheader[t.entitybankindex].desc_s;
			t.entityelement[t.e].bankindex = t.entitybankindex;
			t.entityelement[t.e].eleprof.name_s = t.entityprofileheader[t.entitybankindex].desc_s;
			ebe_updateparent ( t.e );

			// ensure mouse is released before painting on grid
			t.ebe.bReleaseMouseFirst = true;
		}
		else
		{
			// Selected an existing EBE entity from library
		}
	}

	//PE: Moved here as we use the object direction vector for spot lights.
	// update infinilight list with addition
	if (t.entityprofile[t.entitybankindex].ismarker == 2 || t.entityprofile[t.entitybankindex].ismarker == 5 || t.entityelement[t.e].eleprof.usespotlighting)
	{
		lighting_refresh();
	}

	// if new particle emitter, update it when created (to start the particle emission)
	#ifdef WICKEDENGINE
	entity_updateparticleemitter(t.tupdatee);
	#endif

	// when add an entity to the scene, auto flatten if flagged
	#ifdef WICKEDENGINE
	entity_autoFlattenWhenAdded(t.e); //MD: Wrapped this section into a function to use when loading in levels to auto flatten areas
	#endif
}

void entity_deleteentityfrommap ( void )
{
	//  Entity Type To Delete
	t.entitymaintype=1;

	//  Use entity coord to find tile
	t.tupdatee=t.tentitytoselect;

	// remember entity bank index for later light refresh
	int iWasEntID = t.entityelement[t.tupdatee].bankindex;

	//  cleanup character creator
	#ifdef VRTECH
	#else
	t.ccobjToDelete = t.tupdatee;
	characterkit_deleteEntity ( );
	#endif

	// mark as static if it was
	if ( t.entityelement[t.tupdatee].staticflag == 1 ) g.projectmodifiedstatic = 1;

	//  blank from entity element list
	t.entityelement[t.tupdatee].bankindex=0;
	t.entityelement[t.tupdatee].maintype=0;
	t.entityelement[t.tupdatee].iHasParentIndex = 0;
	deleteinternalsound(t.entityelement[t.tupdatee].soundset) ; t.entityelement[t.tupdatee].soundset=0;
	deleteinternalsound(t.entityelement[t.tupdatee].soundset1) ; t.entityelement[t.tupdatee].soundset1=0;
	deleteinternalsound(t.entityelement[t.tupdatee].soundset2) ; t.entityelement[t.tupdatee].soundset2=0;
	deleteinternalsound(t.entityelement[t.tupdatee].soundset3) ; t.entityelement[t.tupdatee].soundset3=0;
	//deleteinternalsound(t.entityelement[t.tupdatee].soundset4) ; t.entityelement[t.tupdatee].soundset4=0;
	#ifdef WICKEDENGINE
	deleteinternalsound(t.entityelement[t.tupdatee].soundset5); t.entityelement[t.tupdatee].soundset5 = 0;
	deleteinternalsound(t.entityelement[t.tupdatee].soundset6); t.entityelement[t.tupdatee].soundset6 = 0;
	#endif
	//  Delete any associated waypoint/trigger zone
	//  SK; This sub can be called from the WP module when it's deleting a whole WP path, and in turn deleting the entity using it.
	//  In this situation we DO NOT want it to try and delete it's own WPs as this becomes cyclic!
	t.waypointindex=t.entityelement[t.tupdatee].eleprof.trigger.waypointzoneindex;
	if (  t.waypointindex > 0 ) 
	{
		if (  t.grideleprof.trigger.waypointzoneindex != t.waypointindex && t.tDontDeleteWPFlag  ==  0 ) 
		{
			t.w=t.waypoint[t.waypointindex].start;
			waypoint_delete ( );
		}
		t.entityelement[t.tupdatee].eleprof.trigger.waypointzoneindex=0;
	}
	t.tDontDeleteWPFlag = 0;

	// update infinilight list with removal
	if ( t.entityprofile[iWasEntID].ismarker == 2 || t.entityprofile[iWasEntID].ismarker == 5 ) 
	{
		//  refresh existing lights
		lighting_refresh ( );
	}
	#ifdef WICKEDENGINE
	//Delete any particle effects.
	if (g_UndoSysObjectIsBeingMoved != true)
	{
		int iParticleEmitter = t.entityelement[t.tupdatee].eleprof.newparticle.emitterid;
		if (iParticleEmitter != -1)
		{
			gpup_deleteEffect(iParticleEmitter);
			t.entityelement[t.tupdatee].eleprof.newparticle.emitterid = -1;
		}
	}
	#endif

	#ifdef WICKEDENGINE
	// remove flatten if any
	if (t.entityelement[t.tupdatee].eleprof.iFlattenID != -1)
	{
		GGTerrain_RemoveFlatArea(t.entityelement[t.tupdatee].eleprof.iFlattenID);
		t.entityelement[t.tupdatee].eleprof.iFlattenID = -1;
	}
	#endif

 	#ifdef WICKEDENGINE
	// deleting and readding objects, MUST wipe out relational data!
	for (int i = 0; i < 10; i++)
	{
		t.entityelement[t.tupdatee].eleprof.iObjectLinkID = 0;
		t.entityelement[t.tupdatee].eleprof.iObjectRelationships[i] = 0;
		t.entityelement[t.tupdatee].eleprof.iObjectRelationshipsData[i] = 0;
		t.entityelement[t.tupdatee].eleprof.iObjectRelationshipsType[i] = 0;
	}
	#endif

	// update real ent obj (.obj=0 inside)
	entity_updateentityobj ( );
}

#ifdef WICKEDENGINE
// new improved system using master and event stacks
void entity_performtheundoaction (eUndoEventType eventtype, void* pEventData)
{
	// if event data
	if (pEventData)
	{
		// perform editor action to undo these events
		switch (eventtype)
		{
		case eUndoSys_Object_Add:
		{
			sUndoSysEventObjectAdd* pEvent = (sUndoSysEventObjectAdd*)pEventData;
			t.tentitytoselect = pEvent->iEntityElementIndex;
			entity_deleteentityfrommap();
			break;
		}
		case eUndoSys_Object_Delete:
		{
			sUndoSysEventObjectDelete* pEvent = (sUndoSysEventObjectDelete*)pEventData;
			int store = t.e;
			t.e = pEvent->e;
			t.grideleprof.trigger.waypointzoneindex = pEvent->grideleprof_trigger_waypointzoneindex;
			t.entid = pEvent->entitybankindex;
			t.entitybankindex = t.entid;
			t.gridentityeditorfixed = pEvent->gridentityeditorfixed;
			t.entitymaintype = pEvent->entitymaintype;
			t.entitybankindex = pEvent->entitybankindex;
			t.gridentitystaticmode = pEvent->gridentitystaticmode;
			t.gridentityhasparent = pEvent->gridentityhasparent;
			t.gridentityposx_f = pEvent->gridentityposx_f;
			t.gridentityposy_f = pEvent->gridentityposy_f;
			t.gridentityposz_f = pEvent->gridentityposz_f;
			t.gridentityrotatex_f = pEvent->gridentityrotatex_f;
			t.gridentityrotatey_f = pEvent->gridentityrotatey_f;
			t.gridentityrotatez_f = pEvent->gridentityrotatez_f;
			t.gridentityrotatequatmode = pEvent->gridentityrotatequatmode;
			t.gridentityrotatequatx_f = pEvent->gridentityrotatequatx_f;
			t.gridentityrotatequaty_f = pEvent->gridentityrotatequaty_f;
			t.gridentityrotatequatz_f = pEvent->gridentityrotatequatz_f;
			t.gridentityrotatequatw_f = pEvent->gridentityrotatequatw_f;
			t.gridentityscalex_f = pEvent->gridentityscalex_f + 100;
			t.gridentityscaley_f = pEvent->gridentityscaley_f + 100;
			t.gridentityscalez_f = pEvent->gridentityscalez_f + 100;
			t.gridentity = t.entid;
			t.gridentityoverwritemode = t.e;
			entity_addentitytomap ();
			t.gridentityoverwritemode = 0;
			t.gridentity = 0;
			t.e = store;
			break;
		}
		case eUndoSys_Object_ChangePosRotScl:
		{
			sUndoSysEventObjectChangePosRotScl* pEvent = (sUndoSysEventObjectChangePosRotScl*)pEventData;
			int te = pEvent->e;
			t.entityelement[te].x = pEvent->posx_f;
			t.entityelement[te].y = pEvent->posy_f;
			t.entityelement[te].z = pEvent->posz_f;
			t.entityelement[te].rx = pEvent->rotatex_f;
			t.entityelement[te].ry = pEvent->rotatey_f;
			t.entityelement[te].rz = pEvent->rotatez_f;		
			t.entityelement[te].quatmode = pEvent->rotatequatmode;
			t.entityelement[te].quatx = pEvent->rotatequatx_f;
			t.entityelement[te].quaty = pEvent->rotatequaty_f;
			t.entityelement[te].quatz = pEvent->rotatequatz_f;
			t.entityelement[te].quatw = pEvent->rotatequatw_f;
			t.entityelement[te].scalex = pEvent->scalex_f;
			t.entityelement[te].scaley = pEvent->scaley_f;
			t.entityelement[te].scalez = pEvent->scalez_f;
			t.tobj = t.entityelement[te].obj;
			t.tte = te;
			entity_positionandscale();
			break;
		}
		case eUndoSys_Object_DeleteWaypoint:
		{
			sUndoSysEventObjectDeleteWaypoint* pEvent = (sUndoSysEventObjectDeleteWaypoint*)pEventData;
			waypoint_undoredo_add(pEvent->waypoint, pEvent->waypointcoords);
			break;
		}
		case eUndoSys_Object_Group:
		{
			sUndoSysEventObjectGroup* pEvent = (sUndoSysEventObjectGroup*)pEventData;

			extern void UnGroupUndoSys(int index);
			UnGroupUndoSys(pEvent->groupindex);
			/*extern void GroupUndoSys(int index, std::vector<sRubberBandType> groupData);
			//GroupUndoSys(pEvent->groupindex, pEvent->groupData);*/

			break;
		}
		case eUndoSys_Object_UnGroup:
		{
			sUndoSysEventObjectUnGroup* pEvent = (sUndoSysEventObjectUnGroup*)pEventData;

			extern void GroupUndoSys(int index, std::vector<sRubberBandType> groupData);
			GroupUndoSys(pEvent->groupindex, pEvent->groupData);
			
			break;
		}
		}

		// and delete the event data (the action consumes the memory created to store the event data)
		delete pEventData;
		pEventData = NULL;
	}
}
void entity_createundoaction (int eventtype, int te, bool bUserAction)
{
	// if user caused this action manually, must clear redo stack which 
	// contains events from previous alternative future
	if (bUserAction == true) undosys_clearredostack();

	// add specific event to stacks
	switch (eventtype)
	{
	case eUndoSys_Object_Add:
	{
		undosys_object_add(te);
		break;
	}
	case eUndoSys_Object_Delete:
	{
		undosys_object_delete (te, t.entityelement[te].eleprof.trigger.waypointzoneindex,
			t.entityelement[te].editorfixed,
			t.entityelement[te].maintype,
			t.entityelement[te].bankindex,
			t.entityelement[te].staticflag,
			t.entityelement[te].iHasParentIndex,
			t.entityelement[te].x,
			t.entityelement[te].y,
			t.entityelement[te].z,
			t.entityelement[te].rx,
			t.entityelement[te].ry,
			t.entityelement[te].rz,
			t.entityelement[te].quatmode,
			t.entityelement[te].quatx,
			t.entityelement[te].quaty,
			t.entityelement[te].quatz,
			t.entityelement[te].quatw,
			t.entityelement[te].scalex,
			t.entityelement[te].scaley,
			t.entityelement[te].scalez);
		break;
	}
	case eUndoSys_Object_ChangePosRotScl:
	{
		undosys_object_changeposrotscl (te, t.entityelement[te].x,
			t.entityelement[te].y,
			t.entityelement[te].z,
			t.entityelement[te].rx,
			t.entityelement[te].ry,
			t.entityelement[te].rz,
			t.entityelement[te].quatmode,
			t.entityelement[te].quatx,
			t.entityelement[te].quaty,
			t.entityelement[te].quatz,
			t.entityelement[te].quatw,
			t.entityelement[te].scalex,
			t.entityelement[te].scaley,
			t.entityelement[te].scalez);
		break;
	}
	case eUndoSys_Object_DeleteWaypoint:
	{
		undosys_object_deletewaypoint(t.entityelement[te].eleprof.trigger.waypointzoneindex, te);
		break;
	}
	case eUndoSys_Object_Group:
	{
		
		undosys_object_group(te);
		break;
	}
	case eUndoSys_Object_UnGroup:
	{
		extern std::vector<sRubberBandType> vEntityGroupList[MAXGROUPSLISTS];
		undosys_object_ungroup(te, vEntityGroupList[te]);
		break;
	}
	}
}
void entity_createtheredoaction (eUndoEventType eventtype, void* pEventData)
{
	switch (eventtype)
	{
	case eUndoSys_Object_Add: 
	{
		// about to undo this add (the _addundo will delete the object), so we create a delete event in the redo stack
		sUndoSysEventObjectAdd* pEvent = (sUndoSysEventObjectAdd*)pEventData;
		int te = pEvent->iEntityElementIndex;
		if (t.entityelement[te].eleprof.trigger.waypointzoneindex > 0)
		{
			undosys_multiplevents_start();
			entity_createundoaction(eUndoSys_Object_Delete, te, false);
			entity_createundoaction(eUndoSys_Object_DeleteWaypoint, te, false);
			undosys_multiplevents_finish();
		}
		else
		{
			entity_createundoaction(eUndoSys_Object_Delete, te, false);
		}
		break;
	}
	case eUndoSys_Object_Delete: 
	{
		// about to undo this delete (which the _deleteundo will add the object back in), so we create an add event in the redo stack
		sUndoSysEventObjectDelete* pEvent = (sUndoSysEventObjectDelete*)pEventData;
		int te = pEvent->e;
		entity_createundoaction(eUndoSys_Object_Add, te, false);
		break;
	}
	case eUndoSys_Object_ChangePosRotScl: 
	{
		// about to undo this move event (which will repos/rot/scl back to what was stored), so we create a move event in the redo stack to put it back the other way
		sUndoSysEventObjectChangePosRotScl* pEvent = (sUndoSysEventObjectChangePosRotScl*)pEventData;
		int te = pEvent->e;
		entity_createundoaction(eUndoSys_Object_ChangePosRotScl, te, false);
		break;
	}
	case eUndoSys_Object_DeleteWaypoint:
	{
		// About to undo a delete, so create an add event in the redo stack.
		sUndoSysEventObjectDeleteWaypoint* pEvent = (sUndoSysEventObjectDeleteWaypoint*)pEventData;
		int te = pEvent->e;
		entity_createundoaction(eUndoSys_Object_Add, te, false);
		break;
	}
	case eUndoSys_Object_Group:
	{
		sUndoSysEventObjectGroup* pEvent = (sUndoSysEventObjectGroup*)pEventData;
		int index = pEvent->groupindex;
		entity_createundoaction(eUndoSys_Object_UnGroup, index, false);
		break;
	}
	case eUndoSys_Object_UnGroup:
	{
		sUndoSysEventObjectUnGroup* pEvent = (sUndoSysEventObjectUnGroup*)pEventData;
		entity_createundoaction(eUndoSys_Object_Group, pEvent->groupindex, false);
		break;
	}
	}
}
void entity_undo (void)
{
	// undo system to control undo stack
	undosys_undoevent();
}
void entity_redo (void)
{
	// undo system to control redo stack
	undosys_redoevent();
}
#else
void entity_recordbuffer_add ( void )
{
	t.terrainundo.bufferfilled=0;
	t.entityundo.action=1;
	t.entityundo.undoperformed=0;
	t.entityundo.entityindex=t.e;
	t.entityundo.bankindex=t.entityelement[t.e].bankindex;
}

void entity_recordbuffer_delete ( void )
{
	t.terrainundo.bufferfilled=0;
	t.entityundo.action=2;
	t.entityundo.undoperformed=0;
	t.entityundo.entityindex=t.tentitytoselect;
	t.entityundo.bankindex=t.entityelement[t.tentitytoselect].bankindex;
}

void entity_recordbuffer_move ( void )
{
	t.terrainundo.bufferfilled=0;
	t.entityundo.action=3;
	t.entityundo.undoperformed=0;
	t.entityundo.entityindex=t.tentitytoselect;
	t.entityundo.posbkup=t.entityundo.pos;
	t.entityundo.pos.staticflag=t.entityelement[t.tentitytoselect].staticflag;
	t.entityundo.pos.x=t.entityelement[t.tentitytoselect].x;
	t.entityundo.pos.y=t.entityelement[t.tentitytoselect].y;
	t.entityundo.pos.z=t.entityelement[t.tentitytoselect].z;
	t.entityundo.pos.rx=t.entityelement[t.tentitytoselect].rx;
	t.entityundo.pos.ry=t.entityelement[t.tentitytoselect].ry;
	t.entityundo.pos.rz=t.entityelement[t.tentitytoselect].rz;
	t.entityundo.pos.scalex=t.entityelement[t.tentitytoselect].scalex;
	t.entityundo.pos.scaley=t.entityelement[t.tentitytoselect].scaley;
	t.entityundo.pos.scalez=t.entityelement[t.tentitytoselect].scalez;
}

void entity_undo ( void )
{
	t.tactioninput=t.entityundo.action;
	if (  t.tactioninput == 1 ) 
	{
		//  undo addition DELETE IT
		t.tentitytoselect=t.entityundo.entityindex;
		entity_deleteentityfrommap ( );
		t.entityundo.action=2;
	}
	if (  t.tactioninput == 2 ) 
	{
		//  undo deletion ADD IT BACK
		if ( t.entityundo.entityindex == -123 )
		{
			// undo delete of rubberbandlist
			for ( int i = 0; i < (int)g.entityrubberbandlistundo.size(); i++ )
			{
				t.e = g.entityrubberbandlistundo[i].entityindex;
				t.entid = g.entityrubberbandlistundo[i].bankindex;
				t.entitybankindex=t.entid;
				t.gridentityeditorfixed=t.entityelement[t.e].editorfixed;
				t.entitymaintype=t.entityelement[t.e].maintype;
				t.gridentitystaticmode=t.entityelement[t.e].staticflag;
				t.gridentityhasparent=t.entityelement[t.e].iHasParentIndex;
				t.gridentityposx_f=t.entityelement[t.e].x;
				t.gridentityposz_f=t.entityelement[t.e].z;
				t.gridentityposy_f=t.entityelement[t.e].y;
				t.gridentityrotatex_f=t.entityelement[t.e].rx;
				t.gridentityrotatey_f=t.entityelement[t.e].ry;
				t.gridentityrotatez_f=t.entityelement[t.e].rz;
				t.gridentityscalex_f=t.entityelement[t.e].scalex+100;
				t.gridentityscaley_f=t.entityelement[t.e].scaley+100;
				t.gridentityscalez_f=t.entityelement[t.e].scalez+100;
				t.grideleprof=t.entityelement[t.e].eleprof;
				t.gridentity=t.entid;
				t.gridentityoverwritemode=t.e;
				entity_addentitytomap ( );
				t.gridentityoverwritemode=0;
			}
			g.entityrubberbandlistundo.clear();
			t.gridentity=0;
			t.entityundo.action=0;
		}
		else
		{
			// undo delete of single entity
			t.e=t.entityundo.entityindex;
			t.entid=t.entityundo.bankindex;
			t.entitybankindex=t.entid;
			t.gridentityeditorfixed=t.entityelement[t.e].editorfixed;
			t.entitymaintype=t.entityelement[t.e].maintype;
			t.gridentitystaticmode=t.entityelement[t.e].staticflag;
			t.gridentityhasparent=t.entityelement[t.e].iHasParentIndex;
			t.gridentityposx_f=t.entityelement[t.e].x;
			t.gridentityposz_f=t.entityelement[t.e].z;
			t.gridentityposy_f=t.entityelement[t.e].y;
			t.gridentityrotatex_f=t.entityelement[t.e].rx;
			t.gridentityrotatey_f=t.entityelement[t.e].ry;
			t.gridentityrotatez_f=t.entityelement[t.e].rz;
			t.gridentityscalex_f=t.entityelement[t.e].scalex+100;
			t.gridentityscaley_f=t.entityelement[t.e].scaley+100;
			t.gridentityscalez_f=t.entityelement[t.e].scalez+100;
			t.grideleprof=t.entityelement[t.e].eleprof;
			t.gridentity=t.entid;
			entity_addentitytomap ( );
			t.gridentity=0;
			t.entityundo.action=1;
		}
	}

	//  These cano update existing entity
	t.tupdatentpos=0;
	if (  t.tactioninput == 3 ) 
	{
		//  undo MOVE and restore previous position
		if ( t.entityundo.entityindex == -123 )
		{
			// undo move of rubberbandlist
			for ( int i = 0; i < (int)g.entityrubberbandlistundo.size(); i++ )
			{
				t.e = g.entityrubberbandlistundo[i].entityindex;
				//t.entityundo.posbkup.staticflag=t.entityelement[t.e].staticflag; // no REDO for rubber band undo!
				//t.entityundo.posbkup.x=t.entityelement[t.e].x;
				//t.entityundo.posbkup.y=t.entityelement[t.e].y;
				//t.entityundo.posbkup.z=t.entityelement[t.e].z;
				//t.entityundo.posbkup.rx=t.entityelement[t.e].rx;
				//t.entityundo.posbkup.ry=t.entityelement[t.e].ry;
				//t.entityundo.posbkup.rz=t.entityelement[t.e].rz;
				//t.entityundo.posbkup.scalex=t.entityelement[t.e].scalex;
				//t.entityundo.posbkup.scaley=t.entityelement[t.e].scaley;
				//t.entityundo.posbkup.scalez=t.entityelement[t.e].scalez;
				t.entityelement[t.e].staticflag=g.entityrubberbandlistundo[i].pos.staticflag;
				t.entityelement[t.e].x=g.entityrubberbandlistundo[i].pos.x;
				t.entityelement[t.e].y=g.entityrubberbandlistundo[i].pos.y;
				t.entityelement[t.e].z=g.entityrubberbandlistundo[i].pos.z;
				t.entityelement[t.e].rx=g.entityrubberbandlistundo[i].pos.rx;
				t.entityelement[t.e].ry=g.entityrubberbandlistundo[i].pos.ry;
				t.entityelement[t.e].rz=g.entityrubberbandlistundo[i].pos.rz;
				t.entityelement[t.e].scalex=g.entityrubberbandlistundo[i].pos.scalex;
				t.entityelement[t.e].scaley=g.entityrubberbandlistundo[i].pos.scaley;
				t.entityelement[t.e].scalez=g.entityrubberbandlistundo[i].pos.scalez;
			}
			t.entityundo.action=0;
		}
		else
		{
			// single entity position undo
			t.e=t.entityundo.entityindex;
			t.entityundo.posbkup.staticflag=t.entityelement[t.e].staticflag;
			t.entityundo.posbkup.x=t.entityelement[t.e].x;
			t.entityundo.posbkup.y=t.entityelement[t.e].y;
			t.entityundo.posbkup.z=t.entityelement[t.e].z;
			t.entityundo.posbkup.rx=t.entityelement[t.e].rx;
			t.entityundo.posbkup.ry=t.entityelement[t.e].ry;
			t.entityundo.posbkup.rz=t.entityelement[t.e].rz;
			t.entityundo.posbkup.scalex=t.entityelement[t.e].scalex;
			t.entityundo.posbkup.scaley=t.entityelement[t.e].scaley;
			t.entityundo.posbkup.scalez=t.entityelement[t.e].scalez;
			t.entityelement[t.e].staticflag=t.entityundo.pos.staticflag;
			t.entityelement[t.e].x=t.entityundo.pos.x;
			t.entityelement[t.e].y=t.entityundo.pos.y;
			t.entityelement[t.e].z=t.entityundo.pos.z;
			t.entityelement[t.e].rx=t.entityundo.pos.rx;
			t.entityelement[t.e].ry=t.entityundo.pos.ry;
			t.entityelement[t.e].rz=t.entityundo.pos.rz;
			t.entityelement[t.e].scalex=t.entityundo.pos.scalex;
			t.entityelement[t.e].scaley=t.entityundo.pos.scaley;
			t.entityelement[t.e].scalez=t.entityundo.pos.scalez;
			t.entityundo.action=4;
		}
		t.tupdatentpos=1;
	}

	if (  t.tactioninput == 4 ) 
	{
		//  restore previous POS from BKUP
		t.e=t.entityundo.entityindex;
		t.entityelement[t.e].staticflag=t.entityundo.posbkup.staticflag;
		t.entityelement[t.e].x=t.entityundo.posbkup.x;
		t.entityelement[t.e].y=t.entityundo.posbkup.y;
		t.entityelement[t.e].z=t.entityundo.posbkup.z;
		t.entityelement[t.e].rx=t.entityundo.posbkup.rx;
		t.entityelement[t.e].ry=t.entityundo.posbkup.ry;
		t.entityelement[t.e].rz=t.entityundo.posbkup.rz;
		t.entityelement[t.e].scalex=t.entityundo.posbkup.scalex;
		t.entityelement[t.e].scaley=t.entityundo.posbkup.scaley;
		t.entityelement[t.e].scalez=t.entityundo.posbkup.scalez;
		t.entityundo.action=3;
		t.tupdatentpos=1;
	}

	// update single entity or list
	if ( t.tupdatentpos == 1 ) 
	{
		if ( g.entityrubberbandlistundo.size() > 0 )
		{
			bool bUpdateLights = false;
			for ( int i = 0; i < (int)g.entityrubberbandlistundo.size(); i++ )
			{
				t.e = g.entityrubberbandlistundo[i].entityindex;
				t.tentid = t.entityelement[t.e].bankindex;
				if ( t.entityprofile[t.tentid].ismarker == 2 || t.entityprofile[t.tentid].ismarker == 5 ) bUpdateLights = true;
				t.tte = t.e; 
				t.tobj = t.entityelement[t.e].obj;
				t.tentid = t.entityelement[t.e].bankindex;
				entity_positionandscale ( );
			}
			if ( bUpdateLights==true )
				lighting_refresh ( );
		}
		else
		{
			t.tentid=t.entityelement[t.e].bankindex;
			if (  t.entityprofile[t.tentid].ismarker == 2 || t.entityprofile[t.tentid].ismarker == 5 ) 
			{
				lighting_refresh ( );
			}
			t.tte=t.e ; t.tobj=t.entityelement[t.e].obj ; t.tentid=t.entityelement[t.e].bankindex;
			entity_positionandscale ( );
		}
	}

}

void entity_redo ( void )
{
	entity_undo ( );
}
#endif

#ifdef WICKEDENGINE
void entity_updateparticleemitterbyID (entityeleproftype* pEleprof, int iObj, float fScale, float fX, float fY, float fZ)
{
	// show or hide based on editor vs test game
	bool bShowThisParticle = false;
	extern bool bImGuiInTestGame;
	if ( bImGuiInTestGame == true )
		bShowThisParticle = pEleprof->newparticle.bParticle_Show_At_Start;
	else
		bShowThisParticle = pEleprof->newparticle.bParticle_Preview;
	
	int iParticleEmitter = pEleprof->newparticle.emitterid;
	if (iParticleEmitter == -1)
	{
		if (bShowThisParticle == true)
		{
			iParticleEmitter = gpup_loadEffect(pEleprof->newparticle.emittername.Get(), 0, 0, 0, 1.0);
			gpup_emitterActive(iParticleEmitter, 0);
			pEleprof->newparticle.emitterid = iParticleEmitter;
		}
	}
	if (iParticleEmitter != -1)
	{
		// set emitter position, rotation and scale
		gpup_setGlobalPosition(iParticleEmitter, fX, fY, fZ);
		gpup_resetLocalPosition(iParticleEmitter);
		float fSpeedX, fSpeedY, fSpeedZ;
		sObject* pObject = GetObjectData(iObj);
		gpup_getEmitterSpeedAngleAdjustment(iParticleEmitter, &fSpeedX, &fSpeedY, &fSpeedZ);
		GGVECTOR3 vecSpeedDirection = GGVECTOR3(fSpeedX - 0.5f, fSpeedY - 0.5f, fSpeedZ - 0.5f);
		if (pObject)
		{
			GGVec3TransformCoord(&vecSpeedDirection, &vecSpeedDirection, &pObject->position.matRotation);
			gpup_setEmitterSpeedAngleAdjustment(iParticleEmitter, 0.5f + vecSpeedDirection.x, 0.5f + vecSpeedDirection.y, 0.5f + vecSpeedDirection.z);
		}
		gpup_setGlobalScale(iParticleEmitter, 100.0f + fScale);

		// set whether burst mode loops
		if (pEleprof->newparticle.bParticle_Looping_Animation == true)
			gpup_emitterBurstLoopAutoMode(iParticleEmitter, 1);
		else
			gpup_emitterBurstLoopAutoMode(iParticleEmitter, 0);

		// switch emitter on or off
		if (bShowThisParticle == true)
			gpup_emitterActive(iParticleEmitter, 1);
		else
			gpup_emitterActive(iParticleEmitter, 0);

		// specify extra emitter attributes
		gpup_setEffectAnimationSpeed(iParticleEmitter, pEleprof->newparticle.fParticle_Speed);
		gpup_setEffectOpacity(iParticleEmitter, pEleprof->newparticle.fParticle_Opacity);
	}
}

void entity_updateparticleemitter ( int e )
{
	// also handle particle emitter entity
	if (t.entityprofile[t.entityelement[e].bankindex].ismarker == 10)
	{
		entity_updateparticleemitterbyID(&t.entityelement[e].eleprof, t.entityelement[e].obj, t.entityelement[e].scalex, t.entityelement[e].x, t.entityelement[e].y, t.entityelement[e].z);
	}
}

void entity_updateautoflatten (int e, int obj)
{
	int entid = t.entityelement[e].bankindex;
	if (entid > 0)
	{
		int iAutoFlattenMode = t.entityprofile[entid].autoflatten;

		if ((iAutoFlattenMode != 0 && !g.isGameBeingPlayed) && (!g_bEnableAutoFlattenSystem || !t.entityelement[e].eleprof.bAutoFlatten) )
		{
			if (t.entityelement[e].eleprof.iFlattenID != -1)
			{
				//PE: Disabled remove any flatten.
				GGTerrain_RemoveFlatArea(t.entityelement[e].eleprof.iFlattenID);
				t.entityelement[e].eleprof.iFlattenID = -1;
				//t.entityelement[e].eleprof.bAutoFlatten = false;
			}
		}
		else if (iAutoFlattenMode != 0 && !g.isGameBeingPlayed)
		{
			//PE: t.entityelement[e].obj is not set in creating process. entity_prepareobj()
			int iObj = t.entityelement[e].obj;
			if (iObj == 0 && obj > 0) iObj = obj; //PE: entity_prepareobj() now set obj for standalone.
			if (iObj > 0)
			{
				float x = t.entityelement[e].x + (GetObjectCollisionCenterX(iObj) * (ObjectScaleX(iObj) / 100.0f));
				float y = t.entityelement[e].y + (GetObjectCollisionCenterY(iObj) * (ObjectScaleY(iObj) / 100.0f)) - (ObjectSizeY(iObj) / 2, 1);
				float z = t.entityelement[e].z + (GetObjectCollisionCenterZ(iObj) * (ObjectScaleZ(iObj) / 100.0f));

				GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ, quatRotationEvent;
				GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(ObjectAngleX(iObj)));
				GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(ObjectAngleY(iObj)));
				GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(ObjectAngleZ(iObj)));
				quatRotationEvent = QuatAroundX * QuatAroundY * QuatAroundZ;
				
				float a = 2 * (quatRotationEvent.x * quatRotationEvent.z + quatRotationEvent.w * quatRotationEvent.y);
				float b = 1 - 2 * (quatRotationEvent.x * quatRotationEvent.x + quatRotationEvent.y * quatRotationEvent.y);

				float angRad = atan2(a, b);
				float angDeg = GGToDegree(angRad);

				if (angDeg < 0) angDeg += 360;
				if (angDeg > 360) angDeg -= 360;

				float sx = ObjectSizeX(iObj, 1)* 1.05f + g_fFlattenMargin;
				float sz = ObjectSizeZ(iObj, 1) * 1.05f + g_fFlattenMargin;
				int iFlattenID = t.entityelement[e].eleprof.iFlattenID;

				//PE: (TEST AGAIN) Disable restoring of flatten area, it simple dont work, before terrain is setup, this need to be delayed somehow.
				/*
				//PE: Create area if needed.
				if (iFlattenID < 0)
				{
					float x = t.entityelement[e].x + GetObjectCollisionCenterX(iObj);
					float z = t.entityelement[e].z + GetObjectCollisionCenterZ(iObj);
					float ang = t.entityelement[e].ry;
					if (t.entityelement[e].eleprof.iFlattenID == -1)
					{
						if (g_bEnableAutoFlattenSystem == true)
						{
							if (iAutoFlattenMode == 1)
							{
								float sx = ObjectSizeX(iObj, 1) * 1.3f;
								float sz = ObjectSizeZ(iObj, 1) * 1.3f;
								//t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatRect(x, z, sx, sz, ang, y);
								t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatRect(x, z, sx, sz, ang);
							}
							else
							{
								float s = ObjectSize(iObj, 1) * 1.3f;
								//t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatCircle(x, z, s, y);
								t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatCircle(x, z, s);
							}
						}
					}
					iFlattenID = t.entityelement[e].eleprof.iFlattenID;
				}
				*/

				if (iFlattenID != -1)
				{
					//PE: This is draining the CPU, constantly updating while selected.
					static float Lastx = -1, Lasty = -1, Lastz = -1, Lastang = -1, LastSizeX = 0.0f, LastSizeZ = 0.0f;
					static int iLastFlattenID = -1;
					if (y != Lasty || x != Lastx || z != Lastz || angDeg != Lastang || LastSizeX != sx || LastSizeZ != sz || iFlattenID != iLastFlattenID)
					{
						Lastx = x;
						Lasty = y;
						Lastz = z;
						Lastang = angDeg;
						LastSizeX = sx;
						LastSizeZ = sz;
						iLastFlattenID = iFlattenID;
						//GGTerrain_UpdateFlatArea(iFlattenID, x, z, angDeg, sx, sz, y);
						GGTerrain_UpdateFlatArea(iFlattenID, x, z, angDeg, sx, sz);
					}
				}
			}
		}
	}
}


void entity_autoFlattenWhenAdded(int e, int obj)
{
	//int iAutoFlattenMode = t.entityprofile[t.entitybankindex].autoflatten;
	int entid = t.entityelement[e].bankindex;
	if (entid > 0)
	{
		int iAutoFlattenMode = t.entityprofile[entid].autoflatten;

		if (iAutoFlattenMode != 0)
		{
			int iObj = t.entityelement[e].obj;
			if (iObj == 0 && obj > 0) iObj = obj; //PE: entity_prepareobj() now set obj for standalone.

			float x = t.entityelement[e].x + (GetObjectCollisionCenterX(iObj) * (ObjectScaleX(iObj) / 100.0f));
			float y = t.entityelement[e].y + (GetObjectCollisionCenterY(iObj) * (ObjectScaleY(iObj) / 100.0f)) - (ObjectSizeY(iObj) / 2, 1);
			float z = t.entityelement[e].z + (GetObjectCollisionCenterZ(iObj) * (ObjectScaleZ(iObj) / 100.0f));

			GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ, quatRotationEvent;
			GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(ObjectAngleX(iObj)));
			GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(ObjectAngleY(iObj)));
			GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(ObjectAngleZ(iObj)));
			quatRotationEvent = QuatAroundX * QuatAroundY * QuatAroundZ;

			float a = 2 * (quatRotationEvent.x * quatRotationEvent.z + quatRotationEvent.w * quatRotationEvent.y);
			float b = 1 - 2 * (quatRotationEvent.x * quatRotationEvent.x + quatRotationEvent.y * quatRotationEvent.y);

			float angRad = atan2(a, b);
			float angDeg = GGToDegree(angRad);

			if (angDeg < 0) angDeg += 360;
			if (angDeg > 360) angDeg -= 360;

			float sx = ObjectSizeX(iObj, 1) * 1.05f + g_fFlattenMargin;
			float sz = ObjectSizeZ(iObj, 1) * 1.05f + g_fFlattenMargin;

			if ((!g_bEnableAutoFlattenSystem || !t.entityelement[e].eleprof.bAutoFlatten))
			{
				if (t.entityelement[e].eleprof.iFlattenID != -1)
				{
					//PE: Disabled remove any flatten.
					GGTerrain_RemoveFlatArea(t.entityelement[e].eleprof.iFlattenID);
					t.entityelement[e].eleprof.iFlattenID = -1;
					//t.entityelement[e].eleprof.bAutoFlatten = false;
				}
			}
			else if (t.entityelement[e].eleprof.iFlattenID == -1)
			{
				if (g_bEnableAutoFlattenSystem == true)
				{
					if (iAutoFlattenMode == 1)
					{
						//t.entityelement[t.e].eleprof.iFlattenID = GGTerrain_AddFlatRect(x, z, sx, sz, ang, y);
						t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatRect(x, z, sx, sz, angDeg);
					}
					else
					{
						float s = ObjectSize(t.entityelement[e].obj, 1) * 1.05f + g_fFlattenMargin;
						//t.entityelement[t.e].eleprof.iFlattenID = GGTerrain_AddFlatCircle(x, z, s, y);
						t.entityelement[e].eleprof.iFlattenID = GGTerrain_AddFlatCircle(x, z, s);
					}
				}
			}
			else
			{
				//GGTerrain_UpdateFlatArea(t.entityelement[t.e].eleprof.iFlattenID, x, z, ang, y);
				GGTerrain_UpdateFlatArea(t.entityelement[e].eleprof.iFlattenID, x, z, angDeg, sx, sz);
			}
		}
		else
		{
			if (t.entityelement[e].eleprof.iFlattenID != -1)
			{
				GGTerrain_RemoveFlatArea(t.entityelement[e].eleprof.iFlattenID);
				t.entityelement[e].eleprof.iFlattenID = -1;
			}
		}
	}
}
#endif

bool ObjectIsEntity(void* pTestObject)
{
	sObject* pObject = (sObject*)pTestObject;
	for (int te = 1; te <= g.entityelementlist; te++)
	{
		if (t.entityelement[te].obj > 0 )
		{
			if (pObject->dwObjectNumber == t.entityelement[te].obj)
				return true;
		}
	}
	return false;
}

#ifdef WICKEDENGINE
//These functions need a pMesh overwrite. so we later can save into map data, for per object material changes.
//pMesh overwrite will not work ,so everything is now moved to t.entityelement[ele_id].eleprof.bCustomWickedMaterialActive

//PE: if Element ID set , use t.entityelement[g_iWickedElementId].eleprof.WEMaterial for everything.
//PE: "Materials" to limit map.ele size we only allow changes to mesh 1 , and per object settings (checkbox+reflectence).

void WickedSetElementId(int ele_id)
{
	//Only set if bCustomWickedMaterialActive has been activated by user.
	if (ele_id > 0 && t.entityelement[ele_id].eleprof.bCustomWickedMaterialActive)
	{
		g_iWickedElementId = ele_id;
	}
	else
	{
		g_iWickedElementId = 0;
	}
}

void WickedSetUseEditorGrideleprof(bool bUse)
{
	g_bUseEditorGrideleprof = bUse;
}

bool IsWickedMaterialActive(void* pvMesh)
{
	sMesh* pMesh = (sMesh*)pvMesh;
	if(g_iWickedEntityId < 0) return false;
	if( g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive )
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive;
	return t.entityprofile[g_iWickedEntityId].WEMaterial.MaterialActive;
}

void WickedSetEntityId(int ent_id)
{
	g_iWickedEntityId = ent_id;
}
int WickedGetEntityId(void)
{
	return g_iWickedEntityId;
}

float WickedGetTreeAlphaRef(void)
{
	float TreeAlphaRef = 1.0f;
	if (g_iWickedEntityId >= 0) { //PE: auto thumbs is now using 0.
		if (strcmp(Lower(t.entityprofile[g_iWickedEntityId].effect_s.Get()), "effectbank\\reloaded\\apbr_tree.fx") == 0)
		{
			TreeAlphaRef = 0.41f;
		}
		if (strcmp(Lower(t.entityprofile[g_iWickedEntityId].effect_s.Get()), "effectbank\\reloaded\\apbr_treea.fx") == 0)
		{
			TreeAlphaRef = 0.41f;
		}
	}
	return(TreeAlphaRef);
}

void WickedSetMeshNumber(int iMNumber)
{
	//PE: Got a crash when reaching 100 as index is only 0-99
	if (g_iWickedEntityId < 0 && iMNumber >= MAXMESHMATERIALS - 1)
		g_iWickedMeshNumber = 0;
	else
	{
		if(iMNumber >= MAXMESHMATERIALS - 1)
			g_iWickedMeshNumber = 0;
		else
			g_iWickedMeshNumber = iMNumber;
	}
}

cStr WickedGetBaseColorName( void )
{
	if (g_iWickedEntityId < 0) return "";
	if ( g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.baseColorMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS ) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.baseColorMapName[g_iWickedMeshNumber];
	}
	return "";
}

cStr WickedGetNormalName(void)
{
	if (g_iWickedEntityId < 0) return "";
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.normalMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.normalMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.normalMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.normalMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.normalMapName[g_iWickedMeshNumber];
	}
	return "";
}

cStr WickedGetSurfaceName(void)
{
	if (g_iWickedEntityId < 0) return "";
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.surfaceMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.surfaceMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.surfaceMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.surfaceMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.surfaceMapName[g_iWickedMeshNumber];
	}
	return "";
}

cStr WickedGetDisplacementName(void)
{
	if (g_iWickedEntityId < 0) return "";
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.displacementMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.displacementMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.displacementMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.displacementMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.displacementMapName[g_iWickedMeshNumber];
	}
	return "";
}

cStr WickedGetEmissiveName(void)
{
	if (g_iWickedEntityId < 0) return "";
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.emissiveMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.emissiveMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.emissiveMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.emissiveMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.emissiveMapName[g_iWickedMeshNumber];
	}
	return "";
}

cStr WickedGetOcclusionName(void)
{
	#ifdef DISABLEOCCLUSIONMAP
	return "";
	#else
	if (g_iWickedEntityId < 0) return "";
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.occlusionMapName[g_iWickedMeshNumber];
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.occlusionMapName[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.occlusionMapName[g_iWickedMeshNumber].Len() > 0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.occlusionMapName[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.occlusionMapName[g_iWickedMeshNumber];
	}
	return "";
	#endif
}

float WickedGetNormalStrength(void)
{
	if (g_iWickedEntityId < 0) return 1.0f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			if(t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fNormal[g_iWickedMeshNumber] >= 0.0f)
				return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fNormal[g_iWickedMeshNumber];
			else
				return(1.0f);
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fNormal[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.fNormal[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fNormal[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS && t.entityprofile[g_iWickedEntityId].WEMaterial.fNormal[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fNormal[g_iWickedMeshNumber];
	}
	return 1.0f;
}

float WickedGetRoughnessStrength(void)
{
	if (g_iWickedEntityId < 0) return 0.2f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fRoughness[g_iWickedMeshNumber] >= 0.0f)
				return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fRoughness[g_iWickedMeshNumber];
			else
				return(0.2f);
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fRoughness[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.fRoughness[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fRoughness[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS && t.entityprofile[g_iWickedEntityId].WEMaterial.fRoughness[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fRoughness[g_iWickedMeshNumber];
	}
	return 0.2f;
}
float WickedGetMetallnessStrength(void)
{
	if (g_iWickedEntityId < 0) return 0.0f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fMetallness[g_iWickedMeshNumber] >= 0.0f)
				return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fMetallness[g_iWickedMeshNumber];
			else
				return(0.0f);
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fMetallness[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.fMetallness[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fMetallness[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS && t.entityprofile[g_iWickedEntityId].WEMaterial.fMetallness[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fMetallness[g_iWickedMeshNumber];
	}
	return 0.0f;
}

float WickedGetEmissiveStrength(void)
{
	if (g_iWickedEntityId < 0) return 0.0f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fEmissive[g_iWickedMeshNumber] >= 0.0f)
				return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fEmissive[g_iWickedMeshNumber];
			else
				return(0.0f);
		}
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fEmissive[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.fEmissive[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fEmissive[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS && t.entityprofile[g_iWickedEntityId].WEMaterial.fEmissive[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fEmissive[g_iWickedMeshNumber];
	}
	return 0.0f;
}

float WickedGetAlphaRef(void)
{
	if (g_iWickedEntityId < 0) return 1.0f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.baseColorMapName[g_iWickedMeshNumber].Len() > 0) 
		{
			if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fAlphaRef[g_iWickedMeshNumber] < 0)
				return(1.0f);
			return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fAlphaRef[g_iWickedMeshNumber];
		}
		if (t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fAlphaRef[g_iWickedMeshNumber] < 0)
			return(1.0f);
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fAlphaRef[g_iWickedMeshNumber];
	}
	if (t.entityprofile[g_iWickedEntityId].WEMaterial.fAlphaRef[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fAlphaRef[g_iWickedMeshNumber];
	}
	if (g_iWickedMeshNumber >= 0 && g_iWickedMeshNumber < MAXMESHMATERIALS && t.entityprofile[g_iWickedEntityId].WEMaterial.fAlphaRef[g_iWickedMeshNumber] >= 0.0) 
	{
		return t.entityprofile[g_iWickedEntityId].WEMaterial.fAlphaRef[g_iWickedMeshNumber];
	}
	return 1.0f;
}

bool WickedDoubleSided(void)
{
	if (g_iWickedEntityId < 0) return false;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.bDoubleSided[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.bDoubleSided[g_iWickedMeshNumber];
}

float WickedRenderOrderBias(void)
{
	if (g_iWickedEntityId < 0) return false;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fRenderOrderBias[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.fRenderOrderBias[g_iWickedMeshNumber];
}

bool WickedGetTransparent(void)
{
	bool bIsTransparent = false;
	if (g_iWickedEntityId < 0) return false;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
		bIsTransparent = t.entityelement[g_iWickedElementId].eleprof.WEMaterial.bTransparency[g_iWickedMeshNumber];
	else
		bIsTransparent = t.entityprofile[g_iWickedEntityId].WEMaterial.bTransparency[g_iWickedMeshNumber];

	return(bIsTransparent);
}

bool WickedGetCastShadows(void)
{
	if (g_iWickedEntityId < 0) return false;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.bCastShadows[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.bCastShadows[g_iWickedMeshNumber];
}

bool WickedPlanerReflection(void)
{
	if (g_iWickedEntityId < 0) return false;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.bPlanerReflection[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.bPlanerReflection[g_iWickedMeshNumber];
}

float WickedGetReflectance(void)
{
	if (g_iWickedEntityId < 0)
		return 0.04f;// 0.002f;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.fReflectance[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.fReflectance[g_iWickedMeshNumber];
}

DWORD WickedGetEmmisiveColor(void)
{
	if (g_iWickedEntityId < 0) return -1;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.dwEmmisiveColor[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.dwEmmisiveColor[g_iWickedMeshNumber];
}

DWORD WickedGetBaseColor(void)
{
	if (g_iWickedEntityId < 0) return -1;
	if (g_iWickedElementId > 0 && t.entityelement[g_iWickedElementId].eleprof.WEMaterial.MaterialActive)
	{
		return t.entityelement[g_iWickedElementId].eleprof.WEMaterial.dwBaseColor[g_iWickedMeshNumber];
	}
	return t.entityprofile[g_iWickedEntityId].WEMaterial.dwBaseColor[g_iWickedMeshNumber];
}

void Wicked_Highlight_ClearAllObjects(void)
{
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		int obj = t.entityelement[e].obj;
		if (obj > 0)
		{
			sObject* pObject = GetObjectData(obj);
			if (pObject)
			{
				WickedCall_SetObjectHighlightBlue(pObject, false);
			}
		}
	}
}

void Wicked_Highlight_AllLogicObjects(void)
{
	for (int e = 1; e <= g.entityelementlist; e++)
	{
		// only highlight the logic objects in blue
		if (t.entityelement[e].staticflag == 0)
		{
			int obj = t.entityelement[e].obj;
			if (obj > 0)
			{
				sObject* pObject = GetObjectData(obj);
				if (pObject)
				{
					WickedCall_SetObjectHighlightBlue(pObject, true);
					g_ObjectHighlightList.push_back(obj);
				}
			}
		}
	}
}

void Wicked_Highlight_Rubberband(void)
{
	if (t.grideditselect == 5 && g.entityrubberbandlist.size() > 0)
	{
		for (int i = 0; i < (int)g.entityrubberbandlist.size(); i++)
		{
			int e = g.entityrubberbandlist[i].e;
			if (e >= t.entityelement.size())
			{
				g.entityrubberbandlist.clear();
				return;
			}
			int tobj = t.entityelement[e].obj;
			if (tobj > 0)
			{
				sObject* pObject = GetObjectData(tobj);
				if (pObject) {
					void WickedCall_DrawObjctBox(sObject* pObject, XMFLOAT4 color, bool bThickLine = false, bool ForceBox = false);
					WickedCall_DrawObjctBox(pObject, XMFLOAT4(0.75f, 0.75f, 0.75f, 0.75f),false,false);
				}
			}
		}
	}
}

std::vector<sRubberBandType> entityselectionlist;
void Wicked_Highlight_Selection(void)
{
	if (t.grideditselect == 5 && entityselectionlist.size() > 0)
	{
		for (int i = 0; i < (int)entityselectionlist.size(); i++)
		{
			int e = entityselectionlist[i].e;
			int tobj = t.entityelement[e].obj;
			if (tobj > 0)
			{
				sObject* pObject = GetObjectData(tobj);
				if (pObject) {
					void WickedCall_DrawObjctBox(sObject* pObject, XMFLOAT4 color, bool bThickLine = false, bool ForceBox = false);
					WickedCall_DrawObjctBox(pObject, XMFLOAT4(1.0f, 0.75f, 0.0f, 0.85f), true,true);
				}
			}
		}
	}
}

extern std::vector<sRubberBandType> vEntityLockedList;
void Wicked_Highlight_LockedList(void)
{
	if (t.grideditselect == 5 && vEntityLockedList.size() > 0)
	{
		for (int i = 0; i < (int)vEntityLockedList.size(); i++)
		{
			int e = vEntityLockedList[i].e;
			#ifdef ALLOWSELECTINGLOCKEDOBJECTS
			//PE: Only diplay red on selected items.
			bool bDisplayRed = false;
			if (e == t.widget.pickedEntityIndex)
				bDisplayRed = true;
			if (!bDisplayRed)
			{
				//PE: Also need rubberband items. (if within a selected group).
				if (g.entityrubberbandlist.size() > 0)
				{
					for (int ir = 0; ir < (int)g.entityrubberbandlist.size(); ir++)
					{
						if (e == g.entityrubberbandlist[ir].e)
						{
							bDisplayRed = true;
							break;
						}
					}
				}
			}
			if (!bDisplayRed)
			{
				//PE: Removed highlight of locked objects.
				////And add hovered object.
				//extern sObject* g_hovered_pobject;
				////g_hovered_pobject
				//sObject* pObject = GetObjectData(t.entityelement[e].obj);
				//if (pObject == g_hovered_pobject)
				//{
				//	bDisplayRed = true;
				//}
			}
				
			if(bDisplayRed)
			{
			#endif

				int tobj = t.entityelement[e].obj;
				if (tobj > 0)
				{
					sObject* pObject = GetObjectData(tobj);
					if (pObject) {
						void WickedCall_DrawObjctBox(sObject* pObject, XMFLOAT4 color, bool bThickLine = false, bool ForceBox = false);
						WickedCall_DrawObjctBox(pObject, XMFLOAT4(1.0f, 0.0f, 0.0f, 0.85f), false, false);
					}
				}
			#ifdef ALLOWSELECTINGLOCKEDOBJECTS
			}
			#endif
		}
	}
}


void Wicked_Hide_Lower_Lod_Meshes(int obj)
{
	PerformCheckListForLimbs(obj);
	int bestlod = -1;
	bool bHasLOD = false;
	for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
	{
		t.tname_s = Lower(ChecklistString(t.c));
		LPSTR pRightFive = "";
		if (strlen(t.tname_s.Get()) >= 5) pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;

		if ((stricmp(pRightFive, "lod_1") == 0 || stricmp(pRightFive, "_lod1") == 0) )
			bHasLOD = true;
		if ((stricmp(pRightFive, "lod_2") == 0 || stricmp(pRightFive, "_lod2") == 0))
			bHasLOD = true;

		if ((stricmp(pRightFive, "lod_1") == 0 || stricmp(pRightFive, "_lod1") == 0) && (bestlod == -1 || bestlod > 1))
		{
			bestlod = 1;
		}
		else
		{
			if ((stricmp(pRightFive, "lod_2") == 0 || stricmp(pRightFive, "_lod2") == 0) && (bestlod == -1))
			{
				bestlod = 2;
			}
			else
			{
				// base (highest) LOD does not need the lod0 or _lod postfix
				//if ((stricmp(pRightFive, "lod_0") == 0 || stricmp(pRightFive, "_lod0") == 0)) bestlod = 0;
				bestlod = 0;
			}
		}

		// also hide ANY collision_mesh frame as this should never be visible
		if (stricmp(t.tname_s.Get(), "collision_mesh") == 0)
		{
			HideLimb(obj, t.c - 1);
		}
	}
	if (bHasLOD)
	{
		for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
		{
			t.tname_s = Lower(ChecklistString(t.c));
			LPSTR pRightFive = "";
			if (strlen(t.tname_s.Get()) >= 5) pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;
			if (bestlod == 0 && (stricmp(pRightFive, "lod_1") == 0 || stricmp(pRightFive, "lod_2") == 0 || stricmp(pRightFive, "lod_3") == 0)) 
			{
				HideLimb(obj, t.c - 1);
			}
			if (bestlod == 0 && (stricmp(pRightFive, "_lod1") == 0 || stricmp(pRightFive, "_lod2") == 0 || stricmp(pRightFive, "_lod3") == 0)) 
			{
				HideLimb(obj, t.c - 1);
			}
			if (bestlod == 1 && (stricmp(pRightFive, "lod_2") == 0 || stricmp(pRightFive, "_lod2") == 0)) 
			{
				HideLimb(obj, t.c - 1);
			}
			if (bestlod == 2 && (stricmp(pRightFive, "lod_3") == 0 || stricmp(pRightFive, "_lod3") == 0)) 
			{
				HideLimb(obj, t.c - 1);
			}
		}
	}
}

#endif

int GetLodLevels(int obj)
{
	PerformCheckListForLimbs(obj);
	int iLodLevels = 0;
	int bestlod = -1;
	bool bHasLOD = false;
	for (t.c = ChecklistQuantity(); t.c >= 1; t.c += -1)
	{
		t.tname_s = Lower(ChecklistString(t.c));
		LPSTR pRightFive = "";
		if (strlen(t.tname_s.Get()) >= 5) pRightFive = t.tname_s.Get() + strlen(t.tname_s.Get()) - 5;
		if ((stricmp(pRightFive, "lod_1") == 0 || stricmp(pRightFive, "_lod1") == 0))
			iLodLevels++;
		else if ((stricmp(pRightFive, "lod_2") == 0 || stricmp(pRightFive, "_lod2") == 0))
			iLodLevels++;
		else if ((stricmp(pRightFive, "lod_3") == 0 || stricmp(pRightFive, "_lod3") == 0))
			iLodLevels++;

	}
	return(iLodLevels);
}

void entity_updatequatfromeuler (int e)
{
	// update entity quat as the preferred source rotation
	GGQUATERNION QuatAroundX, QuatAroundY, QuatAroundZ;
	GGQuaternionRotationAxis(&QuatAroundX, &GGVECTOR3(1, 0, 0), GGToRadian(t.entityelement[e].rx));
	GGQuaternionRotationAxis(&QuatAroundY, &GGVECTOR3(0, 1, 0), GGToRadian(t.entityelement[e].ry));
	GGQuaternionRotationAxis(&QuatAroundZ, &GGVECTOR3(0, 0, 1), GGToRadian(t.entityelement[e].rz));
	GGQUATERNION quatNewOrientation = QuatAroundX * QuatAroundY * QuatAroundZ;
	t.entityelement[e].quatmode = 1;
	t.entityelement[e].quatx = quatNewOrientation.x;
	t.entityelement[e].quaty = quatNewOrientation.y;
	t.entityelement[e].quatz = quatNewOrientation.z;
	t.entityelement[e].quatw = quatNewOrientation.w;
}

void entity_updatequat (int e, float quatx, float quaty, float quatz, float quatw)
{
	t.entityelement[e].quatmode = 1;
	t.entityelement[e].quatx = quatx;
	t.entityelement[e].quaty = quaty;
	t.entityelement[e].quatz = quatz;
	t.entityelement[e].quatw = quatw;
}

void entity_calculateeuleryfromquat (int e)
{
	if (t.entityelement[e].quatmode != 0 ) // seems some corruption can given this value a non-zero but still have quat rot data
	{
		// only do if have a good quat
		GGQUATERNION Quat = GGQUATERNION(t.entityelement[e].quatx, t.entityelement[e].quaty, t.entityelement[e].quatz, t.entityelement[e].quatw);
		GGMATRIX matQuatRot;
		GGMatrixRotationQuaternion(&matQuatRot, &Quat);
		GGVECTOR3 positionalOffset = GGVECTOR3(0, 0, 1);
		GGVec3TransformCoord(&positionalOffset, &positionalOffset, &matQuatRot);
		float fRealWorldYAngle = Atan2(positionalOffset.x, positionalOffset.z);
		t.entityelement[e].rx = 0;
		t.entityelement[e].ry = fRealWorldYAngle;
		t.entityelement[e].rz = 0;

		// ensure generated euler is exactly matching quat (and sets quatmode to 1 in case of older level corruption data)
		entity_updatequatfromeuler(e);
	}
	else
	{
		// otherwise we already have the euler values
	}
}
