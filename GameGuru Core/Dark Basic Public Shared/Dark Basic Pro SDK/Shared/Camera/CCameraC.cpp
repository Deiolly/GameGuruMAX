// GG flag header for preprocessor defines
#include "..\..\..\..\GameGuru\Include\preprocessor-flags.h"
#include ".\..\error\cerror.h"
#include "globstruct.h"
#include "ccamerac.h"
#include "CGfxC.h"
#include "CImageC.h"
#include "cVectorC.h"
#include "CObjectsC.h"

#ifdef WICKEDENGINE
#include ".\..\..\..\..\Guru-WickedMAX\wickedcalls.h"
#endif

// Hack hook to call some master wicked from Camera LIB
extern void camerahook_domydemostuff(float fX, float fY, float fZ, float fAX, float fAY, float fAZ);
extern void camerahook_domydemostuff2(float* fX, float* fY, float* fWidth, float* fHeight);

extern GGFORMAT GetValidStencilBufferFormat ( GGFORMAT BackBufferFormat );

// Externals
extern HINSTANCE				g_GFX;
extern LPGG						m_pDX;
extern GlobStruct*				g_pGlob;
extern bool						g_VR920AdapterAvailable;
extern UINT						g_StereoEyeToggle;
extern bool						g_VR920RenderStereoNow;
extern float					g_fVR920TrackingYaw;
extern float					g_fVR920TrackingPitch;
extern float					g_fVR920TrackingRoll;
extern float					g_fVR920Sensitivity;
extern float					g_fDriverCompensationYaw;
extern float					g_fDriverCompensationPitch;
extern float					g_fDriverCompensationRoll;

// Globals
int								m_iRenderCamera					= 0;
int								m_iCurrentCamera				= 0;
bool							m_bAutoCamState					= true;
bool							m_bActivateBackdrop				= true;
float							lastmovetargetx					= 0.0f;
float							lastmovetargety					= 0.0f;
float							lastmovetargetz					= 0.0f;
sObject*						m_Object_Ptr;
CCameraManager					m_CameraManager;
tagCameraData*					m_ptr;
bool							g_bCameraOutputToImage			= false;
DWORD							m_dwImageDepthWidth;
DWORD							m_dwImageDepthHeight;
int								g_iCameraHasClipPlane			= 0;		// added 310506 - u62
DWORD							g_dwMaxUserClipPlanes			= 0;		// added 310506 - u62
tagCameraData*					g_pStereoscopicCameraUpdated	= NULL;		// added 180508 - u69
IGGTexture*						g_pStereoscopicFinalTexture		= NULL;		// U7.0 - 180608 - WOW autostereo

#if defined(ENABLEIMGUI) && !defined(USEOLDIDE) 
float custom_back_color[4] = { 0,0,0,0 };
#endif

// Core Utility Functions

bool UpdateCameraPtr ( int iID )
{
	// use this to get the camera data, every function needs to call this
	// it's made as a separate function so if we need to make any changes
	// we only need to do it in 1 place

	// set the pointer to null
	m_ptr = NULL;
	
	// see if the camera is valid and we can get the data, if this fails
	// return false and let the caller know we couldn't get the data
	if ( !( m_ptr = m_CameraManager.GetData ( iID ) ) )
		return false;
	
	// return true if everything is ok - we are able to update the pointer
	return true;
}

DARKSDK void* GetCameraInternalData ( int iID )
{
	// get the data for the camera, this can be useful if
	// you want to access the structure directly

	// update internal pointer
	if ( !UpdateCameraPtr ( iID ) )
		return NULL;

	// return a pointer to the data
	return m_ptr;
}

LPGGDEVICE					m_pD3Dave = NULL;

DARKSDK int GetPowerSquareOfSize( int Size )
{
	if ( Size <= 1 ) Size = 1;
	else if ( Size <= 2 ) Size = 2;
	else if ( Size <= 4 ) Size = 4;
	else if ( Size <= 8 ) Size = 8;
	else if ( Size <= 16 ) Size = 16;
	else if ( Size <= 32 ) Size = 32;
	else if ( Size <= 64 ) Size = 64;
	else if ( Size <= 128 ) Size = 128;
	else if ( Size <= 256 ) Size = 256;
	else if ( Size <= 512 ) Size = 512;
	else if ( Size <= 1024 ) Size = 1024;
	else if ( Size <= 2048 ) Size = 2048;
	else if ( Size <= 4096 ) Size = 4096;
	return Size;
}

DARKSDK float Camerawrapangleoffset(float da)
{
	int breakout=100;
	while(da<0.0f || da>=360.0f)
	{
		if(da<0.0f) da=da+360.0f;
		if(da>=360.0f) da=da-360.0f;
		breakout--;
		if(breakout==0) break;
	}
	if(breakout==0) da=0.0f;
	return da;
}

DARKSDK void CameraGetAngleFromPoint(float x1, float y1, float z1, float x2, float y2, float z2, float* ax, float* ay, float* az)
{
	GGVECTOR3 Vector;
	Vector.x = x2-x1;
	Vector.y = y2-y1;
	Vector.z = z2-z1;

	// Find Y and then X axis rotation
	double yangle=atan2(Vector.x, Vector.z);
	if(yangle<0.0) yangle+=GGToRadian(360.0);
	if(yangle>=GGToRadian(360.0)) yangle-=GGToRadian(360.0);

	GGMATRIX yrotate;
	GGMatrixRotationY ( &yrotate, (float)-yangle );
	GGVec3TransformCoord ( &Vector, &Vector, &yrotate );

	double xangle=-atan2(Vector.y, Vector.z);
	if(xangle<0.0) xangle+=GGToRadian(360.0);
	if(xangle>=GGToRadian(360.0)) xangle-=GGToRadian(360.0);

	*ax = wrapangleoffset(GGToDegree((float)xangle));
	*ay = wrapangleoffset(GGToDegree((float)yangle));
	*az = 0.0f;
}

DARKSDK void SetAutoCam ( float fX, float fY, float fZ, float fRadius )
{
	// Initial backdrop activator
	if(m_bActivateBackdrop==true)
	{
		m_bActivateBackdrop=false;
		BackdropOn();
	}

	if(m_bAutoCamState==true)
	{
		// update the pointer
		if ( !UpdateCameraPtr ( m_iCurrentCamera ) )
			return;

		// Position of target
		GGVECTOR3 FullvPos;
		FullvPos.x = fX;
		FullvPos.y = fY;
		FullvPos.z = fZ;

		// Ensure object is in front of default camera
		m_ptr->vecPosition.x = FullvPos.x;
		m_ptr->vecPosition.y = FullvPos.y + (fRadius/2.0f);
		m_ptr->vecPosition.z = FullvPos.z - (fRadius*3.0f);
		if(m_ptr->vecPosition.z>-2.0f) m_ptr->vecPosition.z=-2.0f;

		// Point towards object
		CameraGetAngleFromPoint ( m_ptr->vecPosition.x, m_ptr->vecPosition.y, m_ptr->vecPosition.z,
							fX, fY, fZ, &m_ptr->fXRotate, &m_ptr->fYRotate, &m_ptr->fZRotate);

		// update the camera
		CameraInternalUpdate ( m_iCurrentCamera );
	}
}

DARKSDK GGMATRIX GetMatrixCamera ( int iID )
{
	return GetCameraMatrix ( iID );
}

DARKSDK GGMATRIX GetCameraMatrix ( int iID )
{
	// update the pointer
	if ( !UpdateCameraPtr ( iID ) )
	{
		GGMATRIX d3dNull;
		GGMatrixIdentity ( &d3dNull );
		return d3dNull;
	}

	// return the matrix
	return m_ptr->matView;
}

DARKSDK void CameraAnglesFromMatrix ( GGMATRIX* pmatMatrix, GGVECTOR3* pVecAngles )
{
	// Thanks to Andrew for finding this gem!
	// from http://www.martinb.com/maths/geometry/rotations/conversions/matrixToEuler/index.htm
	float m00 = pmatMatrix->_11;
	float m01 = pmatMatrix->_12;
	float m02 = pmatMatrix->_13;
	float m12 = pmatMatrix->_23;
	float m22 = pmatMatrix->_33;
	float heading = (float)atan2(m01,m00);
	float attitude = (float)atan2(m12,m22);
	float bank = (float)asin(-m02);

	// check for gimbal lock
	if ( fabs ( m02 ) > 1.0f )
	{
		// looking straight up or down
		float PI = GG_PI / 2.0f;
		pVecAngles->x = 0.0f;
		pVecAngles->y = GGToDegree ( PI * m02 );
		pVecAngles->z = 0.0f;
	}
	else
	{
		pVecAngles->x = GGToDegree ( attitude );
		pVecAngles->y = GGToDegree ( bank );
		pVecAngles->z = GGToDegree ( heading );
	}
}

