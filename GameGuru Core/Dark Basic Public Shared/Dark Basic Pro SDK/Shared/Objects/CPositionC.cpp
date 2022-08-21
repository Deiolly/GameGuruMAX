
//
// Position Functions Implementation
//

//////////////////////////////////////////////////////////////////////////////////
// INCLUDE COMMON ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#include ".\..\Objects\CommonC.h"

//////////////////////////////////////////////////////////////////////////////////
// INTERNAL MANAGEMENT FUNCTIONS /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK float ObjectPositionX ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];

	return pObject->position.vecPosition.x;
}

DARKSDK float ObjectPositionY ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecPosition.y;
}

DARKSDK float ObjectPositionZ ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecPosition.z;
}

DARKSDK float ObjectAngleX ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.x;
}

DARKSDK float ObjectAngleY ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.y;
}

DARKSDK float ObjectAngleZ ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0.0f;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.z;
}

DARKSDK GGVECTOR3 GetPosVec ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return GGVECTOR3(0,0,0);

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecPosition;
}

DARKSDK GGVECTOR3 GetRotVec ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return GGVECTOR3(0,0,0);

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate;
}

//////////////////////////////////////////////////////////////////////////////////
// POSITION COMMANDS /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


DARKSDK void ParticleUpdateObject(int iID, float scale, float pfX, float pfY, float pfZ, float cfX, float cfY, float cfZ)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// scale down
	scale /= 100.0f;

	// set the new scale value
	sObject* pObject = g_ObjectList[iID];
	if (!pObject) return;
	pObject->position.vecScale = GGVECTOR3(scale, scale, scale);

	float fBiggestScale = scale;
	pObject->collision.fScaledRadius = pObject->collision.fRadius * fBiggestScale;
	if (pObject->collision.fScaledLargestRadius > 0.0f) pObject->collision.fScaledLargestRadius = pObject->collision.fLargestRadius * fBiggestScale;

	//#### Position ####
	if (pObject->dwObjectNumber == iID)
	{
		pObject->position.vecLastPosition = pObject->position.vecPosition;
		pObject->position.vecPosition = GGVECTOR3(pfX, pfY, pfZ);
	}

	//#### LookAt ####
	CheckRotationConversion(pObject, false);
	GetAngleFromPoint(pObject->position.vecPosition.x, pObject->position.vecPosition.y, pObject->position.vecPosition.z,
		cfX, cfY, cfZ, &pObject->position.vecRotate.x, &pObject->position.vecRotate.y, &pObject->position.vecRotate.z);
	RegenerateLookVectors(pObject);


	pObject->collision.bHasBeenMovedForResponse = true;
	pObject->collision.bColCenterUpdated = false;
	CalcObjectWorld(pObject);
}

#ifdef WICKEDENGINE
DARKSDK void SetFixedScale(int iID, float fX, float fY, float fZ)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// scale down
	fX /= 100.0f;
	fY /= 100.0f;
	fZ /= 100.0f;

	// set the new scale value
	sObject* pObject = g_ObjectList[iID];
	pObject->vecFixedSize = GGVECTOR3(fX, fY, fZ);
	pObject->bUseFixedSize = true;
}
DARKSDK void RemoveFixedScale(int iID)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// set the new scale value
	sObject* pObject = g_ObjectList[iID];
	pObject->bUseFixedSize = false;
}
#endif

