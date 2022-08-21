
//
// DBOFrame Functions Implementation
//

//////////////////////////////////////////////////////////////////////////////////
// DBOFRAME HEADER ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//#define _CRT_SECURE_NO_DEPRECATE
#include "DBOFrame.h"
#include "DBOMesh.h"
#include "DBOFile.h"

// Externals for DBO/Manager relationship
#include <vector>
#ifndef WICKEDENGINE
extern std::vector< sMesh* >		g_vRefreshMeshList;
#endif

//////////////////////////////////////////////////////////////////////////////////
// INTERNAL FRAME FUNCTIONS //////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK_DLL bool UpdateFrame ( sFrame *pFrame, GGMATRIX *pMatrix )
{
	// validate
	SAFE_MEMORY ( pFrame  );
	SAFE_MEMORY ( pMatrix );

	// Multiply nested matrix history with transform of this frame
	GGMatrixMultiply ( &pFrame->matCombined, &pFrame->matTransformed, pMatrix );

	// seems some models zero the _44, causing transforms to null
	if ( pFrame->matCombined._44==0.0f ) pFrame->matCombined._44 = 1.0f;

	// update child frames
	UpdateFrame ( pFrame->pChild, &pFrame->matCombined );

	// update sibling frames
	UpdateFrame ( pFrame->pSibling, pMatrix );

	// okay
	return true;
}

void ResetFrameMatrices ( sFrame* pFrame )
{
	// validate
	if ( pFrame == NULL ) return;

	// reset transform matrix to original data
	pFrame->matTransformed = pFrame->matUserMatrix * pFrame->matOriginal;
	pFrame->bVectorsCalculated = false;
	
	// update child frames
	ResetFrameMatrices ( pFrame->pChild );

	// update sibling frames
	ResetFrameMatrices ( pFrame->pSibling );
}

void CopyFrameToUserFrameMatrices ( sFrame* pFrame )
{
	if ( pFrame == NULL ) return;

	// extract difference from original matrix, that is the 'change' matrix we want the user matrix to have
	float fDet = 0.0f;
	GGMATRIX matChange;
	matChange = pFrame->matOriginal;
	GGMatrixInverse ( &matChange, &fDet, &matChange );
	GGMatrixMultiply ( &matChange, &pFrame->matTransformed, &matChange );
	pFrame->matUserMatrix = matChange;

	CopyFrameToUserFrameMatrices ( pFrame->pChild );
	CopyFrameToUserFrameMatrices ( pFrame->pSibling );
}

void SlerpFrameToUserFrameMatrices ( sFrame* pFrame, float fInterpolationTime )
{
	if ( pFrame == NULL ) return;

	// extract difference from original matrix, that is the 'change' matrix we want the user matrix to have
	float fDet = 0.0f;
	GGMATRIX matChange;
	matChange = pFrame->matOriginal;
	GGMatrixInverse ( &matChange, &fDet, &matChange );
	GGMatrixMultiply ( &matChange, &pFrame->matTransformed, &matChange );

	// in addition, we migrate the destination matrix slowly using fInterpolationTime towards matChange
	GGMATRIX matDifference;
	matDifference = matChange - pFrame->matUserMatrix;
	matDifference /= fInterpolationTime;
	pFrame->matUserMatrix += matDifference;

	SlerpFrameToUserFrameMatrices ( pFrame->pChild, fInterpolationTime );
	SlerpFrameToUserFrameMatrices ( pFrame->pSibling, fInterpolationTime );
}