DARKSDK void CameraInternalUpdateViewAndProj ( GGMATRIX* pmatView, GGMATRIX *pmatProj )
{
	GGSetTransform ( GGTS_VIEW, pmatView );
	GGSetTransform ( GGTS_PROJECTION, pmatProj );
}

DARKSDK void CameraInternalUpdate ( int iID )
{
	// update any properties of the camera and set the transform
	if ( m_ptr->bOverride )
	{
		CameraInternalUpdateViewAndProj ( &m_ptr->matViewOverride, &m_ptr->matProjOverride );
		return;
	}
	
	// variable declarations
	GGMATRIX matRot;
	GGMATRIX matTempRot;
	GGMatrixIdentity ( &m_ptr->matView );
	
	// Create a rotation matrix
	if(m_ptr->bUseFreeFlightRotation==false)
	{
		// STANDARD EULER ANGLE ROTATION HANDLING
		if ( m_ptr->bRotate )
			GGMatrixRotationX ( &matRot, GGToRadian ( m_ptr->fXRotate ) );
		else
			GGMatrixRotationZ ( &matRot, GGToRadian ( m_ptr->fZRotate ) );

		GGMatrixRotationY ( &matTempRot, GGToRadian ( m_ptr->fYRotate ) );
		GGMatrixMultiply ( &matRot, &matRot, &matTempRot );

		if ( m_ptr->bRotate )
			GGMatrixRotationZ ( &matTempRot, GGToRadian ( m_ptr->fZRotate ) );
		else
			GGMatrixRotationX ( &matTempRot, GGToRadian ( m_ptr->fXRotate ) );

		GGMatrixMultiply ( &matRot, &matRot, &matTempRot );

		// rotation recreated each time for accuracy
		m_ptr->vecRight.x=1;
		m_ptr->vecRight.y=0;
		m_ptr->vecRight.z=0;
		m_ptr->vecUp.x=0;
		m_ptr->vecUp.y=1;
		m_ptr->vecUp.z=0;
		m_ptr->vecLook.x=0;
		m_ptr->vecLook.y=0;
		m_ptr->vecLook.z=1;
		
		GGVec3TransformCoord ( &m_ptr->vecRight,	&m_ptr->vecRight,	&matRot );
		GGVec3TransformCoord ( &m_ptr->vecUp,		&m_ptr->vecUp,		&matRot );
		GGVec3TransformCoord ( &m_ptr->vecLook,	&m_ptr->vecLook,	&matRot );

		// Get tracking data
		GGVECTOR3 vecPos = m_ptr->vecPosition;
		if ( g_VR920AdapterAvailable == true && g_VR920RenderStereoNow == true )
		{
			if ( g_pGlob->dwRenderCameraID == 0 )
			{
				// Get iWear tracking yaw, pitch, roll for head tracking
				float fYaw = GGToRadian(g_fVR920TrackingYaw);
				float fPitch = GGToRadian(g_fVR920TrackingPitch);
				float fRoll = GGToRadian(g_fVR920TrackingRoll);
				bool bTracking = SetupGetTracking ( &fYaw, &fPitch, &fRoll, g_fVR920Sensitivity );
				g_fVR920TrackingYaw = GGToDegree(fYaw);
				g_fVR920TrackingPitch = GGToDegree(fPitch);
				g_fVR920TrackingRoll = GGToDegree(fRoll);
				
				// Reorient View vectors for roll, pitch and yaw.
				if( bTracking == true ) 
				{
					// add compensation to angles for view calc
					fYaw += GGToRadian(g_fDriverCompensationYaw);
					fPitch += GGToRadian(g_fDriverCompensationPitch);
					fRoll += GGToRadian(g_fDriverCompensationRoll);

					KMaths::Matrix matOrient;
					// Add Yaw angle.  (About the up vector)
					// Compute the new viewvector.
					// Transform the right vector.
					GGMatrixRotationAxis( &matOrient, &m_ptr->vecUp, -fYaw );
					GGVec3TransformCoord( &m_ptr->vecLook, &m_ptr->vecLook, &matOrient );
					GGVec3TransformCoord( &m_ptr->vecRight, &m_ptr->vecRight, &matOrient );
					// Add pitch angle.  (About the right vector)
					// Compute the new upvector.
					// Transform the View vector.
					GGMatrixRotationAxis( &matOrient, &m_ptr->vecRight, -fPitch );
					GGVec3TransformCoord( &m_ptr->vecUp, &m_ptr->vecUp, &matOrient );
					GGVec3TransformCoord( &m_ptr->vecLook, &m_ptr->vecLook, &matOrient );
					// Add roll angle.   (About the view vector)
					// Compute the new UpVector.
					// Transform the Right vector.
					GGMatrixRotationAxis( &matOrient, &m_ptr->vecLook, -fRoll );
					GGVec3TransformCoord( &m_ptr->vecUp, &m_ptr->vecUp, &matOrient );
					GGVec3TransformCoord( &m_ptr->vecRight, &m_ptr->vecRight, &matOrient );
				}

				// eye separation
				if ( g_StereoEyeToggle == 0 )
				{
					vecPos -= m_ptr->vecRight;
				}
				else
				{
					vecPos += m_ptr->vecRight;
				}
			}
		}

		// store the new values in the view matrix
		m_ptr->matView._11 = m_ptr->vecRight.x; 
		m_ptr->matView._12 = m_ptr->vecUp.x; 
		m_ptr->matView._13 = m_ptr->vecLook.x;
		m_ptr->matView._21 = m_ptr->vecRight.y; 
		m_ptr->matView._22 = m_ptr->vecUp.y; 
		m_ptr->matView._23 = m_ptr->vecLook.y;
		m_ptr->matView._31 = m_ptr->vecRight.z;
		m_ptr->matView._32 = m_ptr->vecUp.z; 
		m_ptr->matView._33 = m_ptr->vecLook.z;
		m_ptr->matView._41 =- GGVec3Dot ( &vecPos, &m_ptr->vecRight );
		m_ptr->matView._42 =- GGVec3Dot ( &vecPos, &m_ptr->vecUp    );
		m_ptr->matView._43 =- GGVec3Dot ( &vecPos, &m_ptr->vecLook  );
	}
	else
	{
		// FREE FLIGHT ROTATION HANDLING
		matRot = m_ptr->matFreeFlightRotate;
		GGMatrixInverse(&matRot, NULL, &matRot);

		// rotation recreated each time for accuracy
		m_ptr->vecRight.x=1;
		m_ptr->vecRight.y=0;
		m_ptr->vecRight.z=0;
		m_ptr->vecUp.x=0;
		m_ptr->vecUp.y=1;
		m_ptr->vecUp.z=0;
		m_ptr->vecLook.x=0;
		m_ptr->vecLook.y=0;
		m_ptr->vecLook.z=1;
		GGVec3TransformCoord ( &m_ptr->vecRight,	&m_ptr->vecRight,	&matRot );
		GGVec3TransformCoord ( &m_ptr->vecUp,		&m_ptr->vecUp,		&matRot );
		GGVec3TransformCoord ( &m_ptr->vecLook,	&m_ptr->vecLook,	&matRot );

		// inverse matrix to get the angles
		GGVECTOR3 vecRot;
		CameraAnglesFromMatrix ( &matRot, &vecRot );
		m_ptr->fXRotate=vecRot.x;
		m_ptr->fYRotate=vecRot.y;
		m_ptr->fZRotate=vecRot.z;

		// Use new current matrix
		m_ptr->matView = m_ptr->matFreeFlightRotate;

		// Apply position of camera to view
		GGMATRIX matTemp;
		GGMatrixTranslation(&matTemp, -m_ptr->vecPosition.x,-m_ptr->vecPosition.y,-m_ptr->vecPosition.z);
		GGMatrixMultiply(&m_ptr->matView, &matTemp, &m_ptr->matView);
	}
	
	#ifdef WICKEDENGINE
	// for camera zero (player camera) we pass position and angle to main system for wicked rendering
	if (iID == 0)
	{
		camerahook_domydemostuff(m_ptr->vecPosition.x, m_ptr->vecPosition.y, m_ptr->vecPosition.z, m_ptr->fXRotate, m_ptr->fYRotate, m_ptr->fZRotate);
		float fX, fY, fWidth, fHeight;
		camerahook_domydemostuff2(&fX, &fY, &fWidth, &fHeight);
		m_ptr->viewPort3D.X = (DWORD)fX;
		m_ptr->viewPort3D.Y = (DWORD)fY;
		m_ptr->viewPort3D.Width = (DWORD)fWidth;
		m_ptr->viewPort3D.Height = (DWORD)fHeight;
	}
	#else
	// finally apply our new matrix to the view
	CameraInternalUpdateViewAndProj ( &m_ptr->matView, &m_ptr->matProjection );
	#endif
}