DARKSDK void ScaleObject ( int iID, float fX, float fY, float fZ )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// scale down
	fX /= 100.0f;
	fY /= 100.0f;
	fZ /= 100.0f;

	// set the new scale value
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecScale = GGVECTOR3 ( fX, fY, fZ );

	// repcalculate scaled radius for visibility culling
	float fBiggestScale = fX;
	if ( fY > fBiggestScale ) fBiggestScale = fY;
	if ( fZ > fBiggestScale ) fBiggestScale = fZ;
	pObject->collision.fScaledRadius = pObject->collision.fRadius * fBiggestScale;

	// leefix - 010306 - u60 - this is cumilative
	if ( pObject->collision.fScaledLargestRadius > 0.0f )
	{
		pObject->collision.fScaledLargestRadius = pObject->collision.fLargestRadius * fBiggestScale;
	}

	// lee - 060406 - u6rc6 - shadow casters have a larger visual cull radius
	if ( pObject->bCastsAShadow==true )
	{
		// increase largest range to encompass possible shadow
		if ( pObject->collision.fScaledLargestRadius > 0.0f ) 
		{
			pObject->collision.fScaledLargestRadius = pObject->collision.fLargestRadius;
			pObject->collision.fScaledLargestRadius += 3000.0f;
		}
	}

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void PositionObject  ( int iID, float fX, float fY, float fZ, int iUpdateAbsWorldImmediately )
{
	// MessageBox ( NULL, "DX10", "", MB_OK );
}

DARKSDK void PositionObject ( int iID, float fX, float fY, float fZ )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// object position setting
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject )
	{
		if ( pObject->dwObjectNumber == iID )
		{

#ifdef WICKEDENGINE
			//PE: Optimizing we are hitting TransformComponent::GetLocalMatrix() so only do this if position changed.
			pObject->position.vecLastPosition = pObject->position.vecPosition;
			GGVECTOR3 vecNewPosition = GGVECTOR3(fX, fY, fZ);
			if (pObject->position.vecLastPosition != vecNewPosition)
			{
				pObject->position.vecPosition = vecNewPosition;

				// also triggers move-response-flag
				pObject->collision.bHasBeenMovedForResponse = true;

				// leefix - 011103 - proper world recalc (with glue)
				CalcObjectWorld(pObject);
			}
#else
			pObject->position.vecLastPosition = pObject->position.vecPosition;
			pObject->position.vecPosition = GGVECTOR3 ( fX, fY, fZ );

			// also triggers move-response-flag
			pObject->collision.bHasBeenMovedForResponse=true;

			// leefix - 011103 - proper world recalc (with glue)
			CalcObjectWorld ( pObject );
#endif
		}
	}
}

DARKSDK void RotateLimbQuat  (int iID, int iLimbID, float fX, float fY, float fZ, float fQX, float fQY, float fQZ, float fQW)
{
	// check the object exists
	if (!ConfirmObjectInstance (iID))
		return;

	// rotation limb
	sObject* pObject = g_ObjectList[iID];
	if (pObject)
	{
		sFrame* pFrame = pObject->ppFrameList[iLimbID];
		if (pFrame)
		{
			// convert quats to euler
			GGQUATERNION quatRot;
			quatRot.x = fQX;
			quatRot.y = fQY;
			quatRot.z = fQZ;
			quatRot.w = fQW;
			GGMATRIX matRot;
			GGMatrixRotationQuaternion(&matRot, &quatRot);
			AnglesFromMatrix (&matRot, &pFrame->vecDirection);

			#ifdef WICKEDENGINE
			// extra prtion of data to retain the quat so we can use it as an initial frame pose
			// for when we do ragdoll which needs the initial frame (yuck yuck - Paul forgive me) ;)
			pFrame->matOriginal._11 = fQX;
			pFrame->matOriginal._12 = fQY;
			pFrame->matOriginal._13 = fQZ;
			pFrame->matOriginal._14 = fQW;
			pFrame->matOriginal._21 = fX;
			pFrame->matOriginal._22 = fY;
			pFrame->matOriginal._23 = fZ;
			#endif
		}
	}
}

DARKSDK void RotateObjectQuat(int iID, float fX, float fY, float fZ, float fW)
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];
	if (pObject)
	{
		// convert quats to euler
		GGQUATERNION quatRot;
		quatRot.x = fX;
		quatRot.y = fY;
		quatRot.z = fZ;
		quatRot.w = fW;
		GGMATRIX matRot;
		GGMatrixRotationQuaternion(&matRot, &quatRot);
	    AnglesFromMatrix ( &matRot, &pObject->position.vecRotate );

		// regenerate look vectors
		RegenerateLookVectors(pObject);

		// also triggers move-response-flag
		pObject->collision.bHasBeenMovedForResponse = true;
		pObject->collision.bColCenterUpdated = false;

		// leefix - 011103 - proper world recalc (with glue)
		CalcObjectWorld(pObject);
	}
}