DARKSDK_DLL void UpdateRealtimeFrameVectors ( sObject* pObject, sFrame* pFrame )
{
	// calculated frame data regenerates
	if ( pFrame->bVectorsCalculated==false )
	{
		// world matrix of frame within object
		GGMATRIX matTemp = pFrame->matCombined * pObject->position.matWorld;

		// 151003 - added as this is a valuable matrix to keep (used for limb collision)
		pFrame->matAbsoluteWorld = matTemp;

		// position
		pFrame->vecPosition.x = matTemp._41;
		pFrame->vecPosition.y = matTemp._42;
		pFrame->vecPosition.z = matTemp._43;

		// eliminta translation from matrix
		matTemp._41=0.0f; matTemp._42=0.0f; matTemp._43=0.0f;

		// work out scale by getting the length of a transformed vector
		GGVECTOR3 vecLine = GGVECTOR3(1,0,0);
		GGVec3TransformCoord ( &vecLine, &vecLine, &matTemp );
		float fScale = GGVec3Length ( &vecLine );

		// eliminate scale
		matTemp._11 /= fScale; matTemp._12 /= fScale; matTemp._13 /= fScale;
		matTemp._21 /= fScale; matTemp._22 /= fScale; matTemp._23 /= fScale;
		matTemp._31 /= fScale; matTemp._32 /= fScale; matTemp._33 /= fScale;

		// direction
		AnglesFromMatrix ( &matTemp, &pFrame->vecDirection );

		// flag that we have updated the vectors
		pFrame->bVectorsCalculated = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// INTERNAL ANIMATION FRAME FUNCTIONS ////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DWORD FindPositionKey ( sAnimation* pAnim, float fTime )
{
	// NOTE: Further optimization can be made by retaining the previous dwKey and quickly checking if it has advanced in sequence
	// Subdivision search to find correct key frame based on time value
	DWORD dwKey=0;
	DWORD dwKeyMax=pAnim->dwNumPositionKeys;
	int keyMin=0;
	int keyMax=(int)dwKeyMax;
	int keyDiff = keyMax-keyMin;
	int keyCentre = (int)(keyMin+((keyDiff)/2.0));
	for(;keyDiff>2;)
	{
		// the divisions are not yet too small; ie, there's still nothing definite to choose from
		if(pAnim->pPositionKeys[keyCentre+1].dwTime>fTime) 
		{
			// the correct key is in the first half of the divided section
			keyMax=keyCentre;
		}
		else
		{
			// the correct key is in the second half of the divided section
			keyMin=keyCentre;
		}

		// subdivide
		keyDiff=keyMax-keyMin;
		keyCentre=(int)(keyMin+((keyDiff)/2.0));
	}

	// the divisions are now small enough to be checked on a one-on-one basis
	if ( keyMax>=(int)dwKeyMax ) keyMax = dwKeyMax-1;
	for(int i=keyMin;i<=keyMax;i++) 
	{
		if(pAnim->pPositionKeys[i].dwTime<=fTime )
			dwKey=i;
		else
			break;
	}

	// return correct key
	return dwKey;
}

DWORD dwKey;	
DWORD dwKeyMax;
int keyMin=0;	
int keyMax;
int keyDiff;
int keyCentre;

DWORD FindRotationKey ( sAnimation* pAnim, float fTime )
{
	// NOTE: Further optimization can be made by retaining the previous dwKey and quickly checking if it has advanced in sequence
	// Subdivision search to find correct key frame based on time value
	dwKey=0;	
	dwKeyMax=pAnim->dwNumRotateKeys;
	keyMin=0;	
	//Dave - put this in then took it out as once with fries testing he had no animations, once is enough to worry!
	//if ( pAnim->pRotateKeys[pAnim->dwLastRotateKey].dwTime < fTime ) keyMin = pAnim->dwLastRotateKey;
	keyMax=(int)dwKeyMax;
	keyDiff = keyMax-keyMin;
	keyCentre = (int)(keyMin+((keyDiff)/2.0));
	for(;keyDiff>2;)
	{
		// the divisions are not yet too small; ie, there's still nothing definite to choose from
		if(pAnim->pRotateKeys[keyCentre+1].dwTime>fTime) 
		{
			// the correct key is in the first half of the divided section
			keyMax=keyCentre;
		}
		else
		{
			// the correct key is in the second half of the divided section
			keyMin=keyCentre;
		}

		// subdivide
		keyDiff=keyMax-keyMin;
		keyCentre=(int)(keyMin+((keyDiff)/2.0));
	}

	// the divisions are now small enough to be checked on a one-on-one basis
	if ( keyMax>=(int)dwKeyMax ) keyMax = dwKeyMax-1;
	for(int i=keyMin;i<=keyMax;i++) 
	{
		if(pAnim->pRotateKeys[i].dwTime<=fTime )
			dwKey=i;
		else
			break;
	}

	// return correct key
	//pAnim->dwLastRotateKey = dwKey;
	return dwKey;
}

DWORD FindScaleKey ( sAnimation* pAnim, float fTime )
{
	// NOTE: Further optimization can be made by retaining the previous dwKey and quickly checking if it has advanced in sequence
	// Subdivision search to find correct key frame based on time value
	DWORD dwKey=0;
	DWORD dwKeyMax=pAnim->dwNumScaleKeys;
	int keyMin=0;
	int keyMax=(int)dwKeyMax;
	int keyDiff = keyMax-keyMin;
	int keyCentre = (int)(keyMin+((keyDiff)/2.0));
	for(;keyDiff>2;)
	{
		// the divisions are not yet too small; ie, there's still nothing definite to choose from
		if(pAnim->pScaleKeys[keyCentre+1].dwTime>fTime) 
		{
			// the correct key is in the first half of the divided section
			keyMax=keyCentre;
		}
		else
		{
			// the correct key is in the second half of the divided section
			keyMin=keyCentre;
		}

		// subdivide
		keyDiff=keyMax-keyMin;
		keyCentre=(int)(keyMin+((keyDiff)/2.0));
	}

	// the divisions are now small enough to be checked on a one-on-one basis
	if ( keyMax>=(int)dwKeyMax ) keyMax = dwKeyMax-1;
	for(int i=keyMin;i<=keyMax;i++) 
	{
		if(pAnim->pScaleKeys[i].dwTime<=fTime )
			dwKey=i;
		else
			break;
	}

	// return correct key
	return dwKey;
}

DWORD FindMatrixKey ( sAnimation* pAnim, float fTime )
{
	// Optimization can be made by retaining the previous dwKey and quickly checking if it has advanced in sequence
	DWORD dwKey = pAnim->dwLastMatrixKey;
	DWORD dwKeyMax = pAnim->dwNumMatrixKeys;

	// Only check next five keyframes for continuance
	DWORD dwContinuanceBuffer = 5;
	if ( dwKey+dwContinuanceBuffer > dwKeyMax ) dwContinuanceBuffer = dwKeyMax-dwKey;
	DWORD dwFirstAttemptMax = dwKey+dwContinuanceBuffer;

	// first attempt from last key
	bool bFoundKey = false;
	for ( DWORD i = dwKey; i < dwFirstAttemptMax-1; i++ )
	{
		if ( fTime >= pAnim->pMatrixKeys [ i ].dwTime && fTime < pAnim->pMatrixKeys [ i+1 ].dwTime )
		{
			dwKey = i;
			bFoundKey = true;
		}
	}
	/* 020914 - problem with this one is it cannot handle time of 134 and lastkey=0 (133 slots difference)
	for ( DWORD i = dwKey; i < dwFirstAttemptMax; i++ )
	{
		if ( pAnim->pMatrixKeys [ i ].dwTime <= fTime )
		{
			bFoundKey = true;
			dwKey = i;
		}
		else
			break;
	}
	*/
	if ( bFoundKey==false )
	{
		// Subdivision search to find correct key frame based on time value
		int keyMin=0;
		int keyMax=(int)dwKeyMax;
		int keyDiff = keyMax-keyMin;
		int keyCentre = (int)(keyMin+((keyDiff)/2.0));
		for(;keyDiff>2;)
		{
			// the divisions are not yet too small; ie, there's still nothing definite to choose from
			if(pAnim->pMatrixKeys[keyCentre+1].dwTime>fTime) 
			{
				// the correct key is in the first half of the divided section
				keyMax=keyCentre;
			}
			else
			{
				// the correct key is in the second half of the divided section
				keyMin=keyCentre;
			}

			// subdivide
			keyDiff=keyMax-keyMin;
			keyCentre=(int)(keyMin+((keyDiff)/2.0));
		}

		// the divisions are now small enough to be checked on a one-on-one basis
		if ( keyMax>=(int)dwKeyMax ) keyMax = dwKeyMax-1;
		for(int i=keyMin;i<=keyMax;i++) 
		{
			if(pAnim->pMatrixKeys[i].dwTime<=fTime )
				dwKey=i;
			else
				break;
		}
	}

	// record last key (to speed up next search event)
	pAnim->dwLastMatrixKey = dwKey;

	// return correct key
	return dwKey;
}

DARKSDK_DLL void GetPositionVectorFromKey ( GGVECTOR3* pvecPos, sAnimation* pAnim, float fTime )
{
	// find key
	DWORD dwKey = FindPositionKey ( pAnim, fTime );

	// get vector
	if ( dwKey == ( pAnim->dwNumPositionKeys - 1 ) )
	{
		// use final frame in animation data
		(*pvecPos) = pAnim->pPositionKeys [ dwKey ].vecPos;
	} 
	else 
	{
		// calculate the time difference and interpolate time
		float fIntTime  = fTime - pAnim->pPositionKeys [ dwKey ].dwTime;
		(*pvecPos) = pAnim->pPositionKeys [ dwKey ].vecPos + pAnim->pPositionKeys [ dwKey ].vecPosInterpolation * ( float ) fIntTime;
	}
}

DARKSDK_DLL void GetPositionVectorFromKeySpline ( GGVECTOR3* pvecPos, sAnimation* pAnim, float fTime )
{
	// find key
	DWORD dwKey = FindPositionKey ( pAnim, fTime );
	DWORD dwKeyMax = pAnim->dwNumPositionKeys;
	//DWORD dwKey = 0;
	//DWORD dwKeyMax = pAnim->dwNumPositionKeys;
	//for ( int i = 0; i < ( int ) dwKeyMax; i++ )
	//{
	//	if ( pAnim->pPositionKeys [ i ].dwTime <= fTime )
	//		dwKey = i;
	//	else
	//		break;
	//}

	// determine four keyframes for spline calc
	DWORD dwKey1 = dwKey-1;
	DWORD dwKey2 = dwKey;
	DWORD dwKey3 = dwKey+1;
	DWORD dwKey4 = dwKey+2;

	// limit keys from leaving scope of data
	if ( dwKey1 < 0 ) dwKey1=0;
	if ( dwKey2 < 0 ) dwKey2=0;
	if ( dwKey3 < 0 ) dwKey3=0;
	if ( dwKey4 < 0 ) dwKey4=0;
	if ( dwKey1 > dwKeyMax ) dwKey1=dwKeyMax;
	if ( dwKey2 > dwKeyMax ) dwKey2=dwKeyMax;
	if ( dwKey3 > dwKeyMax ) dwKey3=dwKeyMax;
	if ( dwKey4 > dwKeyMax ) dwKey4=dwKeyMax;

	// get vector from between control points 2+3
	float fTimeScale = (float)pAnim->pPositionKeys [ dwKey3 ].dwTime - (float)pAnim->pPositionKeys [ dwKey2 ].dwTime;
	float fWeightingFactor = (fTime - (float)pAnim->pPositionKeys [ dwKey2 ].dwTime) / fTimeScale;
	GGVec3CatmullRom ( pvecPos,	&pAnim->pPositionKeys [ dwKey1 ].vecPos,
									&pAnim->pPositionKeys [ dwKey2 ].vecPos,
									&pAnim->pPositionKeys [ dwKey3 ].vecPos,
									&pAnim->pPositionKeys [ dwKey4 ].vecPos,
									fWeightingFactor );
}

DARKSDK_DLL void GetRotationQuaternionFromKey ( GGQUATERNION* pquatSlerp, sAnimation* pAnim, float fTime )
{
	// find the key that fits this time
	DWORD dwKey = FindRotationKey ( pAnim, fTime );
	//DWORD dwKey = 0;
	//for ( int i = 0; i < ( int ) pAnim->dwNumRotateKeys; i++ )
	//{
	//	if ( pAnim->pRotateKeys [ i ].dwTime <= fTime )
	//		dwKey = i;
	//	else
	//		break;
	//}

	// if it's the last key or non - smooth animation, then just set the key value
	if ( dwKey == ( pAnim->dwNumRotateKeys - 1 ) )
	{
		(*pquatSlerp) = pAnim->pRotateKeys [ dwKey ].Quaternion;
	}
	else 
	{
		// calculate the time difference and interpolate time
		float fTimeDiff = ( float ) pAnim->pRotateKeys [ dwKey + 1 ].dwTime - ( float ) pAnim->pRotateKeys [ dwKey ].dwTime;
		float fIntTime  = fTime - pAnim->pRotateKeys [ dwKey ].dwTime;
		GGQuaternionSlerp ( pquatSlerp, &pAnim->pRotateKeys [ dwKey ].Quaternion, &pAnim->pRotateKeys [ dwKey + 1 ].Quaternion, ( ( float ) fIntTime / ( float ) fTimeDiff ) );
	}
}

DARKSDK_DLL void GetScaleVectorFromKey ( GGVECTOR3* pvecScale, sAnimation* pAnim, float fTime )
{
	// find the key that fits this time
	DWORD dwKey = FindScaleKey ( pAnim, fTime );
	//DWORD dwKey = 0;
	//for ( int i = 0; i < ( int ) pAnim->dwNumScaleKeys; i++ )
	//{
	//	if ( pAnim->pScaleKeys [ i ].dwTime <= fTime )
	//		dwKey = i;
	//	else
	//		break;
	//}

	// if it's the last key or non-smooth animation, then just set the key value
	if ( dwKey == ( pAnim->dwNumScaleKeys - 1 ) )
	{
		(*pvecScale) = pAnim->pScaleKeys [ dwKey ].vecScale;
	} 
	else 
	{
		// calculate the time difference and interpolate time
		float fIntTime  = fTime - pAnim->pScaleKeys [ dwKey ].dwTime;
		(*pvecScale) = pAnim->pScaleKeys [ dwKey ].vecScale + pAnim->pScaleKeys [ dwKey ].vecScaleInterpolation * ( float ) fIntTime;
	}
}

DARKSDK_DLL void GetMatrixFromKey ( GGMATRIX* pResultMatrix, sAnimation* pAnim, float fTime )
{
	// get key frame index
	DWORD dwKey = FindMatrixKey ( pAnim, fTime );
	//DWORD dwKey = 0;
	//for ( int i = 0; i < ( int ) pAnim->dwNumMatrixKeys; i++ )
	//{
	//	if ( pAnim->pMatrixKeys [ i ].dwTime <= fTime )
	//		dwKey = i;
	//	else
	//		break;
	//}

	// if it's the last key or non-smooth animation, then just set the matrix
	if ( dwKey == ( pAnim->dwNumMatrixKeys - 1 ) )
	{
		(*pResultMatrix) = pAnim->pMatrixKeys [ dwKey ].matMatrix;
	}
	else 
	{
		// calculate the time difference and interpolate time
		float fIntTime  = fTime - pAnim->pMatrixKeys [ dwKey ].dwTime;

		// set the new interpolation matrix
		(*pResultMatrix)	= pAnim->pMatrixKeys [ dwKey ].matInterpolation * fIntTime;
		(*pResultMatrix)	= (*pResultMatrix) + pAnim->pMatrixKeys [ dwKey ].matMatrix;
	}
}

DARKSDK_DLL bool UpdatePositionKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fTime )
{
	// update position keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim                );
	SAFE_MEMORY ( pMatrix              );
	SAFE_MEMORY ( pAnim->pPositionKeys );

	// make sure we have some position keys
	if ( !pAnim->dwNumPositionKeys )
		return false;

	// get pos vector
	GGVECTOR3 vecPos;
	if ( pAnim->bLinear==TRUE )
		GetPositionVectorFromKey ( &vecPos, pAnim, fTime );
	else
		GetPositionVectorFromKeySpline ( &vecPos, pAnim, fTime );

	// combine with the new matrix
	GGMATRIX  matTemp;
	GGMatrixTranslation ( &matTemp, vecPos.x, vecPos.y, vecPos.z );
	GGMatrixMultiply    ( pMatrix, pMatrix, &matTemp );

	// okay
	return true;
}

DARKSDK_DLL bool UpdateRotationKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fTime )
{
	// update rotation keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim              );
	SAFE_MEMORY ( pMatrix            );
	SAFE_MEMORY ( pAnim->pRotateKeys );

	// make sure we have some rotation keys
	if ( !pAnim->dwNumRotateKeys )
		return false;

	// get rot vector
	GGQUATERNION	quatSlerp;
	GetRotationQuaternionFromKey ( &quatSlerp, pAnim, fTime );

	// combine with the new matrix
	GGMATRIX 	matTemp;
	GGMatrixRotationQuaternion ( &matTemp, &quatSlerp );
	GGMatrixMultiply           ( pMatrix, pMatrix, &matTemp );

	// okay
	return true;
}