DARKSDK int CameraExist ( int iID )
{
	if ( UpdateCameraPtr ( iID ) )
		return true;
	else
		return false;
}

DARKSDK float CameraPositionX ( int iID )
{
	if ( !UpdateCameraPtr ( iID ) )
		return 0.0;

	return m_ptr->vecPosition.x;
}

DARKSDK float CameraPositionY ( int iID )
{
	if ( !UpdateCameraPtr ( iID ) )
		return 0.0;

	return m_ptr->vecPosition.y;
}

DARKSDK float CameraPositionZ ( int iID )
{
	if ( !UpdateCameraPtr ( iID ) )
		return 0.0;

	return m_ptr->vecPosition.z;
}

DARKSDK void PositionCamera ( float fX, float fY, float fZ )
{
	PositionCamera ( m_iCurrentCamera, fX, fY, fZ );
}

DARKSDK void RotateCamera ( float fX, float fY, float fZ )
{
	RotateCamera ( m_iCurrentCamera, fX, fY, fZ );
}

DARKSDK void XRotateCamera ( float fX )
{
	XRotateCamera ( m_iCurrentCamera, fX );
}

DARKSDK void YRotateCamera ( float fY )
{
	YRotateCamera ( m_iCurrentCamera, fY );
}

DARKSDK void ZRotateCamera ( float fZ )
{
	ZRotateCamera ( m_iCurrentCamera, fZ );
}

DARKSDK void PointCamera ( float fX, float fY, float fZ )
{
	PointCamera ( m_iCurrentCamera, fX, fY, fZ );
}

DARKSDK void MoveCamera ( float fStep )
{
	MoveCamera ( m_iCurrentCamera, fStep );
}

DARKSDK void SetCameraRange ( float fFront, float fBack )
{
	SetCameraRange ( m_iCurrentCamera, fFront, fBack );
}

DARKSDK void SetCameraView ( int iLeft, int iTop, int iRight, int iBottom )
{
	SetCameraView ( m_iCurrentCamera, iLeft, iTop, iRight, iBottom );
}

DARKSDK void ClearCameraView ( DWORD dwColorValue )
{
	int iRed = (dwColorValue >> 16) & 255;
	int iGreen = (dwColorValue >> 8) & 255;
	int iBlue = dwColorValue & 255;
	ClearCameraView ( m_iCurrentCamera, iRed, iGreen, iBlue );
}

DARKSDK void ClearCameraViewEx ( int iID, DWORD dwColorValue )
{
	int iRed = (dwColorValue >> 16) & 255;
	int iGreen = (dwColorValue >> 8) & 255;
	int iBlue = dwColorValue & 255;
	ClearCameraView ( iID, iRed, iGreen, iBlue );
}

DARKSDK void SetCameraRotationXYZ ( void )
{
	SetCameraRotationXYZ ( 0 );
}

DARKSDK void SetCameraRotationZYX ( void )
{
	SetCameraRotationZYX ( 0 );
}

DARKSDK void SetCameraFOV ( float fAngle )
{
	SetCameraFOV ( m_iCurrentCamera, fAngle );
}

DARKSDK void SetCameraAspect ( float fAspect )
{
	SetCameraAspect ( m_iCurrentCamera, fAspect );
}

DARKSDK void CameraFollow ( float fX, float fY, float fZ, float fAngle, float fDistance, float fHeight, float fSmooth, int iCollision )
{
	CameraFollow ( m_iCurrentCamera, fX, fY, fZ, fAngle, fDistance, fHeight, fSmooth, iCollision );
}

DARKSDK void SetAutoCamOn ( void )
{
	m_bAutoCamState = true;
}

DARKSDK void SetAutoCamOff ( void )
{
	m_bAutoCamState = false;
}

DARKSDK void TurnCameraLeft ( float fAngle )
{
	TurnCameraLeft ( m_iCurrentCamera, fAngle );
}

DARKSDK void TurnCameraRight ( float fAngle )
{
	TurnCameraRight ( m_iCurrentCamera, fAngle );
}

DARKSDK void PitchCameraUp ( float fAngle )
{
	PitchCameraUp ( m_iCurrentCamera, fAngle );
}

DARKSDK void PitchCameraDown ( float fAngle )
{
	PitchCameraDown ( m_iCurrentCamera, fAngle );
}

DARKSDK void RollCameraLeft ( float fAngle )
{
	RollCameraLeft ( m_iCurrentCamera, fAngle );
}

DARKSDK void RollCameraRight ( float fAngle )
{
	RollCameraRight ( m_iCurrentCamera, fAngle );
}

DARKSDK void SetCameraToObjectOrientation ( int iObjectID )
{
	SetCameraToObjectOrientation ( m_iCurrentCamera, iObjectID );
}

DARKSDK void BackdropOn ( void )
{
	BackdropOn ( m_iCurrentCamera );
}

DARKSDK void BackdropOff ( void )
{
	BackdropOff ( m_iCurrentCamera );
}

DARKSDK void BackdropColor ( DWORD dwColor )
{
	BackdropColor ( m_iCurrentCamera, dwColor );
}

DARKSDK void BackdropTexture ( int iImage )
{
	BackdropTexture ( m_iCurrentCamera, iImage );
}

DARKSDK void BackdropScroll ( int iU, int iV )
{
	BackdropScroll ( m_iCurrentCamera, iU, iV );
}

DARKSDK void		SetCameraDepth					( int iID, int iSourceID )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void		BackdropOff						( int iID, int iNoDepthClear )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void		CopyCameraToImage				( int iID, int iImage )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void SetCamerasToStereoscopic ( int iStereoscopicMode, int iCameraL, int iCameraR, int iBackGrey, int iFrontSideBySide )
{
	// iStereoscopicMode:
	// 1-anaglyph (back)
	// 2-IZ3d (back/front)
	// 3-Philips WOWVX (grey/side-by-side)

	// no action if zero specified
	if ( iStereoscopicMode<=0 )
		return;

	// set-up new render targets to hold back/front surfaces for new functionality of camera image writing
	HRESULT hRes = S_OK;

	// image values must be correct
	if(iBackGrey<1 || iBackGrey>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}
	if(iFrontSideBySide<1 || iFrontSideBySide>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}

	// cameras must have images initially
	// left
	if ( iCameraL < 0 || iCameraL > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iCameraL ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	tagCameraData* m_ptrL = m_ptr;
	if(m_ptrL->pCameraToImageTexture==NULL)
	{
		RunTimeError ( RUNTIMEERROR_IMAGENOTEXIST );
		return;
	}
	// right (not WOW)
	tagCameraData* m_ptrR = NULL;
	if ( iStereoscopicMode == 3 )
	{
		// copy left camera ptr (as right camera ptr used to trigger the event of creating the greyscale/side by side texture)
		m_ptrR = m_ptrL;
	}
	else
	{
		if ( iCameraR < 0 || iCameraR > MAXIMUMVALUE )
		{
			RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
			return;
		}
		if ( !UpdateCameraPtr ( iCameraR ) )
		{
			RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
			return;
		}
		m_ptrR = m_ptr;
		if(m_ptrR->pCameraToImageTexture==NULL)
		{
			RunTimeError ( RUNTIMEERROR_IMAGENOTEXIST );
			return;
		}
	}

	// delete images
	if ( ImageExist ( iBackGrey )) DeleteImage ( iBackGrey );
	if ( ImageExist ( iFrontSideBySide )) DeleteImage ( iFrontSideBySide );

	// set the second camera (right camera) to trigger the generation of the stereoscopic images
	m_ptrL->iStereoscopicMode = 0;
	m_ptrL->pStereoscopicFirstCamera = NULL;
	m_ptrR->iStereoscopicMode = iStereoscopicMode;
	m_ptrR->pStereoscopicFirstCamera = m_ptrL;

	// Set camera to render to stereoscopic image from regular camera image
	m_ptrR->iCameraToStereoImageBack = iBackGrey;
	m_ptrR->iCameraToStereoImageFront = iFrontSideBySide;

	#ifdef DX11
	#else
	// get width and height from existing camera image surface
	D3DSURFACE_DESC surfacedesc;
	m_ptrR->pCameraToImageSurface->GetDesc(&surfacedesc);
	GGFORMAT CommonFormat = surfacedesc.Format;
	int iWidth = surfacedesc.Width;
	int iHeight = surfacedesc.Height;

	// make render target an internal texture
	m_ptrR->pCameraToStereoImageBackTexture = MakeImageRenderTarget ( iBackGrey, iWidth, iHeight, CommonFormat );
	if ( m_ptrR->pCameraToStereoImageBackTexture==NULL )
	{
		m_ptrR->iCameraToStereoImageBack = 0;
		m_ptrR->iCameraToStereoImageFront = 0;
		return;
	}
	m_ptrR->pCameraToStereoImageFrontTexture = MakeImageRenderTarget ( iFrontSideBySide, iWidth, iHeight, CommonFormat );
	if ( m_ptrR->pCameraToStereoImageFrontTexture==NULL )
	{
		m_ptrR->iCameraToStereoImageBack = 0;
		m_ptrR->iCameraToStereoImageFront = 0;
		return;
	}

	// Open image surface for continual usage
	m_ptrR->pCameraToStereoImageBackTexture->GetSurfaceLevel( 0, &m_ptrR->pCameraToStereoImageBackSurface );
	if ( m_ptrR->pCameraToStereoImageBackSurface==NULL )
	{
		SAFE_RELEASE ( m_ptrR->pCameraToStereoImageBackTexture );
		SAFE_RELEASE ( m_ptrR->pCameraToStereoImageFrontTexture );
		return;
	}
	m_ptrR->pCameraToStereoImageFrontTexture->GetSurfaceLevel( 0, &m_ptrR->pCameraToStereoImageFrontSurface );
	if ( m_ptrR->pCameraToStereoImageFrontSurface==NULL )
	{
		SAFE_RELEASE ( m_ptrR->pCameraToStereoImageBackTexture );
		SAFE_RELEASE ( m_ptrR->pCameraToStereoImageFrontTexture );
		return;
	}
	#endif

	// success
	return;
}


//
// DBV1 Expression Functions
//

DARKSDK float CameraPositionX ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetXPositionEx ( 0 );
	return CameraPositionX ( m_iCurrentCamera );
}

