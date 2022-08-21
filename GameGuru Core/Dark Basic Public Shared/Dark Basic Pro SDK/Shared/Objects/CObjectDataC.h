
//
// Object Data Structures Header
//

#ifndef _COBJECTDATA_H_
#define _COBJECTDATA_H_

//////////////////////////////////////////////////////////////////////////////////
// INCLUDE DBO FORMAT  ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#include ".\..\DBOFormat\DBOFormat.h"
#include ".\..\DBOFormat\DBOFrame.h"
#include ".\..\DBOFormat\DBOMesh.h"
#include ".\..\DBOFormat\DBORawMesh.h"
#include ".\..\DBOFormat\DBOEffects.h"
#include ".\..\DBOFormat\DBOFile.h"
#include ".\..\DBOFormat\DBOBlock.h"

//////////////////////////////////////////////////////////////////////////////////
// STRUCTURES ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

enum enumScalingMode
{
	eScalingMode_Off,
	eScalingMode_Meter,
	eScalingMode_Inch,
	eScalingMode_Centimeter,
	eScalingMode_Automatic
};

enum eObjectFormat
{
	eMDL,			// mdl ( half life )
	e3DS,			// 3ds ( 3d studio max )
	eNormalX,		// original x format
	eX,				// x ( direct x format )
	eMD2,			// md2 ( quake 2 )
	eMD3,			// md3 ( quake 3 )
	ePrimitives,	// internal objects e.g. triangle
};

enum CULLSTATE
{
    CS_UNKNOWN,      // cull state not yet computed
    CS_INSIDE,       // object bounding box is at least partly inside the frustum
    CS_OUTSIDE,      // object bounding box is outside the frustum
    CS_INSIDE_SLOW,  // OBB is inside frustum, but it took extensive testing to determine this
    CS_OUTSIDE_SLOW, // OBB is outside frustum, but it took extensive testing to determine this
};

struct CULLINFO
{
    GGVECTOR3 vecFrustum   [ 8 ];    // corners of the view frustum
    GGPLANE   planeFrustum [ 6 ];    // planes of the view frustum
};

class cCullData
{
	public:
		GGVECTOR3  m_vecBoundsLocal[8];			// bounding box coordinates (in local coord space)
		GGVECTOR3  m_vecBoundsWorld[8];			// bounding box coordinates (in world coord space)
		GGPLANE    m_planeBoundsWorld[6];			// bounding box planes (in world coord space)
		CULLSTATE    m_cullstate;					// whether object is in the view frustum

	public:
		cCullData ( )
		{
			m_cullstate = CS_UNKNOWN;
		}

		void UpdateMatrix ( GGMATRIX m_mat )
		{
			// Transform bounding box coords from local space to world space
			for( int i = 0; i < 8; i++ ) GGVec3TransformCoord( &m_vecBoundsWorld[i], &m_vecBoundsLocal[i], &m_mat );

			// Determine planes of the bounding box
			GGPlaneFromPoints( &m_planeBoundsWorld[0], &m_vecBoundsWorld[0], &m_vecBoundsWorld[1], &m_vecBoundsWorld[2] ); // Near
			GGPlaneFromPoints( &m_planeBoundsWorld[1], &m_vecBoundsWorld[6], &m_vecBoundsWorld[7], &m_vecBoundsWorld[5] ); // Far
			GGPlaneFromPoints( &m_planeBoundsWorld[2], &m_vecBoundsWorld[2], &m_vecBoundsWorld[6], &m_vecBoundsWorld[4] ); // Left
			GGPlaneFromPoints( &m_planeBoundsWorld[3], &m_vecBoundsWorld[7], &m_vecBoundsWorld[3], &m_vecBoundsWorld[5] ); // Right
			GGPlaneFromPoints( &m_planeBoundsWorld[4], &m_vecBoundsWorld[2], &m_vecBoundsWorld[3], &m_vecBoundsWorld[6] ); // Top
			GGPlaneFromPoints( &m_planeBoundsWorld[5], &m_vecBoundsWorld[1], &m_vecBoundsWorld[0], &m_vecBoundsWorld[4] ); // Bottom
		}
};

#endif _COBJECTDATA_H_