DARKSDK_DLL bool UpdateScaleKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fTime )
{
	// update scale keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim             );
	SAFE_MEMORY ( pMatrix           );
	SAFE_MEMORY ( pAnim->pScaleKeys );

	// make sure we have some scale keys
	if ( !pAnim->dwNumScaleKeys )
		return false;

	// get scale vector
	GGVECTOR3 vecScale;
	GetScaleVectorFromKey ( &vecScale, pAnim, fTime );

	// combine with the new matrix
	GGMATRIX matTemp;
	GGMatrixScaling  ( &matTemp, vecScale.x, vecScale.y, vecScale.z );
	GGMatrixMultiply ( pMatrix, pMatrix, &matTemp );

	// okay
	return true;
}

DARKSDK_DLL bool UpdateMatrixKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fTime )
{
	// update position keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim              );
	SAFE_MEMORY ( pMatrix            );

	// animation is shared from another clone-obj-anim
	if ( pAnim->pSharedReadAnim )
	{
		// switch to cloned anim data (ONLY FOR READ)
		pAnim = pAnim->pSharedReadAnim;

		// can use own or clone-shared matrix bone anim data
		if ( pAnim->pMatrixKeys==NULL )
			return false;

		// make sure we have some position keys
		if ( pAnim->dwNumMatrixKeys==0 )
			return false;
	}
	else
	{
		// can use own or clone-shared matrix bone anim data
		if ( pAnim->pMatrixKeys==NULL )
			return false;

		// make sure we have some position keys
		if ( pAnim->dwNumMatrixKeys==0 )
			return false;
	}

	GGMATRIX matTransformMatrix;
	GetMatrixFromKey ( &matTransformMatrix, pAnim, fTime );

	// leefix - 140504 - make sure earlier transforms are taken into account (limbs, etc)
	GGMatrixMultiply ( &matTransformMatrix, &matTransformMatrix, pMatrix );

	// apply transformed matrix
	*pMatrix = matTransformMatrix;

	// okay
	return true;
}

DARKSDK_DLL bool SlerpPositionKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fKeyStart, float fKeyEnd, float fTime )
{
	// check the memory we use
	SAFE_MEMORY ( pAnim->pPositionKeys );

	// make sure we have some position keys
	if ( !pAnim->dwNumPositionKeys )
		return false;

	// get interpolated vector from two frame vectors
	GGVECTOR3 vecPosStart, vecPosEnd;
	GetPositionVectorFromKey ( &vecPosStart, pAnim, fKeyStart );
	GetPositionVectorFromKey ( &vecPosEnd, pAnim, fKeyEnd );
	vecPosEnd = vecPosEnd - vecPosStart;
	GGVECTOR3 vecPos = vecPosStart + ( vecPosEnd * fTime );

	// combine with the new matrix
	GGMATRIX  matTemp;
	GGMatrixTranslation ( &matTemp, vecPos.x, vecPos.y, vecPos.z );
	GGMatrixMultiply    ( pMatrix, pMatrix, &matTemp );

	return true;
}

DARKSDK_DLL bool SlerpRotationKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fKeyStart, float fKeyEnd, float fTime )
{
	// update rotation keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim              );
	SAFE_MEMORY ( pMatrix            );
	SAFE_MEMORY ( pAnim->pRotateKeys );

	// make sure we have some rotation keys
	if ( !pAnim->dwNumRotateKeys )
		return false;

	// get rot vector
	GGQUATERNION quatSlerpStart, quatSlerpEnd;
	GetRotationQuaternionFromKey ( &quatSlerpStart, pAnim, fKeyStart );
	GetRotationQuaternionFromKey ( &quatSlerpEnd, pAnim, fKeyEnd );
	quatSlerpEnd = quatSlerpEnd - quatSlerpStart;
	GGQUATERNION quatSlerp = quatSlerpStart + ( quatSlerpEnd * fTime );

	// combine with the new matrix
	GGMATRIX 	matTemp;
	GGMatrixRotationQuaternion ( &matTemp, &quatSlerp );
	GGMatrixMultiply           ( pMatrix, pMatrix, &matTemp );

	// okay
	return true;
}