DARKSDK float CameraPositionY ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetYPositionEx ( 0 );
	return CameraPositionY ( m_iCurrentCamera );
}

DARKSDK float CameraPositionZ ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetZPositionEx ( 0 );
	return CameraPositionZ ( m_iCurrentCamera );
}

DARKSDK float CameraAngleX ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetXAngleEx ( 0 );
	return CameraAngleX ( m_iCurrentCamera );
}

DARKSDK float CameraAngleY ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetYAngleEx ( 0 );
	return CameraAngleY ( m_iCurrentCamera );
}

DARKSDK float CameraAngleZ ( void )
{
	// leefix - 020308 - should use current camera value
	// return GetZAngleEx ( 0 );
	return CameraAngleZ ( m_iCurrentCamera );
}

DARKSDK void CreateCamera ( int iID )
{
	// internal camera data
	tagCameraData	cData;
	cData.pCameraToImageTexture=NULL;

	// created internally so much be released internally (unlike above which was done with make format)
	cData.pCameraToImageAlphaTexture=NULL;

	// add data to list
	m_CameraManager.Add ( &cData, iID );

	// setup default values
	ResetCamera ( iID );
}

DARKSDK void CreateCameraEx ( int iID )
{
	// create a camera
	if ( iID==0 )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOZERO );
		return;
	}
	if ( UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERAALREADYEXISTS );
		return;
	}

	CreateCamera ( iID );
}

DARKSDK void DestroyCamera ( int iID )
{
	// update internal pointer
	if ( !UpdateCameraPtr ( iID ) )
		return;

	// free image surface from camera first
	FreeCameraSurfaces();

	// attempt to delete the camera from the list
	m_CameraManager.Delete ( iID );
}

DARKSDK void DeleteCamera ( int iID )
{
	// destroy a camera
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( iID==0 )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOZERO );
		return;
	}

	DestroyCamera ( iID );
}

DARKSDK void ResetCamera ( int iID )
{
	// variable declarations
	GGVIEWPORT viewport;

	// use camera zero as the only view port to collect initial view
	if ( iID==0 )
	{
		// from device
		memset ( &viewport, 0, sizeof ( viewport ) );
		#ifdef DX11
		#else
		GGVIEWPORT d3dviewport;
		m_pD3D->GetViewport ( &d3dviewport );
		memcpy ( &viewport, &d3dviewport, sizeof ( d3dviewport ) );
		#endif
	}
	else
	{
		// update internal pointer to camera zero
		if ( !UpdateCameraPtr ( 0 ) )
			return;

		// if non-zero camera, from camera zero we take the viewport defaults
		memcpy ( &viewport, &m_ptr->viewPort3D, sizeof ( m_ptr->viewPort3D ) );
	}

	// update internal pointer
	if ( !UpdateCameraPtr ( iID ) )
		return;

	// defaults
	int iWidth = viewport.Width;
	int iHeight = viewport.Height;
	float fMinZ = viewport.MinZ;
	float fMaxZ = viewport.MaxZ;
	if(g_pGlob)
	{
		// From actual surface desc
		iWidth = g_pGlob->iScreenWidth;
		iHeight = g_pGlob->iScreenHeight;
		fMinZ = 0.0f;
		fMaxZ = 1.0f;
	}

	// fill view port2D and copy to view port3D
	m_ptr->viewPort2D.X = 0;
	m_ptr->viewPort2D.Y = 0;
	m_ptr->viewPort2D.Width = iWidth;
	m_ptr->viewPort2D.Height = iHeight;
	m_ptr->viewPort2D.MinZ = fMinZ;
	m_ptr->viewPort2D.MaxZ = fMaxZ;
	memcpy(&m_ptr->viewPort3D, &m_ptr->viewPort2D, sizeof ( m_ptr->viewPort2D ) );

	// Default FOV
	m_ptr->fFOV = GGToDegree(3.14159265358979323846f/2.905f);

	// work out values for perspective, fov, clipping and positions
	m_ptr->fAspect = ( float ) viewport.Width / ( float ) viewport.Height;
	if ( m_ptr->fAspectMod != 0.0f ) m_ptr->fAspect = m_ptr->fAspectMod;

	SetCameraRange       ( iID, m_ptr->fZNear, m_ptr->fZFar );				// default range
	SetCameraView        ( iID, (int)0, 0, (int)iWidth, (int)iHeight );		// default view
	PositionCamera       ( iID, 0.0, 0.0, 0.0 );							// default position

	// Default rotation and rotation-order
	m_ptr->bUseFreeFlightRotation = false;
	m_ptr->bRotate = true;

	// Default non zero camera has green backdrop to help users
	if(iID>0)
	{
		m_ptr->iBackdropState = 1;
		m_ptr->dwBackdropColor = GGCOLOR_XRGB(0,128,0);
	}

	// set the glob struct camera ID
	if ( g_pGlob ) g_pGlob->dwCurrentSetCameraID = iID;

	// additional resets
	m_ptr->bOverride = false;
	m_ptr->iClipPlaneOn = 0;
	memset ( m_ptr->planeClip, 0, sizeof(m_ptr->planeClip) );
}

DARKSDK void SetCurrentCamera ( int iID )
{
	// used to set the current camera, this needs to be called when you
	// end up using multiple camers
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if(iID>0)
	{
		if ( !UpdateCameraPtr ( iID ) )
		{
			RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
			return;
		}
	}

	// Set the current camera
	m_iCurrentCamera = iID;

	// set the glob struct camera ID
	if ( g_pGlob ) g_pGlob->dwCurrentSetCameraID = iID;
}

