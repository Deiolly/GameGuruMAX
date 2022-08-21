#include "DirectXTex.h"
using namespace DirectX;

#include "DBPro Functions.h"
#include "TreeFaceLightmapper.h"
#include <cstdlib>

// Externs
extern LPGGDEVICE				m_pD3D;
extern LPGGIMMEDIATECONTEXT		m_pImmediateContext;

extern HANDLE g_hLMHeap;

bool TreeFaceLightmapper::backFaces = true;
TransparentFace::TextureClass* TransparentFace::pTextureList = 0;

void TreeFaceFreeUsages(void)
{
	// delete global texture list (caled by reset)
	while ( TransparentFace::pTextureList )
	{
		TransparentFace::TextureClass* pTemp = TransparentFace::pTextureList;
		SAFE_DELETE_ARRAY ( pTemp->pSysMemTransTex );
		TransparentFace::pTextureList = TransparentFace::pTextureList->pNextTexture;
		delete pTemp;
	}
	TransparentFace::pTextureList = NULL;
}

bool TreeFaceLightmapper::MakeFace( Point *p1, Point *p2, Point *p3 )
{
	vert1 = *p1;
	vert2 = *p2;
	vert3 = *p3;

	Vector v1(p1,p3);
	Vector v2(p1,p2);

	normal = v1.crossProduct(&v2);
	float length = normal.size();

	//zero area polygons will have an undefined normal, which will cause problems later
	//so if any exist remove them
	if (length<0.00001f) return false;
    
	normal.mult(1.0f/length);

	v1 = normal;
	v2.set(p2->x,p2->y,p2->z);
    
	d = -1.0f*(v1.dotProduct(&v2));

	return true;
}

