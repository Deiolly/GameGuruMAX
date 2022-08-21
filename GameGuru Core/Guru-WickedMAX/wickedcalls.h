//
// Wicked Calls Header
//

// Includes
#include <memory>
#include "..\Dark Basic Public Shared\Dark Basic Pro SDK\Shared\Objects\CObjectDataC.h"

// Custom Layers
#ifndef GGRENDERLAYERSENUM
#define GGRENDERLAYERSENUM
enum GGRENDERLAYERS
{
	GGRENDERLAYERS_VOID = 0,
	GGRENDERLAYERS_NORMAL = 1,
	GGRENDERLAYERS_CURSOROBJECT = 2,
	GGRENDERLAYERS_WIDGETPLANE = 4,
	GGRENDERLAYERS_TERRAIN = 8,
	GGRENDERLAYERS_ALL = GGRENDERLAYERS_NORMAL | GGRENDERLAYERS_CURSOROBJECT | GGRENDERLAYERS_WIDGETPLANE | GGRENDERLAYERS_TERRAIN
};
#endif

// pre-declare Wicked classes
struct wiResource;

// Image Management
#ifndef GGIMAGERESENUM
#define GGIMAGERESENUM
enum eImageResType
{
	IMAGERES_GENERIC,
	IMAGERES_UI,
	IMAGERES_LEVEL
};
struct sImageList
{
	LPSTR pName;
	eImageResType eType;
	std::shared_ptr<wiResource> image;
	int iMemUsedKB = 0;
};
#endif

void WickedCall_InitImageManagement(LPSTR pRootFolder);
void WickedCall_FreeImage(sImageList* pImage);
void WickedCall_FreeAllImagesOfType(eImageResType eType);
void WickedCall_GetRelativeAfterRoot(std::string pFilename, LPSTR pFullRelativeLocationFilename);
int WickedCall_FindImageIndexInList(std::string pFilenameToFind, LPSTR pFullRelativeLocationFilename);
void WickedCall_AddImageToList(std::shared_ptr<wiResource> image, eImageResType eType, std::string pFilenameRef, int iKbused);
std::shared_ptr<wiResource> WickedCall_LoadImage(std::string pFilenameToLoad, eImageResType eType);
std::shared_ptr<wiResource> WickedCall_LoadImage(std::string pFilenameToLoad);
void WickedCall_DeleteImage(std::string pFilenameToLoad);