// Allows for dynamic resolution switching in game
DARKSDK void SetCameraHiRes ( bool bHiRes )
{
	#ifdef DX11
	#else
	// Store camera ID incase it is not 0
	int iOldCam = m_iCurrentCamera;

	if ( !UpdateCameraPtr ( 0 ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	
	// Hardcoded for now, need to get the number from g.postprocessobjectoffset+0 ( which is currently set to 150001 )
	sObject* pObj = GetObjectData ( 150001 );

	if ( !pObj ) return;	

	if ( bHiRes ) 
	{
		m_ptr->pCameraToImageSurface = m_ptr->pCameraToImageSurfaceHiRes;
		pObj->ppMeshList[0]->pTextures [ 0 ].pTexturesRef = GetImagePointer ( pObj->ppMeshList[0]->pTextures [ 0 ].iImageID );
		pObj->ppMeshList[0]->pTextures [ 0 ].pTexturesRefView = GetImagePointerView ( pObj->ppMeshList[0]->pTextures [ 0 ].iImageID );
	}
	else
	{
		m_ptr->pCameraToImageSurface = m_ptr->pCameraToImageSurfaceLowRes;
		pObj->ppMeshList[0]->pTextures [ 0 ].pTexturesRef = m_ptr->pCameraToImageTextureLowRes;
		pObj->ppMeshList[0]->pTextures [ 0 ].pTexturesRefView = NULL;//m_ptr->pCameraToImageTextureLowRes;
	}

	// Restore camera ID
	UpdateCameraPtr ( iOldCam );
	#endif
}

DARKSDK void SetCameraToImage ( int iID, int iImage, int iWidth, int iHeight, int iGenerateCameraAlpha, DWORD dwOwnD3DFMTValue )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// camera to image modes:
	// -1 = detatch camera from image cleanly
	// -2 = detatch camera from image, but prepare it to accept surfaceview overrides (from WMR backbuffer views)
	if ( iImage==-1 || iImage==-2 )
	{
		// no need to free if -2 as references elsehwere (WMR backbuffer)
		if ( m_ptr->iCameraToImage == -2 )
		{
			m_ptr->pImageDepthSurfaceView = NULL;
			m_ptr->pCameraToImageSurfaceView = NULL;
		}
		else
		{
			assert( 0 && "DX11 not in use" );
			/*
			// free internal texture used
			if(m_ptr->pImageDepthResourceView)
			{
				m_ptr->pImageDepthResourceView->Release();
				m_ptr->pImageDepthResourceView = NULL;
			}
			if(m_ptr->pImageDepthSurfaceView)
			{
				m_ptr->pImageDepthSurfaceView->Release();
				m_ptr->pImageDepthSurfaceView = NULL;
			}
			if(m_ptr->pImageDepthSurface)
			{
				m_ptr->pImageDepthSurface->Release();
				m_ptr->pImageDepthSurface = NULL;
			}
			if(m_ptr->pCameraToImageSurfaceView)
			{
				// release this surface before delete underlying texture
				m_ptr->pCameraToImageSurfaceView->Release();
				m_ptr->pCameraToImageSurfaceView = NULL;
			}
			if(m_ptr->pCameraToImageSurface)
			{
				// release this surface before delete underlying texture
				m_ptr->pCameraToImageSurface->Release();
				m_ptr->pCameraToImageSurface = NULL;
			}
			if(m_ptr->pCameraToImageTexture)
			{
				// 070714 - free image ptr if about to release
				bool bReleased = false;
				if ( m_ptr->iCameraToImage > 0 )
				{
					if ( ImageExist ( m_ptr->iCameraToImage ) ) 
					{
						DeleteImage ( m_ptr->iCameraToImage );
						bReleased = true;
					}
				}
				if ( bReleased==false )
				{
					m_ptr->pCameraToImageTexture->Release(); 
				}
				m_ptr->pCameraToImageTexture=NULL;
			}

			// free any alpha texture created
			if(m_ptr->pCameraToImageAlphaTexture)
			{
				//m_ptr->pCameraToImageAlphaTexture->Release();
				m_ptr->pCameraToImageAlphaTexture=NULL;
			}
			*/
		}

		// special camera to image modes
		if ( iImage == -1 )
		{
			// switch back to camera rendering
			m_ptr->iCameraToImage = 0;

			// done
			return;
		}
		if ( iImage == -2 )
		{
			// going to use this camera by overriding the surfaceviews for WMR rendering
			m_ptr->iCameraToImage = -2;
		}
	}

	// valid image value (allows -2 see above)
	if ( iImage != -2 )
	{
		if ( iImage<1 || iImage>MAXIMUMVALUE )
		{
			RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
			return;
		}
	}
	
	// only if camera not assigned to image
	if ( m_ptr->pCameraToImageTexture==NULL )
	{
		if ( iImage > 0 && g_pGlob->pHoldBackBufferPtr )
		{
			// delete image
			if ( ImageExist ( iImage )) DeleteImage ( iImage );

			// Set camera to render to image (texture)
			m_ptr->iCameraToImage = iImage;

			assert( 0 && "DX11 not in use" );
			/*
			#ifdef DX11
			// use backbuffer to determine common GGFORMAT
			D3D11_RENDER_TARGET_VIEW_DESC backbufferdesc;
			g_pGlob->pHoldBackBufferPtr->GetDesc(&backbufferdesc);
			GGFORMAT CommonFormat = backbufferdesc.Format;

			// Alpha Camera Image or not
			if ( iGenerateCameraAlpha==1 )
			{
				// make render target an internal texture
				D3D11_TEXTURE2D_DESC StagedDesc = { iWidth, iHeight, 1, 1, CommonFormat, 1, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
				HRESULT hRes = m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&m_ptr->pCameraToImageTexture );
			}
			else
			{
				// Just want a camera image that retains the alpha data (for things like post process depth of field)
				if ( iGenerateCameraAlpha==2 )
				{
					// lee - 060207 - fix - allow camera alpha mode 2 to be cleared
					GGFORMAT dwStoreFormat = CommonFormat;
					CommonFormat=GGFMT_A8R8G8B8;
					m_ptr->pCameraToImageTexture = MakeImageRenderTarget ( iImage, iWidth, iHeight, CommonFormat );
					CommonFormat=dwStoreFormat;
				}
				else
				{
					// can now choose own D3DFMT value from extra param
					if ( iGenerateCameraAlpha==3 )
					{
						GGFORMAT dwStoreFormat = CommonFormat;
						CommonFormat=(GGFORMAT)dwOwnD3DFMTValue;
						m_ptr->pCameraToImageTexture = MakeImageRenderTarget ( iImage, iWidth, iHeight, CommonFormat );
						CommonFormat=dwStoreFormat;
					}
					else
					{
						// no alpha so use render target as direct texture
						m_ptr->pCameraToImageTexture = MakeImageRenderTarget ( iImage, iWidth, iHeight, CommonFormat );
					}
				}
			}

			// Open image surface for continual usage
			///m_ptr->pCameraToImageTexture->GetSurfaceLevel( 0, &m_ptr->pCameraToImageSurface );
			m_ptr->pCameraToImageSurface = (ID3D11Texture2D*)m_ptr->pCameraToImageTexture;
			m_pD3D->CreateRenderTargetView( m_ptr->pCameraToImageTexture, NULL, &m_ptr->pCameraToImageSurfaceView );

			// add alpha surface
			if ( iGenerateCameraAlpha==1 )
			{
				// get surface to texture alpha deposit area
				m_ptr->pCameraToImageAlphaTexture = MakeImageJustFormat ( iImage, iWidth, iHeight, GGFMT_A8R8G8B8 );
				///m_ptr->pCameraToImageAlphaTexture->GetSurfaceLevel( 0, &m_ptr->pCameraToImageAlphaSurface );
				m_ptr->pCameraToImageAlphaSurface = (ID3D11Texture2D*)m_ptr->pCameraToImageAlphaTexture;
			}

			// Work out size for depth buffer
			GGSURFACE_DESC imagerenderdesc;
			m_ptr->pCameraToImageSurface->GetDesc(&imagerenderdesc);
			DWORD dwDepthWidth = imagerenderdesc.Width;
			DWORD dwDepthHeight = imagerenderdesc.Height;

			// delete old image depth buffer if too small for this one
			if( m_ptr->pImageDepthSurface && (dwDepthWidth>m_dwImageDepthWidth || dwDepthHeight>m_dwImageDepthHeight) )
			{
				// delete any items in the list
				if ( m_ptr->pImageDepthResourceView )
				{
					m_ptr->pImageDepthResourceView->Release();
					m_ptr->pImageDepthResourceView=NULL;
				}
				if ( m_ptr->pImageDepthSurfaceView )
				{
					m_ptr->pImageDepthSurfaceView->Release();
					m_ptr->pImageDepthSurfaceView=NULL;
				}
				if ( m_ptr->pImageDepthSurface )
				{
					m_ptr->pImageDepthSurface->Release();
					m_ptr->pImageDepthSurface=NULL;
				}
			}

			// create image depth surface
			if ( m_ptr->pImageDepthSurface == NULL )
			{
				// Create depth stencil texture
				m_dwImageDepthWidth = dwDepthWidth;
				m_dwImageDepthHeight = dwDepthHeight;
				D3D11_TEXTURE2D_DESC descDepth;
				ZeroMemory( &descDepth, sizeof(descDepth) );
				descDepth.Width = dwDepthWidth;
				descDepth.Height = dwDepthHeight;
				descDepth.MipLevels = 1;
				descDepth.ArraySize = 1;
				descDepth.Format = DXGI_FORMAT_R32_TYPELESS;//GetValidStencilBufferFormat(CommonFormat);//DXGI_FORMAT_D24_UNORM_S8_UINT;
				descDepth.SampleDesc.Count = 1;
				descDepth.SampleDesc.Quality = 0;    
				descDepth.Usage = D3D11_USAGE_DEFAULT;
				descDepth.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
				descDepth.CPUAccessFlags = 0;
				descDepth.MiscFlags = 0;
				HRESULT hRes = m_pD3D->CreateTexture2D( &descDepth, NULL, &m_ptr->pImageDepthSurface );

				// Create the depth stencil view
				D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
				ZeroMemory( &descDSV, sizeof(descDSV) );
				descDSV.Format = DXGI_FORMAT_D32_FLOAT;//descDepth.Format;//DXGI_FORMAT_D24_UNORM_S8_UINT;
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				descDSV.Texture2D.MipSlice = 0;
				hRes = m_pD3D->CreateDepthStencilView( m_ptr->pImageDepthSurface, &descDSV, &m_ptr->pImageDepthSurfaceView );

				D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
				ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
				shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				shaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;//DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				shaderResourceViewDesc.Texture2D.MipLevels = 1;
				hRes = m_pD3D->CreateShaderResourceView ( m_ptr->pImageDepthSurface, &shaderResourceViewDesc, &m_ptr->pImageDepthResourceView );
			}
			#endif
			*/
		}

		// Set View to entire image surface
		SetCameraView ( iID, 0, 0, iWidth, iHeight );
	}
}

DARKSDK void SetCameraToImage ( int iID, int iImage, int iWidth, int iHeight, int iGenerateCameraAlpha )
{
	// Regular camera image alpha specified
	SetCameraToImage ( iID, iImage, iWidth, iHeight, iGenerateCameraAlpha, 0 );
}

DARKSDK void SetCameraToImage ( int iID, int iImage, int iWidth, int iHeight )
{
	// Regular camera image no alpha
	SetCameraToImage ( iID, iImage, iWidth, iHeight, 0, 0 );
}

DARKSDK void SetCameraToView ( int iID, void* pRenderTargetView, void* pDepthStencilView, DWORD dwWidth, DWORD dwHeight, void* pLeftShaderResourceView )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	SetCameraToImage ( iID, -2, dwWidth, dwHeight, 0, 0 ); // -2 mode will strip any image, but get camera ready to carry render target below
	m_ptr->pCameraToImageSurfaceView = pRenderTargetView;
	m_ptr->pImageDepthSurfaceView = pDepthStencilView;
	m_ptr->pImageDepthResourceView = pLeftShaderResourceView;
}