DARKSDK void RotateObject ( int iID, float fX, float fY, float fZ )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];
	if ( pObject )
	{
		if ( pObject->dwObjectNumber == iID )
		{
			// check for rotation conversion
			CheckRotationConversion( pObject, false );

			// object rotation setting
			pObject->position.vecRotate = GGVECTOR3 ( fX, fY, fZ );

			// regenerate look vectors
			RegenerateLookVectors( pObject );

			// also triggers move-response-flag
			pObject->collision.bHasBeenMovedForResponse=true;
			pObject->collision.bColCenterUpdated=false;

			// leefix - 011103 - proper world recalc (with glue)
			CalcObjectWorld ( pObject );
		}
	}
}

DARKSDK void XRotateObject ( int iID, float fX )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, false );

	// object rotation setting
	pObject->position.vecRotate.x = fX;

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void YRotateObject ( int iID, float fY )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, false );

	// object rotation setting
	pObject->position.vecRotate.y = fY;

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void ZRotateObject ( int iID, float fZ )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, false );

	// object rotation setting
	pObject->position.vecRotate.z = fZ;

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void PointObject ( int iID, float fX, float fY, float fZ )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, false );

	// object rotation setting
	GetAngleFromPoint ( pObject->position.vecPosition.x, pObject->position.vecPosition.y, pObject->position.vecPosition.z,
						fX, fY, fZ, &pObject->position.vecRotate.x, &pObject->position.vecRotate.y, &pObject->position.vecRotate.z);

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void MoveObject ( int iID, float fStep )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// move object position
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecLastPosition = pObject->position.vecPosition;
	pObject->position.vecPosition += ( fStep * pObject->position.vecLook );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void MoveObjectLeft ( int iID, float fStep )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// move object position
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecPosition -= ( fStep * pObject->position.vecRight );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void MoveObjectRight ( int iID, float fStep )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// move object position
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecPosition += ( fStep * pObject->position.vecRight );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void MoveObjectUp ( int iID, float fStep )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// move object position
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecPosition += ( fStep * pObject->position.vecUp );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void MoveObjectDown ( int iID, float fStep )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// move object position
	sObject* pObject = g_ObjectList [ iID ];
	pObject->position.vecPosition -= ( fStep * pObject->position.vecUp );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void TurnObjectLeft ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( -fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void TurnObjectRight ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationY ( &matRotation, GGToRadian ( fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void TurnObjectRightWorld(int iID, float fAngle)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// rotation object
	sObject* pObject = g_ObjectList[iID];

	// check for rotation conversion
	CheckRotationConversion(pObject, true);

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationY(&matRotation, GGToRadian(fAngle));
	pObject->position.matFreeFlightRotate = pObject->position.matFreeFlightRotate * matRotation;

	AnglesFromMatrix(&pObject->position.matFreeFlightRotate, &pObject->position.vecRotate);

	// regenerate look vectors
	RegenerateLookVectors(pObject);

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse = true;
	pObject->collision.bColCenterUpdated = false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld(pObject);
}

DARKSDK void PitchObjectUp ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( -fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void PitchObjectUpWorld(int iID, float fAngle)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// rotation object
	sObject* pObject = g_ObjectList[iID];

	// check for rotation conversion
	CheckRotationConversion(pObject, true);

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationX(&matRotation, GGToRadian(-fAngle));
	pObject->position.matFreeFlightRotate = pObject->position.matFreeFlightRotate * matRotation;

	AnglesFromMatrix(&pObject->position.matFreeFlightRotate, &pObject->position.vecRotate);

	// regenerate look vectors
	RegenerateLookVectors(pObject);

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse = true;
	pObject->collision.bColCenterUpdated = false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld(pObject);
}

DARKSDK void PitchObjectDown ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationX ( &matRotation, GGToRadian ( fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void PitchObjectDownWorld(int iID, float fAngle)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// rotation object
	sObject* pObject = g_ObjectList[iID];

	// check for rotation conversion
	CheckRotationConversion(pObject, true);

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationX(&matRotation, GGToRadian(fAngle));
	pObject->position.matFreeFlightRotate = pObject->position.matFreeFlightRotate * matRotation;

	AnglesFromMatrix(&pObject->position.matFreeFlightRotate, &pObject->position.vecRotate);

	// regenerate look vectors
	RegenerateLookVectors(pObject);

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse = true;
	pObject->collision.bColCenterUpdated = false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld(pObject);
}

DARKSDK void RollObjectLeft ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

DARKSDK void RollObjectLeftWorld(int iID, float fAngle)
{
	// check the object exists
	if (!ConfirmObjectInstance(iID))
		return;

	// rotation object
	sObject* pObject = g_ObjectList[iID];

	// check for rotation conversion
	CheckRotationConversion(pObject, true);

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationZ(&matRotation, GGToRadian(fAngle));
	pObject->position.matFreeFlightRotate = pObject->position.matFreeFlightRotate * matRotation;

	AnglesFromMatrix(&pObject->position.matFreeFlightRotate, &pObject->position.vecRotate);

	// regenerate look vectors
	RegenerateLookVectors(pObject);

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse = true;
	pObject->collision.bColCenterUpdated = false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld(pObject);
}

DARKSDK void RollObjectRight ( int iID, float fAngle )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return;

	// rotation object
	sObject* pObject = g_ObjectList [ iID ];

	// check for rotation conversion
	CheckRotationConversion( pObject, true );

	// object rotation setting
	GGMATRIX matRotation;
	GGMatrixRotationZ ( &matRotation, GGToRadian ( -fAngle ) );
	pObject->position.matFreeFlightRotate = matRotation * pObject->position.matFreeFlightRotate;

    AnglesFromMatrix ( &pObject->position.matFreeFlightRotate, &pObject->position.vecRotate );

	// regenerate look vectors
	RegenerateLookVectors( pObject );

	// also triggers move-response-flag
	pObject->collision.bHasBeenMovedForResponse=true;
	pObject->collision.bColCenterUpdated=false;

	// leefix - 011103 - proper world recalc (with glue)
	CalcObjectWorld ( pObject );
}

//////////////////////////////////////////////////////////////////////////////////
// POSITION EXPRESSIONS //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

DARKSDK DWORD GetObjectX ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];

	#ifdef DARKSDK_COMPILE
		if ( pObject->position.bGlued == true )
		{
			sObject* pObjectGlueTo = g_ObjectList [ pObject->position.iGluedToObj ];

			CalcObjectWorld ( pObjectGlueTo );
			CalcObjectWorld ( pObject );

			return pObject->position.matWorld._41;
		}
	#endif

	return pObject->position.vecPosition.x;
}