bool TransparentFace::MakeTransparentFace(Point *p1, Point *p2, Point *p3, int type, float fU1, float fV1, float fU2, float fV2, float fU3, float fV3, sTexture* pSrcTexture)
{
	// find existing texture class in transparent textures list
	TextureClass *pPrevTexture = pTextureList;
	bool bFound = false;
	while ( pPrevTexture )
	{
		if ( pPrevTexture->pTexture==pSrcTexture->pTexturesRef )
		{
			bFound = true;
			break;
		}
		pPrevTexture = pPrevTexture->pNextTexture;
	}
	// if not found existing, we create a new entry
	if ( !bFound && pSrcTexture->pTexturesRef )
	{
		#ifdef DX11
		// is insert a DDS or other
		bool bInsertTextureIsDDS = false;
		if ( strnicmp ( pSrcTexture->pName + strlen(pSrcTexture->pName) - 4, ".dds", 4 ) == NULL )
			bInsertTextureIsDDS = true;

		// load source texture
		HRESULT hRes;
		wchar_t wFilename[512];
		MultiByteToWideChar(CP_ACP, 0, pSrcTexture->pName, -1, wFilename, sizeof(wFilename));
		ScratchImage imageTexture;
		TexMetadata info;
		if ( bInsertTextureIsDDS == true )
		{
			hRes = GetMetadataFromDDSFile( wFilename, DDS_FLAGS_NONE, info );			
			hRes = LoadFromDDSFile( wFilename, DDS_FLAGS_NONE, &info, imageTexture );
		}
		else
		{
			hRes = GetMetadataFromWICFile( wFilename, DDS_FLAGS_NONE, info );			
			hRes = LoadFromWICFile( wFilename, DDS_FLAGS_NONE, &info, imageTexture );
		}

		// uncompress
		ScratchImage* pWrkImage = &imageTexture;
		ScratchImage decompressedTexture;
		if ( info.format >= DXGI_FORMAT_BC1_TYPELESS && info.format <= DXGI_FORMAT_BC5_SNORM )
		{
			hRes = Decompress( imageTexture.GetImages(), imageTexture.GetImageCount(), imageTexture.GetMetadata(),
				DXGIFORMATR8G8B8A8UNORM, decompressedTexture );
			pWrkImage = &decompressedTexture;
		}

		// if not an DXGI_FORMAT_B8G8R8A8_UNORM image, convert it to one (the new(old!) internal format)
		ScratchImage convertedTexture;
		if ( pWrkImage->GetMetadata().format != DXGI_FORMAT_B8G8R8A8_UNORM )
		{
			hRes = Convert ( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(),
				DXGI_FORMAT_B8G8R8A8_UNORM, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, convertedTexture );
			pWrkImage = &convertedTexture;
		}

		// create as texture
		LPGGTEXTURE pLoadedUncompressedImage;
		CreateTexture(m_pD3D, pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), &pLoadedUncompressedImage );

		// copy to a staging texture
		IGGTexture *pTempTexture = NULL;
		GGSURFACE_DESC StagedDesc = { info.width, info.height, 1, 1, pWrkImage->GetMetadata().format, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
		hRes = m_pD3D->CreateTexture2D( &StagedDesc, NULL, (ID3D11Texture2D**)&pTempTexture );
		D3D11_BOX rc = { 0, 0, 0, (LONG)(info.width), (LONG)(info.height), 1 }; 
		m_pImmediateContext->CopySubresourceRegion ( pTempTexture, 0, 0, 0, 0, pLoadedUncompressedImage, 0, &rc );
		SAFE_RELEASE(pLoadedUncompressedImage);

		// record new temp texture and reference
		pPrevTexture = new TextureClass( );
		pPrevTexture->pTexture = pSrcTexture->pTexturesRef;

		// 051214 - as multithead should NOT lock any DX texture (even system memory ones), copy the contents to an array
		pPrevTexture->dwSysMemTransTexWidth = (DWORD)info.width;
		pPrevTexture->dwSysMemTransTexHeight = (DWORD)info.height;
		DWORD dwSysMemTransSize = info.width*info.height;
		pPrevTexture->pSysMemTransTex = new DWORD[dwSysMemTransSize];
		memset ( pPrevTexture->pSysMemTransTex, 0, dwSysMemTransSize*sizeof(DWORD) );

		// copy sys mem tex to array
		D3D11_MAPPED_SUBRESOURCE d3dlr;
		HRESULT hr = m_pImmediateContext->Map( pTempTexture, 0, D3D11_MAP_READ, 0, &d3dlr );
		if ( FAILED( hr ) )
		{
			MessageBox( NULL, "Error Locking Transparent Texture", "Error", 0 ); exit(1);
		}
		DWORD* ppPixels = ( DWORD* ) d3dlr.pData;
		int iPitch  = d3dlr.RowPitch / 4;
		for ( int iY=0; iY<(int)info.height; iY++ )
		{
			for ( int iX=0; iX<(int)info.width; iX++ )
			{
				DWORD dwIndex = iX + ( iY * iPitch );
				DWORD dwColour = *( ppPixels + dwIndex );
				pPrevTexture->pSysMemTransTex[dwIndex] = dwColour;
			}
		}
		m_pImmediateContext->Unmap( pTempTexture, 0 );

		// can release sys mem texture now it's stored in an array
		SAFE_RELEASE ( pTempTexture );
		#else
		// original source texture (diffuse with alpha)
		D3DSURFACE_DESC desc;
		if ( FAILED ( pSrcTexture->pTexturesRef->GetLevelDesc ( 0, &desc ) ) )
		{
			MessageBox( NULL, "Error Getting Transparent Texture Description", "Error", 0 ); exit(1);
		}

		// create a system memory copy of the texture
		GGIMAGE_INFO info;
		IGGTexture *pTempTexture = NULL;
		HRESULT hResI = D3DXGetImageInfoFromFile( pSrcTexture->pName, &info );
		if ( hResI!=S_OK )
		{
			// this can happen with some models (maybe wrong path while lightmapping)
			info.Width = desc.Width;
			info.Height = desc.Height;
		}
		HRESULT hRes2 = D3DXCreateTexture ( m_pD3D,
											desc.Width,
											desc.Height,
											1,
											0,
											GGFMT_A8R8G8B8,
											D3DPOOL_SYSTEMMEM,
											&pTempTexture );

		// copy original file directly into system memory transparency texture (not using vid mem copy)
		LPGGSURFACE pDstSurface;
		pTempTexture->GetSurfaceLevel ( 0, &pDstSurface );
		if ( pDstSurface )
		{
			RECT destrc = { 0, 0, (LONG)info.Width, (LONG)info.Height };
			HRESULT hRes = D3DXLoadSurfaceFromFile( pDstSurface, NULL, &destrc, pSrcTexture->pName, NULL, D3DX_FILTER_POINT, 0, &info );
			pDstSurface->Release();
		}

		// record new temp texture and reference
		pPrevTexture = new TextureClass( );
		pPrevTexture->pTexture = pSrcTexture->pTexturesRef;

		// 051214 - as multithead should NOT lock any DX texture (even system memory ones), copy the contents to an array
		pPrevTexture->dwSysMemTransTexWidth = (DWORD)info.Width;
		pPrevTexture->dwSysMemTransTexHeight = (DWORD)info.Height;
		DWORD dwSysMemTransSize = info.Width*info.Height;
		pPrevTexture->pSysMemTransTex = new DWORD[dwSysMemTransSize];
		memset ( pPrevTexture->pSysMemTransTex, 0, dwSysMemTransSize*sizeof(DWORD) );

		// copy sys mem tex to array
		GGLOCKED_RECT d3dlr;
		if ( FAILED ( pTempTexture->LockRect ( 0, &d3dlr, 0, 0 ) ) )
		{
			MessageBox( NULL, "Error Locking Transparent Texture", "Error", 0 ); exit(1);
		}
		DWORD* ppPixels = ( DWORD* ) d3dlr.pBits;
		int iPitch  = d3dlr.Pitch / 4;
		for ( int iY=0; iY<(int)info.Height; iY++ )
		{
			for ( int iX=0; iX<(int)info.Width; iX++ )
			{
				DWORD dwIndex = iX + ( iY * iPitch );
				DWORD dwColour = *( ppPixels + dwIndex );
				pPrevTexture->pSysMemTransTex[dwIndex] = dwColour;
			}
		}
		if ( FAILED ( pTempTexture->UnlockRect( NULL ) ) )
		{
			MessageBox( NULL, "Error Unlocking Transparent Texture", "Error", 0 ); exit(1);
		}

		// can release sys mem texture now it's stored in an array
		SAFE_RELEASE ( pTempTexture );
		#endif

		// add to texture class list
		pPrevTexture->pNextTexture = pTextureList;
		pTextureList = pPrevTexture;
	}

	// store settings for this face
	u1 = fU1; u2 = fU2; u3 = fU3;
	v1 = fV1; v2 = fV2; v3 = fV3;
	iType = type;
	pTextureClassUsed = pPrevTexture;
	if ( pPrevTexture )
		pTexture = pPrevTexture->pTexture;
	else
		pTexture = NULL;

	return MakeFace( p1, p2, p3 );
}