DARKSDK_DLL bool SlerpScaleKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fKeyStart, float fKeyEnd, float fTime )
{
	// update scale keys for an animation

	// check the memory we use
	SAFE_MEMORY ( pAnim             );
	SAFE_MEMORY ( pMatrix           );
	SAFE_MEMORY ( pAnim->pScaleKeys );

	// make sure we have some scale keys
	if ( !pAnim->dwNumScaleKeys )
		return false;

	// get interpolated vector from two frame vectors
	GGVECTOR3 vecScaleStart, vecScaleEnd;
	GetScaleVectorFromKey ( &vecScaleStart, pAnim, fKeyStart );
	GetScaleVectorFromKey ( &vecScaleEnd, pAnim, fKeyEnd );
	vecScaleEnd = vecScaleEnd - vecScaleStart;
	GGVECTOR3 vecScale = vecScaleStart + ( vecScaleEnd * fTime );

	// combine with the new matrix
	GGMATRIX matTemp;
	GGMatrixScaling  ( &matTemp, vecScale.x, vecScale.y, vecScale.z );
	GGMatrixMultiply ( pMatrix, pMatrix, &matTemp );

	// okay
	return true;
}

DARKSDK_DLL bool SlerpMatrixKeys ( sAnimation* pAnim, GGMATRIX* pMatrix, float fKeyStart, float fKeyEnd, float fTime )
{
	// check the memory we use
	SAFE_MEMORY ( pAnim              );
	SAFE_MEMORY ( pMatrix            );

	// animation is shared from another clone-obj-anim
	if ( pAnim->pSharedReadAnim )
	{
		// switch to cloned anim data (ONLY FOR READ)
		pAnim = pAnim->pSharedReadAnim;

		// can use own or clone-shared matrix bone anim data
		if ( pAnim->pMatrixKeys==NULL )
			return false;

		// make sure we have some position keys
		if ( pAnim->dwNumMatrixKeys==0 )
			return false;
	}
	else
	{
		// can use own or clone-shared matrix bone anim data
		if ( pAnim->pMatrixKeys==NULL )
			return false;

		// make sure we have some position keys
		if ( pAnim->dwNumMatrixKeys==0 )
			return false;
	}

	// get interpolated vector from two frame matrices
	GGMATRIX matMatrixStart, matMatrixEnd;
	GetMatrixFromKey ( &matMatrixStart, pAnim, fKeyStart );
	GetMatrixFromKey ( &matMatrixEnd, pAnim, fKeyEnd );
	matMatrixEnd = matMatrixEnd - matMatrixStart;
	GGMATRIX matTransformMatrix = matMatrixStart + ( matMatrixEnd * fTime );

	// leefix - 140504 - make sure earlier transforms are taken into account (limbs, etc)
	GGMatrixMultiply ( &matTransformMatrix, &matTransformMatrix, pMatrix );

	// apply transformed matrix
	*pMatrix = matTransformMatrix;

	// okay
	return true;
}

DARKSDK_DLL bool UpdateAnimationData ( sAnimation* pAnim, float fTime )
{
	// check the animation is valid
	SAFE_MEMORY ( pAnim         );
	SAFE_MEMORY ( pAnim->pFrame );

	// start with un-animated user defined matrix
	pAnim->pFrame->matTransformed = pAnim->pFrame->matUserMatrix;
	pAnim->pFrame->bVectorsCalculated = false;

	// update matrices for animation
	UpdateScaleKeys		( pAnim, &pAnim->pFrame->matTransformed, fTime );	// update scale keys
	UpdateRotationKeys	( pAnim, &pAnim->pFrame->matTransformed, fTime );	// update rotation keys
	UpdatePositionKeys	( pAnim, &pAnim->pFrame->matTransformed, fTime );	// update position keys
	UpdateMatrixKeys	( pAnim, &pAnim->pFrame->matTransformed, fTime );	// update matrix keys ( for bone animation )

	return true;
}

DARKSDK_DLL bool SlerpAnimationData ( sAnimation* pAnim, float fKeyStart, float fKeyEnd, float fTime )
{
	// check the animation is valid
	SAFE_MEMORY ( pAnim->pFrame );

	// start with un-animated user defined matrix
	pAnim->pFrame->matTransformed = pAnim->pFrame->matUserMatrix;
	pAnim->pFrame->bVectorsCalculated = false;

	// update matrices for manual slerp
	SlerpScaleKeys		( pAnim, &pAnim->pFrame->matTransformed, fKeyStart, fKeyEnd, fTime );
	SlerpRotationKeys	( pAnim, &pAnim->pFrame->matTransformed, fKeyStart, fKeyEnd, fTime );
	SlerpPositionKeys	( pAnim, &pAnim->pFrame->matTransformed, fKeyStart, fKeyEnd, fTime );
	SlerpMatrixKeys		( pAnim, &pAnim->pFrame->matTransformed, fKeyStart, fKeyEnd, fTime );

	// okay
	return true;
}

DARKSDK_DLL bool UpdateAllFrameData ( sObject* pObject, float fTime )
{
	// check the object is okay
	SAFE_MEMORY ( pObject );

	// validate any animation data
	sAnimation* pAnim = NULL;
	if ( pObject->pAnimationSet )
	{
		if ( pObject->pAnimationSet->pAnimation )
		{
			// get a pointer to the animation
			pAnim = pObject->pAnimationSet->pAnimation;
		}
	}

	// set original frame data (animation or not)
	ResetFrameMatrices ( pObject->pFrame );

	// if model slerping via interpolation
	if ( pObject->bAnimManualSlerp )
	{
		// run through all animations and perform manual slerp
		while ( pAnim != NULL )
		{
			float fUseTime = -1.0f;
			if ( pObject->pfAnimLimbFrame )
			{
				if ( pAnim->pFrame )
				{
					// only allow non-limb-frame updates, limb-overide frames updated below
					int iFrameIndex = pAnim->pFrame->iID;
					fUseTime = pObject->pfAnimLimbFrame [ iFrameIndex ];
				}
			}
			if ( fUseTime>=0.0f )
			{
				// use frame override on this frame if override active (even for slerp)
				UpdateAnimationData ( pAnim, fUseTime );
			}
			else
			{
				// slerp animation data
				SlerpAnimationData ( pAnim, pObject->fAnimSlerpStartFrame, pObject->fAnimSlerpEndFrame, pObject->fAnimSlerpTime );
			}

			// move to the next sequence
			pAnim = pAnim->pNext;
		}
	}
	else
	{
		// run through all animations and update them
		if ( pAnim )
		{
			while ( pAnim != NULL )
			{
				// U75 - 240909 - can override per frame animation frame
				float fUseTime = fTime;
				if ( pObject->pfAnimLimbFrame )
				{
					if ( pAnim->pFrame )
					{
						// only allow non-limb-frame updates, limb-overide frames updated below
						fUseTime = -1.0f;
						int iFrameIndex = pAnim->pFrame->iID;
						fUseTime = pObject->pfAnimLimbFrame [ iFrameIndex ];
						if ( fUseTime==-1.0f ) fUseTime = fTime;
					}
				}
				if ( fUseTime>=0.0f )
				{
					// update animation data (if not entire disabled with -2.0)
					UpdateAnimationData ( pAnim, fUseTime );
				}

				// move to the next sequence
				pAnim = pAnim->pNext;
			}
		}
	}

	return true;
}

DARKSDK_DLL void UpdateUserMatrix ( sFrame* pFrame )
{
	// temp var
	GGMATRIX matTemp;

	// user matrix ptr
	GGMATRIX*	pWorkMat = &pFrame->matUserMatrix;
    GGMatrixIdentity		( pWorkMat );

	// scale
	GGMatrixScaling		( &matTemp, pFrame->vecScale.x, pFrame->vecScale.y, pFrame->vecScale.z );
	GGMatrixMultiply		( pWorkMat, pWorkMat, &matTemp );

	// rotation
	GGMatrixRotationX		( &matTemp, GGToRadian ( pFrame->vecRotation.x ) );
	GGMatrixMultiply		( pWorkMat, pWorkMat, &matTemp );
	GGMatrixRotationY		( &matTemp, GGToRadian ( pFrame->vecRotation.y ) );
	GGMatrixMultiply		( pWorkMat, pWorkMat, &matTemp );
	GGMatrixRotationZ		( &matTemp, GGToRadian ( pFrame->vecRotation.z ) );
	GGMatrixMultiply		( pWorkMat, pWorkMat, &matTemp );

	// translation
	GGMatrixTranslation	( &matTemp, pFrame->vecOffset.x, pFrame->vecOffset.y, pFrame->vecOffset.z );
	GGMatrixMultiply		( pWorkMat, pWorkMat, &matTemp );
}