DARKSDK void MoveCameraUp ( int iID, float fStep )
{
	// move the camera up
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move upwards, up is defined from the up vector so it will move
	// up dependant on which way is up for the camera
	m_ptr->vecPosition += ( fStep * m_ptr->vecUp );

	// put this back in and comment out the last line if you want
	// to have up always the same direction
	// m_ptr->vecPosition.y += fStep;

	// apply the change
	CameraInternalUpdate ( iID );
}

DARKSDK void MoveCameraDown ( int iID, float fStep )
{
	// move the camera down
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move downwards, down is defined from the down vector so it will move
	// down dependant on which way is down for the camera
	m_ptr->vecPosition -= ( fStep * m_ptr->vecUp );

	// put this back in and comment out the last line if you want
	// to have down always the same direction
	// m_ptr->vecPosition.y -= fStep;

	// apply the change
	CameraInternalUpdate ( iID );
}

DARKSDK void MoveCameraLeft ( int iID, float fStep )
{
	// move the camera left
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move left, left is defined from the right vector so it will move
	// left dependant on which way is left for the camera
	m_ptr->vecPosition -= ( fStep * m_ptr->vecRight );

	// put this back in and comment out the last line if you want
	// to have left always the same direction
	// m_ptr->vecPosition -= fStep * m_ptr->vecRight;

	// update internal data
	CameraInternalUpdate ( iID );
}

DARKSDK void MoveCameraRight ( int iID, float fStep )
{
	// move the camera right
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move right, right is defined from the right vector so it will move
	// right dependant on which way is right for the camera
	m_ptr->vecPosition += ( fStep * m_ptr->vecRight );

	// update internal data
	CameraInternalUpdate ( iID );
}

DARKSDK void ControlWithArrowKeys ( int iID, float fVelocity, float fTurnSpeed )
{
	// move thewith the arrow keys
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move camera based on direct arrow presses
	if ( GetAsyncKeyState ( VK_UP ) ) m_ptr->vecPosition += ( fVelocity * m_ptr->vecLook );
	if ( GetAsyncKeyState ( VK_DOWN ) ) m_ptr->vecPosition -= ( fVelocity * m_ptr->vecLook );
	if ( GetAsyncKeyState ( VK_LEFT ) ) m_ptr->fYRotate -= fTurnSpeed;
	if ( GetAsyncKeyState ( VK_RIGHT ) ) m_ptr->fYRotate += fTurnSpeed;

	// ensure camera is simplified
	m_ptr->bUseFreeFlightRotation=false;
	m_ptr->fXRotate = 0.0f;
	m_ptr->fZRotate = 0.0f;

	// update internal data
	CameraInternalUpdate ( iID );
}

DARKSDK void SetCameraPositionVector3 ( int iID, int iVector )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( !VectorExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	m_ptr->vecPosition = GetVector3 ( iVector );

	// apply the change
	CameraInternalUpdate ( iID );
}