bool TransparentFace::intersects( const Point* p, const Vector* v, Lumel *pColour, float *pShadow ) const
{
//	char str[256];
	//MessageBox( NULL, "Checking Transparent Polygon", "Info", 0 );
	
	bool bHit = false;
	float dist1,dist2;
    Point intersect;
	//Point *p = &(CollisionTreeLightmapper::p);
	//Vector *v = &(CollisionTreeLightmapper::vec);
                                    
    dist1 = normal.x*(p->x+v->x) + normal.y*(p->y+v->y) + normal.z*(p->z+v->z) + d;
    dist2 = normal.x*p->x + normal.y*p->y + normal.z*p->z + d;
    
    if ( dist2*dist1 > 0 ) return false;
                
    dist1=-(normal.x*v->x) - (normal.y*v->y) - (normal.z*v->z);
    if (dist1<-0.0001 || dist1>0.0001) dist1=dist2/dist1;
    else dist1=0;
    
    intersect.x = p->x + v->x*dist1;
    intersect.y = p->y + v->y*dist1;
    intersect.z = p->z + v->z*dist1;
                
    if ( !pointInPoly(&intersect) ) return false;

	//MessageBox( NULL, "Hit Transparent Polygon", "Info", 0 );

	if ( !pTexture ) 
	{
		if ( pShadow ) *pShadow = 1.0;
		return true;
	}

	float fU=0, fV=0;
	InterpolateUV( &intersect, &fU, &fV );

	//sprintf_s(str, "U1: %3.3f, V1: %3.3f, U2: %3.3f, V2: %3.3f, U3: %3.3f, V3: %3.3f, UI: %3.3f, VI: %3.3f", u1, v1, u2, v2, u3, v3, fU, fV );
	//MessageBox( NULL, str, "Info", 0 );

	if ( fU < 0.0 )			while ( fU < 0.0 ) fU++;
	else if ( fU >= 1.0 )	while ( fU >= 1.0 ) fU--;

	if ( fV < 0.0 )			while ( fV < 0.0 ) fV++;
	else if ( fV >= 1.0 )	while ( fV >= 1.0 ) fV--;

	int iU = (int) floor(fU * (pTextureClassUsed->dwSysMemTransTexWidth-1));
	int iV = (int) floor(fV * (pTextureClassUsed->dwSysMemTransTexHeight-1));
	float fDiffU = (fU*(pTextureClassUsed->dwSysMemTransTexWidth-1)) - iU;
	float fDiffV = (fV*(pTextureClassUsed->dwSysMemTransTexHeight-1)) - iV;

	// get stored DWORDs from tex array (originally from trans texture)
	DWORD dwColour1=0, dwColour2=0, dwColour3=0, dwColour4=0;
	DWORD dwIndex = iU + ( iV * pTextureClassUsed->dwSysMemTransTexWidth );
	dwColour1 = *( pTextureClassUsed->pSysMemTransTex + dwIndex );
	int iU2 = iU;
	if ( iU2 < (int)pTextureClassUsed->dwSysMemTransTexWidth-1 ) iU2++;
	dwIndex = iU2 + ( iV * pTextureClassUsed->dwSysMemTransTexWidth );
	dwColour2 = *( pTextureClassUsed->pSysMemTransTex + dwIndex );
	int iV2 = iV;
	if ( iV2 < (int)pTextureClassUsed->dwSysMemTransTexHeight-1 ) iV2++;
	dwIndex = iU + ( iV2 * pTextureClassUsed->dwSysMemTransTexWidth );
	dwColour3 = *( pTextureClassUsed->pSysMemTransTex + dwIndex );
	dwIndex = iU2 + ( iV2 * pTextureClassUsed->dwSysMemTransTexWidth );
	dwColour4 = *( pTextureClassUsed->pSysMemTransTex + dwIndex );

	/*
	GGLOCKED_RECT d3dlr;
	bool bSuccess = false;
	if ( g_pShared )
	{
		//semaphore
		int counter = 2;

		while( !bSuccess && counter > 0 )
		{
			bSuccess = g_pShared->GrabTextureLockMutex( );
			counter--;
		}

		if ( !bSuccess ) { MessageBox( NULL, "Texture lock timed out", "Error", 0 ); exit(1); }
	}

	if ( FAILED ( pTexture->LockRect ( 0, &d3dlr, 0, 0 ) ) )
	{
		MessageBox( NULL, "Error Locking Transparent Texture", "Error", 0 ); exit(1);
	}

	DWORD* ppPixels = ( DWORD* ) d3dlr.pBits;
	int iPitch  = d3dlr.Pitch / 4;

	DWORD dwIndex = iU + ( iV * iPitch );
	dwColour1 = *( ppPixels + dwIndex );
	
	int iU2 = iU;
	if ( iU2 < iWidth-1 ) iU2++;
	dwIndex = iU2 + ( iV * iPitch );
	dwColour2 = *( ppPixels + dwIndex );
	
	int iV2 = iV;
	if ( iV2 < iHeight-1 ) iV2++;
	dwIndex = iU + ( iV2 * iPitch );
	dwColour3 = *( ppPixels + dwIndex );

	dwIndex = iU2 + ( iV2 * iPitch );
	dwColour4 = *( ppPixels + dwIndex );

	if ( FAILED ( pTexture->UnlockRect( NULL ) ) )
	{
		MessageBox( NULL, "Error Unlocking Transparent Texture", "Error", 0 );
	}

	if ( g_pShared )
	{
		if ( bSuccess ) g_pShared->ReleaseTextureLockMutex( );
	}
	*/

	float fColour[4], fColour1[4], fColour2[4], fColour3[4], fColour4[4];

	for (int i = 0; i < 4; i++ ) 
	{
		fColour1[i] = (float) ((dwColour1 >> ((3-i)*8) ) & 0x000000ff);
		fColour2[i] = (float) ((dwColour2 >> ((3-i)*8) ) & 0x000000ff);
		fColour3[i] = (float) ((dwColour3 >> ((3-i)*8) ) & 0x000000ff);
		fColour4[i] = (float) ((dwColour4 >> ((3-i)*8) ) & 0x000000ff);
	}

	for (int i = 0; i < 4; i++ )
	{
		fColour[i] = ( fColour1[i]*(1-fDiffU) + fColour2[i]*(fDiffU) )*(1-fDiffV) + ( fColour3[i]*(1-fDiffU) + fColour4[i]*(fDiffU) )*(fDiffV);
		fColour[i] /= 255.0f;
	}
	
	if ( iType == 0 )
	{
		//black transparency
		if ( fColour[1] < 0.0001 && fColour[2] < 0.0001 && fColour[3] < 0.0001 ) return false;
		if ( pShadow ) *pShadow = 1.0;
		return true;
	}
	else
	{
		// interferes with transparent intersect detect
		if ( !pColour ) return false;

		//alpha transparency
		float fAlpha	= fColour[0];
		if ( fAlpha > 0.9999f && iType == 1 ) 
		{
			pColour->SetCol( 0,0,0 );
			*pShadow = 1.0f;
			return false;
		}
		float fRed		= fColour[1];
		float fGreen	= fColour[2];
		float fBlue		= fColour[3];
		
		float fFade = 1 - fAlpha*fAlpha;
		if ( iType == 2 ) 
		{
			fFade = 1;
			fAlpha = 1.0;
		}

		fRed	= ( pColour->GetColR( )*(1-fAlpha) + pColour->GetColR( )*fRed*fAlpha );
		fGreen	= ( pColour->GetColG( )*(1-fAlpha) + pColour->GetColG( )*fGreen*fAlpha );
		fBlue	= ( pColour->GetColB( )*(1-fAlpha) + pColour->GetColB( )*fBlue*fAlpha );
		pColour->SetCol( fRed, fGreen, fBlue );
		if ( pShadow ) *pShadow = 1.0f - ( (1.0f-(*pShadow)) * fFade );

		return false;
	}
}