DARKSDK DWORD GetObjectY ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];

	#ifdef DARKSDK_COMPILE
		if ( pObject->position.bGlued == true )
		{
			sObject* pObjectGlueTo = g_ObjectList [ pObject->position.iGluedToObj ];

			CalcObjectWorld ( pObjectGlueTo );
			CalcObjectWorld ( pObject );

			return pObject->position.matWorld._42;
		}
	#endif


	return pObject->position.vecPosition.y;
}

DARKSDK DWORD GetObjectZ ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];

	#ifdef DARKSDK_COMPILE
		if ( pObject->position.bGlued == true )
		{
			sObject* pObjectGlueTo = g_ObjectList [ pObject->position.iGluedToObj ];

			CalcObjectWorld ( pObjectGlueTo );
			CalcObjectWorld ( pObject );

			return pObject->position.matWorld._43;
		}
	#endif

	return pObject->position.vecPosition.z;
}

DARKSDK DWORD GetObjectAngleX ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.x;
}

DARKSDK DWORD GetObjectAngleY ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.y;
}

DARKSDK DWORD GetObjectAngleZ ( int iID )
{
	// check the object exists
	if ( !ConfirmObjectInstance ( iID ) )
		return 0;

	// object information
	sObject* pObject = g_ObjectList [ iID ];
	return pObject->position.vecRotate.z;
}