DARKSDK_DLL void CopyFrameAnimToUserFrame ( sObject* pObject, float fChosenAnimFrame )
{
	// can be used to grab an anim frame to impose over the regular animation (mouth anim while walking, etc)

	// first reset previous usermatrix so as not to corrupt new frame to simulate
	for ( int iLimbID=0; iLimbID < pObject->iFrameCount; iLimbID++ )
		GGMatrixIdentity ( &pObject->ppFrameList[iLimbID]->matUserMatrix );

	// set the anim frame
	UpdateAllFrameData ( pObject, fChosenAnimFrame );

	// copy the final matrices from the anim to the user frame
	CopyFrameToUserFrameMatrices ( pObject->pFrame );

	// restore the anim frame of the object
	UpdateAllFrameData ( pObject, pObject->fAnimFrame );
}

DARKSDK_DLL void SlerpFrameAnimToUserFrame ( sObject* pObject, float fChosenAnimFrame, float fInterpolationTime )
{
	#ifdef WICKEDENGINE
	// wicked uses own animation system
	#else
	// store previous usermatrix so can slerp later on
	GGMATRIX* pMatrixArray = new GGMATRIX[pObject->iFrameCount];
	for ( int iLimbID=0; iLimbID < pObject->iFrameCount; iLimbID++ )
		pMatrixArray[iLimbID] = pObject->ppFrameList[iLimbID]->matUserMatrix;

	// reset previous usermatrix so as not to corrupt new frame to simulate
	for ( int iLimbID=0; iLimbID < pObject->iFrameCount; iLimbID++ )
		GGMatrixIdentity ( &pObject->ppFrameList[iLimbID]->matUserMatrix );

	// set the anim frame
	UpdateAllFrameData ( pObject, fChosenAnimFrame );

	// put user matrix back so can slerp from current to destination
	for ( int iLimbID=0; iLimbID < pObject->iFrameCount; iLimbID++ )
		pObject->ppFrameList[iLimbID]->matUserMatrix = pMatrixArray[iLimbID];

	// slerp the final matrices from the anim to the user frame
	SlerpFrameToUserFrameMatrices ( pObject->pFrame, fInterpolationTime );

	// restore the anim frame of the object
	UpdateAllFrameData ( pObject, pObject->fAnimFrame );

	// free usages
	SAFE_DELETE_ARRAY ( pMatrixArray );
	#endif
}