DARKSDK void GetCameraPositionVector3 ( int iVector, int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( !VectorExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	SetVector3 ( iVector, m_ptr->vecPosition.x, m_ptr->vecPosition.y, m_ptr->vecPosition.z );
}

DARKSDK void SetCameraRotationVector3 ( int iID, int iVector )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( !VectorExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	GGVECTOR3 vec = GetVector3 ( iVector );
	m_ptr->fXRotate = vec.x;
	m_ptr->fYRotate = vec.y;
	m_ptr->fZRotate = vec.z;

	// apply the change
	CameraInternalUpdate ( iID );
}

DARKSDK void GetCameraRotationVector3 ( int iVector, int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( !VectorExist ( iVector ) )
	{
		RunTimeError ( RUNTIMEERROR_VECTORNOTEXIST );
		return;
	}
	SetVector3 ( iVector, m_ptr->fXRotate, m_ptr->fYRotate, m_ptr->fZRotate );
}

DARKSDK void PositionCamera ( int iID, float fX, float fY, float fZ )
{
	// sets the position of the camera
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// now set the position
	m_ptr->vecPosition = GGVECTOR3 ( fX, fY, fZ );

	// apply the change
	CameraInternalUpdate ( iID );
}

DARKSDK void RotateCamera ( int iID, float fX, float fY, float fZ )
{
	// rotate the camera around all 3 axes
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Rotate forcee euler rotation
	m_ptr->bUseFreeFlightRotation=false;
	m_ptr->fXRotate = fX;
	m_ptr->fYRotate = fY;
	m_ptr->fZRotate = fZ;
	
	// update the camera
	CameraInternalUpdate ( iID );
}

DARKSDK void XRotateCamera ( int iID, float fX )
{
	// rotate the camera on it's x axis
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Rotate forcee euler rotation
	m_ptr->bUseFreeFlightRotation=false;
	m_ptr->fXRotate = fX;

	// update the camera
	CameraInternalUpdate ( iID );
}

DARKSDK void YRotateCamera ( int iID, float fY )
{
	// rotate the camera on it's y axis
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Rotate forcee euler rotation
	m_ptr->bUseFreeFlightRotation=false;
	m_ptr->fYRotate = fY;

	// update the camera
	CameraInternalUpdate ( iID );
}

DARKSDK void ZRotateCamera ( int iID, float fZ )
{
	// rotate the camera on it's z axis
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Rotate forcee euler rotation
	m_ptr->bUseFreeFlightRotation=false;
	m_ptr->fZRotate = fZ;

	// update the camera
	CameraInternalUpdate ( iID );
}

DARKSDK void PointCamera ( int iID, float fX, float fY, float fZ )
{
	// point the camera in the location
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// rotation from xyz diff
	CameraGetAngleFromPoint ( m_ptr->vecPosition.x, m_ptr->vecPosition.y, m_ptr->vecPosition.z,
						fX, fY, fZ, &m_ptr->fXRotate, &m_ptr->fYRotate, &m_ptr->fZRotate);

	// mike - 011005 - change to using euler rotation
	m_ptr->bUseFreeFlightRotation = false;

	// update internal camera data to reflect changes
	CameraInternalUpdate ( iID );
}

DARKSDK void MoveCamera ( int iID, float fStep )
{
	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// move the camera forward
	m_ptr->vecPosition += ( fStep * m_ptr->vecLook );

	// update internal data
	CameraInternalUpdate ( iID );
}

DARKSDK void SetCameraRange ( int iID, float fFront, float fBack)
{
	// sets the near and far z ranges for the camera
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// save properties
	m_ptr->fZNear = fFront;	// front
	m_ptr->fZFar  = fBack;	// far

	// setup the perspective matrix
	GGMatrixPerspectiveFovLH ( &m_ptr->matProjection, GGToRadian(m_ptr->fFOV), m_ptr->fAspect, m_ptr->fZNear, m_ptr->fZFar );
	// and set the projection transform if current camera
	if(iID==m_iCurrentCamera)
	{
		GGSetTransform ( GGTS_PROJECTION, &m_ptr->matProjection );
	}
}

DARKSDK void SetCameraView ( int iID, int iLeft, int iTop, int iRight, int iBottom )
{
	// set up the viewing area for the 3D scene
	int iWidth = iRight - iLeft;
	int iHeight = iBottom - iTop;

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// clear the current viewport
	memset ( &m_ptr->viewPort3D, 0, sizeof ( m_ptr->viewPort3D ) );
	
	// setup new viewport
	m_ptr->viewPort3D.X      = iLeft;	// top left
	m_ptr->viewPort3D.Y      = iTop;	// top
	m_ptr->viewPort3D.Width  = iWidth;	// far right
	m_ptr->viewPort3D.Height = iHeight;	// and bottom
	m_ptr->viewPort3D.MinZ   = 0.0;		// default as 0.0
	m_ptr->viewPort3D.MaxZ   = 1.0;		// default as 1.0
	
	// if viewport larger than 2D (adjust 2D to match) - double wide buffers
	if ( m_ptr->viewPort3D.Width > m_ptr->viewPort2D.Width )
		m_ptr->viewPort2D.Width = m_ptr->viewPort3D.Width;
}

DARKSDK void ClearCameraView ( int iID, int iRed, int iGreen, int iBlue )
{
	// clears the camera view to the specified colour

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	assert( 0 && "DX11 not in use" );
	/*

	// only clear specified view
	#ifdef DX11
	float color[4] = {(float)iRed/255.0f,(float)iGreen/255.0f,(float)iBlue/255.0f,1};
	m_pImmediateContext->ClearRenderTargetView(g_pGlob->pCurrentRenderView, color);
	m_pImmediateContext->ClearDepthStencilView(g_pGlob->pCurrentDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	#else
	DWORD dwCustomBackCol = GGCOLOR_XRGB(iRed, iGreen, iBlue);
	D3DRECT areatoclear;
	areatoclear.x1 = m_ptr->viewPort3D.X;
	areatoclear.y1 = m_ptr->viewPort3D.Y;
	areatoclear.x2 = m_ptr->viewPort3D.X + m_ptr->viewPort3D.Width;
	areatoclear.y2 = m_ptr->viewPort3D.Y + m_ptr->viewPort3D.Height;
	// clear the screen
	m_pD3D->Clear ( 1, &areatoclear, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwCustomBackCol, 1.0f, 0 );
	#endif
	*/
}

DARKSDK void SetCameraRotationXYZ ( int iID )
{
	// set the rotation mode to x, y and then z

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set normal rotation on
	m_ptr->bRotate = true;
}

DARKSDK void SetCameraRotationZYX ( int iID )
{
	// set the rotation mode to z, y and then x

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set different rotation on
	m_ptr->bRotate = false;
}

DARKSDK void SetCameraFOV ( int iID, float fAngle )
{
	// wicked only uses camera zero
	#ifdef WICKEDENGINE
	if (iID != 0) return;
	#endif

	// set the cameras field of view
	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// store the new angle
	m_ptr->fFOV = fAngle;

	#ifdef WICKEDENGINE
	WickedCall_SetCameraFOV(m_ptr->fFOV);
	#else
	// setup the projection matrix to apply the change
	SetCameraRange ( iID, m_ptr->fZNear, m_ptr->fZFar );
	#endif
}

//Dave 4th August 2014
DARKSDK void SetCameraSwitchBank ( int iID, int flags )
{
	// set the cameras field of view

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// store the new angle
	m_ptr->dwCameraSwitchBank = flags;

}

DARKSDK void SetCameraAspect ( int iID, float fAspect )
{
	// set the cameras field of view

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// store the new angle
	m_ptr->fAspectMod = fAspect;
	m_ptr->fAspect = fAspect;

	// setup the projection matrix to apply the change
	SetCameraRange ( iID, m_ptr->fZNear, m_ptr->fZFar );
}

DARKSDK void SetCameraClip ( int iID, int iOnOff, float fX, float fY, float fZ, float fNX, float fNY, float fNZ )
{
	// leeadd - 310506 - u62 - set camera clip plane

	// validate inputs
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// update if available
	#ifdef DX11
    g_dwMaxUserClipPlanes = 1;
	#else
    GGCAPS d3dCaps;
    m_pD3D->GetDeviceCaps( &d3dCaps );
    g_dwMaxUserClipPlanes = d3dCaps.MaxUserClipPlanes;
	#endif

	// set the clip settings
	if ( g_dwMaxUserClipPlanes>0 )
	{
		m_ptr->iClipPlaneOn = iOnOff;
		GGVECTOR3 vecPos = GGVECTOR3(fX,fY,fZ);
		GGVECTOR3 vecNormal = GGVECTOR3(fNX,fNY,fNZ);
		if ( iOnOff!=0 ) GGPlaneFromPointNormal ( &m_ptr->planeClip, &vecPos, &vecNormal );
	}
	else
		m_ptr->iClipPlaneOn = 0;
}

DARKSDK void CameraFollow ( int iID, float targetx, float targety, float targetz, float targetangle, float camerarange, float cameraheight, float cameraspeed, int usestaticcollision )
{
	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Angle to radians
	targetangle = GGToRadian( targetangle );

	// Create move target (where camera actually goes)
	bool bUntouched=true;
	float movetargetx = targetx - (float)(sin(targetangle)*camerarange);
	float movetargety = targety + cameraheight; // leefix - 200703 - added targety
	float movetargetz = targetz - (float)(cos(targetangle)*camerarange);

	// Calculate distance between camera and move position
	float dx = movetargetx - m_ptr->vecPosition.x;
	float dy = movetargety - m_ptr->vecPosition.y;
	float dz = movetargetz - m_ptr->vecPosition.z;

	// Have camera follow target at speed
	float moveangle = (float)atan2( (double)dx, (double)dz );
	float movedist = (float)sqrt(fabs(dx*dx)+fabs(dz*dz));
	float speed = movedist/cameraspeed;

	float newx, newy, newz;
	newx = m_ptr->vecPosition.x + (float)(sin(moveangle)*speed);
	newy = m_ptr->vecPosition.y + (dy/cameraspeed);
	newz = m_ptr->vecPosition.z + (float)(cos(moveangle)*speed);

	// Calculate look angle
	dx = targetx - newx;
	dz = targetz - newz;
	float lookangle = (float)atan2( (double)dx, (double)dz );

	// Calculate smoothing of camera look angle
	float a = GGToDegree ( lookangle );
	float da = m_ptr->fYRotate;
	float diff = a-da;
	if(diff<-180.0f) diff=(a+360.0f)-da;
	if(diff>180.0f) diff=a-(da+360.0f);
	da=da+(diff/1.5f);

	m_ptr->fYRotate = wrapangleoffset(da);
	m_ptr->vecPosition.x = newx;
	m_ptr->vecPosition.y = newy;
	m_ptr->vecPosition.z = newz;

	// apply changes
	CameraInternalUpdate ( iID );

	if(bUntouched==true)
	{
		lastmovetargetx=movetargetx;
		lastmovetargety=movetargety;
		lastmovetargetz=movetargetz;
	}
}

DARKSDK void CheckRotationConversion ( tagCameraData* m_ptr, bool bUseFreeFlightMode )
{
	// has there been a change?
	if ( bUseFreeFlightMode != m_ptr->bUseFreeFlightRotation )
	{
		// Caluclates equivilant rotation data if switch rotation-modes
		if( bUseFreeFlightMode==true )
		{
			// Euler to Freeflight
			m_ptr->matFreeFlightRotate = m_ptr->matView;
			m_ptr->matFreeFlightRotate._41=0.0f;
			m_ptr->matFreeFlightRotate._42=0.0f;
			m_ptr->matFreeFlightRotate._43=0.0f;
		}
	}

	// always calculate freeflight to euler (for angle feedback)
	if( bUseFreeFlightMode==true )
	{
		// Freeflight to Euler
		GGVECTOR3 vecRotate;
		CameraAnglesFromMatrix ( &m_ptr->matFreeFlightRotate, &vecRotate );
		m_ptr->fXRotate = vecRotate.x;
		m_ptr->fYRotate = vecRotate.y;
		m_ptr->fZRotate = vecRotate.z;
	}

	// new rotation mode
	m_ptr->bUseFreeFlightRotation = bUseFreeFlightMode;
}

DARKSDK void TurnCameraLeft ( int iID, float fAngle )
{
	// turns the camera left

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment (leefix-210703-big change to freeflightcameracommands)
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void TurnCameraRight ( int iID, float fAngle )
{
	// turns the camera right

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void PitchCameraUp ( int iID, float fAngle )
{
	// pitch the camera up

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void PitchCameraDown ( int iID, float fAngle )
{
	// pitch the camera down

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void RollCameraLeft ( int iID, float fAngle )
{
	// roll the camera left

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( -fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void RollCameraRight ( int iID, float fAngle )
{
	// roll the camera right

	// update internal pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// execute possible conversion
	CheckRotationConversion ( m_ptr, true );

	// rotation adjustment
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( fAngle ) );
	m_ptr->matFreeFlightRotate = m_ptr->matFreeFlightRotate * matRotation;
	m_ptr->bUseFreeFlightRotation=true;

	// apply changes
	CameraInternalUpdate ( iID );
}

DARKSDK void SetCameraToObjectOrientation ( int iID, int iObjectID )
{
	// Get Camera Ptr
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// Get Object Ptr
	m_Object_Ptr = GetObjectData ( iObjectID );
	if ( m_Object_Ptr==NULL)
		return;

	// Assign Rotation Details to Camera
	m_ptr->vecLook					= m_Object_Ptr->position.vecLook;
	m_ptr->vecUp					= m_Object_Ptr->position.vecUp;
	m_ptr->vecRight					= m_Object_Ptr->position.vecRight;
	m_ptr->fXRotate					= m_Object_Ptr->position.vecRotate.x;
	m_ptr->fYRotate					= m_Object_Ptr->position.vecRotate.y;
	m_ptr->fZRotate					= m_Object_Ptr->position.vecRotate.z;
	m_ptr->bUseFreeFlightRotation	= m_Object_Ptr->position.bFreeFlightRotation;

	FLOAT fDeterminant;
	GGMatrixInverse ( &m_ptr->matFreeFlightRotate, &fDeterminant, &m_Object_Ptr->position.matFreeFlightRotate );

	if ( m_Object_Ptr->position.dwRotationOrder == ROTORDER_XYZ )
		m_ptr->bRotate					= true;
	else
		m_ptr->bRotate					= false;

	// update camera
	CameraInternalUpdate ( iID );
}

//
// New Expression Functions
//

DARKSDK float CameraPositionXEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.x;
}

DARKSDK float CameraPositionYEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.y;
}

DARKSDK float CameraPositionZEx ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->vecPosition.z;
}

DARKSDK float CameraAngleX ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fXRotate;
}

DARKSDK float CameraAngleY ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fYRotate;
}

DARKSDK float CameraAngleZ ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}
	return m_ptr->fZRotate;
}