// Functions
void WickedCall_RefreshObjectAnimations(sObject* pObject, void* pstate);
void WickedCall_AddObject( sObject* pObject );
void WickedCall_SetObjectSpeed(sObject* pObject, float fSpeed);
void WickedCall_CheckAnimationDone(sObject* pObject);
bool WickedCall_GetAnimationPlayingState (sObject* pObject);
void WickedCall_SetAnimationLerpFactor (sObject* pObject);
void WickedCall_PlayObject(sObject* pObject, float fStart, float fEnd, bool bLooped);
void WickedCall_InstantObjectFrameUpdate(sObject* pObject);
void WickedCall_GetObjectAnimationData(sObject* pObject, float* pStart, float* pFinish);
void WickedCall_StopObject(sObject* pObject);
void WickedCall_SetObjectFrame(sObject* pObject, float fFrame);
void WickedCall_SetObjectFrameEx(sObject* pObject, float fFrame);
float WickedCall_GetObjectFrame(sObject* pObject);
float WickedCall_GetObjectRealFrame(sObject* pObject);
void WickedCall_RemoveObject( sObject* pObject );
void WickedCall_SetTexturePath ( LPSTR pPath );
void WickedCall_TextureMesh ( sMesh* pMesh );
void WickedCall_SetReflectance (sMesh* pMesh, float fReflectance);
void WickedCall_SetObjectReflectance(sObject* pObject, float fReflectance);
float WickedCall_GetObjectReflectance(sObject* pObject);
void WickedCall_SetMeshMaterial ( sMesh* pMesh );
void WickedCall_TextureObjectAsEmissive(sObject* pObject);
void WickedCall_SetAsEmissiveMaterial(sMesh* pMesh);
void WickedCall_TextureObject ( sObject* pObject, sMesh* pJustThisMesh );
void WickedCall_SetObjectBaseColor(sObject* pObject, int r, int g, int b);
void WickedCall_GetObjectBaseColor(sObject* pObject, int* r, int* g, int* b);
void WickedCall_SetObjectEmissiveColor(sObject* pObject, int r, int g, int b);
void WickedCall_GetObjectEmissiveColor(sObject* pObject, int* r, int* g, int* b);
void WickedCall_SetObjectNormalness(sObject* pObject, float normalness);
float WickedCall_GetObjectNormalness(sObject* pObject);
void WickedCall_SetObjectRoughness(sObject* pObject, float roughness);
float WickedCall_GetObjectRoughness(sObject* pObject);
void WickedCall_SetObjectMetalness(sObject* pObject, float metalness);
float WickedCall_GetObjectMetalness(sObject* pObject);
void WickedCall_SetObjectEmissiveStrength(sObject* pObject, float strength);
float WickedCall_GetObjectEmissiveStrength(sObject* pObject);
void WickedCall_SetObjectTransparentDirect(sObject* pObject, bool bTransparent);
bool WickedCall_GetObjectTransparentDirect(sObject* pObject);
void WickedCall_TextureMeshWithImagePtr(sMesh* pMesh, int iPutInEmissivemode);
void WickedCall_TextureObjectWithImagePtr ( sObject* pObject, int iPutInEmissivemode );
void WickedCall_UpdateObject(sObject* pObject);
void WickedCall_UpdateLimbsOfObject( sObject* pObject );
void WickedCall_UpdateSceneForPick(void);
void WickedCall_SetRenderOrderBias(sMesh* pMesh, float fDistanceToAdd);
float WickedCall_GetRenderOrderBias(sMesh* pMesh);
void WickedCall_SetObjectRenderOrderBias(sObject* pObject, float fRenderOrderBias);
float WickedCall_GetObjectRenderOrderBias(sObject* pObject);
void WickedCall_SetObjectPlanerReflection(sObject* pObject, bool bPlanerReflection);
bool WickedCall_GetObjectPlanerReflection(sObject* pObject);
void WickedCall_SetLimbVisible(sFrame* pFrame, bool bVisible);
void WickedCall_SetObjectVisible ( sObject* pObject, bool bVisible );
void WickedCall_GlueObjectToObject ( sObject* pObjectToGlue, sObject* pParentObject, int iLimb);
void WickedCall_UnGlueObjectToObject ( sObject* pObjectToUnGlue);
void WickedCall_PresetObjectRenderLayer(int iLayerMask);
void WickedCall_PresetObjectLimbRenderLayer(int iLayerMask, int iLimb);
void WickedCall_PresetObjectUVScale(float fUScale, float fVScale);
void WickedCall_PresetObjectCreateOnDemand(bool bCreateOnlyWhenUsed);
void WickedCall_PresetObjectIgnoreTextures(bool bIgnoreTextureInfo);
void WickedCall_PresetObjectTextureFromImagePtr(bool bUseImagePtrInsteadOfTexFile, int iPutInEmissiveMode);
void WickedCall_PresetObjectPutInEmissive(int iPutInEmissiveMode);
void WickedCall_RotateLimb(sObject* pObject, sFrame* pFrame, float fAX, float fAY, float fAZ);
void WickedCall_CalculateWorldQuat(GGVECTOR4* pQuatA, GGVECTOR4* pQuatB, GGVECTOR4* pQuatResult);
void WickedCall_CalculateQuatDiff(GGVECTOR4* pQuatA, GGVECTOR4* pQuatB, GGVECTOR4* pQuatDiff);
void WickedCall_CalculateQuatFromCombined(sObject* pObject, sFrame* pFrame, GGVECTOR4* pQuat);
void WickedCall_OverrideLimbWithCombined(sObject* pObject, sFrame* pFrame, bool bIncludeTranslation);
void WickedCall_OverrideLimbOff(sObject* pObject, sFrame* pFrame);
void WickedCall_SetBip01Position(sObject* pObject, sFrame* pFrame, int iUseMode, float fX, float fZ);
void WickedCall_SetBip01Rotation(sObject* pObject, sFrame* pFrame, int iUseMode, float fY);
void WickedCall_SetObjectPreFrames(sObject* pObject, LPSTR pParentFrameName, float fFrameToUse, float fSmoothSlerpToNextShape, int iPreFrameMode);
void WickedCall_SetObjectRenderLayer ( sObject* pObject, int iLayerMask );
bool WickedCall_GetPick2(float fMouseX, float fMouseY, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask);
bool WickedCall_GetPick ( float* pOutX, float* pOutY, float* pOutZ, float* pNormalX, float* pNormalY, float* pNormalZ, uint64_t* pObject, int iLayerMask );
bool WickedCall_SentRay(float originx, float originy, float originz, float directionx, float directiony, float directionz, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask);
bool WickedCall_SentRay2(float originx, float originy, float originz, float directionx, float directiony, float directionz, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, uint64_t* pHitEntity, int iLayerMask);
bool WickedCall_SentRay3(float originx, float originy, float originz, float directionx, float directiony, float directionz, float fDistanceOfRay, float* pOutX, float* pOutY, float* pOutZ, float* pNormX, float* pNormY, float* pNormZ, DWORD* pdwObjectNumberHit);
void WickedCall_GetMouseDeltas(float* pfX, float* pfY);
uint32_t WickedCall_GetTextureWidth(void* ptex);
uint32_t WickedCall_GetTextureHeight(void* ptex);
void WickedCall_SetObjectHighlight(sObject* pObject, bool bHighlight); 
void WickedCall_SetObjectHighlightColor(sObject* pObject, bool bHighlight, int highlightColorType);
void WickedCall_SetObjectHighlightRed(sObject* pObject, bool bHighlight);
void WickedCall_SetObjectHighlightBlue(sObject* pObject, bool bHighlight);
void WickedCall_RenderEditorFunctions(void);
void WickedCall_SetSelectedObject(sObject* pObject);
uint64_t WickedCall_GetFirstRootEntityID(sObject* pObject);
int WickedCall_GetNumberOfRootEntityIDs(sObject* pObject);
void WickedCall_SetMeshTransparent(sMesh* pMesh);
void WickedCall_SetMeshCullmode(sMesh* pMesh);
void WickedCall_SetObjectCullmode(sObject* pObject);
void WickedCall_SetObjectDoubleSided(sObject* pObject, bool bDoubleSided);
bool WickedCall_GetObjectDoubleSided(sObject* pObject);
void WickedCall_SetObjectTransparent(sObject* pObject);
void WickedCall_SetObjectDisableDepth(sObject* pObject);
std::string WickedCall_GetAllTexturesUsed(sObject* pObject);
void WickedCall_SetMeshAlpha(sMesh* pMesh, float fPercentage);
LPSTR WickedCall_GetMeshMaterialName(sMesh* pMesh);
void WickedCall_GetFrameWorldPos(sFrame* pFrame, float* pfX, float* pfY, float* pfZ);
void WickedCall_GetLimbDataEx(sObject* pObject, int iLimbID, bool bAdjustLimb, float fX, float fY, float fZ, float fAX, float fAY, float fAZ, float* pX, float* pY, float* pZ, float* pQAX, float* pQAY, float* pQAZ, float* pQAW);
void WickedCall_GetLimbLocalPosAndRot(sObject* pObject, int iLimbID, float* pX, float* pY, float* pZ, float* pQAX, float* pQAY, float* pQAZ, float* pQAW);
void WickedCall_GetLimbData(sObject* pObject, int iLimbID, float* pX, float* pY, float* pZ, float* pQAX, float* pQAY, float* pQAZ, float* pQAW);
void WickedCall_UpdateMeshVertexData(sMesh* pMesh);
void WickedCall_SetObjectAlpha(sObject* pObject, float fPercentage);
float WickedCall_GetObjectAlpha(sObject* pObject);
void WickedCall_SetObjectCastShadows(sObject* pObject, bool bCastShadow);
bool WickedCall_GetObjectCastShadows(sObject* pObject);
void WickedCall_SetObjectLightToUnlit(sObject* pObject, int shaderType);
uint64_t WickedCall_AddLight(int iLightType);
void WickedCall_DeleteLight(uint64_t wickedlightindex);
void WickedCall_UpdateLight(uint64_t wickedlightindex, float fX, float fY, float fZ, float fAX, float fAY, float fAZ, float fRange, float fSpotRadius, int iColR, int iColG, int iColB, bool bCastShadow);
void WickedCall_UpdateProbes(void);
int WickedCall_Get2DShadowLights(void);
int WickedCall_GetCubeShadowLights(void);
void WickedCall_SetEditorCameraLight(bool bSwitchOn);
void WickedCall_SetSpriteBoundBox(bool bShow,float fX1, float fY1,float fX2, float fY2);
void WickedCall_SetSunDirection(float fAx, float fAy, float fAz);
void WickedCall_SetSunColors(float fRed, float fGreen, float fBlue, float fEnergy, float fFov, float fShadowBias);
void WickedCall_SetVisualizerEnabled(bool bVisualizer);
void WickedCall_SunSetVolumetricsEnabled(bool bVolumetrics);
void WickedCall_SunSetSetStatic(bool bSetStatic);
void WickedCall_SunSetRange(float fRange);
void WickedCall_SetTextureName(int obj, char *texturename);
//void WickedCall_DrawImguiNow(void);
void WickedCall_SetCameraFOV(float fFOV);
int WickedCall_GetSkinable(void);
int WickedCall_GetSkinableVisible(void);

void WickedCall_SetShadowRange(float ShadowFar);
void WickedCall_SetMeshAlphaRef(sMesh* pMesh, float fAlphaRef);
void WickedCall_SetObjectAlphaRef(sObject* pObject, float fAlphaRef);
float WickedCall_GetObjectAlphaRef(sObject* pObject);
void WickedCall_EnableCameraLight(bool On);
void WickedCall_EnableThumbLight(bool On);
void WickedCall_SetLDSSkinningEnabled(bool enabled);
void WickedCall_DisplayCubes(bool Visible);

void WickedCall_CreateReflectionProbe(float x, float y, float z, char *name, float size);
void WickedCall_DeleteReflectionProbe(char *name);
void WickedCall_MoveReflectionProbe(float x, float y, float z, char *name, float size);

void WickedCall_SetLightShaftParameters(float density, float weight, float decay, float exposure);
void WickedCall_GetLightShaftParameters(float& density, float& weight, float& decay, float& exposure);
void WickedCall_SetRenderTargetMouseFocus(bool focus);

void WickedCall_UpdateWaterHeight(float height);