void TransparentFace::InterpolateUV( const Point *p, float *pU, float *pV ) const
{
	float fDiffX = vert1.x - p->x;
	float fDiffY = vert1.y - p->y;
	float fDiffZ = vert1.z - p->z;
	float fDist1 = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;

	fDiffX = vert2.x - p->x;
	fDiffY = vert2.y - p->y;
	fDiffZ = vert2.z - p->z;
	float fDist2 = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;

	fDiffX = vert3.x - p->x;
	fDiffY = vert3.y - p->y;
	fDiffZ = vert3.z - p->z;
	float fDist3 = fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ;

	Point pVertA = vert1;
	Point pVertB = vert2;
	Point pVertC = vert3;
	float fUA = u1;
	float fVA = v1;
	float fUB = u2;
	float fVB = v2;
	float fUC = u3;
	float fVC = v3;

	if ( fDist2 > fDist1 && fDist2 > fDist3 )
	{
		pVertA = vert2; pVertB = vert3; pVertC = vert1;
		fUA = u2; fVA = v2; 
		fUB = u3; fVB = v3; 
		fUC = u1; fVC = v1; 
	}

	if ( fDist3 > fDist1 && fDist3 > fDist2 )
	{
		pVertA = vert3; pVertB = vert1; pVertC = vert2;
		fUA = u3; fVA = v3; 
		fUB = u1; fVB = v1; 
		fUC = u2; fVC = v2; 
	}

	Vector BCNormal;
	fDiffX = pVertC.x-pVertB.x;
	fDiffY = pVertC.y-pVertB.y;
	fDiffZ = pVertC.z-pVertB.z;
	BCNormal.set( fDiffX, fDiffY, fDiffZ );
	BCNormal = BCNormal.crossProduct( &normal );
	//BCNormal.normalise( );

	//fDist1 = BCNormal.x*pVertB.x + BCNormal.y*pVertB.y + BCNormal.z*pVertB.z;
	fDist1 = BCNormal.x*(pVertA.x-pVertB.x) + BCNormal.y*(pVertA.y-pVertB.y) + BCNormal.z*(pVertA.z-pVertB.z);
	fDist2 = (pVertA.x - p->x)*BCNormal.x + (pVertA.y - p->y)*BCNormal.y + (pVertA.z - p->z)*BCNormal.z;
	fDist1 = fDist1 / fDist2;

	Point intersect;
	intersect.x = pVertA.x + (p->x - pVertA.x)*fDist1;
	intersect.y = pVertA.y + (p->y - pVertA.y)*fDist1;
	intersect.z = pVertA.z + (p->z - pVertA.z)*fDist1;

	fDist1 = (intersect.x - pVertB.x)*fDiffX + (intersect.y - pVertB.y)*fDiffY + (intersect.z - pVertB.z)*fDiffZ;
	fDist1 /= ( fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ );

	float fUBC = fUB*(1-fDist1) + fUC*(fDist1);
	float fVBC = fVB*(1-fDist1) + fVC*(fDist1);

	fDiffX = intersect.x - pVertA.x;
	fDiffY = intersect.y - pVertA.y;
	fDiffZ = intersect.z - pVertA.z;
	fDist1 = (p->x - pVertA.x)*fDiffX + (p->y - pVertA.y)*fDiffY + (p->z - pVertA.z)*fDiffZ;
	fDist1 /= ( fDiffX*fDiffX + fDiffY*fDiffY + fDiffZ*fDiffZ );

	*pU = fUA*(1-fDist1) + fUBC*(fDist1);
	*pV = fVA*(1-fDist1) + fVBC*(fDist1);
}