DARKSDK float GetCameraLookX ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.x;
}

DARKSDK float GetCameraLookY ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.y;
}

DARKSDK float GetCameraLookZ ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return 0;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return 0;
	}

	// return the value
	return m_ptr->vecLook.z;
}

DARKSDK void BackdropOn ( int iID ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set state
	m_ptr->iBackdropState = 1;
}

DARKSDK void BackdropOff ( int iID ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// deactivate backdrop activation
	m_bActivateBackdrop=false;

	// set state
	m_ptr->iBackdropState = 0;

}

DARKSDK void BackdropColor ( int iID, DWORD dwColor, DWORD dwForeColor ) 
{
	// update the pointer
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	// set color
	m_ptr->dwBackdropColor = dwColor;
	m_ptr->dwForegroundColor = dwForeColor;

	// lee - 310306 - u6rc4 - if camera zero, community colour for sprite backdrop
	if ( g_pGlob && iID==0 ) g_pGlob->dw3DBackColor = dwColor;
}

DARKSDK void BackdropColor ( int iID, DWORD dwColor ) 
{
	// passed to core function
	BackdropColor ( iID, dwColor, 0 );
}

DARKSDK void BackdropTexture ( int iID, int iImage ) 
{
	// obsolete function - prior to 060409
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}
	if ( iImage==0 )
	{
		// stop backdrop texture
		m_ptr->iBackdropTextureMode = 0;
		m_ptr->pBackdropTexture = NULL;
	}
	else
	{
		// use backdrop texture
		if(iImage<1 || iImage>MAXIMUMVALUE)
		{
			RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
			return;
		}
		if ( !ImageExist ( iImage ))
		{
			RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
			return;
		}

		// set backdrop texture
		m_ptr->iBackdropTextureMode = 1;
		m_ptr->pBackdropTexture = GetImagePointer ( iImage );
	}
}

DARKSDK void BackdropScroll ( int iID, int iU, int iV )
{
	// obsolete function
	RunTimeError ( RUNTIMEERROR_COMMANDNOWOBSOLETE );
}

DARKSDK void SetCameraMatrix ( int iID, GGMATRIX* pMatrix )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = true;
	m_ptr->matViewOverride = *pMatrix;
	m_ptr->matView = m_ptr->matViewOverride;
	GGMATRIX matCurrentTrans;
	float f;
	GGMatrixInverse ( &matCurrentTrans, &f, &m_ptr->matView );
	m_ptr->vecPosition.x = matCurrentTrans._41;
	m_ptr->vecPosition.y = matCurrentTrans._42;
	m_ptr->vecPosition.z = matCurrentTrans._43;

	// get euler angles from view matrix
	GGVECTOR3 sCamright, sCamup, sCamlook;
	sCamright.x = m_ptr->matView._11;
	sCamright.y = m_ptr->matView._21;
	sCamright.z = m_ptr->matView._31;
	sCamup.x    = m_ptr->matView._12;
	sCamup.y    = m_ptr->matView._22;
	sCamup.z    = m_ptr->matView._32;
	sCamlook.x  = m_ptr->matView._13;
	sCamlook.y  = m_ptr->matView._23;
	sCamlook.z  = m_ptr->matView._33;

	// copy vector values into camera (for frustum culling)
	m_ptr->vecUp = sCamlook;
	m_ptr->vecLook = sCamup;
	m_ptr->vecRight = sCamright;

	// Calculate yaw and pitch and roll
	float lookLengthOnXZ = sqrtf( (sCamlook.z*sCamlook.z) + (sCamlook.x*sCamlook.x) );
	float fPitch = atan2f( sCamlook.y, lookLengthOnXZ );
	float fYaw   = atan2f( sCamlook.x, sCamlook.z );
	float fRoll  = atan2f( sCamup.y, sCamright.y ) - GG_PI/2;
	m_ptr->fXRotate = GGToDegree(-fPitch);
	m_ptr->fYRotate = GGToDegree(fYaw);
	m_ptr->fZRotate = GGToDegree(fRoll);
}

DARKSDK void ReleaseCameraMatrix ( int iID )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = false;
}

GGMATRIX GetCameraViewMatrix ( int iID )
{
	GGMATRIX matReturn;
	GGMatrixIdentity ( &matReturn );
	if ( !UpdateCameraPtr ( iID ) )
		return matReturn;

	return m_ptr->matView;
}

GGMATRIX GetCameraProjectionMatrix ( int iID )
{
	GGMATRIX matReturn;
	GGMatrixIdentity ( &matReturn );
	if ( !UpdateCameraPtr ( iID ) )
		return matReturn;

	return m_ptr->matProjection;
}

void SetCameraProjectionMatrix ( int iID, GGMATRIX* pMatrix )
{
	if ( iID < 0 || iID > MAXIMUMVALUE )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANUMBERILLEGAL );
		return;
	}
	if ( !UpdateCameraPtr ( iID ) )
	{
		RunTimeError ( RUNTIMEERROR_CAMERANOTEXIST );
		return;
	}

	m_ptr->bOverride = true;
	m_ptr->matProjection = *pMatrix;
}