DARKSDK_DLL bool AppendAnimationDataCore(sAnimation* pCurrentAnimationToAppend, sAnimationSet** ppAnimationSetToAddTo, int iFrameStart, bool bCompleteLoopWithFrameZeroCopy)
{
	// account for underscores being replaced with spaces
	sAnimation* pLoadedAnim = pCurrentAnimationToAppend;
	while (pLoadedAnim)
	{
		char pLoadAnimNameWithUnderscores[MAX_STRING];
		strcpy(pLoadAnimNameWithUnderscores, pLoadedAnim->szName);
		for (int n = 0; n < strlen(pLoadAnimNameWithUnderscores); n++)
		{
			if (pLoadAnimNameWithUnderscores[n] == ' ') pLoadAnimNameWithUnderscores[n] = '_';
		}
		strcpy(pLoadedAnim->szName, pLoadAnimNameWithUnderscores);
		pLoadedAnim = pLoadedAnim->pNext;
	}

	// append current animation list into existing specified animationset
	bool bAnimationAppended = false;
	sAnimation* pOrigCurrent = NULL;
	if (*ppAnimationSetToAddTo) pOrigCurrent = (*ppAnimationSetToAddTo)->pAnimation;
	if (pOrigCurrent == NULL)
	{
		// object being appended to has no animations, so create blank ones from structure of imported one
		if (*ppAnimationSetToAddTo == NULL) *ppAnimationSetToAddTo = new sAnimationSet();
		sAnimation* pLastAnim = NULL;
		sAnimation* pLoadedAnim = pCurrentAnimationToAppend;
		while (pLoadedAnim)
		{
			// create new anim
			sAnimation* pAnim = new sAnimation();
			if ((*ppAnimationSetToAddTo)->pAnimation == NULL)
				(*ppAnimationSetToAddTo)->pAnimation = pAnim;
			else
				pLastAnim->pNext = pAnim;

			// copy name over
			strcpy(pAnim->szName, pLoadedAnim->szName);

			// go to next loaded anim
			pLastAnim = pAnim;
			pLoadedAnim = pLoadedAnim->pNext;
		}
		pOrigCurrent = (*ppAnimationSetToAddTo)->pAnimation;
	}
	else
	{
		// check if the newly appending anim needs animation structure adding to animset zero
		if (*ppAnimationSetToAddTo)
		{
			sAnimation* pLastAnim = NULL;
			sAnimation* pLoadedAnim = pCurrentAnimationToAppend;
			while (pLoadedAnim)
			{
				// find if name exists
				bool bWeHaveThisAnimName = false;
				sAnimation* pFindAnim = (*ppAnimationSetToAddTo)->pAnimation;
				while (pFindAnim)
				{
					if (stricmp(pFindAnim->szName, pLoadedAnim->szName) == NULL)
					{
						bWeHaveThisAnimName = true;
						break;
					}
					pFindAnim = pFindAnim->pNext;
				}
				if (bWeHaveThisAnimName == false)
				{
					// we are appending this anim name, but it does not exist in animset zero
					sAnimation* pLastAnim = (*ppAnimationSetToAddTo)->pAnimation;
					while (pLastAnim->pNext) pLastAnim = pLastAnim->pNext;

					// so we add it now
					sAnimation* pAnim = new sAnimation();
					if ((*ppAnimationSetToAddTo)->pAnimation == NULL)
						(*ppAnimationSetToAddTo)->pAnimation = pAnim;
					else
						pLastAnim->pNext = pAnim;

					// copy name over
					strcpy(pAnim->szName, pLoadedAnim->szName);
				}

				// next one
				pLoadedAnim = pLoadedAnim->pNext;
			}
		}
	}
	while (pCurrentAnimationToAppend)
	{
		// ensure animnames match
		bool bNoMatch = false;
		if (pCurrentAnimationToAppend->szName && pOrigCurrent->szName)
		{
			if (_stricmp(pCurrentAnimationToAppend->szName, pOrigCurrent->szName) == NULL)
			{
				// found a match
				bNoMatch = true;

				// some animation appended
				DWORD dwTimeOffset = (DWORD)iFrameStart;
				bAnimationAppended = true;

				// 070220 - if iFrameStart is zero, means we are ERASING the existing animation
				// data that is in this model, so flag this alternative to appending
				bool bOverwriteOldAnimCompletely = false;
				if (iFrameStart == 0) bOverwriteOldAnimCompletely = true;

				// lee - 200306 - u6b4 - missed out matrix data for some reason (added below)
				// old and new data
				DWORD           dwOrigNumMatrixKeys = pOrigCurrent->dwNumMatrixKeys;
				sMatrixKey*		pOrigMatrixKeys = pOrigCurrent->pMatrixKeys;
				DWORD           dwAppNumMatrixKeys = pCurrentAnimationToAppend->dwNumMatrixKeys;
				sMatrixKey*		pAppMatrixKeys = pCurrentAnimationToAppend->pMatrixKeys;

				// create new animation arrays
				sMatrixKey*		pNewMatrixKeys = NULL;
				DWORD           dwNewNumMatrixKeys = dwOrigNumMatrixKeys + dwAppNumMatrixKeys;
				if (bOverwriteOldAnimCompletely == true) dwNewNumMatrixKeys = dwAppNumMatrixKeys;
				if (dwNewNumMatrixKeys > 0) pNewMatrixKeys = new sMatrixKey[dwNewNumMatrixKeys];

				// before create new animation data, erase ALL animation instructions beyond "dwTimeOffset"
				// so as not to interfeer with new appended animation
				if (bOverwriteOldAnimCompletely == false)
				{
					// only if appending
					for (DWORD o = 0; o < dwOrigNumMatrixKeys; o++)
						if (pOrigMatrixKeys[o].dwTime >= dwTimeOffset)
							pOrigMatrixKeys[o].dwTime = 9999999;

					// modify additional data to account for keyframe-start-shift
					for (DWORD p = 0; p < dwAppNumMatrixKeys; p++) pAppMatrixKeys[p].dwTime += dwTimeOffset;

					// copy old animation
					if (pNewMatrixKeys) memcpy(pNewMatrixKeys, pOrigMatrixKeys, dwOrigNumMatrixKeys * sizeof(sMatrixKey));
				}

				// copy appended animation
				LPSTR pMiddleM = (LPSTR)pNewMatrixKeys + dwOrigNumMatrixKeys * sizeof(sMatrixKey);
				if (bOverwriteOldAnimCompletely == true) pMiddleM = (LPSTR)pNewMatrixKeys;
				if (pNewMatrixKeys) memcpy(pMiddleM, pAppMatrixKeys, dwAppNumMatrixKeys * sizeof(sMatrixKey));

				// delete original animation arrays
				if (pOrigMatrixKeys) SAFE_DELETE_ARRAY(pOrigMatrixKeys);

				// assign new ptrs and totals
				pOrigCurrent->dwNumMatrixKeys = dwNewNumMatrixKeys;
				pOrigCurrent->pMatrixKeys = pNewMatrixKeys;

				// origianl animation data
				DWORD           dwOrigNumPositionKeys = pOrigCurrent->dwNumPositionKeys;
				sPositionKey*	pOrigPositionKeys = pOrigCurrent->pPositionKeys;
				DWORD           dwOrigNumRotateKeys = pOrigCurrent->dwNumRotateKeys;
				sRotateKey*		pOrigRotateKeys = pOrigCurrent->pRotateKeys;
				DWORD           dwOrigNumScaleKeys = pOrigCurrent->dwNumScaleKeys;
				sScaleKey*		pOrigScaleKeys = pOrigCurrent->pScaleKeys;

				// new animation data
				DWORD           dwAppNumPositionKeys = pCurrentAnimationToAppend->dwNumPositionKeys;
				sPositionKey*	pAppPositionKeys = pCurrentAnimationToAppend->pPositionKeys;
				DWORD           dwAppNumRotateKeys = pCurrentAnimationToAppend->dwNumRotateKeys;
				sRotateKey*		pAppRotateKeys = pCurrentAnimationToAppend->pRotateKeys;
				DWORD           dwAppNumScaleKeys = pCurrentAnimationToAppend->dwNumScaleKeys;
				sScaleKey*		pAppScaleKeys = pCurrentAnimationToAppend->pScaleKeys;

				// if copying frame zero to end for loop, add one more
				DWORD dwOneMorePos = 0;
				DWORD dwOneMoreRot = 0;
				DWORD dwOneMoreScl = 0;
				if (bCompleteLoopWithFrameZeroCopy == true)
				{
					if (dwAppNumPositionKeys > 0 ) dwOneMorePos = 1;
					if (dwAppNumRotateKeys > 0) dwOneMoreRot = 1;
					if (dwAppNumScaleKeys > 0) dwOneMoreScl = 1;
				}

				// create new animation arrays
				sPositionKey*	pNewPositionKeys = NULL;
				DWORD           dwNewNumPositionKeys = dwOrigNumPositionKeys + dwAppNumPositionKeys + dwOneMorePos;
				if (bOverwriteOldAnimCompletely == true)		dwNewNumPositionKeys = dwAppNumPositionKeys + dwOneMorePos;
				if (dwNewNumPositionKeys > 0) pNewPositionKeys = new sPositionKey[dwNewNumPositionKeys];
				sRotateKey*		pNewRotateKeys = NULL;
				DWORD           dwNewNumRotateKeys = dwOrigNumRotateKeys + dwAppNumRotateKeys + dwOneMoreRot;
				if (bOverwriteOldAnimCompletely == true)		dwNewNumRotateKeys = dwAppNumRotateKeys + dwOneMoreRot;
				if (dwNewNumRotateKeys > 0) pNewRotateKeys = new sRotateKey[dwNewNumRotateKeys];
				sScaleKey*		pNewScaleKeys = NULL;
				DWORD           dwNewNumScaleKeys = dwOrigNumScaleKeys + dwAppNumScaleKeys + dwOneMoreScl;
				if (bOverwriteOldAnimCompletely == true)		dwNewNumScaleKeys = dwAppNumScaleKeys + dwOneMoreScl;
				if (dwNewNumScaleKeys > 0) pNewScaleKeys = new sScaleKey[dwNewNumScaleKeys];

				// modify additional data to account for keyframe-start-shift
				if (bOverwriteOldAnimCompletely == false)
				{
					// only if appending
					for (DWORD p = 0; p < dwAppNumPositionKeys; p++) pAppPositionKeys[p].dwTime += dwTimeOffset;
					for (DWORD r = 0; r < dwAppNumRotateKeys; r++) pAppRotateKeys[r].dwTime += dwTimeOffset;
					for (DWORD s = 0; s < dwAppNumScaleKeys; s++) pAppScaleKeys[s].dwTime += dwTimeOffset;

					// copy old animation
					if (pNewPositionKeys) memcpy(pNewPositionKeys, pOrigPositionKeys, dwOrigNumPositionKeys * sizeof(sPositionKey));
					if (pNewRotateKeys) memcpy(pNewRotateKeys, pOrigRotateKeys, dwOrigNumRotateKeys * sizeof(sRotateKey));
					if (pNewScaleKeys) memcpy(pNewScaleKeys, pOrigScaleKeys, dwOrigNumScaleKeys * sizeof(sScaleKey));
				}

				// copy appended animation
				LPSTR pMiddleP = (LPSTR)pNewPositionKeys + dwOrigNumPositionKeys * sizeof(sPositionKey);
				if (bOverwriteOldAnimCompletely == true) pMiddleP = (LPSTR)pNewPositionKeys;
				if (pNewPositionKeys) memcpy(pMiddleP, pAppPositionKeys, dwAppNumPositionKeys * sizeof(sPositionKey));
				LPSTR pMiddleR = (LPSTR)pNewRotateKeys + dwOrigNumRotateKeys * sizeof(sRotateKey);
				if (bOverwriteOldAnimCompletely == true) pMiddleR = (LPSTR)pNewRotateKeys;
				if (pNewRotateKeys) memcpy(pMiddleR, pAppRotateKeys, dwAppNumRotateKeys * sizeof(sRotateKey));
				LPSTR pMiddleS = (LPSTR)pNewScaleKeys + dwOrigNumScaleKeys * sizeof(sScaleKey);
				if (bOverwriteOldAnimCompletely == true) pMiddleS = (LPSTR)pNewScaleKeys;
				if (pNewScaleKeys) memcpy(pMiddleS, pAppScaleKeys, dwAppNumScaleKeys * sizeof(sScaleKey));

				// copy frame zero to last frame (for perfect looping lerps) (introduced a small delay, did not work)
				if (bCompleteLoopWithFrameZeroCopy == true)
				{
					// position
					if (dwOneMorePos == 1)
					{
						DWORD dwAnimPosIndex = dwOrigNumPositionKeys + dwAppNumPositionKeys;
						pNewPositionKeys[dwAnimPosIndex].dwTime = pNewPositionKeys[dwAnimPosIndex - 1].dwTime + 33;
						pNewPositionKeys[dwAnimPosIndex].vecPos = pNewPositionKeys[dwOrigNumPositionKeys].vecPos;
					}
					// rotation
					if (dwOneMoreRot == 1)
					{
						DWORD dwAnimRotIndex = dwOrigNumRotateKeys + dwAppNumRotateKeys;
						pNewRotateKeys[dwAnimRotIndex].dwTime = pNewRotateKeys[dwAnimRotIndex - 1].dwTime + 33;
						pNewRotateKeys[dwAnimRotIndex].Quaternion = pNewRotateKeys[dwOrigNumRotateKeys].Quaternion;
					}
					// scale
					if (dwOneMoreScl == 1)
					{
						DWORD dwAnimSclIndex = dwOrigNumScaleKeys + dwAppNumScaleKeys;
						pNewScaleKeys[dwAnimSclIndex].dwTime = pNewScaleKeys[dwAnimSclIndex - 1].dwTime + 33;
						pNewScaleKeys[dwAnimSclIndex].vecScale = pNewScaleKeys[dwOrigNumScaleKeys].vecScale;
					}
				}

				// delete original animation arrays
				if (pOrigPositionKeys) SAFE_DELETE_ARRAY(pOrigPositionKeys);
				if (pOrigRotateKeys) SAFE_DELETE_ARRAY(pOrigRotateKeys);
				if (pOrigScaleKeys) SAFE_DELETE_ARRAY(pOrigScaleKeys);

				// assign new ptrs and totals
				pOrigCurrent->dwNumPositionKeys = dwNewNumPositionKeys;
				pOrigCurrent->pPositionKeys = pNewPositionKeys;
				pOrigCurrent->dwNumRotateKeys = dwNewNumRotateKeys;
				pOrigCurrent->pRotateKeys = pNewRotateKeys;
				pOrigCurrent->dwNumScaleKeys = dwNewNumScaleKeys;
				pOrigCurrent->pScaleKeys = pNewScaleKeys;

				// increment both
				pOrigCurrent = pOrigCurrent->pNext;

				// LB: this caused an entire frame in the appended animation to be skipped (if the needed frame was earlier in the pOrigCurrent list!)
				//pCurrentAnimationToAppend = pCurrentAnimationToAppend->pNext;
			}
		}
		if (bNoMatch == false)
		{
			// when reach end, reset so we we repeat searches through orig
			if (pOrigCurrent == NULL)
			{
				// at end of original traverse, reset so we can go through all current limbs
				pOrigCurrent = (*ppAnimationSetToAddTo)->pAnimation;

				// advance slowly and once through pCurrent
				pCurrentAnimationToAppend = pCurrentAnimationToAppend->pNext;
			}
			else
			{
				pOrigCurrent = pOrigCurrent->pNext;
			}
		}
	}

	// return true if append successful
	return bAnimationAppended;
}