//------------------------------------------TreeNodeLightmapper Class-----------------------------------------
TreeNodeLightmapper::~TreeNodeLightmapper()
{
    if (right==0)
    {
        TreeFaceLightmapper* temp;
        
        while(left!=0)
        {
            temp = (TreeFaceLightmapper*)left;
            left = (TreeNodeLightmapper*)(temp->nextFace);
            delete (TreeFaceLightmapper*)temp;
        }
    }
    else
    {
        delete left;
        delete right;
    }
}
     
void TreeNodeLightmapper::buildTree(TreeFaceLightmapper* faces,unsigned fnum,int facesPerNode)
{
    left=0;
    right=0;
    
    //sprintf_s(str,255,"Creating Node... %d",fnum);
    //MessageBox(NULL, str, "Tree", MB_OK);
    
    //build a bounding box for this node
    float minx = faces->vert1.x;
    float miny = faces->vert1.y;
    float minz = faces->vert1.z;
    float maxx = faces->vert1.x;
    float maxy = faces->vert1.y;
    float maxz = faces->vert1.z;
    
    for(TreeFaceLightmapper* thisFace=faces; thisFace!=0; thisFace = thisFace->nextFace) 
    {
        if (thisFace->vert1.x>maxx) maxx = thisFace->vert1.x;
        if (thisFace->vert1.x<minx) minx = thisFace->vert1.x;
        if (thisFace->vert1.y>maxy) maxy = thisFace->vert1.y;
        if (thisFace->vert1.y<miny) miny = thisFace->vert1.y;
        if (thisFace->vert1.z>maxz) maxz = thisFace->vert1.z;
        if (thisFace->vert1.z<minz) minz = thisFace->vert1.z;
        
        if (thisFace->vert2.x>maxx) maxx = thisFace->vert2.x;
        if (thisFace->vert2.x<minx) minx = thisFace->vert2.x;
        if (thisFace->vert2.y>maxy) maxy = thisFace->vert2.y;
        if (thisFace->vert2.y<miny) miny = thisFace->vert2.y;
        if (thisFace->vert2.z>maxz) maxz = thisFace->vert2.z;
        if (thisFace->vert2.z<minz) minz = thisFace->vert2.z;
        
        if (thisFace->vert3.x>maxx) maxx = thisFace->vert3.x;
        if (thisFace->vert3.x<minx) minx = thisFace->vert3.x;
        if (thisFace->vert3.y>maxy) maxy = thisFace->vert3.y;
        if (thisFace->vert3.y<miny) miny = thisFace->vert3.y;
        if (thisFace->vert3.z>maxz) maxz = thisFace->vert3.z;
        if (thisFace->vert3.z<minz) minz = thisFace->vert3.z;
    }
    
    bounds.set(minx,miny,minz,maxx,maxy,maxz);
    bounds.correctBox();
    
    //sprintf_s(str,255,"Building Tree... %5.3f,%5.3f,%5.3f,%5.3f,%5.3f,%5.3f",maxx,maxy,maxz,minx,miny,minz);
    //MessageBox(NULL, str, "Intersect Object", MB_OK);
    
    //if this is a leaf node then place the linked list into this node's pointer
    if ((int)fnum<=facesPerNode)
    {
        left = (TreeNodeLightmapper*)faces;
        return;
    }
    
    //otherwise divide the linked list into two new lists
    float difx = maxx-minx;
    float dify = maxy-miny;
    float difz = maxz-minz;
    
    //x=0, y=1, z=2
    //axis is biggest extent, axis1 is second, axis2 is smallest
    int axis = 0;
    int axis1 = 0;
    int axis2 = 0;
    
    if (difx>dify)
    {
        if (difx<difz) { axis = 2; axis1 = 0; axis2 = 1; }
        else
        {
            axis = 0;
            if (dify>difz) { axis1 = 1; axis2 = 2; }
            else { axis1 = 2; axis2 = 1; }
        } 
    }
    else
    {
        if (dify<difz) { axis = 2; axis1 = 1; axis2 = 0; }
        else
        {
            axis = 1;
            if (difx>difz) { axis1 = 0; axis2 = 2; }
            else { axis1 = 2; axis2 = 0; }
        }
    }
    //axes stored for later when handling unequal splits
    
    float limit = 0;
    TreeFaceLightmapper* leftFaces = 0; //new linked list 1
    TreeFaceLightmapper* rightFaces = 0; //new linked list 2
    TreeFaceLightmapper* lastFace = 0; //points to the last element in the new leftFaces list, helps reconstruct the whole list
    TreeFaceLightmapper* temp = 0;
    unsigned fnum1=0, fnum2=0;
    
    //limit for deciding which side of the split a triangle belongs                                                                            
    switch(axis)
    {
        case 0 : limit=difx/2.0f + minx;break;
        case 1 : limit=dify/2.0f + miny;break;
        case 2 : limit=difz/2.0f + minz;break;
    }
    
    float avg;
    float total = 0;
            
    while (faces>0)
    {
        //get the triangles average distance along this axis and compare with the limit
        switch(axis)
        {
            case 0 : avg = (faces->vert1.x + faces->vert2.x + faces->vert3.x)/3.0f;break;
            case 1 : avg = (faces->vert1.y + faces->vert2.y + faces->vert3.y)/3.0f;break;
            case 2 : avg = (faces->vert1.z + faces->vert2.z + faces->vert3.z)/3.0f;break;
        }
            
        total += avg;
                
        if (avg>=limit)
        {
            if (fnum1==0) lastFace = faces;
            
            //pull the first item from the @faces list and attach to the front of @leftFaces
            temp = faces->nextFace;
            faces->nextFace = leftFaces;
            leftFaces = faces;
            faces = temp;
            
            fnum1++;
        }
        else
        {
            temp = faces->nextFace;
            faces->nextFace = rightFaces;
            rightFaces = faces;
            faces = temp;
            
            fnum2++;
        }
    }
    
    //sprintf_s(str,255,"Attempt 1: %d, %d, %d, %d",fnum,fnum1,fnum2,axis);
    //MessageBox(NULL, str, "Debug", MB_OK);
    
    //check for an uneven split - more than 90% of the triangles on one side
    //if so then try changing axis
    if (abs((int) (fnum1-fnum2))>fnum*0.8)
    {
        fnum1=0;
        fnum2=0;
        limit=0;
        
        //check the second largest axis
        switch(axis1)
        {
            case 0 : limit=difx/2.0f + minx;break;
            case 1 : limit=dify/2.0f + miny;break;
            case 2 : limit=difz/2.0f + minz;break;
        }
        
        //reconstruct the list, if none in @leftFaces, already done in @rightFaces
        //otherwise use @lastFace to append the two together
        if (leftFaces)
        {
            faces = leftFaces;
            lastFace->nextFace = rightFaces;
        }
        else faces = rightFaces;
        
        leftFaces = 0;
        rightFaces = 0;
        
        while (faces>0)
        {
            switch(axis1)
            {
                case 0 : avg = (faces->vert1.x + faces->vert2.x + faces->vert3.x)/3.0f;break;
                case 1 : avg = (faces->vert1.y + faces->vert2.y + faces->vert3.y)/3.0f;break;
                case 2 : avg = (faces->vert1.z + faces->vert2.z + faces->vert3.z)/3.0f;break;
            }
                
            if (avg>=limit)
            {
                if (fnum1==0) lastFace = faces;
                
                temp = faces->nextFace;
                faces->nextFace = leftFaces;
                leftFaces = faces;
                faces = temp;
                
                fnum1++;
            }
            else
            {
                temp = faces->nextFace;
                faces->nextFace = rightFaces;
                rightFaces = faces;
                faces = temp;
                
                fnum2++;
            }
        }
        
        //sprintf_s(str,255,"Attempt 2: %d, %d, %d, %d",fnum,fnum1,fnum2,axis1);
        //MessageBox(NULL, str, "Debug", MB_OK);
    }
    
    //if we still don't get a decent split try the final axis
    if (abs((int) (fnum1-fnum2))>fnum*0.8)
    {
        fnum1=0;
        fnum2=0;
        
        //check the third largest axis
        switch(axis2)
        {
            case 0 : limit=difx/2.0f + minx;break;
            case 1 : limit=dify/2.0f + miny;break;
            case 2 : limit=difz/2.0f + minz;break;
        }
        
        if (leftFaces)
        {
            faces = leftFaces;
            lastFace->nextFace = rightFaces;
        }
        else faces = rightFaces;
        
        leftFaces = 0;
        rightFaces = 0;
        
        while (faces>0)
        {
            switch(axis2)
            {
                case 0 : avg = (faces->vert1.x + faces->vert2.x + faces->vert3.x)/3.0f;break;
                case 1 : avg = (faces->vert1.y + faces->vert2.y + faces->vert3.y)/3.0f;break;
                case 2 : avg = (faces->vert1.z + faces->vert2.z + faces->vert3.z)/3.0f;break;
            }
                
            if (avg>=limit)
            {
                if (fnum1==0) lastFace = faces;
                
                temp = faces->nextFace;
                faces->nextFace = leftFaces;
                leftFaces = faces;
                faces = temp;
                
                fnum1++;
            }
            else
            {
                temp = faces->nextFace;
                faces->nextFace = rightFaces;
                rightFaces = faces;
                faces = temp;
                
                fnum2++;
            }
        }
        
        //sprintf_s(str,255,"Attempt 3: %d, %d, %d, %d",fnum,fnum1,fnum2,axis2);
        //MessageBox(NULL, str, "Debug", MB_OK);
    }
    
    //if we get another uneven split then split based on the average distance
    //along the original axis
    if (abs((int) (fnum1-fnum2))>fnum*0.8)
    {
        fnum1=0;
        fnum2=0;
        limit = total/fnum;
        
        if (leftFaces)
        {
            faces = leftFaces;
            lastFace->nextFace = rightFaces;
        }
        else faces = rightFaces;
        
        leftFaces = 0;
        rightFaces = 0;
        
        while (faces>0)
        {
            switch(axis)
            {
                case 0 : avg = (faces->vert1.x + faces->vert2.x + faces->vert3.x)/3.0f;break;
                case 1 : avg = (faces->vert1.y + faces->vert2.y + faces->vert3.y)/3.0f;break;
                case 2 : avg = (faces->vert1.z + faces->vert2.z + faces->vert3.z)/3.0f;break;
            }
                
            if (avg>=limit)
            {
                if (fnum1==0) lastFace = faces;
                
                temp = faces->nextFace;
                faces->nextFace = leftFaces;
                leftFaces = faces;
                faces = temp;
                
                fnum1++;
            }
            else
            {
                temp = faces->nextFace;
                faces->nextFace = rightFaces;
                rightFaces = faces;
                faces = temp;
                
                fnum2++;
            }
        }
        
        //sprintf_s(str,255,"Attempt 4: %d, %d, %d, %d",fnum,fnum1,fnum2,axis);
        //MessageBox(NULL, str, "Debug", MB_OK);
    }
                
    //If this still doesn't produce a decent split, split them randomly
    if (abs((int) (fnum1-fnum2))>fnum*0.9)
    {
        fnum1=0;
        fnum2=0;
        
        if (leftFaces)
        {
            faces = leftFaces;
            lastFace->nextFace = rightFaces;
        }
        else faces = rightFaces;
        
        leftFaces = 0;
        rightFaces = 0;
        
        int flag = 1;
        
        while (faces>0)
        {
            if (flag)
            {
                temp = faces->nextFace;
                faces->nextFace = leftFaces;
                leftFaces = faces;
                faces = temp;
                
                fnum1++;
                flag = 0;
            }
            else
            {
                temp = faces->nextFace;
                faces->nextFace = rightFaces;
                rightFaces = faces;
                faces = temp;
                
                fnum2++;
                flag = 1;
            }
        }
    }
    
	left = new TreeNodeLightmapper();
	right = new TreeNodeLightmapper();
    left->buildTree(leftFaces,fnum1,facesPerNode);
    right->buildTree(rightFaces,fnum2,facesPerNode);
}

bool TreeNodeLightmapper::intersects( const Point* p, const Vector* vec, const Vector* vecI, Lumel* pColour, float* pShadow, TreeFaceLightmapper** ppLastHit) const
{
	if (bounds.intersectBox(p,vecI)==0) return false;
    
    //if not a leaf node then simply pass ray details to the two child nodes
    if (right!=0)
    {
        if ( left->intersects(p,vec,vecI,pColour,pShadow,ppLastHit) ) return true;
        if ( right->intersects(p,vec,vecI,pColour,pShadow,ppLastHit) ) 
		{
			//optimize tree based on last collision, not compatible with multithreading!
			//TreeNodeLightmapper* pTemp = left;
			//left = right;
			//right = pTemp;
			return true;
		}
    }
    else
    {
        //else cycle through all the faces in it
        for(TreeFaceLightmapper* thisFace=(TreeFaceLightmapper*)left; thisFace!=0; thisFace=thisFace->nextFace)
        {
			if (thisFace->intersects(p,vec,pColour,pShadow))
			{ 
				if ( ppLastHit ) *ppLastHit = (TreeFaceLightmapper*)left; 
				return true; 
			}
        }
    }
                        
    return false; 
}