///////////////////////

#ifdef DARKSDK_COMPILE

float dbtObjectGetXPosition ( int iID )
{
	return GetXPosition ( iID );
}

float dbtObjectGetYPosition ( int iID )
{
	return GetYPosition ( iID );
}

float dbtObjectGetZPosition ( int iID )
{
	return GetZPosition ( iID );
}

float dbtObjectGetXRotation ( int iID )
{
	return GetXRotation ( iID );
}

float dbtObjectGetYRotation ( int iID )
{
	return GetYRotation ( iID );
}

float dbtObjectGetZRotation ( int iID )
{
	return GetZRotation ( iID );
}

void dbtObjectScale ( int iID, float fX, float fY, float fZ )
{
	Scale ( iID, fX, fY, fZ );
}

void dbtObjectPosition ( int iID, float fX, float fY, float fZ )
{
	Position ( iID, fX, fY, fZ );
}

void dbtObjectRotate ( int iID, float fX, float fY, float fZ )
{
	Rotate ( iID, fX, fY, fZ );
}

void dbtObjectXRotate ( int iID, float fX )
{
	XRotate ( iID, fX );
}

void dbtObjectYRotate ( int iID, float fY )
{
	YRotate ( iID, fY );
}

void dbtObjectZRotate ( int iID, float fZ )
{
	ZRotate ( iID, fZ );
}

void dbtObjectPoint ( int iID, float fX, float fY, float fZ )
{
	Point ( iID, fX, fY, fZ );
}

void dbtObjectMove ( int iID, float fStep )
{
	Move ( iID, fStep );
}

void dbtObjectMoveUp ( int iID, float fStep )
{
	MoveUp ( iID, fStep );
}

void dbtObjectMoveDown ( int iID, float fStep )
{
	MoveDown ( iID, fStep );
}

void dbtObjectMoveLeft ( int iID, float fStep )
{
	MoveLeft ( iID, fStep );
}

void dbtObjectMoveRight ( int iID, float fStep )
{
	MoveRight ( iID, fStep );
}

void dbtObjectTurnLeft ( int iID, float fAngle )
{
	TurnLeft ( iID, fAngle );
}

void dbtObjectTurnRight ( int iID, float fAngle )
{
	TurnRight ( iID, fAngle );
}

void dbtObjectPitchUp ( int iID, float fAngle )
{
	PitchUp ( iID, fAngle );
}

void dbtObjectPitchDown ( int iID, float fAngle )
{
	PitchDown ( iID, fAngle );
}

void dbtObjectRollLeft ( int iID, float fAngle )
{
	RollLeft ( iID, fAngle );
}

void dbtObjectRollRight ( int iID, float fAngle )
{
	RollRight ( iID, fAngle );
}

DWORD dbtObjectGetXPositionEx ( int iID )
{
	return GetXPositionEx ( iID );
}

DWORD dbtObjectGetYPositionEx ( int iID )
{
	return GetYPositionEx ( iID );
}

DWORD dbtObjectGetZPositionEx ( int iID )
{
	return GetZPositionEx ( iID );
}

DWORD dbtObjectGetXRotationEx ( int iID )
{
	return GetXRotationEx ( iID );
}

DWORD dbtObjectGetYRotationEx ( int iID )
{
	return GetYRotationEx ( iID );
}

DWORD dbtObjectGetZRotationEx ( int iID )
{
	return GetZRotationEx ( iID );
}

#endif