DARKSDK_DLL bool AppendAnimationData ( sObject* pObject, LPSTR szFilename, int iFrameStart )
{
	#ifdef WICKEDENGINE
	// clear old anim data if entirely replacing
	if (iFrameStart == 0)
	{
		extern void animsystem_clearoldanimationfromobject(sObject*);
		animsystem_clearoldanimationfromobject(pObject);
	}
	#endif

	// load file into temp load-object
	sObject* pLoadObject = NULL;
	if ( !LoadDBO ( (LPSTR)szFilename, &pLoadObject ) )
	{
		// count not load file
		return false;
	}

	// no animation data
	if ( pLoadObject->pAnimationSet==NULL )
	{
		// no animation data
		return false;
	}

	// new animation data available?
	sAnimation* pAnim = pLoadObject->pAnimationSet->pAnimation;

	// get number of new frames
	DWORD dwNewFrames = pLoadObject->pAnimationSet->ulLength;

	// append some animation data
	sAnimation* pCurrent = pLoadObject->pAnimationSet->pAnimation;
	bool bAnimationAppended = AppendAnimationDataCore(pCurrent, &pObject->pAnimationSet, iFrameStart, false);
	if (bAnimationAppended == false)
	{
		SAFE_DELETE(pLoadObject);
		return false;
	}

	#ifdef WICKEDENGINE
	// DBO now contains mode-124 animsets which contain specific animslot details (name, start, finish)
	if (pLoadObject->pAnimationSet)
	{
		if (iFrameStart == 0)
		{
			// and populate with loaded animset structure
			sAnimationSet* pLoadedAnimSet = pLoadObject->pAnimationSet;
			sAnimationSet* pAnimSet = pObject->pAnimationSet;
			pAnimSet->ulLength = pLoadedAnimSet->ulLength;
			strcpy (pAnimSet->szName, pLoadedAnimSet->szName);
			pAnimSet->fAnimSetStart = pLoadedAnimSet->fAnimSetStart;
			pAnimSet->fAnimSetFinish = pLoadedAnimSet->fAnimSetFinish;
			pAnimSet->fAnimSetStep1 = pLoadedAnimSet->fAnimSetStep1;
			pAnimSet->fAnimSetStep2 = pLoadedAnimSet->fAnimSetStep2;
			pAnimSet->fAnimSetStep3 = pLoadedAnimSet->fAnimSetStep3;
			pLoadedAnimSet = pLoadedAnimSet->pNext;
			while (pLoadedAnimSet)
			{
				// create ref animset to store animation slots
				sAnimationSet* pNewAnimSet = new sAnimationSet;
				pNewAnimSet->dwAnimSetType = 124;
				pNewAnimSet->ulLength = pLoadedAnimSet->ulLength;
				strcpy (pNewAnimSet->szName, pLoadedAnimSet->szName);
				pNewAnimSet->fAnimSetStart = pLoadedAnimSet->fAnimSetStart;
				pNewAnimSet->fAnimSetFinish = pLoadedAnimSet->fAnimSetFinish;
				pNewAnimSet->fAnimSetStep1 = pLoadedAnimSet->fAnimSetStep1;
				pNewAnimSet->fAnimSetStep2 = pLoadedAnimSet->fAnimSetStep2;
				pNewAnimSet->fAnimSetStep3 = pLoadedAnimSet->fAnimSetStep3;
				pAnimSet->pNext = pNewAnimSet;
				pAnimSet = pNewAnimSet;
				pLoadedAnimSet = pLoadedAnimSet->pNext;
			}
		}
		else
		{
			// append to end
			// find end of target objects animsets
			sAnimationSet* pAnimSet = pObject->pAnimationSet;
			while (pAnimSet->pNext) pAnimSet = pAnimSet->pNext;

			// go through all animsets of loaded animation object
			sAnimationSet* pLoadedAnimSet = pLoadObject->pAnimationSet->pNext;
			while (pLoadedAnimSet)
			{
				// create ref animset to store animation slots
				sAnimationSet* pNewAnimSet = new sAnimationSet;
				pNewAnimSet->dwAnimSetType = 124;
				pNewAnimSet->ulLength = pLoadedAnimSet->ulLength;
				strcpy (pNewAnimSet->szName, pLoadedAnimSet->szName);
				pNewAnimSet->fAnimSetStart = pLoadedAnimSet->fAnimSetStart;
				pNewAnimSet->fAnimSetFinish = pLoadedAnimSet->fAnimSetFinish;
				pNewAnimSet->fAnimSetStep1 = pLoadedAnimSet->fAnimSetStep1;
				pNewAnimSet->fAnimSetStep2 = pLoadedAnimSet->fAnimSetStep2;
				pNewAnimSet->fAnimSetStep3 = pLoadedAnimSet->fAnimSetStep3;
				pAnimSet->pNext = pNewAnimSet;
				pAnimSet = pNewAnimSet;

				// next animset of loaded animation object
				pLoadedAnimSet = pLoadedAnimSet->pNext;
			}
		}
	}
	#endif

	// update total number of frames
	if (iFrameStart == 0)
	{
		// when frame zero is specified, we are entirely replacing the old animation set
		pObject->pAnimationSet->ulLength = dwNewFrames;
	}
	else
	{
		// otherwise, we account for the added animations to the total frame count
		//pObject->pAnimationSet->ulLength += dwNewFrames; //LB: this did not work, needs to represent the appended anim right to the end!
		pObject->pAnimationSet->ulLength = iFrameStart + dwNewFrames;
	}

	#ifdef WICKEDENGINE
	// we assume all MAX ready objects have sorted animation keyframes to gain performance
	bool bCostlySortingOfKeyframes = false;
	MapFramesToAnimations (pObject, bCostlySortingOfKeyframes);
	#else
	// recalculate animation data and bounds for object
	MapFramesToAnimations ( pObject, true );
	#endif

	// delete load object when finished with it
	SAFE_DELETE(pLoadObject);

	// okay
	return true;
}

DARKSDK_DLL bool AppendAnimationFromFile ( sObject* pObject, LPSTR szFilename, int iFrame )
{
	// Add animation only from file
	if ( AppendAnimationData ( pObject, szFilename, iFrame ) )
	{
		// set new end frame when append
		pObject->fAnimFrameEnd = (float)pObject->pAnimationSet->ulLength;
	}
	else
	{
		// failed to append
		return false;
	}

	// Complete
	return true;
}

DARKSDK_DLL void ClearAnimationFromObject (sObject* pObject)
{
	// go through all animation sets
	SAFE_DELETE(pObject->pAnimationSet);
	pObject->pAnimationSet = NULL;
}

//////////////////////////////////////////////////////////////////////////////////
// FRAME FUNCTIONS ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

// Frame Basic Functions

DARKSDK_DLL void Offset ( sFrame* pFrame, float fX, float fY, float fZ )
{
	// apply new offset
	pFrame->vecOffset.x = fX;
	pFrame->vecOffset.y = fY;
	pFrame->vecOffset.z = fZ;

	// update user matrix
	UpdateUserMatrix(pFrame);
}

DARKSDK_DLL void Rotate ( sFrame* pFrame, float fX, float fY, float fZ )
{
	// apply new rotation
	pFrame->vecRotation.x = fX;
	pFrame->vecRotation.y = fY;
	pFrame->vecRotation.z = fZ;

	// update user matrix
	UpdateUserMatrix(pFrame);
}

DARKSDK_DLL void Scale ( sFrame* pFrame, float fX, float fY, float fZ )
{
	// apply new scale
	pFrame->vecScale.x = fX;
	pFrame->vecScale.y = fY;
	pFrame->vecScale.z = fZ;

	// update user matrix
	UpdateUserMatrix(pFrame);
}

// Animation Construction Functions

DARKSDK_DLL bool AddNewAnimationFrame ( sObject* pObject, sFrame* pFrameToAdd )
{
	if ( pObject->pAnimationSet )
	{
		if ( pObject->pAnimationSet->pAnimation )
		{
			// get a pointer to the last
			sAnimation* pLastAnim = pObject->pAnimationSet->pAnimation;
			while ( pLastAnim->pNext )
				pLastAnim = pLastAnim->pNext;

			// create new
			pLastAnim->pNext = new sAnimation;

			// assign to new
			strcpy ( pLastAnim->pNext->szName, pFrameToAdd->szName );
			pLastAnim->pNext->pFrame = pFrameToAdd;
		}
	}

	// okay
	return true;
}

DARKSDK_DLL bool RemoveAnimationRec ( sObject* pObject, sAnimation** ppCurrentBase, sFrame* pFrameToRemove )
{
	// find frame
	sAnimation* pPrevious = NULL;
	sAnimation* pCurrent = (*ppCurrentBase);
	while ( pCurrent )
	{
		// determine next frame now
		sAnimation* pNext = pCurrent->pNext;

		// if this frame to be deleted
		if ( pCurrent->pFrame==pFrameToRemove )
		{
			// sever frame from higher nodes
			pCurrent->pNext=NULL;

			// remove from list
			if ( pPrevious==NULL )
			{
				// new list-base start
				(*ppCurrentBase) = pNext;
			}
			else
			{
				// skip sybling
				pPrevious->pNext = pNext;
			}

			// delete frame
			delete pCurrent;

			// complete
			return false;
		}

		// try next one
		pPrevious = pCurrent;
		pCurrent = pNext;
	}

	// carry on
	return true;
}

DARKSDK_DLL bool RemoveAnimationFrame ( sObject* pObject, sFrame* pFrameToRemove )
{
	// remove animation from hierarchy
	if ( pObject->pAnimationSet )
	{
		if ( pObject->pAnimationSet->pAnimation )
		{
			RemoveAnimationRec ( pObject, &pObject->pAnimationSet->pAnimation, pFrameToRemove );
		}
	}

	// okay
	return true;
}

DARKSDK_DLL bool RemoveAnimationFrameChildRec ( sObject* pObject, sFrame* pFrameToRemove )
{
	sFrame* pFrame = pFrameToRemove;
	while ( pFrame ) 
	{
		// remove this frame
		RemoveAnimationFrame ( pObject, pFrameToRemove );

		// remove any children
		if ( pFrame->pChild ) RemoveAnimationFrameChildRec ( pObject, pFrame->pChild );

		// next frame
		pFrame = pFrame->pSibling;
	}

	// okay
	return true;
}

// Frame Hierarchy Construction Functions

DARKSDK_DLL bool AddNewFrame ( sObject* pObject, sMesh* pMesh, LPSTR pName )
{
	// seek end of frame hierarchy
	sFrame* pLastRootFrame = pObject->pFrame;
	while ( pLastRootFrame->pSibling )
		pLastRootFrame = pLastRootFrame->pSibling;

	// create new frame at end
	pLastRootFrame->pSibling = new sFrame;

	// assign mesh to new frame
	strcpy ( pLastRootFrame->pSibling->szName, pName );
	pLastRootFrame->pSibling->pMesh = pMesh;

	// add frame to animation set if available
	AddNewAnimationFrame ( pObject, pLastRootFrame->pSibling );

	// okay
	return true;
}

DARKSDK_DLL void AttachFrameToList ( sFrame* pFrameToLinkTo, sFrame* pFrameToMove )
{
	// link frame to linkto frame
	if ( pFrameToLinkTo->pChild==NULL )
	{
		// add as child first
		pFrameToLinkTo->pChild = pFrameToMove;
		pFrameToMove->pParent = pFrameToLinkTo;
	}
	else
	{
		// add as a sybling to existing linkto child then
		sFrame* pLastFrame = pFrameToLinkTo->pChild;
		while ( pLastFrame->pSibling )
			pLastFrame = pLastFrame->pSibling;

		// add frame as last sybling of linkto child frame
		pLastFrame->pSibling = pFrameToMove;
		pFrameToMove->pParent = pFrameToLinkTo;
	}
}

DARKSDK_DLL void DetatchFrameFromList ( sFrame* pPrevious, sFrame** ppCurrentBase, sFrame* pFrameToDetatch )
{
	// determine frame info
	sFrame* pParent = pFrameToDetatch->pParent;
	sFrame* pNext = pFrameToDetatch->pSibling;

	// sever frame from higher nodes
	pFrameToDetatch->pSibling=NULL;
	pFrameToDetatch->pParent=NULL;

	// remove from list
	if ( pPrevious==NULL )
	{
		if ( pParent==NULL )
		{
			// new list-base start
			(*ppCurrentBase) = pNext;
		}
		else
		{
			// new parent
			if ( pNext ) pNext->pParent = pParent;
			pParent->pChild = pNext;
		}
	}
	else
	{
		// skip sybling
		pPrevious->pSibling = pNext;
	}
}

DARKSDK_DLL bool DetatchFrameFromListRec ( sObject* pObject, sFrame** ppCurrentBase, sFrame* pFrameToDetatch )
{
	// find frame
	sFrame* pPrevious = NULL;
	sFrame* pCurrent = (*ppCurrentBase);
	while ( pCurrent )
	{
		// determine next frame now
		sFrame* pNext = pCurrent->pSibling;

		// if this frame to be deleted
		if ( pCurrent==pFrameToDetatch )
		{
			// detatch frame from list
			DetatchFrameFromList ( pPrevious, ppCurrentBase, pFrameToDetatch );

			// complete
			return false;
		}

		// if current has a child, recurse function
		if ( pCurrent->pChild )
			if ( !DetatchFrameFromListRec ( pObject, &pCurrent->pChild, pFrameToDetatch ) )
				return false;

		// try next one
		pPrevious = pCurrent;
		pCurrent = pNext;
	}

	// carry on
	return true;
}

DARKSDK_DLL bool RemoveFrame ( sObject* pObject, sFrame* pFrameToRemove )
{
	// remove all animation frames related to pFrameToRemove
	RemoveAnimationFrame ( pObject, pFrameToRemove );
	if ( pFrameToRemove->pChild ) RemoveAnimationFrameChildRec ( pObject, pFrameToRemove->pChild );

	// remove frame from hierarchy
	DetatchFrameFromListRec ( pObject, &pObject->pFrame, pFrameToRemove );

	// delete frame (mesh buffer resources freed before this call)
	delete pFrameToRemove;

	// okay
	return true;
}

DARKSDK_DLL bool LinkFrame ( sObject* pObject, sFrame* pFrameToMove, sFrame* pFrameToLinkTo )
{
	// find parent frame of moving frame
	sFrame** ppBaseFrame = &pObject->pFrame;
	if ( pFrameToMove->pParent ) ppBaseFrame = &pFrameToMove->pParent->pChild;

	// find frame and detatch it from list
	DetatchFrameFromListRec ( pObject, &pObject->pFrame, pFrameToMove );

	// attach to link frame
	AttachFrameToList ( pFrameToLinkTo, pFrameToMove );

	// okay
	return true;
}

DARKSDK_DLL bool ReplaceFrameMesh ( sFrame* pFrame, sMesh* pMesh )
{
	// mesh already exists, delete it
	sMesh* pOldMesh = pFrame->pMesh;
	if ( pOldMesh )
	{
		// delete old mesh
		SAFE_DELETE ( pOldMesh );

		// replace with new mesh
		pFrame->pMesh = pMesh;

		// inform of mesh change
		pFrame->pMesh->bMeshHasBeenReplaced = true;
	}
	else
		return false;

	// okay
	return true;
}

// Frame Bound Meshes

DARKSDK_DLL bool CreateBoundBoxMesh ( sFrame* pFrame )
{
	return true;
}

DARKSDK_DLL bool UpdateBoundBoxMesh ( sFrame* pFrame )
{
	return true;
}

DARKSDK_DLL bool CreateBoundSphereMesh ( sFrame* pFrame )
{
	return true;
}

bool UpdateBoundSphereMesh ( sFrame* pFrame )
{
	return true;
}