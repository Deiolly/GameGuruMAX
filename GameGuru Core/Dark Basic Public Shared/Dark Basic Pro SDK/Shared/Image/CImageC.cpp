// INCLUDES / LIBS ///////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

//#include <DDSTextureLoader.h>
#include "DirectXTex.h"
//#include "wincodec.h"

#include "cimagec.h"
#include ".\..\error\cerror.h"
#include "globstruct.h"
#include <stdio.h>
#include <direct.h>
#include <vector>
#include <map>
#include ".\..\Core\SteamCheckForWorkshop.h"

#include "CSpritesC.h"
#include "CMemblocks.h"
#include "CObjectsC.h"
#include "DarkLUA.h"
#include <thread>

#include "wiRenderer.h"
#include "wiGraphics.h"

extern "C" HANDLE GG_CreateFile( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
extern "C" int GG_GetRealPath( char* fullPath, int create );

#define PETESTIMAGEUSAGE

// Forward declclaration of timestampactivity since it is housed elsewhere
#ifndef NOSTEAMORVIDEO
void timestampactivity(int i, char* desc_s);
#endif

// Externs
extern GlobStruct*				g_pGlob;
extern LPGG						m_pDX;
extern LPGGDEVICE				m_pD3D;

// globals for single thread (THREAD-SAFE)
std::thread* g_pT1 = NULL;
bool g_bT1 = false;
bool g_bRequestCleanInteruptionT1 = false;
std::vector<sPreLoadedTexture> g_image_list;
std::vector<sPreLoadedTexture> g_image_outputv;
int iResizeLoadImageX = 0, iResizeLoadImageY = 0;

bool g_bCriticalSectionCreated = false;
CRITICAL_SECTION CriticalSection; 

#ifdef WICKEDENGINE
// used to fool image system into thinking it has successfully loaded an image
LPVOID g_pDummyImage = NULL;
GGIMAGE_INFO g_pDummyInfo;
#endif
bool g_bAllowLegacyImageLoadingForUI = false;
void image_setlegacyimageloading(bool bEnable) { g_bAllowLegacyImageLoadingForUI = bEnable; }
bool g_bUseRGBAFormat = false;
bool g_bDontUseImageAlpha = false;
//PE: Silent 502 , log instead.
int iMaxPasteImageLogs = 10;

// function to execute thread code
void image_thread_function(const std::vector<sPreLoadedTexture> &v)
{
	// in this thread, load each DX11 texture in turn
	g_bT1 = true;
	g_image_outputv.clear();
	for ( int n = 0; n < v.size(); n++ )
	{
		// Request ownership of the critical section.
		EnterCriticalSection(&CriticalSection); 
		sPreLoadedTexture item = v[n];
		if ( item.pTexture == NULL ) item.pTexture = PreloadThreadSafeImage ( item.pFilename , item.iMipMaps );
		g_image_outputv.push_back(item);
		if (g_bRequestCleanInteruptionT1 == true)
		{
			// can interupt this load thread if required
			LeaveCriticalSection(&CriticalSection);
			break;
		}
		// Release ownership of the critical section.
		LeaveCriticalSection(&CriticalSection);
	}
	g_bT1 = false;
}

void image_preload_files_start ( void )
{
	// Initialize the critical section one time only.
	if (g_bCriticalSectionCreated == false)
	{
		if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400) ) 
			return;

		// we can use this to ensure only one thing done at once
		g_bCriticalSectionCreated = true;
	}

	// ensure previous thread has ended
	image_preload_files_strictwaittoend();

	// get number of CPUs we can run threads on, in case we need it
	unsigned int iCPUCores = std::thread::hardware_concurrency();
	char CPUnium[32];
	sprintf ( CPUnium, "%d", iCPUCores );

	// load texture file into member well ahead of needing it (see below for getting the preloaded textures)
	g_image_list.clear();
}

void image_preload_files_add ( LPSTR pFilename , int iMipMaps)
{
	//Moved here so we can check if its already in the list.
	char *cUseFilename;
	char cResolvePath[MAX_PATH];
	//Resolve path , if some function later in code change current dir.
	if (GetFullPathNameA(pFilename, MAX_PATH, &cResolvePath[0], NULL) > 0) {
		cUseFilename = &cResolvePath[0];
	}
	else {
		cUseFilename = pFilename;
	}

	// check to make sure we don't add something we already have in the list
	for ( int n = 0; n < g_image_list.size(); n++ )
		if ( stricmp ( g_image_list[n].pFilename, cUseFilename) == NULL )
			return;

	// add item to list of work
	sPreLoadedTexture item;
	item.pTexture = NULL;
	item.iMipMaps = iMipMaps;
	strcpy(item.pFilename, cUseFilename);
	g_image_list.push_back(item);
}

void image_preload_files_finish ( void )
{
	// before send list to thread, load up list with previous preloaded file 'data' still in memory
	for ( int n = 0; n < g_image_list.size(); n++ )
	{
		if ( n < g_image_outputv.size() )
		{
			if ( stricmp ( g_image_list[n].pFilename, g_image_outputv[n].pFilename ) == NULL )
			{
				// this ensures we avoid reloading something we 'might need' that we have already preloaded previously
				g_image_list[n].pTexture = g_image_outputv[n].pTexture;
			}
		}
	}

	// start preloading
	g_bRequestCleanInteruptionT1 = false; //Start fresh.
	g_pT1 = new std::thread(image_thread_function, std::ref(g_image_list));
}

void image_preload_files_strictwaittoend ( void )
{
	// wait for all work to finish
	if ( g_pT1 )
	{
		g_pT1->join();
		delete g_pT1;
		g_pT1 = NULL;
		g_bRequestCleanInteruptionT1 = false;
	}
}

void image_preload_files_wait ( void )
{
	g_bRequestCleanInteruptionT1 = true;
	image_preload_files_strictwaittoend();
}

bool image_preload_files_in_progress ( void )
{
	// wait for all work to finish
	return g_bT1;
}

void image_preload_files_reset ( void )
{
	// clear finished list for next batch of work
	for ( int n = 0; n < g_image_outputv.size(); n++ )
	{
		if ( g_image_outputv[n].pTexture ) 
		{
			if ( g_image_outputv[n].pTexture ) delete (wiGraphics::Texture*) g_image_outputv[n].pTexture;
			g_image_outputv[n].pTexture = NULL;
		}
	}
	g_image_outputv.clear();
}

#ifdef WICKEDENGINE
	std::vector<void*> lpBadTexture;
#endif

namespace
{
    typedef std::map<int, tagImgData*>		ImageList_t;
    typedef ImageList_t::iterator			ImagePtr;

    // Image Block Globals

    bool								g_bImageBlockActive = false;
    LPSTR								g_iImageBlockFilename = NULL;
    LPSTR								g_iImageBlockRootPath = NULL;
    char								g_pImageBlockExcludePath[512];
    int									g_iImageBlockMode = -1;
    DWORD								g_dwImageBlockSize = 0;
    LPSTR								g_pImageBlockPtr = NULL;
    std::vector<LPSTR>					g_ImageBlockListFile;
    std::vector<DWORD>					g_ImageBlockListOffset;
    std::vector<DWORD>					g_ImageBlockListSize;

    ImageList_t							m_List;
    int									m_iWidth		= 0;				// width of current texture
    int									m_iHeight		= 0;				// height of current texture
    int									m_iMipMapNum	= -1;				// default number of mipmaps
    int									m_iMemory		= 0;				// default memory pool
    bool								m_bSharing		= true;				// sharing flag
    bool								m_bMipMap		= true;				// mipmap on / off
    GGCOLOR								m_Color         = GGCOLOR_ARGB ( 255, 0, 0, 0 );// default transparent color
    tagImgData*							m_imgptr = NULL;
    int									m_CurrentId = 0;
	#ifdef WICKEDENGINE
    GGFORMAT							g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	#else
    GGFORMAT							g_DefaultGGFORMAT;
	#endif
	DWORD								g_dwMipMapGenMode;

    bool RemoveImage( int iID )
    {
        // Clear the cached value if the image being deleted is the current cached image.
        if (m_CurrentId == iID)
        {
            m_CurrentId = 0;
            m_imgptr = NULL;
        }

        // Locate the image, and if found, release all of it's resources.
        ImagePtr pImage = m_List.find( iID );
        if (pImage != m_List.end())
        {
			#ifdef DX11
			#ifdef WICKEDENGINE
			//PE: Mark bad textures that have been deleted.
			

			// release all 'except' any dummy view if active
			bool bIsDummy = false;

			if (g_pDummyImage && g_pDummyImage == pImage->second->lpTexture)
			{
				// do not attempt to delete this, we need it to remain alive for all other images that use this dummy
				bIsDummy = true;
			}
			else
			{
				lpBadTexture.push_back(pImage->second->lpTexture);
				if ( pImage->second->lpTexture ) delete (wiGraphics::Texture*) pImage->second->lpTexture;
				pImage->second->lpTexture = 0;
			}
			 
			//PE: @Lee we are leaking memory, not sure why DX11 dont free the texture ?
			//PE: @Lee I dont know the reason so im using the g_bAllowLegacyImageLoadingForUI so i can control if texture should be released.
			//PE: @Lee this could also be a problem for Classic not releasing the texture, should always release it ? test must wait until we do classic fixes :)
			//PE: Mainly you see this if you quickly hover over thumbs that use backdrops, you can use many many gb in less then a minute.
			//PE: Also dynamic loading of new thumbs and the old icons also leaked.
			//PE: Use same legacy flag to see if we can delete the image (for now).
			if (!bIsDummy && g_bAllowLegacyImageLoadingForUI)
			{
			//	if ( pImage->second->lpTexture ) delete (wiGraphics::Texture*) pImage->second->lpTexture;
			//	pImage->second->lpTexture = 0;
			}

			#else
            SAFE_RELEASE( pImage->second->lpTextureView );
			#endif
			#else
            SAFE_RELEASE( pImage->second->lpTexture );
			#endif
            SAFE_DELETE( pImage->second->lpName );
            delete pImage->second;

            m_List.erase(pImage);

            return true;
        }

        return false;
    }

    bool UpdatePtrImage ( int iID )
    {
        // If the image required is not already cached, refresh the cached value
        if (!m_imgptr || iID != m_CurrentId)
        {
            m_CurrentId = iID;

            ImagePtr p = m_List.find( iID );
            if (p == m_List.end())
            {
                m_imgptr = NULL;
            }
            else
            {
                m_imgptr = p->second;
            }
        }

        return m_imgptr != NULL;
    }
}

DARKSDK int GetPowerSquareOfSize( int Size );

DARKSDK void ImageConstructorD3D ( void )
{
	// setup the image library
	//m_iMipMapNum			= 9; //Default was used in thread loader should be -1.
	m_iMipMapNum = -1;
}

DARKSDK void ImageConstructor ( void )
{
	ImageConstructorD3D ( );
	ImagePassCoreData ( NULL );
}

int DumpImageListCount = 1;

//Clear all "entitybank" memory used.


DARKSDK void DumpImageList(void)
{
	assert( 0 && "DX11 not in use" );
	/*
#ifdef PETESTIMAGEUSAGE
	char timestampMsg[1024];
	long imageuse = 0;
	long notused = 0;
	long notusedsize = 0;
	long usedsize = 0;
	long used = 0;
	long totalmipmaps = 0;
	long savedbybaking = 0;
	long minusmem = 0;
	long GPUcachedimagesSize = 0;

	int TotalLoadTime = 0;

	for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagImgData* ptr = pCheck->second;


		GGSURFACE_DESC imgdesc;

		LPGGSURFACE pTextureInterface = NULL;
		ptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);

		pTextureInterface->GetDesc(&imgdesc);
		SAFE_RELEASE ( pTextureInterface );

		//BC1: 5:6 : 5 color(5 bits red, 6 bits green, 5 bits blue).This is true even if the data also contains 1 - bit alpha.
		//Assuming a 4�4 texture using the largest data format possible, the BC1 format reduces the memory required from 48 bytes(16 colors � 3 components / color � 1 byte / component) to 8 bytes of memory.
		//So to calculate all 4 bits per pixels should be devided by 8.
		//BC1: 48 bytes (16 colors � 3 components/color � 1 byte/component) to 8 bytes of memory.
		//BC2: 64 bytes (16 colors � 4 components/color � 1 byte/component) to 16 bytes of memory.
		//BC3: 64 bytes (16 colors � 4 components/color � 1 byte/component) to 16 bytes of memory.
		//BC1 / 8.0 = * 0,125 (-alpha 4bit) (4 bits per pixel )
		//BC2 / 4.0 = * 0.25 (8 bits per pixel )
		//BC3 / 4.0 = * 0.25 (8 bits per pixel )
		//So: Width*Height*4(RGBA)*(getBitsPerPixel/32.0)
		//Or: Width*Height*(getBitsPerPixel/8.0)
		//Substract BC1 alpha. = 0.5/4 = (-0.125)

		float bperpixel = (float)getBitsPerPixel(imgdesc.Format) / 8;
		if (bperpixel == 0.5 ) {
			bperpixel -= 0.125; // remove alpha count from BC1
		}

		//in kb , so dont need to be so precise just use int.
		int addmipmapssize;
		
		if (imgdesc.MipLevels > 1) {
			addmipmapssize = (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize - 1; // Full mipmaps always give size -1.
			if (addmipmapssize <= 0) addmipmapssize = 0;
		}
		else {
			addmipmapssize = 0;
		}
		totalmipmaps += addmipmapssize;


		if( strlen(ptr->szShortFilename) > 0 )
			sprintf(timestampMsg, "DumpImgList%d(%d,%d): %s (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int) pCheck->first , ptr->iImageLoadTime, ptr->szShortFilename, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int) ( (float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize , imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format).c_str());
		else
			sprintf(timestampMsg, "DumpImgList%d(%d): unknown (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first , ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format).c_str());

		imageuse += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;  //PE: Real mem use.

		if ((int)pCheck->first < 0) {
			minusmem += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize; 
		}

		//if (ptr->AccessCountCPU <= 1 && ptr->AccessCountGPU <= 1) {
		if (ptr->AccessCountCPU == 0 && ptr->AccessCountGPU == 0 ) { //
			notused++;
			notusedsize += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}
		else {
			used++;
			usedsize += ((int) (((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}

		if (ptr->AccessCountCPU >= 1 && ptr->AccessCountGPU >= 1000) {
			//PE: Estimate that 1000 accesses will make the GPU cache the image. ( not releasing it ).
			//PE: If this get larger then 2 gb ( depends on GPU ) we will get stuttering.
			//PE: todo - reset GPU access between levels for standalone.
			GPUcachedimagesSize += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize; //PE: Real mem use.
		}
		TotalLoadTime += ptr->iImageLoadTime;
		#ifndef NOSTEAMORVIDEO
		timestampactivity(0, timestampMsg);
		#endif
	}
	//PE: until GPU access is reset , dont display GPUcachedimagesSize as this will only work for the first level you load and can confuse, now that you know this you can enable this line :)
	//sprintf(timestampMsg, "DumpImageList: Total mem=%ld kb. mipmaps=%ld kb. Used Images: %ld mem %ld kb. - Notused Images: %ld mem %ld kb. - MinusMem: %ld kb. (GPUcachedimagesSize : %ld)", imageuse, totalmipmaps, used, usedsize, notused, notusedsize, minusmem, GPUcachedimagesSize);
	sprintf(timestampMsg, "DumpImageList: LoadTime: %d Total mem=%ld kb. mipmaps=%ld kb. Used Images: %ld mem %ld kb. - Notused Images: %ld mem %ld kb. - MinusMem: %ld kb.", TotalLoadTime , imageuse, totalmipmaps, used, usedsize, notused, notusedsize, minusmem);
	#ifndef NOSTEAMORVIDEO
	timestampactivity(0, timestampMsg);
	#ifndef WICKEDENGINE
	//PE: not used not for wicked, we cant see GPU accesses.
	timestampactivity(0, "NOTUSED");
	#endif
	#endif
	#ifndef WICKEDENGINE
	int totalNUtime = 0;
	for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagImgData* ptr = pCheck->second;

		GGSURFACE_DESC imgdesc;

		LPGGSURFACE pTextureInterface = NULL;
		ptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);

		pTextureInterface->GetDesc(&imgdesc);
		SAFE_RELEASE(pTextureInterface);

		float bperpixel = (float)getBitsPerPixel(imgdesc.Format) / 8;
		if (bperpixel == 0.5) {
			bperpixel -= 0.125; // remove alpha count from BC1
		}

		//in kb , so dont need to be so precise just use int.
		int addmipmapssize;

		if (imgdesc.MipLevels > 1) {
			addmipmapssize = (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize - 1; // Full mipmaps always give size -1.
			if (addmipmapssize <= 0) addmipmapssize = 0;
		}
		else {
			addmipmapssize = 0;
		}
		totalmipmaps += addmipmapssize;


//		if (strlen(ptr->szShortFilename) > 0)
//			sprintf(timestampMsg, "NU%d(%d): %s (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first, ptr->szShortFilename, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format));
//		else
//			sprintf(timestampMsg, "NU%d(%d): unknown (Calls CPU: %d , GPU: %d) (%ld,%ld, %ld kb.+ mipm %ld kb.) mipmaps %d array %d format %s ", DumpImageListCount, (int)pCheck->first, ptr->AccessCountCPU, ptr->AccessCountGPU, ptr->iWidth, ptr->iHeight, (int)((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024 * imgdesc.ArraySize, addmipmapssize, imgdesc.MipLevels, imgdesc.ArraySize, getImageformat(imgdesc.Format));
		if (strlen(ptr->szShortFilename) > 0) {
			sprintf(timestampMsg, "NU: %s (%d)", ptr->szShortFilename, ptr->iImageLoadTime);
		}

		imageuse += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;  //PE: Real mem use.

		if ((int)pCheck->first < 0) {
			minusmem += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
		}

		//if (ptr->AccessCountCPU <= 1 && ptr->AccessCountGPU <= 1) {
		if (ptr->AccessCountCPU == 0 && ptr->AccessCountGPU == 0 && strlen(ptr->szShortFilename) > 0 ) { //
			notused++;
			notusedsize += ((int)(((float)(ptr->iWidth*ptr->iHeight) * bperpixel) / 1024) * imgdesc.ArraySize) + addmipmapssize;
			totalNUtime += ptr->iImageLoadTime;
			#ifndef NOSTEAMORVIDEO
			timestampactivity(0, timestampMsg);
			#endif
		}

	}

	#ifndef NOSTEAMORVIDEO
	sprintf(timestampMsg, "NUTIME: %d", totalNUtime);
	timestampactivity(0, timestampMsg);
	#endif

	#endif

	#ifdef WICKEDENGINE
	void Wicked_Memory_Use_Textures(void);
	Wicked_Memory_Use_Textures();
	#endif

#endif
*/
}

DARKSDK void ImageDestructorD3D ( void )
{
    m_CurrentId = 0;
    m_imgptr = NULL;

    for (ImagePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
    {
        // Release the texture and texture name
        tagImgData* ptr = pCheck->second;
		#ifdef DX11
		if ( ptr->lpTexture ) delete (wiGraphics::Texture*) ptr->lpTexture;
		ptr->lpTexture = 0;
		#else
        SAFE_RELEASE( ptr->lpTexture );
		#endif
        SAFE_DELETE( ptr->lpName );

        // Release the rest of the image storage
        delete ptr;

        // NOTE: Not removing from m_List at this point:
        // 1 - it makes moving to the next item harder
        // 2 - it's less efficient - we'll clear the entire list at the end
    }

    // Now clear the list
    m_List.clear();

}

DARKSDK void ImageDestructor ( void )
{
	ImageDestructorD3D();
}

DARKSDK void ImageSetErrorHandler ( LPVOID pErrorHandlerPtr )
{
	// Update error handler pointer
	g_pErrorHandler = (CRuntimeErrorHandler*)pErrorHandlerPtr;
}

DARKSDK void PassSpriteInstance (  )
{
}

#ifdef DX11
DARKSDK void CreateShaderResourceViewFor ( tagImgData* pImgPtr, int iTextureFlag, GGFORMAT format )
{
	assert( 0 && "Tried to create image view" );
}
#endif

DARKSDK int ImageGetBitDepthFromFormat(GGFORMAT Format)
{
	#ifdef DX11
	switch(Format)
	{
		case DXGI_FORMAT_UNKNOWN : 
			return 0;

		case DXGI_FORMAT_R32G32B32A32_TYPELESS : 
		case DXGI_FORMAT_R32G32B32A32_FLOAT : 
		case DXGI_FORMAT_R32G32B32A32_UINT : 
		case DXGI_FORMAT_R32G32B32A32_SINT : 
			return 128;

		case DXGI_FORMAT_R32G32B32_TYPELESS : 
		case DXGI_FORMAT_R32G32B32_FLOAT : 
		case DXGI_FORMAT_R32G32B32_UINT : 
		case DXGI_FORMAT_R32G32B32_SINT : 
			return 96;

		case DXGI_FORMAT_R16G16B16A16_TYPELESS : 
		case DXGI_FORMAT_R16G16B16A16_FLOAT : 
		case DXGI_FORMAT_R16G16B16A16_UNORM : 
		case DXGI_FORMAT_R16G16B16A16_UINT : 
		case DXGI_FORMAT_R16G16B16A16_SNORM : 
		case DXGI_FORMAT_R16G16B16A16_SINT : 
		case DXGI_FORMAT_R32G32_TYPELESS : 
		case DXGI_FORMAT_R32G32_FLOAT : 
		case DXGI_FORMAT_R32G32_UINT : 
		case DXGI_FORMAT_R32G32_SINT : 
		case DXGI_FORMAT_R32G8X24_TYPELESS : 
			return 64;

		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT : 
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS : 
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT : 
		case DXGI_FORMAT_R10G10B10A2_TYPELESS : 
		case DXGI_FORMAT_R10G10B10A2_UNORM : 
		case DXGI_FORMAT_R10G10B10A2_UINT : 
		case DXGI_FORMAT_R11G11B10_FLOAT : 
		case DXGI_FORMAT_R8G8B8A8_TYPELESS : 
		case DXGI_FORMAT_B8G8R8A8_UNORM : 
		case DXGI_FORMAT_R8G8B8A8_UNORM : 
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : 
		case DXGI_FORMAT_R8G8B8A8_UINT : 
		case DXGI_FORMAT_R8G8B8A8_SNORM : 
		case DXGI_FORMAT_R8G8B8A8_SINT : 
		case DXGI_FORMAT_R16G16_TYPELESS : 
		case DXGI_FORMAT_R16G16_FLOAT : 
		case DXGI_FORMAT_R16G16_UNORM : 
		case DXGI_FORMAT_R16G16_UINT : 
		case DXGI_FORMAT_R16G16_SNORM : 
		case DXGI_FORMAT_R16G16_SINT : 
		case DXGI_FORMAT_R32_TYPELESS : 
		case DXGI_FORMAT_D32_FLOAT : 
		case DXGI_FORMAT_R32_FLOAT : 
		case DXGI_FORMAT_R32_UINT : 
		case DXGI_FORMAT_R32_SINT : 
		case DXGI_FORMAT_R24G8_TYPELESS : 
		case DXGI_FORMAT_D24_UNORM_S8_UINT : 
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS : 
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT : 
			return 32;

		case DXGI_FORMAT_R8G8_TYPELESS : 
		case DXGI_FORMAT_R8G8_UNORM : 
		case DXGI_FORMAT_R8G8_UINT : 
		case DXGI_FORMAT_R8G8_SNORM : 
		case DXGI_FORMAT_R8G8_SINT : 
		case DXGI_FORMAT_R16_TYPELESS : 
		case DXGI_FORMAT_R16_FLOAT : 
		case DXGI_FORMAT_D16_UNORM : 
		case DXGI_FORMAT_R16_UNORM : 
		case DXGI_FORMAT_R16_UINT : 
		case DXGI_FORMAT_R16_SNORM : 
		case DXGI_FORMAT_R16_SINT : 
			return 16;

		case DXGI_FORMAT_R8_TYPELESS : 
		case DXGI_FORMAT_R8_UNORM : 
		case DXGI_FORMAT_R8_UINT : 
		case DXGI_FORMAT_R8_SNORM : 
		case DXGI_FORMAT_R8_SINT : 
		case DXGI_FORMAT_A8_UNORM : 
			return 8;

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return 4;

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return 8;
	}
	return 0;
	#else
	switch(Format)
	{
		case GGFMT_R8G8B8 :		return 24;	break;
		case GGFMT_A8R8G8B8 :		return 32;	break;
		case GGFMT_X8R8G8B8 :		return 32;	break;
		case GGFMT_R5G6B5 :		return 16;	break;
		case GGFMT_X1R5G5B5 :		return 16;	break;
		case GGFMT_A1R5G5B5 :		return 16;	break;
		case GGFMT_A4R4G4B4 :		return 16;	break;
		case GGFMT_A8	:			return 8;	break;
		case GGFMT_R3G3B2 :		return 8;	break;
		case GGFMT_A8R3G3B2 :		return 16;	break;
		case GGFMT_X4R4G4B4 :		return 16;	break;
		case GGFMT_A2B10G10R10 :	return 32;	break;
		case GGFMT_G16R16 :		return 32;	break;
		case GGFMT_A8P8 :			return 8;	break;
		case GGFMT_P8 :			return 8;	break;
		case GGFMT_L8 :			return 8;	break;
		case GGFMT_A8L8 :			return 16;	break;
		case GGFMT_A4L4 :			return 8;	break;
	}
	return 0;
	#endif
}

DARKSDK void ImagePassCoreDataDX9 ( LPVOID pGlobPtr )
{
	#ifndef DX11
	// only if have display
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if ( m_pDX == NULL || pBackBuffer == NULL )
		return;

	// Get default GGFORMAT from backbuffer
	D3DSURFACE_DESC backdesc;
	if(pBackBuffer)
	{
		HRESULT hRes = pBackBuffer->GetDesc(&backdesc);
		DWORD dwDepth=ImageGetBitDepthFromFormat(backdesc.Format);
		if(dwDepth==16) g_DefaultGGFORMAT = GGFMT_A1R5G5B5;
		if(dwDepth==32) g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	}
	else
	{
		g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	}

	// Ensure textureformat is valid, else choose next valid..
	HRESULT hRes = m_pDX->CheckDeviceFormat(	GGADAPTER_DEFAULT,
												D3DDEVTYPE_HAL,
												backdesc.Format,
												0, D3DRTYPE_TEXTURE,
												g_DefaultGGFORMAT);
	if ( FAILED( hRes ) )
	{
		// Need another texture format with an alpha
		for(DWORD t=0; t<12; t++)
		{
			switch(t)
			{
				case 0  : g_DefaultGGFORMAT = GGFMT_A8R8G8B8;		break;
				case 1  : g_DefaultGGFORMAT = GGFMT_X8R8G8B8;		break;
				case 2  : g_DefaultGGFORMAT = GGFMT_A1R5G5B5;		break;
				case 3  : g_DefaultGGFORMAT = GGFMT_A2B10G10R10;	break;
				case 4  : g_DefaultGGFORMAT = GGFMT_A4R4G4B4;		break;
				case 5  : g_DefaultGGFORMAT = GGFMT_A8R3G3B2;		break;
				case 6  : g_DefaultGGFORMAT = GGFMT_R8G8B8;		break;
				case 7  : g_DefaultGGFORMAT = GGFMT_R5G6B5;		break;
				case 8  : g_DefaultGGFORMAT = GGFMT_X1R5G5B5;		break;
				case 9  : g_DefaultGGFORMAT = GGFMT_R3G3B2;		break;
				case 10 : g_DefaultGGFORMAT = GGFMT_X4R4G4B4;		break;
				case 11 : g_DefaultGGFORMAT = GGFMT_G16R16;		break;
			}
			HRESULT hRes = m_pDX->CheckDeviceFormat(	GGADAPTER_DEFAULT,
														D3DDEVTYPE_HAL,
														backdesc.Format,
														0, D3DRTYPE_TEXTURE,
														g_DefaultGGFORMAT);
			if ( SUCCEEDED( hRes ) )
			{
				// Found a texture we can use
				return;
			}
		}
	}
	#endif
}

DARKSDK void ImagePassCoreDataDX11 ( LPVOID pGlobPtr )
{
	//  work out default GGFORMAT for current device
	#ifdef DX11
	g_DefaultGGFORMAT = GGFMT_A8R8G8B8;
	#endif
}

DARKSDK void ImagePassCoreData( LPVOID pGlobPtr )
{
	#ifdef DX11
	ImagePassCoreDataDX11(pGlobPtr);
	#else
	ImagePassCoreDataDX9(pGlobPtr);
	#endif
}

DARKSDK void ImageRefreshGRAFIX ( int iMode )
{
	if(iMode==0)
	{
		// Remove all traces of old D3D usage
		ImageDestructorD3D();
	}
	if(iMode==1)
	{
		// Get new D3D and recreate everything D3D related
		ImageConstructorD3D ( );
		ImagePassCoreData ( g_pGlob );
		PassSpriteInstance ( );
	}
}

void GetFileInMemory ( LPSTR szFilename, LPVOID* ppFileInMemoryData, DWORD* pdwFileInMemorySize, LPSTR pFinalRelPathAndFileRef )
{
	*pdwFileInMemorySize = 0;
	*ppFileInMemoryData = NULL;
	if ( g_bImageBlockActive )
	{
		// final storage string of path and file resolver (makes the filename and path uniform for imageblock retrieval)
		char pFinalRelPathAndFile[512];

		// store old directory
		char pOldDir [ 512 ];
		_getcwd ( pOldDir, 512 );

		// get combined path only
		char pPath[1024];
		char pFile[1024];
		strcpy ( pPath, pOldDir );
		strcat ( pPath, "\\" );
		strcat ( pPath, szFilename );
		strcat ( pFile, "" );
		for ( int n=strlen(pPath); n>0; n-- )
		{
			if ( pPath[n]=='\\' || pPath[n]=='/' )
			{
				// split file and path
				strcpy ( pFile, pPath + n + 1 );
				pPath [ n + 1 ] = 0;
				break;
			}
		}

		// Combine current working folder and filename to get a resolved path (removes ..\..\ stuff)
		char pResolvedDir[512];
		strcpy ( pResolvedDir, pPath );

		// Remove the part which represents the root location of the Image Block (g_iImageBlockRootPath)
		if ( strlen ( pResolvedDir ) <= strlen(g_iImageBlockRootPath) )
			strcpy ( pFinalRelPathAndFile, "" );
		else
			strcpy ( pFinalRelPathAndFile, pResolvedDir + strlen(g_iImageBlockRootPath) );

		// Ensure a \ is added
		if ( strlen ( pFinalRelPathAndFile ) > 0 )
		{
			if ( pFinalRelPathAndFile [ strlen(pFinalRelPathAndFile)-1 ]!='\\' )
			{
				// add folder divide at end of path string
				int iLen = strlen(pFinalRelPathAndFile);
				pFinalRelPathAndFile [ iLen+0 ] = '\\';
				pFinalRelPathAndFile [ iLen+1 ] = 0;
			}
		}

		// Add the filename back in
		strcat ( pFinalRelPathAndFile, pFile );

		// Restore folder
		_chdir ( pOldDir );

		// Retrieve file in memory
		if ( g_iImageBlockMode==1 )
		{
			*ppFileInMemoryData = RetrieveFromImageBlock ( pFinalRelPathAndFile, pdwFileInMemorySize );
		}

		// copy final rel path and file
		strcpy ( pFinalRelPathAndFileRef, pFinalRelPathAndFile );
	}
	else
	{
		strcpy ( pFinalRelPathAndFileRef, "" );
	}
}

DARKSDK LPGGTEXTURE GetTextureCore ( char* szFilename, GGIMAGE_INFO* info, int iOneToOnePixels, int iFullTexturePlateMode, int iDivideTextureSize )
{
	// new feature IMAGEBLOCK
	DWORD dwFileInMemorySize = 0;
	LPVOID pFileInMemoryData = NULL;
	if ( g_bImageBlockActive )
	{
		// final storage string of path and file resolver (makes the filename and path uniform for imageblock retrieval)
		// and work out true file and path, then look for it in imageblock
		char pFinalRelPathAndFile[512];
		GetFileInMemory ( szFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile );

		// Add relative path and file to image block
		if ( g_iImageBlockMode==0 )
		{
			_chdir ( g_iImageBlockRootPath );
			AddToImageBlock ( pFinalRelPathAndFile );
		}
	}

	// loads a texture and returns a pointer to it

	// variable declarations
	LPGGTEXTURE	lpTexture = NULL;	// set texture to null

	DARKSDK int Timer(void);
	int ts = Timer();

	// get file image info
	HRESULT hRes = 0;
	if ( g_iImageBlockMode==1 && pFileInMemoryData )
	{
		#ifdef DX11
		hRes = D3DX11GetImageInfoFromMemory( pFileInMemoryData, dwFileInMemorySize, NULL, info, NULL );
		if (FAILED(hRes)) 
		{
			char szRealFilename[ MAX_PATH ];
			strcpy_s( szRealFilename, MAX_PATH, szFilename );
			GG_GetRealPath( szRealFilename, 0 );
			hRes = D3DX11GetImageInfoFromFile( szRealFilename, NULL, info, NULL );
		}
		#else
		hRes = D3DXGetImageInfoFromFileInMemory( pFileInMemoryData, dwFileInMemorySize, info );
		if (FAILED(hRes)) hRes = D3DXGetImageInfoFromFile( szFilename, info );
		#endif
	}
	else
	{
		#ifdef DX11
		char szRealFilename[ MAX_PATH ];
		strcpy_s( szRealFilename, MAX_PATH, szFilename );
		GG_GetRealPath( szRealFilename, 0 );
		hRes = D3DX11GetImageInfoFromFile( szRealFilename, NULL, info, NULL );
		#else
		hRes = D3DXGetImageInfoFromFile( szFilename, info );
		#endif
	}

	// If failed to get image information, then can't be any image there either
	if (FAILED(hRes)) {
		return NULL;
	}

	// if texture size needs diviing, do so now
	if ( iDivideTextureSize>0 )
	{
		if ( iDivideTextureSize==16384 )
		{
			// notextureloadmode
			(*info).Width = 1;
			(*info).Height = 1;
		}
		else if (iDivideTextureSize == 8192)
		{
			(*info).Width = iResizeLoadImageX;
			(*info).Height = iResizeLoadImageY;
		}
		else
		{
			// divide by specified value (reduce texture consumption)
			(*info).Width /= iDivideTextureSize;
			(*info).Height /= iDivideTextureSize;
			if ( (*info).Width < 4 ) (*info).Width = 4;
			if ( (*info).Height < 4 ) (*info).Height = 4;
		}
	}

	// if mode is CUBE(2) or VOLUME(3), direct cube loader
	if ( iFullTexturePlateMode==2 || iFullTexturePlateMode==3 || iDivideTextureSize==16384 )
	{
		if ( iDivideTextureSize==16384 )
		{
			// support for quick-fake-texture-load (apply texture to scene without loading it)
			LPGGTEXTURE pFakeTex = NULL;
			#ifdef DX11
			#else
			hRes = D3DXCreateTexture ( m_pD3D,
									   (*info).Width,
									   (*info).Height,
									   1,//one mipmap only for one-to-one pixels
									   0,
									   g_DefaultGGFORMAT,
									   D3DPOOL_MANAGED,
									   &pFakeTex );
			#endif
			lpTexture = pFakeTex;
		}
		else
		{
			if ( iFullTexturePlateMode==2 ) 
			{
				// support for cube textures when specify texture flag of two (2)
				HRESULT hRes = 0;
				LPGGCUBETEXTURE pCubeTex = NULL;
				#ifdef DX11
				D3DX11_IMAGE_LOAD_INFO loadinfo;
				loadinfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

				char szRealFilename[ MAX_PATH ];
				strcpy_s( szRealFilename, MAX_PATH, szFilename );
				GG_GetRealPath( szRealFilename, 0 );

				hRes = RendererLoadTexture2D( m_pD3D, szRealFilename, &loadinfo, &pCubeTex );
				#endif
				if ( SUCCEEDED( hRes ) ) 
				{
					// cube loaded fine
					lpTexture = (LPGGTEXTURE)pCubeTex;
				}
				else
					return NULL;
			}
			if ( iFullTexturePlateMode==3 ) 
			{
				// support for volume textures when specify 3
			}
		}
	}
	else
	{
		// texture flag can control if image is GPU only or MANAGED (SYS memory copy)
		#ifdef DX11

		// just as about to load texture file, check if we already have done in preload thread

		void* pPreCreatedTexture = NULL;
		for ( int n = 0; n < g_image_outputv.size(); n++ )
		{
			int iSearchStrLen = strlen(szFilename);
			if (!(iSearchStrLen > strlen(g_image_outputv[n].pFilename))) { //PE: Got exception iSearchStrLen > strlen(g_image_outputv[n].pFilename)
				if (strnicmp(g_image_outputv[n].pFilename + strlen(g_image_outputv[n].pFilename) - iSearchStrLen, szFilename, iSearchStrLen) == NULL)
				{
					if (g_image_outputv[n].pTexture == NULL) {
						char mdebug[512];
						sprintf(mdebug, "image_preload_files_wait(): %s ", szFilename);
						#ifndef NOSTEAMORVIDEO
						timestampactivity(0, mdebug);
						#endif
						image_preload_files_wait();
					}
					if (g_image_outputv[n].pTexture)
					{
						pPreCreatedTexture = g_image_outputv[n].pTexture;
						g_image_outputv[n].pTexture = NULL;
						break;
					}
				}
			}
		}

		// not unknown, use file format
		GGFORMAT newImageFormat = (*info).Format;
					
		// if DDS, load directly with original mipmap data intact
		if ( g_iImageBlockMode==1 && pFileInMemoryData )
		{
			//hRes = D3DX11CreateTextureFromFileInMemoryEx(	m_pD3D,	pFileInMemoryData, dwFileInMemorySize, info.Width,info.Height,D3DX_DEFAULT,0,GGFMT_UNKNOWN,
			//dwPoolType,//D3DPOOL_MANAGED, lee - 010314 - preserve SYS MEM!
			//D3DX_DEFAULT,D3DX_DEFAULT,0,&info,NULL,&lpTexture );
		}
		else
		{
			bool bFormatDDSLoadFriendly = true;
			if ( (*info).Format >= DXGI_FORMAT_R8G8B8A8_TYPELESS && (*info).Format <= DXGI_FORMAT_R8G8B8A8_SINT ) bFormatDDSLoadFriendly = false;
			if ( m_iMipMapNum == -1 && (*info).MipLevels == 1 && (*info).Format >= DXGI_FORMAT_BC1_TYPELESS && (*info).Format <= DXGI_FORMAT_BC5_SNORM ) bFormatDDSLoadFriendly = false;

			#ifdef WIP_PROLOADLEVELTEXTURES
			//PE: record all level textures that can be preloaded.
			extern std::vector<std::string> preload_setup;
			//Only record mipmap -1.
			if (m_iMipMapNum == -1)
				preload_setup.push_back(szFilename);
			#endif

			//PE: Used to see what files can be thread preloaded.
			//if (pPreCreatedTexture == NULL) { // && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL) {
			//	char mdebug[2048];
			//	sprintf(mdebug, "DLOAD POSSIBLE:(%d) %s ", m_iMipMapNum, szFilename);
			//	timestampactivity(0, mdebug);
			//}

			if (pPreCreatedTexture != NULL && m_iMipMapNum == -1 && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL) {
				//PE: Already loaded with mipmaps generated.
				lpTexture = pPreCreatedTexture;
				pPreCreatedTexture = NULL;
			}
			else if ( strnicmp ( szFilename + strlen(szFilename) - 4, ".dds", 4 ) == NULL && bFormatDDSLoadFriendly == true )
			{

				if ( iDivideTextureSize <= 1 )
				{
					if (pPreCreatedTexture != NULL ) //PE: Should always be correct when here.
					{
						lpTexture = pPreCreatedTexture;
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );

						// effort to speed up loading of DDS texture files (above took 0.1s per 512K texture, .4s per 2K texture)
						// as above, you cannot auto gen mipmaps if the texture is compressed (do this as part of file)
						hRes = RendererLoadTexture2D( m_pD3D, szRealFilename, 0, &lpTexture );
					}
				}
				else
				{
					assert( 0 && "DX11 not in use" );
					/*
					//PE: We need support for resize inside thread loader.

					// meets all fast load requirements, but need to reduce texture size when loading
					wchar_t wTexFilename[512];
					MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wTexFilename, sizeof(wTexFilename));
					DirectX::TexMetadata imageData;
					DirectX::ScratchImage imageTexture;
					hRes = GetMetadataFromDDSFile( wTexFilename, DirectX::DDS_FLAGS_NONE, imageData );			
					hRes = LoadFromDDSFile( wTexFilename, DirectX::DDS_FLAGS_NONE, &imageData, imageTexture );
					DirectX::ScratchImage* pWrkImage = &imageTexture;
					DirectX::ScratchImage convertedTexture;
					bool bWasCompressed = false;
					DXGI_FORMAT storeFormat = imageTexture.GetMetadata().format;
					if ( storeFormat >= DXGI_FORMAT_BC1_TYPELESS && storeFormat <= DXGI_FORMAT_BC5_SNORM )
					{
						hRes = DirectX::Decompress( imageTexture.GetImages(), imageTexture.GetImageCount(), imageTexture.GetMetadata(), DXGI_FORMAT_B8G8R8A8_UNORM, convertedTexture );
						pWrkImage = &convertedTexture;
						imageTexture.Release();
						bWasCompressed = true;
					}
					DirectX::ScratchImage resizedTexture;
					hRes = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), (*info).Width, (*info).Height, DirectX::TEX_FILTER_SEPARATE_ALPHA, resizedTexture );
					pWrkImage->Release();
					pWrkImage = &resizedTexture;

					DirectX::ScratchImage resizedCompressedTexture;
					if ( bWasCompressed == true )
					{
						hRes = DirectX::Compress(pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(),
						storeFormat, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, resizedCompressedTexture );
						//PE: MEM bug resizedTexture was never released.
						pWrkImage->Release();
						pWrkImage = &resizedCompressedTexture;
					}

					hRes = DirectX::CreateTexture ( m_pD3D, pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), &lpTexture );
					pWrkImage->Release();
					*/
				}
			}
			else
			{
				//PE: Make sure we keep current compression, if we just need to generate mipmaps.
				if ( m_iMipMapNum == -1 && (*info).MipLevels == 1 && strnicmp(szFilename + strlen(szFilename) - 4, ".dds", 4) == NULL ) 
				{

					//Only need to generate mipmaps so keep original compression.
					D3DX11_IMAGE_LOAD_INFO loadinfo;
					loadinfo.Format = (*info).Format;
					loadinfo.MipLevels = m_iMipMapNum; // set using SetMipmapNum when want no mipmaps (i.e. vegmask)
					loadinfo.Width = (*info).Width;
					loadinfo.Height = (*info).Height;

					// special THREAD-SAFE way to preload resources, then use it instead of doing it in the main thread
					if ( pPreCreatedTexture != NULL )
					{
						lpTexture = pPreCreatedTexture;
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );
						hRes = RendererLoadTexture2D( m_pD3D, szRealFilename, &loadinfo, &lpTexture );
					}
				}
				else 
				{
					//PE: Normally only png and bmp goes here.
					if (pPreCreatedTexture != NULL) //PE: Should always be correct when here.
					{
						lpTexture = pPreCreatedTexture;
						pPreCreatedTexture = NULL;
					}
					else
					{
						char szRealFilename[ MAX_PATH ];
						strcpy_s( szRealFilename, MAX_PATH, szFilename );
						GG_GetRealPath( szRealFilename, 0 );

						// to conform to internal BGRA format (DX9 to DX11 nonesense)
						D3DX11_IMAGE_LOAD_INFO loadinfo;
						#ifdef WICKEDENGINE
						//PE: In Wicked we need  DXGI_FORMAT_R8G8B8A8_UNORM (28)
						loadinfo.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
						(*info).Format = loadinfo.Format;
						#else
						loadinfo.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
						(*info).Format = loadinfo.Format;
						#endif
						loadinfo.MipLevels = m_iMipMapNum; // set using SetMipmapNum when want no mipmaps (i.e. vegmask)
						loadinfo.Width = (*info).Width;
						loadinfo.Height = (*info).Height;
						hRes = RendererLoadTexture2D( m_pD3D, szRealFilename, &loadinfo, &lpTexture );
					}
				}
			}
		}

		// 010205 - default mem can run out
		if (lpTexture == NULL) {
			return NULL;
		}

		// adjust to actual size if texture smaller
		//D3DSURFACE_DESC desc;
		//lpTexture->GetLevelDesc(0,&desc);
		//if(desc.Width<info.Width) info.Width=desc.Width;
		//if(desc.Height<info.Height) info.Height=desc.Height;

		#endif
	}

	// check the texture loaded in ok
	if ( !lpTexture )
		Error1 ( "Failed to load texture for image library" );

	// needed image info
	m_iWidth  = (*info).Width;		// file width
	m_iHeight = (*info).Height;	// file height

	// finally return the pointer
	return lpTexture;
}

DARKSDK void* PreloadThreadSafeImage(LPSTR szFilename, int iMipMaps)
{
	// this is called from a thread, needs to be THREAD-SAFE! (pretty sure it is now)

	// can replace this and transport format, size, etc via list!
	GGIMAGE_INFO info;

	// LB: Seems the 'GG_GetRealPath' call is not thread safe (crashes on create_file) so preload of images
	// will not only be applicable to files in the Program Files Root Area
	//LPSTR szRealFilename = szFilename;
	// LB: But since then added the CRITICAL SECTION which protects this so only one use at a time on same thread :)
	char szRealFilename[ MAX_PATH ];
	strcpy_s( szRealFilename, MAX_PATH, szFilename );
	GG_GetRealPath( szRealFilename, 0 );

	// Load in image info from file (thread safe)
	HRESULT hRes = D3DX11GetImageInfoFromFile(szRealFilename, NULL, &info, NULL);

	// can we call a DX11 create texture function from a thread (DX11 supposed to be threadsafe and thread pump undocumented!!)
	void* pPreCreatedTexture = NULL;
	D3DX11_IMAGE_LOAD_INFO loadinfo;
	loadinfo.Format = (info).Format;
	loadinfo.MipLevels = iMipMaps; //PE: Enable us to set per file mipmaps, so we can load more in the background.
	loadinfo.Width = (info).Width;
	loadinfo.Height = (info).Height;

	// Create texture (thread safe as m_pD3D can be used by multiple threads)
	hRes = RendererLoadTexture2D( m_pD3D, szRealFilename, &loadinfo, &pPreCreatedTexture );

	// return
	if (hRes == 0)
		return pPreCreatedTexture;
	else
		return NULL;
}

DARKSDK void SetImageAutoMipMap ( int iGenerateMipMaps )
{
	#ifdef DX11
	#else
	if ( iGenerateMipMaps==1 )
		g_dwMipMapGenMode = D3DX_DEFAULT;
	else
		g_dwMipMapGenMode = D3DX_FROM_FILE;
	#endif
}

DARKSDK bool LoadImageCoreFullTex ( char* szFilename, LPGGTEXTURE* pImage, GGIMAGE_INFO* info, int iFullTexturePlateMode, int iDivideTextureSize )
{
	bool bUseLegacyImageLoader = true;
	#ifdef WICKEDENGINE
	// wicked uses its own resource image loader, this now only required for IMGUI UI image loading
	// though a global flag determines if we load/re-use a dummy 32x32 texture, or allow the real image to load
	bUseLegacyImageLoader = g_bAllowLegacyImageLoadingForUI;
	#endif

	// load in a real image using the legacy system
	bool bRes = false;
	if (bUseLegacyImageLoader == true)
	{
		// Uses actual or virtual file..
		char VirtualFilename[_MAX_PATH];
		strcpy(VirtualFilename, szFilename);
		CheckForWorkshopFile(VirtualFilename);

		// Decrypt and use media
		g_pGlob->Decrypt(VirtualFilename);

		// load the media
		*pImage = GetTextureCore(VirtualFilename, info, 0, iFullTexturePlateMode, iDivideTextureSize);
		if (*pImage) bRes = true;

		// get media info
		HRESULT hRes = 0;
		if (g_bImageBlockActive && g_iImageBlockMode == 1)
		{
			DWORD dwFileInMemorySize = 0;
			LPVOID pFileInMemoryData = NULL;
			char pFinalRelPathAndFile[512];
			GetFileInMemory(VirtualFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile);
			#ifdef DX11
			hRes = D3DX11GetImageInfoFromMemory(pFileInMemoryData, dwFileInMemorySize, NULL, info, NULL);
			#else
			hRes = D3DXGetImageInfoFromFileInMemory(pFileInMemoryData, dwFileInMemorySize, info);
			#endif
		}

		// get info from physical file if not in image block
		#ifdef DX11
		/// 230817 - so do not overwrite info data thats been changed
		/// if ( hRes==0 || hRes!=GG_OK ) hRes = D3DX11GetImageInfoFromFile( VirtualFilename, NULL, info, NULL );
		#else
		if (hRes == 0 || hRes != GG_OK) hRes = D3DXGetImageInfoFromFile(VirtualFilename, info);
		#endif

		// re-encrypt if applicable
		g_pGlob->Encrypt(VirtualFilename);
	}
	#ifdef WICKEDENGINE
	else
	{
		// load or use the dummy image (preserved logical flow by allowing images to think they have been loaded in)
		// can clean up this for the wicked engine down the line once Classic is obsolete
		if (g_pDummyImage == NULL)
		{
			if (stricmp(szFilename, "Files\\editors\\gfx\\dummy.png") == NULL)
			{
				*pImage = GetTextureCore(szFilename, info, 0, iFullTexturePlateMode, iDivideTextureSize);
				if (*pImage)
				{
					g_pDummyImage = *pImage;
					g_pDummyInfo = *info;
					bRes = true;
				}
			}
		}
		else
		{
			*pImage = g_pDummyImage;
			*info = g_pDummyInfo;
			bRes = true;
		}
	}
	#endif

	// success or no
	return bRes;
}

DARKSDK bool LoadImageCore ( char* szFilename, LPGGTEXTURE* pImage, GGIMAGE_INFO* info )
{
	// default is full texture plate mode zero = simple surface
	return LoadImageCoreFullTex ( szFilename, pImage, info, 0, 0 );
}

//PE: 50000+ to be used for internal images inside dbo's.

DARKSDK bool FindInternalImage ( char* szFilename, int* pImageID )
{
	if ( szFilename && szFilename[0] )
	{
    	int iFindFilenameLength = strlen(szFilename);

		ImagePtr pCheck = m_List.begin();
		while ( pCheck != m_List.end() ) //PE: scan everything. && pCheck->first < 0
		{
			//PE: We cant reuse IDs from CCP as they can change, so always create new id's.
			//g. undefined: if (!(pCheck->first > g.charactercreatorEditorImageoffset && pCheck->first < g.charactercreatorEditorImageoffset + 500))
			if (!(pCheck->first > 95000 && pCheck->first < 95500))
			{
				// get a pointer to the actual data
				tagImgData* ptr = pCheck->second;
				if (ptr && ptr->szShortFilename && ptr->lpTexture)
				{
					if (_stricmp(ptr->szShortFilename, szFilename) == 0)
					{
						*pImageID = pCheck->first;
						return true;
					}
				}
			}
            ++pCheck;
		}
	}

	//PE: We need + imageID if we are going to reuse them there is a lot of ID > 0 around the code.

	static int iTry = 50000;
	ImagePtr pCheck = m_List.find(iTry);
	while (pCheck != m_List.end())
	{
		pCheck = m_List.find(iTry++);
		if (iTry > 58000) break;
	}

	if (iTry > 58000) {

		//PE: None positive found or MAX , use the old way.

		// Static, so not reset between function calls - almost guaranteed to find the
		// next free image that way.
		static int iTry = -1;

		// Check to see if the image id is in use
		ImagePtr pCheck = m_List.find(iTry);
		while (pCheck != m_List.end())
		{
			// Is in use, check for the next number, but make sure that underflow is dealt with correctly
			// ie, iTry MUST stay negative. Note that this is not too likely to happen anyway.
			--iTry;
			if (iTry > 0)
				iTry = -1;

			pCheck = m_List.find(iTry);
		}
	}
	// this image can be used = new slot
	*pImageID = iTry;

	// not found existing image..
	return false;
}

#ifndef NOSTEAMORVIDEO
extern int addinternaltexture(char* tfile_s);
extern int findinternaltexture(char* tfile_s);
extern void removeinternaltexture(int teximg);
extern void deleteinternaltexture(char* tfile_s);
#endif

DARKSDK void ClearAnyLightMapInternalTextures ( void )
{
	ImagePtr pCheck = m_List.begin();
	while ( pCheck != m_List.end() ) //PE: removed scan everything. && pCheck->first < 0
	{
        // get a pointer to the actual data
		tagImgData* ptr = pCheck->second;
		int iFoundID = 0;
		char *shortname;
        if ( ptr && ptr->szShortFilename && ptr->lpTexture )
		{
			if ( ptr->szShortFilename!=NULL )
			{
				if ( strlen(ptr->szShortFilename)>11 )
				{
					for (int n = 0; n < (int)strlen(ptr->szShortFilename) - 11; n++) {
						if (strnicmp(ptr->szShortFilename + n, "lightmaps\\", 10) == NULL) {
							shortname = ptr->szShortFilename;
							iFoundID = pCheck->first;
							break;
						}
					}
				}
			}
        }
		if ( iFoundID!=0 ) 
		{
			//PE: Also need to be removed from internal list.
			#ifndef NOSTEAMORVIDEO
			deleteinternaltexture(shortname);
			#endif
			RemoveImage( iFoundID );
			pCheck = m_List.begin();
		}
		else
		{
	        ++pCheck;
		}
	}
}
//PE:
DARKSDK void ClearAnyEntitybankInternalTextures(void)
{
	ImagePtr pCheck = m_List.begin();
	while (pCheck != m_List.end()) //PE: removed scan everything. && pCheck->first < 0
	{
		// get a pointer to the actual data
		tagImgData* ptr = pCheck->second;
		int iFoundID = 0;
		char *shortname;
		if (ptr && ptr->szShortFilename && ptr->lpTexture)
		{
			if (ptr->szShortFilename != NULL)
			{
				if (strlen(ptr->szShortFilename)>11)
				{
					for (int n = 0; n < (int)strlen(ptr->szShortFilename) - 11; n++) {
						if (strnicmp(ptr->szShortFilename + n, "entitybank\\", 10) == NULL) {
							shortname = ptr->szShortFilename;
							iFoundID = pCheck->first;
							break;
						}
					}
				}
			}
		}
		if (iFoundID != 0)
		{
			//PE: Also need to be removed from internal list.
			#ifndef NOSTEAMORVIDEO
			deleteinternaltexture(shortname);
			#endif
			RemoveImage(iFoundID);
			pCheck = m_List.begin();
		}
		else
		{
			++pCheck;
		}
	}
}

// This load is NOT the main DBPro image loader - it is used here though (Load(x,x,x,x))
DARKSDK int LoadImageCoreInternal ( char* szFilename, int iDivideTextureSize )
{
	// does image already exist?
	int iImageID = 0;
	if ( !FindInternalImage ( szFilename, &iImageID ) )
	{
		//PE: Check if we have it in the internal list.
		#ifndef NOSTEAMORVIDEO
		int tmpid = findinternaltexture(szFilename);
		if (tmpid > 0) {
			//char mdebug[2048];
			//sprintf(mdebug, "findinternaltexture: %s, return: %d ", szFilename, tmpid );
			//timestampactivity(0, mdebug);
			return(tmpid);
		}
		#endif
		// copy of filename to attempt to load
		char VirtualFilename[MAX_PATH];
		strcpy ( VirtualFilename, szFilename );

		CheckForWorkshopFile ( VirtualFilename );

		// no, use standard loader
		g_pGlob->Decrypt( VirtualFilename );

		//PE: Add to list so it can be reused.
		#ifndef NOSTEAMORVIDEO
		tmpid = addinternaltexture(szFilename);
		if ( tmpid > 0 ) {
			iImageID = tmpid;
		}
		#endif

		if ( LoadImageCoreRetainName ( szFilename, (LPSTR)VirtualFilename, iImageID, 0, true, iDivideTextureSize ) )
		{
			// new image returned
		}
		else
		{
			// load failed
			//PE: Remove entry again.
			#ifndef NOSTEAMORVIDEO
			removeinternaltexture(iImageID);
			#endif
			iImageID=0;
		}
		g_pGlob->Encrypt( VirtualFilename );
	}
	return iImageID;
}

DARKSDK int LoadImageInternalEx ( char* szFilename, int iDivideTextureSize )
{
	return LoadImageCoreInternal ( szFilename, iDivideTextureSize );
}

DARKSDK int LoadImageInternal ( char* szFilename )
{
	return LoadImageCoreInternal ( szFilename, 0 );
}

DARKSDK LPGGTEXTURE GetTexture ( char* szFilename, GGIMAGE_INFO* info, int iOneToOnePixels )
{
	// Uses actual or virtual file..
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, szFilename);
	//g_pGlob->UpdateFilenameFromVirtualTable( VirtualFilename);

	CheckForWorkshopFile ( VirtualFilename );

	// Decrypt and use media, re-encrypt
	g_pGlob->Decrypt( VirtualFilename );
	LPGGTEXTURE Res = GetTextureCore ( VirtualFilename, info, iOneToOnePixels, 0, 0 );
	g_pGlob->Encrypt( VirtualFilename );
	return Res;
}

DARKSDK LPGGTEXTURE GetImagePointer ( int iID )
{
	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return NULL;

	if (m_imgptr->lpTexture) {

#ifdef PETESTIMAGEUSAGE
		m_imgptr->AccessCountCPU++;
#endif
		return m_imgptr->lpTexture;
	}
	else
		return NULL;
}

// views are now buried in WickedEngine
/*
DARKSDK LPGGTEXTUREREF GetImagePointerView ( int iID )
{
	#ifdef DX11
	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return NULL;

	if (m_imgptr->lpTextureView) {
#ifdef PETESTIMAGEUSAGE
		m_imgptr->AccessCountGPU++;
#endif
		return m_imgptr->lpTextureView;
	}
	else
		return NULL;
	#else
	return GetImagePointer(iID);
	#endif
}

DARKSDK void SetImagePointerView ( int iID, LPGGRENDERTARGETVIEW pView )
{
	#ifdef DX11
	//hmm, maybe render target view is different from shader resource view
	//if ( !UpdatePtrImage ( iID ) ) return;
	//m_imgptr->lpTextureView = (ID3D11ShaderResourceView*)pView;
	#endif
}
*/

DARKSDK int ImageWidth ( int iID )
{
	// get the width of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return -1;

	// return the width
	return m_imgptr->iWidth;
}

DARKSDK int ImageHeight ( int iID )
{
	// get the height of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return -1;

	// return the height
	return m_imgptr->iHeight;
}

DARKSDK float ImageUMax ( int iID )
{
	// get the height of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return 1.0f;

	// return the fTexUMax
	return m_imgptr->fTexUMax;
}

DARKSDK float ImageVMax ( int iID )
{
	// get the width of an image

	// update internal data
	if ( !UpdatePtrImage ( iID ) )
		return 1.0f;

	// return the fTexVMax
	return m_imgptr->fTexVMax;
}

DARKSDK void SetImageSharing ( bool bMode )
{
	// sets up the sharing mode, this helps to conserve
	// memory as it won't load in the same image more
	// than once

	// save mode
	m_bSharing = bMode;
}

DARKSDK void SetImageMemory ( int iMode )
{
	// sets the memory mode for the images

	// iMode possible values
	// 0 - default
	// 1 - managed ( must use this to lock textures )
	// 2 - system

	// check that the mode is valid
	if ( iMode < 0 || iMode > 2 )
		Error1 ( "Invalid memory mode specified" );

	// save the mode
	m_iMemory = iMode;
}

DARKSDK void LockImage ( int iID )
{
	// lock the specified image, this allows you to perform
	// actions like plotting pixels etc
	if ( !UpdatePtrImage ( iID ) )
		return;

	#ifdef DX11
	#else
	HRESULT hr;
	if ( !m_imgptr->bLocked )
	{
		memset ( &m_imgptr->d3dlr, 0, sizeof ( GGLOCKED_RECT ) );
		if ( FAILED ( hr = m_imgptr->lpTexture->LockRect ( 0, &m_imgptr->d3dlr, 0, 0 ) ) )
			Error ( "Failed to lock image for image library" );
		m_imgptr->bLocked = true;
	}
	else
		Error ( "Failed to lock image for image library as it's already locked" );
	#endif
}

DARKSDK void UnlockImage ( int iID )
{
	if ( !UpdatePtrImage ( iID ) )
		return;

	#ifdef DX11
	#else
	m_imgptr->lpTexture->UnlockRect ( NULL );
	m_imgptr->bLocked = false;
	#endif
}

DARKSDK void WriteImage ( int iID, int iX, int iY, int iA, int iR, int iG, int iB )
{
	if ( !UpdatePtrImage ( iID ) )
		return;

	if ( !m_imgptr->bLocked )
		Error1 ( "Unable to modify texture data for image library as it isn't locked" );
	
	#ifdef DX11
	#else
	DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
	pPix [ ( ( m_imgptr->d3dlr.Pitch >> 4 ) * iX ) + iY ] = GGCOLOR_ARGB ( iA, iR, iG, iB );
	#endif
}

DARKSDK void GetImage ( int iID, int iX, int iY, int* piR, int* piG, int* piB )
{
	// get a pixel at x, y

	// check pointers are valid
	if ( !piR || !piG || !piB )
		return;

	// update the internal data
	if ( !UpdatePtrImage ( iID ) )
		return;

	// check the image is locked
	if ( !m_imgptr->bLocked )
		Error1 ( "Unable to get texture data for image library as it isn't locked" );

	#ifdef DX11
	#else
	// get a pointer to the data
	DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
	DWORD  pGet;

	// get offset in file
	pGet = pPix [ ( ( m_imgptr->d3dlr.Pitch >> 4 ) * iX ) + iY ];

	// break value down
	DWORD dwAlpha = pGet >> 24;						// get alpha
	DWORD dwRed   = ((( pGet ) >> 16 ) & 0xff );	// get red
	DWORD dwGreen = ((( pGet ) >>  8 ) & 0xff );	// get green
	DWORD dwBlue  = ((( pGet ) )       & 0xff );	// get blue

	// save values
	*piR = dwRed;
	*piG = dwGreen;
	*piB = dwBlue;
	#endif
}

DARKSDK LPGGTEXTURE MakeFormat ( int iID, int iWidth, int iHeight, GGFORMAT format, DWORD dwUsageRenderTarget )
{
	// make a new image
	// mike : can specify iID of -1 which means image is added to list (internal use only)

	// variable declarations
	tagImgData*	test = NULL;		// pointer to data structure
	
	// create a new block of memory
	test = new tagImgData;
	memset(test, 0, sizeof(tagImgData));

	// check the memory was created
	if ( test == NULL )
		return NULL;

	// clear out the memory
	memset ( test, 0, sizeof ( tagImgData ) );

	#ifdef DX11

	wiGraphics::TextureDesc desc;
	desc.ArraySize = 1;
	desc.BindFlags = wiGraphics::BIND_SHADER_RESOURCE;
	if ( dwUsageRenderTarget != 0 ) desc.BindFlags |= wiGraphics::BIND_RENDER_TARGET;
	desc.CPUAccessFlags = 0;
	desc.Width = iWidth;
	desc.Height = iHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.MiscFlags = 0;
	desc.Usage = wiGraphics::USAGE_DEFAULT;
	desc.layout = wiGraphics::IMAGE_LAYOUT_SHADER_RESOURCE;
	desc.Format = wiGraphics::FORMAT_B8G8R8A8_UNORM;
	desc.type = wiGraphics::TextureDesc::TEXTURE_2D;
		
	wiGraphics::Texture* pTexture = new wiGraphics::Texture();
	test->lpTexture = pTexture;
	bool result = wiRenderer::GetDevice()->CreateTexture( &desc, nullptr, pTexture );
	if ( !result )
	{
		Error1 ( "Failed to create new image" );
		return NULL;
	}

	// setup properties
	test->iHeight = iHeight;		// store the width
	test->iWidth  = iWidth;			// store the height
	test->iDepth  = ImageGetBitDepthFromFormat ( format );
	test->bLocked = false;			// set locked to false

	test->fTexUMax=1.0f;
	test->fTexVMax=1.0f;

	#endif

	// Ensure smalltextres are handled
	if(test->fTexUMax>1.0f) test->fTexUMax=1.0f;
	if(test->fTexVMax>1.0f) test->fTexVMax=1.0f;

	// add to the list
    m_List.insert( std::make_pair(iID, test) );

	// ensure sprites all updated
	UpdateAllSprites();

	return test->lpTexture;
}

DARKSDK LPGGTEXTURE MakeImage ( int iID, int iWidth, int iHeight )
{
	return MakeFormat ( iID, iWidth, iHeight, g_DefaultGGFORMAT, 0 );
}

DARKSDK LPGGTEXTURE MakeImageUsage ( int iID, int iWidth, int iHeight, DWORD dwUsage )
{
	return MakeFormat ( iID, iWidth, iHeight, g_DefaultGGFORMAT, dwUsage );
}

DARKSDK LPGGTEXTURE MakeImageJustFormat ( int iID, int iWidth, int iHeight, GGFORMAT Format )
{
	return MakeFormat ( iID, iWidth, iHeight, Format, 0 );
}

DARKSDK LPGGTEXTURE MakeImageRenderTarget ( int iID, int iWidth, int iHeight, GGFORMAT Format )
{
	return MakeFormat ( iID, iWidth, iHeight, Format, GGUSAGE_RENDERTARGET );
}

DARKSDK void SetCubeFace ( int iID, LPGGCUBETEXTURE pCubeMap, int iFace )
{
	// get ptr to image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// set cube reference details
	m_imgptr->pCubeMapRef = pCubeMap;
	m_imgptr->iCubeMapFace = iFace;
}

DARKSDK void GetCubeFace ( int iID, LPGGCUBETEXTURE* ppCubeMap, int* piFace )
{
	// get ptr to image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// return cube reference details
	if ( ppCubeMap ) *ppCubeMap = m_imgptr->pCubeMapRef;
	if ( piFace ) *piFace = m_imgptr->iCubeMapFace;
}

DARKSDK void SetMipmapMode ( bool bMode )
{
	// switch mip mapping on or off
	#ifdef DX11
	#else
	if ( !bMode )
	{
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
	}
	else
	{
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, GGTEXF_LINEAR );
		m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );
	}
	#endif
}

DARKSDK void SetMipmapType ( int iType )
{
	// set the type of mip mapping used
	// iType possible values
	// 0 - none
	// 1 - point
	// 2 - linear ( default )

	#ifdef DX11
	#else
	switch ( iType )
	{
		case 0:
		{
			// use no mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );//GGTEXF_NONE ); // may not exist on HW driver any more!
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );//GGTEXF_NONE );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );//GGTEXF_NONE );
		}
		break;

		case 1:
		{
			// set up point mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
		}
		break;

		case 2:
		{
			// set up linear mip mapping
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MAGFILTER, GGTEXF_LINEAR );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MINFILTER, GGTEXF_LINEAR );
			m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPFILTER, GGTEXF_LINEAR );	
		}
		break;
	}
	#endif
}
	
DARKSDK void SetMipmapBias ( float fBias )
{
	// set the bias for the mip mapping, this allows you
	// to set the distance at which point the mip mapping
	// is brought into action
	#ifdef DX11
	#else
	m_pD3D->SetSamplerState ( 0, D3DSAMP_MIPMAPLODBIAS, *( ( LPDWORD ) ( &fBias ) ) );
	#endif
}

DARKSDK void SetMipmapNum ( int iNum )
{
	// set the number of mip maps used, remember that if
	// you increase this value it will take up a lot more
	// video memory - turn this down if you don't need it
	m_iMipMapNum = iNum;
}

DARKSDK void SetImageTranslucency ( int iID, int iPercent )
{
	// set the translucency for an image
	if ( !UpdatePtrImage ( iID ) )
		return;

	// now lock the surface
	#ifdef DX11
	#else
	HRESULT hr;
	if ( FAILED ( hr = m_imgptr->lpTexture->LockRect ( 0, &m_imgptr->d3dlr, 0, 0 ) ) )
		Error ( "Failed to lock texture in translucency for image library" );
	{
		// get a pointer to surface data
		DWORD* pPix = ( DWORD* ) m_imgptr->d3dlr.pBits;
		DWORD  pGet;

		for ( DWORD y = 0; y < (DWORD)m_imgptr->iHeight; y++ )
		{
			for ( DWORD x = 0; x < (DWORD)m_imgptr->iWidth; x++ )
			{
				pGet = *pPix;

				DWORD dwAlpha = pGet >> 24;
				DWORD dwRed   = ((( pGet ) >> 16 ) & 0xff );
				DWORD dwGreen = ((( pGet ) >>  8 ) & 0xff );
				DWORD dwBlue  = ((( pGet ) )       & 0xff );
				
				if ( *pPix != 0 )
					*pPix++ = GGCOLOR_ARGB ( iPercent, dwRed, dwGreen, dwBlue );
				else
					*pPix++;
			}
		}
	}
	m_imgptr->lpTexture->UnlockRect ( NULL );
	#endif
}

DARKSDK bool ImageExist ( int iID )
{
	// returns true if the image exists
	if ( !UpdatePtrImage ( iID ) )
		return false;

	// return true
	return true;
}

int LoadImageCoreRetainNameTime = 0;

// This load is the MAIN IMAGE LOADER
DARKSDK bool LoadImageCoreRetainName ( char* szRealName, char* szFilename, int iID, int iTextureFlag, bool bIgnoreNegLimit, int iDivideTextureSize )
{
	// iTextureFlag (0-default/1-sectionofplate/2-cube)
	// iDivideTextureSize (0-leave size,0>divide by)
	if ( bIgnoreNegLimit==false )
	{
		if( iID<1 || iID>MAXIMUMVALUE )
		{
			RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER, szFilename);
			return false;
		}
	}

	DARKSDK int Timer(void);
	int ts = Timer();

	//PE: Debug , show list of combination checked.,
	//char mdebug[2048];
	//sprintf(mdebug, "TEST: %s (%s), %d ", szFilename, szRealName, iID);
	//timestampactivity(0, mdebug);

	//PE: Check if same image is already loaded into this ID.
	int			iLen1;				// used for checking length of strings
	LPSTR pNameForInternalList = szFilename;
	if (strlen(szRealName) > 1) pNameForInternalList = szRealName;
	// get length of filename and copy to shortfilename
	iLen1 = lstrlen(pNameForInternalList);											// get length

	//Disabled for now, need a way to force reload.
//	ImagePtr pImage = m_List.find(iID);
//	if (pImage != m_List.end())
//	{
//		if( _strnicmp(pNameForInternalList, pImage->second->szShortFilename, iLen1) == 0 ) {
////			char mdebug[2048];
////			sprintf(mdebug, "CACHE HIT: %s ", pNameForInternalList);
////			timestampactivity(0, mdebug);
//			return true;
//		}
//	}

	
    RemoveImage( iID );

	// loads in image into the bank
	tagImgData*	test = NULL;		// pointer to data structure
	int			iLen2;				// used for checking length of strings
	char		szTest [ 256 ];		// text buffer
	bool		bFound = false;		// flag which checks if we need to load the texture

	// create a new block of memory
	test = new tagImgData;

	// check the memory was created
	if ( test == NULL )
		return false;

	// clear out the memory
	memset(test, 0, sizeof(tagImgData));

	// clear out text buffers
	memset ( test->szLongFilename,  0, sizeof ( char ) * 256 );		// clear out long filename
	memset ( test->szShortFilename, 0, sizeof ( char ) * 256 );		// clear out short filename
	memset ( szTest,                0, sizeof ( char ) * 256 );		// clear out test buffer

	memcpy ( test->szShortFilename, pNameForInternalList, sizeof ( char ) * iLen1 );	// copy to short filename

	// sort out full filename
	GetCurrentDirectory ( 256, szTest );	// get the apps full directory e.g. "c:\test\"
	iLen1 = lstrlen ( szTest );				// get the length of the full directory
	iLen2 = lstrlen ( pNameForInternalList );			// get the length of the passed filename
	
	// copy memory
	memcpy ( test->szLongFilename,         szTest,         sizeof ( char ) * iLen1 );	// copy the full dir to the long filename

	// U74 BETA9 - 030709 - fix longfilename if full dir has no \ and filename does not start with
	// memcpy ( test->szLongFilename + iLen1, szFilename + 1, sizeof ( char ) * iLen2 );	// append the filename onto the long file name
	if ( pNameForInternalList[0]!='\\' )
	{
		if ( szTest[iLen1]!='\\' )
		{
			test->szLongFilename[iLen1]='\\';
			memcpy ( test->szLongFilename + iLen1 + 1, pNameForInternalList, sizeof ( char ) * iLen2 );
		}
		else
			memcpy ( test->szLongFilename + iLen1, pNameForInternalList, sizeof ( char ) * iLen2 );
	}
	else
	{
		// legacy support
		memcpy ( test->szLongFilename + iLen1, pNameForInternalList + 1, sizeof ( char ) * iLen2 );
	}

	// The default is a setting of zero (0)
	GGIMAGE_INFO info;
	if( iTextureFlag==1 )
	{
		// loads image into a section of the texture plate
		if ( m_bSharing && !bFound )
			test->lpTexture = GetTexture ( szFilename, &info, 1 );	// load the perfect texture
		else
			test->lpTexture = GetTexture ( szFilename, &info, 1 );	// load the perfect texture

		// load failed
		if ( test->lpTexture==NULL )
		{
			SAFE_DELETE(test);
			return false;
		}

		// Set image settings
		test->iHeight = m_iHeight;					// store the width
		test->iWidth  = m_iWidth;					// store the height

		// get actual dimensions of texture/image
		#ifdef DX11
		test->fTexUMax=1.0f;
		test->fTexVMax=1.0f;
		#else
		D3DSURFACE_DESC imageddsd;
		test->lpTexture->GetLevelDesc(0, &imageddsd);
		test->fTexUMax=(float)test->iWidth/(float)imageddsd.Width;
		test->fTexVMax=(float)test->iHeight/(float)imageddsd.Height;
		#endif

		// Ensure smalltextres are handled
		if(test->fTexUMax>1.0f) test->fTexUMax=1.0f;
		if(test->fTexVMax>1.0f) test->fTexVMax=1.0f;
	}
	else
	{
		// Load Image Into Whole Texture Plate
		LoadImageCoreFullTex ( szFilename, &test->lpTexture, &info, iTextureFlag, iDivideTextureSize );	// loads into whole texture

		// load failed
		if ( test->lpTexture==NULL )
		{
			SAFE_DELETE(test);
			return false;
		}

		// get file image info
		test->iHeight = info.Height;
		test->iWidth  = info.Width;

		// Entire texture used
		test->fTexUMax=1.0f;
		test->fTexVMax=1.0f;
	}

	// Get depth of texture
	#ifdef DX11

	// Get depth from format
	test->iDepth = ImageGetBitDepthFromFormat(info.Format);

	#else
	D3DSURFACE_DESC desc;
	test->lpTexture->GetLevelDesc(0, &desc);
	test->iDepth  = ImageGetBitDepthFromFormat(desc.Format);
	#endif

	// load failed
	if ( test->lpTexture==NULL )
	{
		SAFE_DELETE(test);
		return false;
	}

	// fill out rest of structure
	test->bLocked = false;

#ifdef PETESTIMAGEUSAGE
	test->AccessCountGPU = 0;
	test->AccessCountCPU = 0;
	test->iImageLoadTime = Timer() - ts;
	LoadImageCoreRetainNameTime += Timer() - ts;
#endif

	// add to the list
    m_List.insert( std::make_pair(iID, test) );

	// ensure sprites all updated
	// V109 BETA3 - only need to update image/textures, not internal textures (negatives)
	if ( iID>0 ) UpdateAllSprites();

	return true;
}

DARKSDK bool LoadImageCore ( char* szFilename, int iID, int iTextureFlag, bool bIgnoreNegLimit, int iDivideTextureSize )
{
	return LoadImageCoreRetainName ( "", szFilename, iID, iTextureFlag, bIgnoreNegLimit, iDivideTextureSize );
}

DARKSDK bool LoadImageCore ( char* szFilename, int iID )
{
	return LoadImageCore ( szFilename, iID, 1, false, 0 );
}

DARKSDK bool SaveImageCoreAsTexSurface(char* szFilename, LPGGSURFACE* pTexSurface, int iCompressionMode)
{
	assert( 0 && "DX11 not in use" );
	return false;
	/*
	D3DX11_IMAGE_FILE_FORMAT DestFormat = D3DX11_IFF_BMP;
	LPSTR szFilenameExt = szFilename + strlen(szFilename)-4;
	if ( _strnicmp ( szFilenameExt, ".bmp", 4 )==NULL ) DestFormat = D3DX11_IFF_BMP;
	if ( _strnicmp ( szFilenameExt, ".dds", 4 )==NULL ) DestFormat = D3DX11_IFF_DDS;
	if ( _strnicmp ( szFilenameExt, ".jpg", 4 )==NULL ) DestFormat = D3DX11_IFF_JPG;
	if ( _strnicmp ( szFilenameExt, ".png", 4 )==NULL ) DestFormat = D3DX11_IFF_PNG;

	GGSURFACE_DESC srcddsd;
	if (*pTexSurface)
	{
		// determine size of surface
		HRESULT hRes;
		(*pTexSurface)->GetDesc(&srcddsd);

		// use automatic compression
		GGFORMAT dwD3DSurfaceFormat = DXGIFORMATR8G8B8A8UNORM;
		switch (iCompressionMode)
		{
		case 1: dwD3DSurfaceFormat = GGFMT_DXT1; break;
		case 2: dwD3DSurfaceFormat = GGFMT_DXT2; break;
		case 3: dwD3DSurfaceFormat = GGFMT_DXT3; break;
		case 4: dwD3DSurfaceFormat = GGFMT_DXT4; break;
		case 5: dwD3DSurfaceFormat = GGFMT_DXT5; break;
		}

		// final format to save the image file
		DXGI_FORMAT pFinalSaveFormat = srcddsd.Format;

		// if not DDS saving, and format a BC3/DXT, change to DXGI_FORMAT_R8G8B8A8_UNORM
#ifdef WICKEDENGINE
// need to select allowable conversions
		if (DestFormat != D3DX11_IFF_DDS && srcddsd.Format >= DXGI_FORMAT_BC1_TYPELESS && srcddsd.Format <= DXGI_FORMAT_BC5_SNORM)
		{
			// filenames to WCHAR
			wchar_t wFilenamePlate[512];
			MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wFilenamePlate, sizeof(wFilenamePlate));

			// create and load the texture selected
			LPGGTEXTURE pPlateSurface = NULL;

			// load compressed texture to a plate
			DirectX::ScratchImage imageTexturePlate;
			HRESULT hr = CaptureTexture(m_pD3D, m_pImmediateContext, *pTexSurface, imageTexturePlate);
			if (SUCCEEDED(hr))
			{
				// decompress the plate
				DirectX::ScratchImage uncompressedTexturePlate;
				hr = Decompress(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
					DXGI_FORMAT_B8G8R8A8_UNORM, uncompressedTexturePlate);

				// save as an uncompressed file
				const DirectX::Image* img = uncompressedTexturePlate.GetImages();
				hr = SaveToWICFile(*img, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wFilenamePlate, NULL);

				// create mipmaps for final image and save
				//DirectX::ScratchImage mipChain;
				//hr = GenerateMipMaps( uncompressedTexturePlate.GetImages(), uncompressedTexturePlate.GetImageCount(), uncompressedTexturePlate.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );
				//const DirectX::Image* img = mipChain.GetImages();
				//hr = SaveToWICFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				if (FAILED(hr))
				{
					char pStrClue[512];
					wsprintf(pStrClue, "Failed to save texture:%s", szFilename);
					RunTimeError(RUNTIMEERROR_IMAGEERROR, pStrClue);
					SAFE_RELEASE(*pTexSurface);
					return false;
				}

				// free surface and leave, completed save with DirectXTex functions (as needed format conversion)
				SAFE_RELEASE(*pTexSurface);
				return true;
			}
		}
		#else
		//if (DestFormat != D3DX11_IFF_DDS) //messed up terrian texture saving!!
		//{
		//	pFinalSaveFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		//}
		#endif

		LPGGSURFACE pSourceDDS = NULL;
		GGSURFACE_DESC StagedDesc = { srcddsd.Width, srcddsd.Height, srcddsd.MipLevels, srcddsd.ArraySize, pFinalSaveFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
		HRESULT hr = m_pD3D->CreateTexture2D(&StagedDesc, NULL, (ID3D11Texture2D**)&pSourceDDS);
		if (pSourceDDS)
		{
			// first copy texture to a stage ready for reading (BGRA)
			m_pImmediateContext->CopyResource(pSourceDDS, *pTexSurface);

			// Copy Resource cannot convert pixel formats, so do it manually to get (RGBA)
			if ( srcddsd.Format == DXGI_FORMAT_B8G8R8A8_UNORM && dwD3DSurfaceFormat == DXGIFORMATR8G8B8A8UNORM)
			{
				LPGGSURFACE pDestDDS = NULL;
				GGSURFACE_DESC DestDesc = { srcddsd.Width, srcddsd.Height, 1, 1, dwD3DSurfaceFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0 };
				HRESULT hr = m_pD3D->CreateTexture2D(&DestDesc, NULL, (ID3D11Texture2D**)&pDestDDS);
				if (pDestDDS)
				{
					D3D11_MAPPED_SUBRESOURCE srcMapped;
					HRESULT hr = m_pImmediateContext->Map(pSourceDDS, 0, D3D11_MAP_READ, 0, &srcMapped);
					if (SUCCEEDED(hr))
					{
						D3D11_MAPPED_SUBRESOURCE destMapped;
						//HRESULT hr = m_pImmediateContext->Map( pDestDDS, 0, D3D11_MAP_READ, 0, &destMapped );
						HRESULT hr = m_pImmediateContext->Map(pDestDDS, 0, D3D11_MAP_WRITE, 0, &destMapped);
						if (SUCCEEDED(hr))
						{
							const size_t size = srcddsd.Width*srcddsd.Height * 4;
							unsigned char* pSrc = static_cast<unsigned char*>(srcMapped.pData);
							unsigned char* pDest = static_cast<unsigned char*>(destMapped.pData);
							int offsetSrc = 0;
							int offsetDst = 0;
							int rowOffset = srcMapped.RowPitch % srcddsd.Width;
							//PE: Saveimage did not work on NPO2 images, we need the destMapped.RowPitch.
							int rowDestOffset = destMapped.RowPitch % DestDesc.Width;
							for (int row = 0; row < srcddsd.Height; ++row)
							{
								for (int col = 0; col < srcddsd.Width; ++col)
								{
									pDest[offsetDst] = pSrc[offsetSrc + 2];
									pDest[offsetDst + 1] = pSrc[offsetSrc + 1];
									pDest[offsetDst + 2] = pSrc[offsetSrc];
									if(g_bDontUseImageAlpha)
										pDest[offsetDst + 3] = 255;
									else
										pDest[offsetDst + 3] = pSrc[offsetSrc + 3];
									offsetSrc += 4;
									offsetDst += 4;
								}
								offsetSrc += rowOffset;
								offsetDst += rowDestOffset;
							}
							m_pImmediateContext->Unmap(pDestDDS, 0);
						}
						m_pImmediateContext->Unmap(pSourceDDS, 0);
					}
				}
				SAFE_RELEASE(pSourceDDS);
				SAFE_RELEASE(*pTexSurface);
				*pTexSurface = pDestDDS;
			}
			else
			{
				SAFE_RELEASE(*pTexSurface);
				*pTexSurface = pSourceDDS;
			}
		}

		// save surface of image to file
		try
		{
			#ifdef WICKEDENGINE
			//PE: Test save. to remove DX10. cant convert a cubemap.
			//LB: Reactivated this approach to saving DDS files (no need to save cube maps right now that I recall)
			DirectX::ScratchImage imageTexturePlate;
			HRESULT hr = CaptureTexture(m_pD3D, m_pImmediateContext, *pTexSurface, imageTexturePlate);
			//std::unique_ptr<DirectX::ScratchImage> convertedImage(new (std::nothrow)DirectX::ScratchImage);
			//hr = DirectX::Convert(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), dwD3DSurfaceFormat, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, *convertedImage);
			wchar_t wTexSaveFilename[512];
			MultiByteToWideChar(CP_ACP, 0, szFilename, -1, wTexSaveFilename, sizeof(wTexSaveFilename));
			//hRes = DirectX::SaveToDDSFile(convertedImage->GetImages(), convertedImage->GetImageCount(), convertedImage->GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexSaveFilename);
			hRes = DirectX::SaveToDDSFile(imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexSaveFilename);
			#else
			hRes = D3DX11SaveTextureToFile(m_pImmediateContext, *pTexSurface, DestFormat, szFilename);
			if (FAILED(hRes))
			{
				char pStrClue[512];
				wsprintf(pStrClue, "tex:%d filename:%s", (int)*pTexSurface, szFilename);
				RunTimeError(RUNTIMEERROR_IMAGEERROR, pStrClue);
				SAFE_RELEASE(*pTexSurface);
				return false;
			}
			#endif
		}
		catch (...)
		{
			// this can fail when file is locked, so try waiting 3 seconds, then try again, else silent fail so no crash!
			Sleep(3000);
			hRes = D3DX11SaveTextureToFile(m_pImmediateContext, *pTexSurface, DestFormat, szFilename);
			if (FAILED(hRes))
			{
				// so joy, silent fail!
				SAFE_RELEASE(*pTexSurface);
				return false;
			}
		}
	}
	return true;
	*/
}

DARKSDK bool SaveImageCore ( char* szFilename, int iID, int iCompressionMode )
{
	assert( 0 && "DX11 not in use" );
	return false;
	/*
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	if ( !UpdatePtrImage ( iID ) )
	{
		#ifdef WICKEDENGINE
		//Silent error now log instead. So we know exactly where the error is.
		if (iMaxPasteImageLogs > 0)
		{
			char pErr[512]; sprintf(pErr, "Error Image Not Found: SaveImageCore(%s,%d,%d) .", szFilename , iID, iCompressionMode );
			timestampactivity(0, pErr);
			iMaxPasteImageLogs--;
		}
		#else
		RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
		#endif
		return false;
	}
	if ( szFilename==NULL )
	{
		RunTimeError(RUNTIMEERROR_FILENOTEXIST);
		return false;
	}

	// determine format from extension
	#ifdef DX11

	// determine region of surface to save
	LPGGSURFACE pTexSurface = NULL;
	m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTexSurface);
	SaveImageCoreAsTexSurface(szFilename, &pTexSurface, iCompressionMode);
	
	// free surface
	SAFE_RELEASE(pTexSurface);
	#endif

	// success
	return true;
	*/
}

DARKSDK bool SaveImageCore ( char* szFilename, int iID )
{
	// default behaviour
	return SaveImageCore ( szFilename, iID, 0 );
}

DARKSDK void CreateReplaceImage ( int iID, int iTexSize, ID3D11Texture2D* pTex, ID3D11ShaderResourceView* pView )
{
	assert( 0 && "DX11 not in use" );
	/*
	// establish if image exists or not
	if( iID < 1 || iID > MAXIMUMVALUE )
		return;

	if ( !UpdatePtrImage ( iID ) )
	{
		// create a new image
		MakeFormat ( iID, iTexSize, iTexSize, DXGIFORMATR8G8B8A8UNORM, 0 );
		UpdatePtrImage ( iID );
	}

	// valid image can be be overridden with new image details
	m_imgptr->lpTexture = pTex;
	m_imgptr->lpTextureView = pView;
	m_imgptr->iWidth = iTexSize;
	m_imgptr->iHeight = iTexSize;
	SAFE_DELETE ( m_imgptr->lpName );
	strcpy ( m_imgptr->szLongFilename, "" );
	strcpy ( m_imgptr->szShortFilename, "" );
	m_imgptr->fTexUMax = 1.0f;
	m_imgptr->fTexVMax = 1.0f;
	*/
}
#ifdef WICKEDENGINE
//PE: GameGuru IMGUI.
#include "..\..\..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include ".\..\..\Guru-WickedMAX\wickedcalls.h"

int g_iSpecialGrabImageMode = 0;

DARKSDK void SetGrabImageMode(int iMode)
{
	// set this mode to 1 when you want to use DirectXTex method to convert and grab from the wicked backbuffer
	g_iSpecialGrabImageMode = iMode;
}

DARKSDK bool GrabImageCore(int iID, int iX1, int iY1, int iX2, int iY2, int iTextureFlagForGrab)
{
	//PE: Perhaps wait for GPU to finish its threads ?

	assert( 0 && "DX11 not in use" );
	return false;
	/*
	if (iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	if (iX1 >= iX2 || iY1 >= iY2)
	{
		RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
		return false;
	}

	//Finish any imgui stuff.
	extern LPGGDEVICE				m_pD3D;
	extern LPGGIMMEDIATECONTEXT		m_pImmediateContext;
	extern bool bRenderTabTab;
	extern bool bRenderNextFrame;
	if (bRenderTabTab) {
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::Render();
		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
		bRenderTabTab = false;
		bRenderNextFrame = true;
		
		//DARKSDK void SetCurrentBitmap(int iID);
		//SetCurrentBitmap(2);
		//CLS(Rgb(64, 64, 64));

		//PE: Draw now.
		WickedCall_DrawImguiNow();
	}

	if (!g_pGlob->pCurrentBitmapSurface)
	{
		return false;
	}

	// Size of grab
	GGFORMAT backFormat;
	GGSURFACE_DESC ddsd;
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if (pBackBuffer)
	{
		// get format of backbuffer
		pBackBuffer->GetDesc(&ddsd);
		backFormat = ddsd.Format;
		if (iX2 > (int)ddsd.Width || iY2 > (int)ddsd.Height)
		{
			iX2 = ddsd.Width;
			iY2 = ddsd.Height;
			if (iX1 >= iX2 || iY1 >= iY2) return false;
			//RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
			//return false;
		}
	}

	// Image size
	int iImageWidth = iX2 - iX1;
	int iImageHeight = iY2 - iY1;

	// Get current render target surface
	if (pBackBuffer)
	{
		// check if image already exists of same size and type
		if (UpdatePtrImage(iID))
		{
			// check against existing
			if (m_imgptr->iWidth != iImageWidth || m_imgptr->iHeight != iImageHeight)
			{
				// existing image and new image are different sizes - so delete any existing image
				RemoveImage(iID);
			}
		}

		// if image format not internal texture format, delete so can be recreated
		if (m_imgptr)
		{
			GGSURFACE_DESC imgddsd;
			LPGGSURFACE pTextureInterface = NULL;
			m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
			pTextureInterface->GetDesc(&imgddsd);
			SAFE_RELEASE(pTextureInterface);
			if (imgddsd.Format != g_DefaultGGFORMAT)
			{
				RemoveImage(iID);
			}
		}

		// determine grabimage mode
		if ( g_iSpecialGrabImageMode == 1 )
		{
			// special case for creating thumbnails, grabbed from wicked backbuffer
			// if backbuffer is strange format, need to convert it to compatible one
			//PE: Wicked backbuffer is DXGI_FORMAT_R10G10B10A2_UNORM . GG use DXGI_FORMAT_B8G8R8A8_UNORM.
			LPGGTEXTURE pNewCroppedTexture = NULL;
			LPGGTEXTURE pNewCorrectBBTexture = NULL;
			// then crop it to required new texture
			if (backFormat == g_DefaultGGFORMAT)
			{
				//PE: Backbuffer already in correct format. no need to convert. way faster.
				D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
				pNewCroppedTexture = CreateCroppedTexture(pBackBuffer, rc);
			}
			else if (iX1 == 0 && iY1 == 0 && ddsd.Width == iX2 && ddsd.Height == iY2)
			{
				//PE: No need to crop.
				pNewCroppedTexture = ConvertBackBufferToNewFormat(pBackBuffer, g_DefaultGGFORMAT);;
			}
			else
			{
				pNewCorrectBBTexture = ConvertBackBufferToNewFormat(pBackBuffer, g_DefaultGGFORMAT);
				D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
				pNewCroppedTexture = CreateCroppedTexture(pNewCorrectBBTexture, rc);
				// free corrected backbuffer we created to do the copy
				SAFE_RELEASE(pNewCorrectBBTexture);

			}
			// create image to store cropped texture
			if (m_imgptr == NULL) {
				MakeFormat(iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0);
			}
			if (UpdatePtrImage(iID))
			{
				
				// put cropped texture in image
				SAFE_RELEASE(m_imgptr->lpTexture);
				m_imgptr->lpTexture = pNewCroppedTexture;

				if(m_imgptr->lpTextureView) //PE: Fix memory leak.
					SAFE_RELEASE(m_imgptr->lpTextureView);

				m_imgptr->lpTextureView = NULL;
				CreateShaderResourceViewFor(m_imgptr, 0, g_DefaultGGFORMAT);

				
				// get desc of image
				GGSURFACE_DESC srcddsd;
				LPGGSURFACE pTextureInterface = NULL;
				m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
				pTextureInterface->GetDesc(&srcddsd);
				SAFE_RELEASE(pTextureInterface);

				// get actual dimensions of texture/image
				m_imgptr->fTexUMax = (float)m_imgptr->iWidth / (float)srcddsd.Width;
				m_imgptr->fTexVMax = (float)m_imgptr->iHeight / (float)srcddsd.Height;

				// Ensure smalltextres are handled
				if (m_imgptr->fTexUMax > 1.0f) m_imgptr->fTexUMax = 1.0f;
				if (m_imgptr->fTexVMax > 1.0f) m_imgptr->fTexVMax = 1.0f;
				
			}
			else
			{
				RunTimeError(RUNTIMEERROR_IMAGEERROR);
				return false;
			}

			// free corrected backbuffer we created to do the copy
			//SAFE_RELEASE(pNewCorrectBBTexture);
		}
		else
		{
			// normal grabimage, grabbing from old graphics engine surface (prompt3d text creation)
			// create temp image texture to copy backbuffer to
			LPGGSURFACE pTempTexture = NULL;
			GGSURFACE_DESC TempTextureDesc = { iImageWidth, iImageHeight, 1, 1, backFormat, 1, 0, D3D11_USAGE_DEFAULT, 0, 0, 0 };
			m_pD3D->CreateTexture2D(&TempTextureDesc, NULL, &pTempTexture);
			if (pTempTexture)
			{
				// copy backbuffer to temp texture
				D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1 + iImageWidth), (LONG)(iY1 + iImageHeight), 1 };
				m_pImmediateContext->CopySubresourceRegion(pTempTexture, 0, 0, 0, 0, pBackBuffer, 0, &rc);

				// create image
				if (m_imgptr == NULL) MakeFormat(iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0);
				if (UpdatePtrImage(iID))
				{
					// get desc of destination texture
					GGSURFACE_DESC srcddsd;
					LPGGSURFACE pTextureInterface = NULL;
					m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
					pTextureInterface->GetDesc(&srcddsd);
					SAFE_RELEASE(pTextureInterface);

					// load grabbed surface data into destination texture
					D3D11_BOX rc = { 0, 0, 0, (LONG)(iImageWidth), (LONG)(iImageHeight), 1 };
					m_pImmediateContext->CopySubresourceRegion(m_imgptr->lpTexture, 0, 0, 0, 0, pTempTexture, 0, &rc);

					// get actual dimensions of texture/image
					m_imgptr->fTexUMax = (float)m_imgptr->iWidth / (float)srcddsd.Width;
					m_imgptr->fTexVMax = (float)m_imgptr->iHeight / (float)srcddsd.Height;

					// Ensure smalltextres are handled
					if (m_imgptr->fTexUMax > 1.0f) m_imgptr->fTexUMax = 1.0f;
					if (m_imgptr->fTexVMax > 1.0f) m_imgptr->fTexVMax = 1.0f;
				}
				else
				{
					RunTimeError(RUNTIMEERROR_IMAGEERROR);
					return false;
				}

				// free work-newsurface
				SAFE_RELEASE(pTempTexture);
			}
		}
	}
	else
	{
		RunTimeError(RUNTIMEERROR_IMAGEERROR);
		return false;
	}
	return true;
	*/
}
#else
DARKSDK bool GrabImageCore ( int iID, int iX1, int iY1, int iX2, int iY2, int iTextureFlagForGrab )
{
	#ifdef WICKEDENGINE
	// cannot grab from old graphics system - need to new one exclusive to wicked
	#else
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	if(iX1>=iX2 || iY1>=iY2)
	{
		RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
		return false;
	}

	//#ifdef WICKEDENGINE
	// No grab from backbuffer functionality for now (is this best way to do thumbnails, etc)
	//#else

	#ifdef DX11
	// Size of grab
	GGFORMAT backFormat;
	GGSURFACE_DESC ddsd;
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if(pBackBuffer)
	{
		// get format of backbuffer
		pBackBuffer->GetDesc(&ddsd);
		backFormat = ddsd.Format;
		if(iX2>(int)ddsd.Width || iY2>(int)ddsd.Height)
		{
			RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
			return false;
		}
	}

	// Image size
	int iImageWidth=iX2-iX1;
	int iImageHeight=iY2-iY1;

	// Get current render target surface
	if(pBackBuffer)
	{
		// check if image already exists of same size and type
		if ( UpdatePtrImage ( iID ) )
		{
			// check against existing
			if(m_imgptr->iWidth != iImageWidth || m_imgptr->iHeight != iImageHeight )
			{
				// existing image and new image are different sizes - so delete any existing image
                RemoveImage( iID );
			}
		}

		// if image format not internal texture format, delete so can be recreated
		if ( m_imgptr )
		{
			GGSURFACE_DESC imgddsd;
			LPGGSURFACE pTextureInterface = NULL;
			m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
			pTextureInterface->GetDesc(&imgddsd);
			SAFE_RELEASE ( pTextureInterface );
			if ( imgddsd.Format != g_DefaultGGFORMAT )
			{
				RemoveImage( iID );
			}
		}

		// create temp image texture to copy backbuffer to
		LPGGSURFACE pTempTexture = NULL;
		GGSURFACE_DESC TempTextureDesc = { iImageWidth, iImageHeight, 1, 1, backFormat, 1, 0, D3D11_USAGE_DEFAULT, 0, 0, 0 };
		m_pD3D->CreateTexture2D( &TempTextureDesc, NULL, &pTempTexture );
		if ( pTempTexture )
		{
			// copy backbuffer to temp texture
			//D3D11_BOX rc = { 0, 0, 0, (LONG)(iImageWidth), (LONG)(iImageHeight), 1 }; 
			D3D11_BOX rc = { iX1, iY1, 0, (LONG)(iX1+iImageWidth), (LONG)(iY1+iImageHeight), 1 }; 
			m_pImmediateContext->CopySubresourceRegion ( pTempTexture, 0, 0, 0, 0, pBackBuffer, 0, &rc );

			// create image
			if ( m_imgptr == NULL ) MakeFormat ( iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0 );
			if ( UpdatePtrImage ( iID ) )
			{
				// get desc of destination texture
				GGSURFACE_DESC srcddsd;
				LPGGSURFACE pTextureInterface = NULL;
				m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
				pTextureInterface->GetDesc(&srcddsd);
				SAFE_RELEASE ( pTextureInterface );

				/* no need for this, can simply delete image and recreate as correct format
				// first convert texture to BGRA if RGBA (from a load) - for internal resource copying
				if ( srcddsd.Format == DXGI_FORMAT_R8G8B8A8_UNORM )
				{
					LPGGSURFACE pDestDDS = NULL;
					LPGGSURFACE pSourceDDS = pTempTexture;
					GGSURFACE_DESC DestDesc = { srcddsd.Width, srcddsd.Height, 1, 1, g_DefaultGGFORMAT, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, 0 };
					HRESULT hr = m_pD3D->CreateTexture2D( &DestDesc, NULL, (ID3D11Texture2D**)&pDestDDS );
					if ( pDestDDS )
					{
						D3D11_MAPPED_SUBRESOURCE srcMapped;
						HRESULT hr = m_pImmediateContext->Map( pSourceDDS, 0, D3D11_MAP_READ, 0, &srcMapped );
						if ( SUCCEEDED( hr ) )
						{
							D3D11_MAPPED_SUBRESOURCE destMapped;
							HRESULT hr = m_pImmediateContext->Map( pDestDDS, 0, D3D11_MAP_READ, 0, &destMapped );
							if ( SUCCEEDED( hr ) )
							{
								const size_t size = srcddsd.Width*srcddsd.Height*4;
								unsigned char* pSrc = static_cast<unsigned char*>( srcMapped.pData );
								unsigned char* pDest = static_cast<unsigned char*>( destMapped.pData );
								int offsetSrc = 0;
								int offsetDst = 0;
								int rowOffset = srcMapped.RowPitch % srcddsd.Width;
								for (int row = 0; row < srcddsd.Height; ++row)
								{
									for (int col = 0; col < srcddsd.Width; ++col)
									{
										pDest[offsetDst] = pSrc[offsetSrc+2];
										pDest[offsetDst+1] = pSrc[offsetSrc+1];
										pDest[offsetDst+2] = pSrc[offsetSrc];
										pDest[offsetDst+3] = pSrc[offsetSrc+3];
										offsetSrc += 4;
										offsetDst += 4;
									}
									offsetSrc += rowOffset;
								}
								m_pImmediateContext->Unmap( pDestDDS, 0 );
							}
							m_pImmediateContext->Unmap( pSourceDDS, 0 );
						}

						// create new image in converted format
						m_imgptr->lpTexture

						// release temp staged dest
						SAFE_RELEASE ( pDestDDS );
					}
				}
				*/

				// load grabbed surface data into destination texture
				D3D11_BOX rc = { 0, 0, 0, (LONG)(iImageWidth), (LONG)(iImageHeight), 1 }; 
				m_pImmediateContext->CopySubresourceRegion ( m_imgptr->lpTexture, 0, 0, 0, 0, pTempTexture, 0, &rc );

				// get actual dimensions of texture/image
				m_imgptr->fTexUMax=(float)m_imgptr->iWidth/(float)srcddsd.Width;
				m_imgptr->fTexVMax=(float)m_imgptr->iHeight/(float)srcddsd.Height;

				// Ensure smalltextres are handled
				if(m_imgptr->fTexUMax>1.0f) m_imgptr->fTexUMax=1.0f;
				if(m_imgptr->fTexVMax>1.0f) m_imgptr->fTexVMax=1.0f;
			}
			else
			{
				RunTimeError(RUNTIMEERROR_IMAGEERROR);
				return false;
			}

			// free work-newsurface
			SAFE_RELEASE(pTempTexture);
		}
	}
	else
	{
		RunTimeError(RUNTIMEERROR_IMAGEERROR);
		return false;
	}

	// An additional feature is that images by default are stretched to complete fit a texture (no stretching in DX11=performance)
	///if ( iTextureFlagForGrab==0 || iTextureFlagForGrab==2 )
	///{
	///	StretchImage ( iID, GetPowerSquareOfSize(iImageWidth), GetPowerSquareOfSize(iImageHeight) );
	///}
	#else
	// Size of grab
	LPGGSURFACE pBackBuffer = g_pGlob->pCurrentBitmapSurface;
	if(pBackBuffer)
	{
		// get format of backbuffer
		D3DSURFACE_DESC ddsd;
		HRESULT hRes = pBackBuffer->GetDesc(&ddsd);
		GGFORMAT GGFORMAT = ddsd.Format;
		if(iX2>(int)ddsd.Width || iY2>(int)ddsd.Height)
		{
			RunTimeError(RUNTIMEERROR_IMAGEAREAILLEGAL);
			return false;
		}
	}

	// Image size
	int iImageWidth=iX2-iX1;
	int iImageHeight=iY2-iY1;

	// Get current render target surface
	if(pBackBuffer)
	{
		// get format of backbuffer
		D3DSURFACE_DESC ddsd;
		HRESULT hRes = pBackBuffer->GetDesc(&ddsd);
		GGFORMAT GGFORMAT = ddsd.Format;

		// check if image already exists of same size and type
		if ( UpdatePtrImage ( iID ) )
		{
			// check against existing
			if(m_imgptr->iWidth != iImageWidth || m_imgptr->iHeight != iImageHeight)
			{
				// existing image and new image are different sizes - so delete any existing image
                RemoveImage( iID );
			}
		}

		// create temp image texture to copy backbuffer to (same format at first)
		LPGGTEXTURE pTempTexture=NULL;
		hRes = D3DXCreateTexture (m_pD3D,
								  iImageWidth,
								  iImageHeight,
								  D3DX_DEFAULT,
								  0,
								  GGFORMAT,
								  D3DPOOL_MANAGED,
								  &pTempTexture	       );

		if ( pTempTexture )
		{
			// lock surface
			GGLOCKED_RECT d3dlr;
			hRes = pTempTexture->LockRect ( 0, &d3dlr, 0, 0 );
			if ( SUCCEEDED(hRes) )
			{
				// get size of single pixel (16bit, 24bit or 32bit)
				DWORD dwPixelSize = ImageGetBitDepthFromFormat(GGFORMAT)/8;

				// copy from backbuffer lock to texture lock
				RECT rc = { iX1, iY1, iX2, iY2 };
				GGLOCKED_RECT backlock;
				pBackBuffer->LockRect(&backlock, &rc, D3DLOCK_READONLY);
				LPSTR pPtr = (LPSTR)backlock.pBits;
				if ( pPtr==NULL )
				{
					MessageBox ( NULL, "Tried to read a surface which has no read permissions!", "System Memory Bitmap Error", MB_OK );
					RunTimeError(RUNTIMEERROR_IMAGEERROR);
					SAFE_RELEASE(pTempTexture);
					return false;
				}

				// straight copy or stretch copy
				bool bStretchCopy=false;
				D3DSURFACE_DESC imageddsd;
				pTempTexture->GetLevelDesc(0, &imageddsd);
				if(imageddsd.Width<(DWORD)iImageWidth) bStretchCopy=true;
				if(imageddsd.Height<(DWORD)iImageHeight) bStretchCopy=true;
				if(bStretchCopy==true)
				{
					DWORD dwClipWidth = iImageWidth;
					DWORD dwClipHeight = iImageHeight;
					if(imageddsd.Width<dwClipWidth) dwClipWidth=imageddsd.Width;
					if(imageddsd.Height<dwClipHeight) dwClipHeight=imageddsd.Height;
					float fXBit = (float)iImageWidth/(float)dwClipWidth;
					float fYBit = (float)iImageHeight/(float)dwClipHeight;
					LPSTR pImagePtr = (LPSTR)d3dlr.pBits;
					float fY=0.0f;
					for(DWORD y=0; y<dwClipHeight; y++)
					{
						LPSTR pImgPtr = pImagePtr + (y*d3dlr.Pitch);
						LPSTR pPtr = (LPSTR)backlock.pBits + ((int)fY*backlock.Pitch);
						float fX=0.0f;
						for(DWORD x=0; x<dwClipWidth; x++)
						{
							switch(dwPixelSize)
							{
								case 2 : *(WORD*)(pImgPtr) = *(WORD*)(pPtr+((int)fX*dwPixelSize));	break;
								case 4 : *(DWORD*)(pImgPtr) = *(DWORD*)(pPtr+((int)fX*dwPixelSize));	break;
							}
							pImgPtr+=dwPixelSize;
							fX+=fXBit;
						}
						fY+=fYBit;
					}
				}
				else
				{
					LPSTR pImagePtr = (LPSTR)d3dlr.pBits;
					for(int y=0; y<iImageHeight; y++)
					{
						memcpy(pImagePtr, pPtr, iImageWidth*dwPixelSize);
						pImagePtr+=d3dlr.Pitch;
						pPtr+=backlock.Pitch;
					}
				}
				pBackBuffer->UnlockRect();

				// unlock texture
				pTempTexture->UnlockRect(NULL);
			}

			// create image
			if(m_imgptr==NULL) MakeFormat ( iID, iImageWidth, iImageHeight, g_DefaultGGFORMAT, 0 );
			if ( UpdatePtrImage ( iID ) )
			{
				// load grabbed surface data into destination texture
				LPGGSURFACE pNewSurface = NULL;
				pTempTexture->GetSurfaceLevel(0, &pNewSurface);
				LPGGSURFACE pTexSurface = NULL;
				m_imgptr->lpTexture->GetSurfaceLevel(0, &pTexSurface);

				// LEEFIX - 071002 - No srtetching or filtering if DBV1 mode used
				if ( iTextureFlagForGrab==0 )
				{
					// leefix - dx8->dx9 - if texture exact size of image, no scaling required
					D3DSURFACE_DESC ddsdNewTexture;
					HRESULT hRes = pTexSurface->GetDesc(&ddsdNewTexture);
					if ( iImageWidth==(int)ddsdNewTexture.Width && iImageHeight==(int)ddsdNewTexture.Height )
						hRes = D3DXLoadSurfaceFromSurface(pTexSurface, NULL, NULL, pNewSurface, NULL, NULL, D3DX_FILTER_NONE, m_Color);
					else
						hRes = D3DXLoadSurfaceFromSurface(pTexSurface, NULL, NULL, pNewSurface, NULL, NULL, D3DX_DEFAULT, m_Color);
				}
				else
				{
					if ( iTextureFlagForGrab==2 )
						hRes = D3DXLoadSurfaceFromSurface(pTexSurface, NULL, NULL, pNewSurface, NULL, NULL, D3DX_FILTER_NONE, NULL );
					else
					{
						// leeadd - 201008 - u71 - should not have used color key (mode3)
						if ( iTextureFlagForGrab==3 )
							hRes = D3DXLoadSurfaceFromSurface(pTexSurface, NULL, NULL, pNewSurface, NULL, NULL, D3DX_FILTER_NONE, NULL );
						else
							hRes = D3DXLoadSurfaceFromSurface(pTexSurface, NULL, NULL, pNewSurface, NULL, NULL, D3DX_FILTER_NONE, m_Color);
					}
				}

				SAFE_RELEASE(pNewSurface);
				SAFE_RELEASE(pTexSurface);

				// get actual dimensions of texture/image
				D3DSURFACE_DESC imageddsd;
				m_imgptr->lpTexture->GetLevelDesc(0, &imageddsd);
				m_imgptr->fTexUMax=(float)m_imgptr->iWidth/(float)imageddsd.Width;
				m_imgptr->fTexVMax=(float)m_imgptr->iHeight/(float)imageddsd.Height;

				// Ensure smalltextres are handled
				if(m_imgptr->fTexUMax>1.0f) m_imgptr->fTexUMax=1.0f;
				if(m_imgptr->fTexVMax>1.0f) m_imgptr->fTexVMax=1.0f;
			}
			else
			{
				RunTimeError(RUNTIMEERROR_IMAGEERROR);
				SAFE_RELEASE(pTempTexture);
				return false;
			}

			// free work-newsurface
			SAFE_RELEASE(pTempTexture);
		}
	}
	else
	{
		RunTimeError(RUNTIMEERROR_IMAGEERROR);
		return false;
	}

	// An additional feature is that images by default are stretched to complete fit a texture
	if ( iTextureFlagForGrab==0 || iTextureFlagForGrab==2 )
	{
		StretchImage ( iID, GetPowerSquareOfSize(iImageWidth), GetPowerSquareOfSize(iImageHeight) );
	}
	#endif
	//#endif
	#endif

	// Complete
	return true;
}
#endif

DARKSDK bool GrabImageCore ( int iID, int iX1, int iY1, int iX2, int iY2 )
{
	// Stretch image to fit texture by default (0)
	return GrabImageCore ( iID, iX1, iY1, iX2, iY2, 0 );
}

DARKSDK void TransferImage ( int iDestImgID, int iSrcImageID, int iTransferMode, int iMemblockAssistor )
{
	// iTransferMode:
	// 1 = blue channel specifies one of sixteen IDs, each representing a small 4x4 min-texture described in the indexed memblock passed in

	// validate
	if(iDestImgID<1 || iDestImgID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}
	if(iSrcImageID<1 || iSrcImageID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}

	#ifdef DX11
	#else
	// get destination details
	D3DSURFACE_DESC ddsd;
	LPGGSURFACE pDstSurface = NULL;
	if ( !UpdatePtrImage ( iDestImgID ) ) return;
	tagImgData* pDestImagePtr = m_imgptr;
	pDestImagePtr->lpTexture->GetSurfaceLevel(0, &pDstSurface);
	pDstSurface->GetDesc(&ddsd);
	DWORD dwDestImgWidth = ddsd.Width;
	DWORD dwDestImgHeight = ddsd.Height;

	// get source details
	LPGGSURFACE pSrcSurface = NULL;
	if ( !UpdatePtrImage ( iSrcImageID ) ) return;
	tagImgData* pSrcImagePtr = m_imgptr;
	pSrcImagePtr->lpTexture->GetSurfaceLevel(0, &pSrcSurface);
	pSrcSurface->GetDesc(&ddsd);
	DWORD dwSrcImgWidth = ddsd.Width;
	DWORD dwSrcImgHeight = ddsd.Height;

	// must be same size 
	if ( dwSrcImgWidth!=dwDestImgWidth || dwSrcImgHeight!=dwDestImgHeight ) return;

	// go through each pixel and apply transfer logic
	GGLOCKED_RECT d3dDstlock;
	RECT rc = { 0, 0, (LONG)dwSrcImgWidth, (LONG)dwSrcImgHeight };
	if(SUCCEEDED(pDstSurface->LockRect ( &d3dDstlock, &rc, 0 ) ) )
	{
		GGLOCKED_RECT d3dSrclock;
		if(SUCCEEDED(pSrcSurface->LockRect ( &d3dSrclock, &rc, 0 ) ) )
		{
			for(DWORD y=0; y<dwSrcImgHeight; y++)
			{
				LPSTR pDst = (LPSTR)d3dDstlock.pBits + (y*d3dDstlock.Pitch);
				LPSTR pSrc = (LPSTR)d3dSrclock.pBits + (y*d3dSrclock.Pitch);
				for(DWORD x=0; x<dwSrcImgWidth; x++)
				{
					if ( iTransferMode==1 )
					{
						// first get the pixel to work on
						DWORD dwPixelValue = *(DWORD*)pSrc;
						float fTexSelectorV = ((dwPixelValue & 0x00000FF))/255.0f;
						float fTexSelectorCol[17];
						fTexSelectorCol[1] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*0)))*16.0f;
						if ( fTexSelectorCol[1] < 0 ) fTexSelectorCol[1] = 0;
						fTexSelectorCol[2] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*1)))*16.0f;
						if ( fTexSelectorCol[2] < 0 ) fTexSelectorCol[2] = 0;
						fTexSelectorCol[3] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*2)))*16.0f;
						if ( fTexSelectorCol[3] < 0 ) fTexSelectorCol[3] = 0;
						fTexSelectorCol[4] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*3)))*16.0f;
						if ( fTexSelectorCol[4] < 0 ) fTexSelectorCol[4] = 0;
						fTexSelectorCol[5] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*4)))*16.0f;
						if ( fTexSelectorCol[5] < 0 ) fTexSelectorCol[5] = 0;
						fTexSelectorCol[6] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*5)))*16.0f;
						if ( fTexSelectorCol[6] < 0 ) fTexSelectorCol[6] = 0;
						fTexSelectorCol[7] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*6)))*16.0f;
						if ( fTexSelectorCol[7] < 0 ) fTexSelectorCol[7] = 0;
						fTexSelectorCol[8] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*7)))*16.0f;
						if ( fTexSelectorCol[8] < 0 ) fTexSelectorCol[8] = 0;
						fTexSelectorCol[9] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*8)))*16.0f;
						if ( fTexSelectorCol[9] < 0 ) fTexSelectorCol[9] = 0;
						fTexSelectorCol[10] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*9)))*16.0f;
						if ( fTexSelectorCol[10] < 0 ) fTexSelectorCol[10] = 0;
						fTexSelectorCol[11] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*10)))*16.0f;
						if ( fTexSelectorCol[11] < 0 ) fTexSelectorCol[11] = 0;
						fTexSelectorCol[12] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*11)))*16.0f;
						if ( fTexSelectorCol[12] < 0 ) fTexSelectorCol[12] = 0;
						fTexSelectorCol[13] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*12)))*16.0f;
						if ( fTexSelectorCol[13] < 0 ) fTexSelectorCol[13] = 0;
						fTexSelectorCol[14] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*13)))*16.0f;
						if ( fTexSelectorCol[14] < 0 ) fTexSelectorCol[14] = 0;
						fTexSelectorCol[15] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*14)))*16.0f;
						if ( fTexSelectorCol[15] < 0 ) fTexSelectorCol[15] = 0;
						fTexSelectorCol[16] = (float)(0.0625f-fabs(fTexSelectorV-(0.0625f*15)))*16.0f;
						if ( fTexSelectorCol[16] < 0 ) fTexSelectorCol[16] = 0;

						// get reference into memblock mini-texture lookup
						int tx = x-(int(x/4)*4);
						int tz = y-(int(y/4)*4);

						// get pointer to memblock
						DWORD* pMemBlockPot = (DWORD*)GetMemblockPtr ( iMemblockAssistor );

						// get weighted contrib from each pot
						DWORD dwTexPartD[17];
						int texpartdr[17];
						int texpartdg[17];
						int texpartdb[17];
						for ( int i = 1; i <= 16; i++ )
						{
							dwTexPartD[i] = *(pMemBlockPot+(i*16)+(tz*4)+tx);
							texpartdr[i] = (int)(((dwTexPartD[i] & 0x00FF0000) >> 16) * fTexSelectorCol[i]);
							texpartdg[i] = (int)(((dwTexPartD[i] & 0x0000FF00) >> 8 ) * fTexSelectorCol[i]);
							texpartdb[i] = (int)(((dwTexPartD[i] & 0x000000FF)      ) * fTexSelectorCol[i]);
						}

						// combine to a single colour
						int diffusemapr = 0;
						int diffusemapg = 0;
						int diffusemapb = 0;
						for ( int i = 1; i <= 16; i++ ) diffusemapr += texpartdr[i];
						for ( int i = 1; i <= 16; i++ ) diffusemapg += texpartdg[i];
						for ( int i = 1; i <= 16; i++ ) diffusemapb += texpartdb[i];
						if ( diffusemapr>255 ) diffusemapr=255;
						if ( diffusemapg>255 ) diffusemapg=255;
						if ( diffusemapb>255 ) diffusemapb=255;

						// write result into destination, preserve the alpha channel of the dest
						DWORD trgb = (diffusemapr<<16)+(diffusemapg<<8)+(diffusemapb);
						DWORD trgba = *(DWORD*)pDst;
						DWORD talpha = trgba & 0xFF000000;
						*(DWORD*)pDst = talpha + trgb;
					}
					pSrc+=4;
					pDst+=4;
				}
			}
			pSrcSurface->UnlockRect();
		}
		pDstSurface->UnlockRect();
	}
	SAFE_RELEASE(pDstSurface);
	SAFE_RELEASE(pSrcSurface);
	#endif
}

DARKSDK void PasteImageCore ( int iID, int iX, int iY, int iFlag )
{
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}
	if ( !UpdatePtrImage ( iID ) )
	{
		#ifdef WICKEDENGINE
		//Silent error now log instead. So we know exactly where the error is.
		if (iMaxPasteImageLogs > 0)
		{
			char pErr[256]; sprintf(pErr, "Error Image Not Found: PasteImageCore(%d,%d,%d,%d) .", iID, iX, iID, iY, iFlag);
			timestampactivity(0, pErr);
			iMaxPasteImageLogs--;
		}
		#else
		RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
		#endif
		return;
	}

	// use sprite library to paste image(texture) with polys!
	PasteImage( iID, iX, iY, m_imgptr->fTexUMax, m_imgptr->fTexVMax, iFlag );

	return;
}

DARKSDK void StretchImage ( int iID, int Width, int Height )
{
	// returns true if the image exists
	if ( !UpdatePtrImage ( iID ) )
		return;

	assert( 0 && "DX11 not in use" );
	/*
	// First ensure texture is not already stretched
	#ifdef DX11
	GGSURFACE_DESC ddsd;
	LPGGSURFACE pSurface = NULL;
	m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pSurface);
	pSurface->GetDesc(&ddsd);
	SAFE_RELEASE(pSurface);
	if ( ddsd.Width==(DWORD)GetPowerSquareOfSize(ddsd.Width)
	&& ddsd.Height==(DWORD)GetPowerSquareOfSize(ddsd.Height)
	&& m_imgptr->fTexUMax==1.0f && m_imgptr->fTexVMax==1.0f)
	{
		// Image is already power of two and fills entire surface
		return;
	}
	#endif
	*/
}

DARKSDK void PasteImageCore ( int iID, int iX, int iY )
{
	PasteImageCore ( iID, iX, iY, 0 );
}

DARKSDK void DeleteImageCore ( int iID )
{
	if ( !UpdatePtrImage ( iID ) )
		return;

	#ifdef WICKEDENGINE
	//PE: Mark bad textures that have been deleted.
	//PE: Moved to RemoveImage to catch more.
	//lpBadTexture.push_back(m_imgptr->lpTextureView);
	#endif

	// before release, remove the reference from ALL objects
	// leeadd - 220604 - u54 - scans every object that uses this texture address
	ClearObjectsOfTextureRef ( m_imgptr->lpTexture );

	// clear cube details
	m_imgptr->pCubeMapRef = NULL;
	m_imgptr->iCubeMapFace = 0;

    RemoveImage ( iID );
}

//
// Command Functions
//

DARKSDK void LoadImage ( LPSTR szFilename, int iID, int iTextureFlag, int iDivideTextureSize, int iSilentError )
{
	if(LoadImageCore( szFilename, iID, iTextureFlag, false, iDivideTextureSize )==false)
	{
		if ( iSilentError==0 )
		{
			char pCWD[256]; _getcwd ( pCWD, 256 );
			char pErr[256]; sprintf ( pErr, "CWD:%s\nLOAD IMAGE %s,%d,%d,%d", pCWD, szFilename, iID, iTextureFlag, iDivideTextureSize);
			#ifndef NOSTEAMORVIDEO
			timestampactivity(0, pErr);
			#endif
		}
	}
}

DARKSDK void LoadImageSize(LPSTR szFilename, int iID, int x, int y)
{
	iResizeLoadImageX = x;
	iResizeLoadImageY = y;
	return LoadImage(szFilename, iID, 0, 8192, 0);
}

DARKSDK void LoadImage ( LPSTR szFilename, int iID, int iKindOfTexture, int iDivideTextureSize )
{
	return LoadImage ( szFilename, iID, iKindOfTexture, iDivideTextureSize, 0 );
}

DARKSDK void LoadImage ( LPSTR szFilename, int iID, int iKindOfTexture )
{
    LoadImage ( szFilename, iID, iKindOfTexture, 0 );
}

DARKSDK void LoadImage ( LPSTR szFilename, int iID )
{
	int iKindOfTexture = 0;
    LoadImage ( szFilename, iID, iKindOfTexture, 0 );
}

DARKSDK void SaveImage ( LPSTR szFilename, int iID )
{
	SaveImageCore ( szFilename, iID );
}

DARKSDK void SaveImage ( LPSTR szFilename, int iID, int iCompressionMode )
{
	SaveImageCore ( szFilename, iID, iCompressionMode );
}

DARKSDK void GrabImage ( int iID, int iX1, int iY1, int iX2, int iY2 )
{
	GrabImageCore ( iID, iX1, iY1, iX2, iY2 );
}

DARKSDK void GrabImage ( int iID, int iX1, int iY1, int iX2, int iY2, int iTextureFlag )
{
	GrabImageCore ( iID, iX1, iY1, iX2, iY2, iTextureFlag );
}

DARKSDK void PasteImage ( int iID, int iX, int iY )
{
	PasteImageCore ( iID, iX, iY, 0 );
}

DARKSDK void PasteImage ( int iID, int iX, int iY, int iFlag )
{
	PasteImageCore ( iID, iX, iY, iFlag );
}

DARKSDK void DeleteImage ( int iID )
{
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return;
	}
	if ( !UpdatePtrImage ( iID ) )
	{
		#ifdef WICKEDENGINE
		//Silent error now log instead. So we know exactly where the error is.
		if (iMaxPasteImageLogs > 0)
		{
			char pErr[512]; sprintf(pErr, "Error Image Not Found: DeleteImage(%d) .", iID);
			timestampactivity(0, pErr);
			iMaxPasteImageLogs--;
		}
		#else
		RunTimeError(RUNTIMEERROR_IMAGENOTEXIST);
		#endif
		return;
	}
	DeleteImageCore ( iID );
}

DARKSDK void RotateImage ( int iID, int iAngle )
{
	// Not Implemented in DBPRO V1 RELEASE
	RunTimeError(RUNTIMEERROR_COMMANDNOWOBSOLETE);
}

DARKSDK int GetImageExistEx ( int iID )
{
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return 0;
	}

	// returns true if the image exists
	if ( !UpdatePtrImage ( iID ) )
		return 0;

	// return true
	return 1;
}

DARKSDK LPSTR GetImageName ( int iID )
{
	if(iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return NULL;
	}

	// returns true if the image exists
	if ( !UpdatePtrImage ( iID ) )
		return NULL;

	// return true
	return m_imgptr->szShortFilename;
}

DARKSDK LPSTR SetImageName(int iID,char *name)
{
	if (!name) return NULL;
	if (iID<1 || iID>MAXIMUMVALUE)
	{
		//RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return NULL;
	}

	// returns true if the image exists
	if (!UpdatePtrImage(iID))
		return NULL;

	strcpy(m_imgptr->szShortFilename, name);

	// return true
	return m_imgptr->szShortFilename;
}

//
// New Command Functions
//

DARKSDK void SetImageColorKey ( int iR, int iG, int iB )
{
	// set the color key of an image
	m_Color = GGCOLOR_ARGB ( 255, iR, iG, iB );
}

DARKSDK bool FileExist ( LPSTR szFilename )
{
	GGIMAGE_INFO info;
	HRESULT hRes = 0;
	#ifdef DX11
	#else
	if ( g_bImageBlockActive && g_iImageBlockMode==1 )
	{
		DWORD dwFileInMemorySize = 0;
		LPVOID pFileInMemoryData = NULL;
		char pFinalRelPathAndFile[512];
		GetFileInMemory ( szFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile );
		hRes = D3DXGetImageInfoFromFileInMemory( pFileInMemoryData, dwFileInMemorySize, &info );
	}
	else
		hRes = D3DXGetImageInfoFromFile( szFilename, &info );
	#endif

	if ( hRes==GG_OK )
		return true;
	else
		return false;
}

DARKSDK DWORD LoadIcon ( LPSTR pFilename )
{
	char szRealFilename[ MAX_PATH ];
	strcpy_s( szRealFilename, MAX_PATH, pFilename );
	GG_GetRealPath( szRealFilename, 0 );

	// load icon
	HICON hIconHandle = (HICON)LoadImageA ( NULL, szRealFilename, IMAGE_ICON, 48, 48, LR_LOADFROMFILE );

	// complete
	return (DWORD)hIconHandle;
}

DARKSDK void FreeIcon ( DWORD dwIcon )
{
	// free icon handle
    CloseHandle ( (HICON)dwIcon );
}

//
// Data Access Functions
//

DARKSDK void GetImageData( int iID, DWORD* dwWidth, DWORD* dwHeight, DWORD* dwDepth, LPSTR* pData, DWORD* dwDataSize, bool bLockData )
{
	// Read Data
	if(bLockData==true)
	{
		if ( !UpdatePtrImage ( iID ) )
			return;

		if ( m_imgptr->lpTexture==NULL )
			return;

		assert( 0 && "DX11 not in use" );
		/*
		// data
		*dwWidth = m_imgptr->iWidth;
		*dwHeight = m_imgptr->iHeight;
		*dwDepth = m_imgptr->iDepth;
		DWORD bitdepth = m_imgptr->iDepth/8;

		#ifdef DX11
		// use actual size, not image size
		LPGGSURFACE pTextureInterface = NULL;
		m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTextureInterface);
		D3D11_TEXTURE2D_DESC desc;
		pTextureInterface->GetDesc(&desc);
		if(desc.Width<*dwWidth) *dwWidth=desc.Width;
		if(desc.Height<*dwHeight) *dwHeight=desc.Height;
		SAFE_RELEASE ( pTextureInterface );

		// create system memory version
		ID3D11Texture2D* pTempSysMemTexture = NULL;
		D3D11_TEXTURE2D_DESC StagedDesc = { desc.Width, desc.Height, 1, 1, desc.Format, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0 };
		m_pD3D->CreateTexture2D( &StagedDesc, NULL, &pTempSysMemTexture );
		if ( pTempSysMemTexture )
		{
			// and copy texture image to it
			D3D11_BOX rc = { 0, 0, 0, (LONG)(*dwWidth), (LONG)(*dwHeight), 1 }; 
			m_pImmediateContext->CopySubresourceRegion(pTempSysMemTexture, 0, 0, 0, 0, m_imgptr->lpTexture, 0, &rc);

			// lock for reading staging texture
			GGLOCKED_RECT d3dlock;
			if(SUCCEEDED(m_pImmediateContext->Map(pTempSysMemTexture, 0, D3D11_MAP_READ, 0, &d3dlock)))
			{
				// copy data
				DWORD dwSizeOfBitmapData = (*dwWidth)*(*dwHeight)*bitdepth;
				*pData = new char[dwSizeOfBitmapData];
				*dwDataSize = dwSizeOfBitmapData;

				// copy from surface
				LPSTR pSrc = (LPSTR)d3dlock.pData;
				LPSTR pPtr = *pData;
				DWORD dwDataWidth=*(dwWidth)*bitdepth;
				for(DWORD y=0; y<*dwHeight; y++)
				{
					memcpy(pPtr, pSrc, dwDataWidth);
					pPtr+=dwDataWidth;
					pSrc+=d3dlock.RowPitch;
				}
				m_pImmediateContext->Unmap(pTempSysMemTexture, 0);
			}
			else
			{
				// leefix - 250604 - u54 - place a one in size to indicate could not lock (exists but protected)
				*dwDataSize = 1;
			}

			// free temp system surface
			SAFE_RELEASE(pTempSysMemTexture);
		}
		#endif
		*/
	}
	else
	{
		// free memory
		delete *pData;
	}
}

DARKSDK void SetImageData( int iID, DWORD dwWidth, DWORD dwHeight, DWORD dwDepth, LPSTR pData, DWORD dwDataSize )
{
	if ( UpdatePtrImage ( iID ) )
	{
		if ( m_imgptr->lpTexture==NULL )
			return;

		// Check new specs with existing one
		if(dwWidth==(DWORD)m_imgptr->iWidth && dwHeight==(DWORD)m_imgptr->iHeight && dwDepth==(DWORD)m_imgptr->iDepth)
		{
			// Same size
		}
		else
		{
			// Recreate
			DeleteImageCore ( iID );
			m_imgptr=NULL;
		}
	}

	// new image
	GGFORMAT destImageFormat = GGFMT_A8R8G8B8;
	if(g_bUseRGBAFormat) destImageFormat = DXGIFORMATR8G8B8A8UNORM;

	if(m_imgptr==NULL)
	{
		MakeFormat ( iID, dwWidth, dwHeight, destImageFormat, 0 );
	}

	// may have changed
	if ( !UpdatePtrImage ( iID ) ) return;
	if ( m_imgptr->lpTexture==NULL ) return;

	assert( 0 && "DX11 not in use" );
	/*

	// write Data
	#ifdef DX11
	LPGGSURFACE pTempTexture = NULL;
	GGSURFACE_DESC TempTextureDesc = { dwWidth, dwHeight, 1, 1, destImageFormat, 1, 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE, 0 };
	m_pD3D->CreateTexture2D( &TempTextureDesc, NULL, &pTempTexture );
	if ( pTempTexture )
	{
		GGLOCKED_RECT d3dlock;
		DWORD bitdepth = m_imgptr->iDepth/8;
		if(SUCCEEDED(m_pImmediateContext->Map(pTempTexture, 0, D3D11_MAP_WRITE, 0, &d3dlock)))
		{
			LPSTR pDest = (LPSTR)d3dlock.pData;
			LPSTR pPtr = pData;
			DWORD dwDataWidth=dwWidth*bitdepth;
			for(DWORD y=0; y<dwHeight; y++)
			{
				memcpy(pDest, pPtr, dwDataWidth);
				pPtr+=dwDataWidth;
				pDest+=d3dlock.RowPitch;
			}
			m_pImmediateContext->Unmap(pTempTexture,0);
		}

		// copy staging texture to shader resource
		m_pImmediateContext->CopyResource ( m_imgptr->lpTexture, pTempTexture );

		// free work resources
		SAFE_RELEASE(pTempTexture);
	}
	#endif
	*/

	// ensure sprites all updated
	UpdateAllSprites();
}

//
// IMAGE BLOCK CODE
//

void OpenImageBlock	( char* szFilename, int iMode )
{
	// cannot open if already open
	if ( g_iImageBlockMode!=-1 )
		return;

	// Reset exclude path
	strcpy ( g_pImageBlockExcludePath, "" );

	// Create image block details
	g_iImageBlockFilename = new char [ strlen ( szFilename ) + 1 ];
	strcpy ( g_iImageBlockFilename, szFilename );

	// Create path to image block
	char current [ 512 ];
	_getcwd ( current, 512 );
	g_iImageBlockRootPath = new char [ strlen ( current ) + 2 ];
	strcpy ( g_iImageBlockRootPath, current );
	if ( g_iImageBlockRootPath [ strlen(g_iImageBlockRootPath)-1 ]!='\\' )
	{
		// add folder divide at end of path string
		int iLen = strlen(g_iImageBlockRootPath);
		g_iImageBlockRootPath [ iLen+0 ] = '\\';
		g_iImageBlockRootPath [ iLen+1 ] = 0;
	}
	
	// Set the imageblock mode (0-write, 1-read)
	g_bImageBlockActive = true;
	g_iImageBlockMode = iMode;

	// U77 - 060211 - does previously written image block exist, if so, we append to it
	bool bPreviousImageBlockExists = false;
	HANDLE hFile = GG_CreateFile ( g_iImageBlockFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile!=INVALID_HANDLE_VALUE )
	{
		bPreviousImageBlockExists = true;
		CloseHandle ( hFile );
	}

	// Load imageblock (for reading or to load last imageblock from previous write)
	if ( g_iImageBlockMode==1 || bPreviousImageBlockExists==true )
	{
		// open to read
		HANDLE hFile = GG_CreateFile ( g_iImageBlockFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		DWORD dwReadBytes = 0;

		// read file list
		int iListMax = 0;
		g_ImageBlockListFile.clear();
		ReadFile ( hFile, &iListMax, sizeof(int), &dwReadBytes, NULL );
		for ( int i = 0; i < iListMax; i++ )
		{
			// write filename length and string
			DWORD dwFilenameLength = 0;
			ReadFile ( hFile, &dwFilenameLength, sizeof(DWORD), &dwReadBytes, NULL );
			LPSTR pFile = new char [ dwFilenameLength ];
			ReadFile ( hFile, pFile, dwFilenameLength, &dwReadBytes, NULL );
			g_ImageBlockListFile.push_back ( pFile );

			// write file offset in data
			DWORD dwListOffset = 0;
			ReadFile ( hFile, &dwListOffset, sizeof(DWORD), &dwReadBytes, NULL );
			g_ImageBlockListOffset.push_back ( dwListOffset );

			// write file size in data
			DWORD dwListSize = 0;
			ReadFile ( hFile, &dwListSize, sizeof(DWORD), &dwReadBytes, NULL );
			g_ImageBlockListSize.push_back ( dwListSize );
		}

		// write the imageblock data itself
		ReadFile ( hFile, &g_dwImageBlockSize, sizeof(DWORD), &dwReadBytes, NULL );
		g_pImageBlockPtr = new char [ g_dwImageBlockSize ];
		ReadFile ( hFile, g_pImageBlockPtr, g_dwImageBlockSize, &dwReadBytes, NULL );
	
		// close file
		CloseHandle ( hFile );
	}
	else
	{
		// Create memory block for saving
		g_dwImageBlockSize = 0;
		g_pImageBlockPtr = NULL;

		// Clear file list
		g_ImageBlockListFile.clear();
		g_ImageBlockListOffset.clear();
		g_ImageBlockListSize.clear();
	}
}

void ExcludeFromImageBlock ( char* szExcludePath )
{
	// exclude any file starting with this string
    if (szExcludePath)
    	strcpy ( g_pImageBlockExcludePath, szExcludePath );
    else
        g_pImageBlockExcludePath[0] = 0;
}

bool AddToImageBlock ( LPSTR pAddFilename )
{
	// can only add in write mode
	if ( g_iImageBlockMode!=0 ) return true;

	// if exist
	if ( !pAddFilename ) return false;

	// exclude if path matches excluder, but only if excluder has a value
    if (g_pImageBlockExcludePath && g_pImageBlockExcludePath[0])
    	if ( strnicmp ( g_pImageBlockExcludePath, pAddFilename, strlen(g_pImageBlockExcludePath) )==NULL )
	    	return true;

	// ensure it does not already exist
	for ( int i = 0; i < (int)g_ImageBlockListFile.size ( ); i++ )
		if ( _stricmp ( g_ImageBlockListFile [ i ], pAddFilename )==NULL )
			return true;

	// open the file
	HANDLE hFile = GG_CreateFile ( pAddFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile==INVALID_HANDLE_VALUE ) return false;

	// read the file data
	DWORD dwReadBytes = 0;
	DWORD dwFileSize = GetFileSize ( hFile, NULL );
	LPSTR pFileData = new char [ dwFileSize ];
	ReadFile ( hFile, pFileData, dwFileSize, &dwReadBytes, NULL );
	
	// close file
	CloseHandle ( hFile );

	// create space in the imageblock
	DWORD dwNewSize = g_dwImageBlockSize + dwFileSize;

	// U76 - 020710 - image blocks can get LARGE enough to overrun virtual address space
	// so catch the exeption raised in this case and end the image block creation process gracefully
	LPSTR pNewData = NULL;
	try
	{
		pNewData = new char [ dwNewSize ];
	}
	catch(...)
	{
		// failed to create a new "continuous" block of memory in virtual address space
		// so end image block creation and exit here
		SAFE_DELETE ( pFileData );
		CloseImageBlock();
		return false;
	}
	memcpy ( pNewData, g_pImageBlockPtr, g_dwImageBlockSize );

	// add data to the imageblock
	LPSTR pNewDataInsertPtr = pNewData + g_dwImageBlockSize;
	DWORD dwOffsetToDataInImageBlock = g_dwImageBlockSize;
	memcpy ( pNewDataInsertPtr, pFileData, dwFileSize );

	// free individual file data
	SAFE_DELETE ( pFileData );

	// erase old imageblock and use new one
	SAFE_DELETE ( g_pImageBlockPtr );
	g_dwImageBlockSize = dwNewSize;
	g_pImageBlockPtr = pNewData;

	// add it to list
	LPSTR pListFilename = new char [ strlen ( pAddFilename )+1 ];
	strcpy ( pListFilename, pAddFilename );
	g_ImageBlockListFile.push_back ( pListFilename );
	g_ImageBlockListOffset.push_back ( dwOffsetToDataInImageBlock );
	g_ImageBlockListSize.push_back ( dwFileSize );

	// success
	return true;
}

LPSTR RetrieveFromImageBlock ( LPSTR pRetrieveFilename, DWORD* pdwFileSize )
{
	// find the file
	int iIndexInListFound = -1;
	for ( int iIndexInList = 0; iIndexInList < (int)g_ImageBlockListFile.size ( ); iIndexInList++ )
	{
		if ( _stricmp ( g_ImageBlockListFile [ iIndexInList ], pRetrieveFilename )==NULL )
		{
			iIndexInListFound = iIndexInList;
			break;
		}
	}
	if ( iIndexInListFound==-1 )
	{
		// not found 
		return NULL;
	}

	// locate file within imageblock
	DWORD dwOffset = g_ImageBlockListOffset [ iIndexInListFound ];
	DWORD dwSize = g_ImageBlockListSize [ iIndexInListFound ];

	// return ptr and size
	if ( pdwFileSize ) *pdwFileSize = dwSize;
	return g_pImageBlockPtr + dwOffset;
}

void CloseImageBlock ( void )
{
	// cannot close if already closed
	if ( g_iImageBlockMode==-1 )
		return;

	// Save imageblock
	if ( g_iImageBlockMode==0 )
	{
		// set original path
		char storedir [ 512 ];
		_getcwd ( storedir, 512 );
		_chdir ( g_iImageBlockRootPath );

		// open to write
		HANDLE hFile = GG_CreateFile ( g_iImageBlockFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		DWORD dwWrittenBytes = 0;

		// write file list
		int iListMax = g_ImageBlockListFile.size ( );
		WriteFile ( hFile, &iListMax, sizeof(int), &dwWrittenBytes, NULL );
		for ( int i = 0; i < iListMax; i++ )
		{
			// write filename length and string
			LPSTR pListPtr = g_ImageBlockListFile [ i ];
			DWORD dwFilenameLength = strlen(pListPtr)+1;
			WriteFile ( hFile, &dwFilenameLength, sizeof(DWORD), &dwWrittenBytes, NULL );
			WriteFile ( hFile, pListPtr, dwFilenameLength, &dwWrittenBytes, NULL );

			// write file offset in data
			DWORD dwListOffset = g_ImageBlockListOffset [ i ];
			WriteFile ( hFile, &dwListOffset, sizeof(DWORD), &dwWrittenBytes, NULL );

			// write file size in data
			DWORD dwListSize = g_ImageBlockListSize [ i ];
			WriteFile ( hFile, &dwListSize, sizeof(DWORD), &dwWrittenBytes, NULL );
		}

		// write the imageblock data itself
		WriteFile ( hFile, &g_dwImageBlockSize, sizeof(DWORD), &dwWrittenBytes, NULL );
		WriteFile ( hFile, g_pImageBlockPtr, g_dwImageBlockSize, &dwWrittenBytes, NULL );
	
		// close file
		CloseHandle ( hFile );

		// restore folder
		_chdir ( storedir );
	}

	// free strings in imageblock file list
	for ( int i = 0; i < (int)g_ImageBlockListFile.size(); i++ )
		SAFE_DELETE ( g_ImageBlockListFile [ i ] );
	g_ImageBlockListFile.clear();
	g_ImageBlockListOffset.clear();
	g_ImageBlockListSize.clear();

	// free filename
	SAFE_DELETE ( g_iImageBlockFilename );
	SAFE_DELETE ( g_iImageBlockRootPath );

	// Close imageblock
	SAFE_DELETE ( g_pImageBlockPtr );

	// Switch off imageblock
	g_bImageBlockActive = false;
	g_iImageBlockMode = -1;
}

void PerformChecklistForImageBlockFiles ( void )
{
	// Generate Checklist
	DWORD dwMaxStringSizeInEnum=0;
	bool bCreateChecklistNow=false;
	g_pGlob->checklisthasvalues=false;
	g_pGlob->checklisthasstrings=true;
	for(int pass=0; pass<2; pass++)
	{
		if(pass==1)
		{
			// Ensure checklist is large enough
			bCreateChecklistNow=true;
			for(int c=0; c<g_pGlob->checklistqty; c++)
				GlobExpandChecklist(c, dwMaxStringSizeInEnum);
		}

		// Look at parameters
		g_pGlob->checklistqty=0;
		for ( int i = 0; i < (int)g_ImageBlockListFile.size ( ); i++ )
		{
			// write filename length and string
			LPSTR pListPtr = g_ImageBlockListFile [ i ];
			if ( !pListPtr ) continue;

			// Add to checklist
			DWORD dwSize = strlen(pListPtr);
			if(dwSize>dwMaxStringSizeInEnum) dwMaxStringSizeInEnum=dwSize;
			if(bCreateChecklistNow)
			{
				// New checklist item
				strcpy(g_pGlob->checklist[g_pGlob->checklistqty].string, pListPtr);
				g_pGlob->checklist[g_pGlob->checklistqty].valuea = 0;
				g_pGlob->checklist[g_pGlob->checklistqty].valueb = 0;
				g_pGlob->checklist[g_pGlob->checklistqty].valuec = 0;
				g_pGlob->checklist[g_pGlob->checklistqty].valued = 0;
			}
			g_pGlob->checklistqty++;
		}
	}
 
	// Determine if checklist has any contents
	if(g_pGlob->checklistqty>0)
		g_pGlob->checklistexists=true;
	else
		g_pGlob->checklistexists=false;
}

int GetImageFileExist ( LPSTR pFilename )
{
	// If no string, no file
	if ( pFilename==NULL ) return 0;
	char VirtualFilename[_MAX_PATH];
	strcpy(VirtualFilename, pFilename);

	CheckForWorkshopFile ( VirtualFilename );

	// if image block file, quick early out using imageblock
	if ( g_bImageBlockActive && g_iImageBlockMode==1 )
	{
		// final storage string of path and file resolver (makes the filename and path uniform for imageblock retrieval)
		// and work out true file and path, then look for it in imageblock
		char pFinalRelPathAndFile[512];
		LPVOID pFileInMemoryData = 0;
		DWORD dwFileInMemorySize = 0;
		GetFileInMemory ( VirtualFilename, &pFileInMemoryData, &dwFileInMemorySize, pFinalRelPathAndFile );
		if ( pFileInMemoryData )
			return 1;
	}

	// real file
	HANDLE hfile = GG_CreateFile ( VirtualFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hfile==INVALID_HANDLE_VALUE)
		return 0;

	// success, it exists
	CloseHandle(hfile);
	return 1;
}

bool LoadAndSaveUsingDirectXTex ( LPSTR pLoadFile, LPSTR pSaveFile )
{
	HRESULT hRes;

	// load image
	wchar_t wTexLoadFilename[512];
	MultiByteToWideChar(CP_ACP, 0, pLoadFile, -1, wTexLoadFilename, sizeof(wTexLoadFilename));
	DirectX::TexMetadata imageData;
	DirectX::ScratchImage imageTexture;
	if ( strnicmp ( pLoadFile + strlen(pLoadFile) - 4, ".tga", 4 ) == NULL )
		hRes = DirectX::LoadFromTGAFile( wTexLoadFilename, &imageData, imageTexture );
	else
		hRes = DirectX::LoadFromWICFile( wTexLoadFilename, 0, &imageData, imageTexture );

	// save DDS image
	wchar_t wTexSaveFilename[512];
	MultiByteToWideChar(CP_ACP, 0, pSaveFile, -1, wTexSaveFilename, sizeof(wTexSaveFilename));
	const DirectX::Image* img = imageTexture.GetImages();
	hRes = SaveToDDSFile( img, imageTexture.GetImageCount(), imageTexture.GetMetadata(), DirectX::DDS_FLAGS_NONE, wTexSaveFilename );

	// result
	if ( FileExist ( pSaveFile ) == 1 )
		return 1;
	else
		return 0;
}

void ImageCreateCubeMapFile(LPSTR pCubeToSave, LPSTR pUp, LPSTR pDown, LPSTR pLeft, LPSTR pRight, LPSTR pBack, LPSTR pFront)
{
	// collect images
	HRESULT hr;
    size_t images = 0;
    DirectX::TexMetadata info;
    std::vector<std::unique_ptr<DirectX::ScratchImage>> loadedImages;
	for (int cubeside = 0; cubeside < 6; cubeside++ )
	{
		// get filenames as wchars
		wchar_t wOneSideFileName[512];
		if ( cubeside == 0 ) MultiByteToWideChar(CP_ACP, 0, pRight, -1, wOneSideFileName, sizeof(wOneSideFileName));
		if ( cubeside == 1 ) MultiByteToWideChar(CP_ACP, 0, pLeft, -1, wOneSideFileName, sizeof(wOneSideFileName));
		if ( cubeside == 2 ) MultiByteToWideChar(CP_ACP, 0, pUp, -1, wOneSideFileName, sizeof(wOneSideFileName));
		if ( cubeside == 3 ) MultiByteToWideChar(CP_ACP, 0, pDown, -1, wOneSideFileName, sizeof(wOneSideFileName));
		if ( cubeside == 4 ) MultiByteToWideChar(CP_ACP, 0, pFront, -1, wOneSideFileName, sizeof(wOneSideFileName));
		if ( cubeside == 5 ) MultiByteToWideChar(CP_ACP, 0, pBack, -1, wOneSideFileName, sizeof(wOneSideFileName));

		// load the image
	    std::unique_ptr<DirectX::ScratchImage> image(new (std::nothrow)DirectX::ScratchImage);
		if ( strnicmp ( pUp + strlen(pUp)-4, ".dds", 4 ) == NULL )
			hr = DirectX::LoadFromDDSFile( wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);
		else
			hr = DirectX::LoadFromWICFile( wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);

		// store in loadedimages list
		images++; loadedImages.push_back(std::move(image));
	}

	// create image array storing the sides
	std::vector<DirectX::Image> imageArray;
	imageArray.reserve(images);
	for (auto it = loadedImages.cbegin(); it != loadedImages.cend(); ++it)
	{
		const DirectX::ScratchImage* simage = it->get();
		assert(simage != 0);
		for (size_t j = 0; j < simage->GetMetadata().arraySize; ++j)
		{
			const DirectX::Image* img = simage->GetImage(0, j, 0);
			assert(img != 0);
			imageArray.push_back(*img);
		}
	}

	// create the cube map from the image array
	DirectX::ScratchImage result;
	hr = result.InitializeCubeFromImages(&imageArray[0], imageArray.size());
	if ( hr == S_OK )
	{
		wchar_t wSaveCubeFile[512];
		MultiByteToWideChar(CP_ACP, 0, pCubeToSave, -1, wSaveCubeFile, sizeof(wSaveCubeFile));
        hr = DirectX::SaveToDDSFile(result.GetImages(), result.GetImageCount(), result.GetMetadata(), DirectX::DDS_FLAGS_NONE, wSaveCubeFile);
	}

	// anything to free
	result.Release();
	for (auto it = loadedImages.cbegin(); it != loadedImages.cend(); ++it)
	{
		DirectX::ScratchImage* simage = it->get();
		simage->Release();
	}
}

void ImageCreateSurfaceTextureChannels(LPSTR pSurfaceToSave, LPSTR pAO, LPSTR pGloss, LPSTR pMetalness, int iO, int iG, int iM, int iA )
{
	// work to largest texture dimension for surface
	int iResultSurfaceWidth = 0;
	int iResultSurfaceHeight = 0;

	// old version of this function didn't work if one texture was missing 
	// e.g. the second texture found would always be placed in gloss/roughness channel even if it was the metalness texture

	// load in contributing textures
	HRESULT hr;
    size_t images = 0;
	bool bLoadedAtLeastOne = false;
    DirectX::TexMetadata info;
	DirectX::ScratchImage* scratchImages[3];
	scratchImages[0] = 0; // AO
	scratchImages[1] = 0; // Roughness
	scratchImages[2] = 0; // Metalness
	for (int contributiontex = 0; contributiontex < 3; contributiontex++ )
	{
		// the filename
		LPSTR pTheFilename = NULL;
		if (contributiontex == 0) pTheFilename = pAO;
		if (contributiontex == 1) pTheFilename = pGloss;
		if (contributiontex == 2) pTheFilename = pMetalness;

		// substitute with stock texture if none supplied
		if (contributiontex == 0 && (pTheFilename == NULL || *pTheFilename == NULL)) pTheFilename = "editors\\gfx\\13.png";

		// can omit AO
		if (pTheFilename && *pTheFilename)
		{
			// get filenames as wchars
			wchar_t wOneSideFileName[512];
			MultiByteToWideChar(CP_ACP, 0, pTheFilename, -1, wOneSideFileName, sizeof(wOneSideFileName));

			// load the image
			DirectX::ScratchImage* image = new DirectX::ScratchImage();
			if (strnicmp(pTheFilename + strlen(pTheFilename) - 4, ".dds", 4) == NULL)
				hr = DirectX::LoadFromDDSFile(wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);
			else
				hr = DirectX::LoadFromWICFile(wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);

			// adjust final result surface size
			if (hr == S_OK)
			{
				bLoadedAtLeastOne = true;
				if (iResultSurfaceWidth < info.width) iResultSurfaceWidth = info.width;
				if (iResultSurfaceHeight < info.height) iResultSurfaceHeight = info.height;
			}

			// uncompress so we can use them
			images++;
			if (info.format >= DXGI_FORMAT_BC1_TYPELESS && info.format <= DXGI_FORMAT_BC5_SNORM)
			{
				// store uncompressed in loadedimages list
				scratchImages[ contributiontex ] = new DirectX::ScratchImage();
				hr = DirectX::Decompress(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DXGIFORMATR8G8B8A8UNORM, *scratchImages[contributiontex] );
				image->Release();
				delete image;
			}
			else
			{
				if (info.format != DXGIFORMATR8G8B8A8UNORM)
				{
					// must be in RGBA format, so convert
					scratchImages[ contributiontex ] = new DirectX::ScratchImage();
					hr = DirectX::Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DXGIFORMATR8G8B8A8UNORM, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, *scratchImages[contributiontex]);
					image->Release();
					delete image;
				}
				else
				{
					// store in loadedimages list
					scratchImages[contributiontex] = image;
				}
			}
		}
	}

	// if no AO, METALNESS or GLOSS (arg), then assume a small surface with all default sutff in it
	if (bLoadedAtLeastOne == false)
	{
		iResultSurfaceWidth = 32;
		iResultSurfaceHeight = 32;
	}

	// ensure all component images are resized to match surface
    for( int i = 0; i < 3; i++ )
	{
		// resize to match surface dimension
		if ( !scratchImages[i] ) continue;
		DirectX::ScratchImage* resizedImage = new DirectX::ScratchImage();
		hr = DirectX::Resize(scratchImages[i]->GetImages(), scratchImages[i]->GetImageCount(), scratchImages[i]->GetMetadata(), iResultSurfaceWidth, iResultSurfaceHeight, DirectX::TEX_FILTER_DITHER, *resizedImage);
		scratchImages[i]->Release();
		delete scratchImages[i];
		scratchImages[i] = resizedImage;
	}

	// create image for surface
	DirectX::ScratchImage result;
	hr = result.Initialize2D(DXGIFORMATR8G8B8A8UNORM, iResultSurfaceWidth, iResultSurfaceHeight, 1, 1, DirectX::DDS_FLAGS_NONE);

	// copy each texture to a channel in the surface

	uint8_t* pDstPtr = result.GetPixels();
	uint8_t* pAOPtr = scratchImages[0] ? scratchImages[0]->GetPixels() : 0;
	uint8_t* pRoughnessPtr = scratchImages[1] ? scratchImages[1]->GetPixels() : 0;
	uint8_t* pMetalnessPtr = scratchImages[2] ? scratchImages[2]->GetPixels() : 0;
	for (int y = 0; y < iResultSurfaceHeight; y++)
	{
		for (int x = 0; x < iResultSurfaceWidth; x++)
		{
			// ambient occlusion
			if ( pAOPtr ) 
			{
				*(pDstPtr + 0) = *(pAOPtr + iO);
				pAOPtr += 4;
			}
			else *(pDstPtr + 0) = 255; // white if no AO

			// roughness
			if ( pRoughnessPtr ) 
			{
				*(pDstPtr + 1) = *(pRoughnessPtr + iG);
				pRoughnessPtr += 4;
			}
			else *(pDstPtr + 1) = 128; // grey if no roughness

			// metalness
			if ( pMetalnessPtr )
			{
				*(pDstPtr + 2) = *(pMetalnessPtr + iM);
				pMetalnessPtr += 4;
			}
			else *(pDstPtr + 2) = 0; // black if no metalness

			// alpha reflectance
			// iA = in the future we may want to pass in a reflectance map and specify channel where reflectance data is stored
			// for now we will assume reflectance is FULL to allow the per-material reflectance to affect surface globally
			*(pDstPtr + 3) = 255;

			// next pixel please
			pDstPtr += 4;
		}
	}

	// create mipmaps for surface
	DirectX::ScratchImage mipChain;
	hr = DirectX::GenerateMipMaps( result.GetImages(), result.GetImageCount(), result.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );

	// compress resulting surface
	DirectX::ScratchImage compressedSurface;
	hr = DirectX::Compress(mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), DXGI_FORMAT_BC1_UNORM, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, compressedSurface);
	if ( hr == S_OK )
	{
		// ensure destination folder exists (if using imported_models)
		char pPathToSaveTo[MAX_PATH];
		strcpy (pPathToSaveTo, pSurfaceToSave);
		for (int n = strlen(pPathToSaveTo) - 1; n > 0; n--)
		{
			if (pPathToSaveTo[n] == '\\' || pPathToSaveTo[n] == '/')
			{
				pPathToSaveTo[n] = 0;
				break;
			}
		}
		extern int PathExist ( LPSTR pFilename );
		if (PathExist(pPathToSaveTo) == 0)
		{
			// create the imported_models folder
			LPSTR pOnFolderBack = strstr (pPathToSaveTo, "imported_models");
			if ( pOnFolderBack != NULL)
			{
				*pOnFolderBack = 0;
				char pOldDir[MAX_PATH];
				GetCurrentDirectoryA(MAX_PATH, pOldDir);
				SetCurrentDirectoryA(pPathToSaveTo);
				CreateDirectoryA("imported_models", NULL);
				SetCurrentDirectoryA(pOldDir);
			}
		}

		// save the suyrface as a DDS
		wchar_t wSaveSurfaceFile[512];
		MultiByteToWideChar(CP_ACP, 0, pSurfaceToSave, -1, wSaveSurfaceFile, sizeof(wSaveSurfaceFile));
        hr = DirectX::SaveToDDSFile(compressedSurface.GetImages(), compressedSurface.GetImageCount(), compressedSurface.GetMetadata(), DirectX::DDS_FLAGS_NONE, wSaveSurfaceFile);
		compressedSurface.Release();
	}

	// anything to free
	mipChain.Release();
	result.Release();
	for( int i = 0; i < 3; i++)
	{
		if ( scratchImages[i] ) 
		{
			scratchImages[i]->Release();
			delete scratchImages[ i ];
		}
	}
}

void ImageCreateSurfaceTexture(LPSTR pSurfaceToSave, LPSTR pAO, LPSTR pGloss, LPSTR pMetalness )
{
	ImageCreateSurfaceTextureChannels(pSurfaceToSave, pAO, pGloss, pMetalness, 0, 0, 0, 0);
}

void ImageCreateNormalTextureInvertedGreen(LPSTR pSurfaceToSave, LPSTR pAO, LPSTR pGloss, LPSTR pMetalness, int iO, int iG, int iM, int iA)
{
	// work to largest texture dimension for normal.
	int iResultSurfaceWidth = 0;
	int iResultSurfaceHeight = 0;

	// load in contributing textures
	HRESULT hr;
	size_t images = 0;
	bool bLoadedAtLeastOne = false;
	DirectX::TexMetadata info;
	DirectX::ScratchImage* scratchImages[3];
	scratchImages[0] = 0; 
	scratchImages[1] = 0; 
	scratchImages[2] = 0; 
	for (int contributiontex = 0; contributiontex < 3; contributiontex++)
	{
		// the filename
		LPSTR pTheFilename = NULL;
		if (contributiontex == 0) pTheFilename = pAO;
		if (contributiontex == 1) pTheFilename = pGloss;
		if (contributiontex == 2) pTheFilename = pMetalness;

		// substitute with stock texture if none supplied
		if (contributiontex == 0 && (pTheFilename == NULL || *pTheFilename == NULL)) pTheFilename = "editors\\gfx\\13.png";

		if (pTheFilename && *pTheFilename)
		{
			// get filenames as wchars
			wchar_t wOneSideFileName[512];
			MultiByteToWideChar(CP_ACP, 0, pTheFilename, -1, wOneSideFileName, sizeof(wOneSideFileName));

			// load the image
			DirectX::ScratchImage* image = new DirectX::ScratchImage();
			if (strnicmp(pTheFilename + strlen(pTheFilename) - 4, ".dds", 4) == NULL)
				hr = DirectX::LoadFromDDSFile(wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);
			else
				hr = DirectX::LoadFromWICFile(wOneSideFileName, DirectX::DDS_FLAGS_NONE, &info, *image);

			// adjust final result size
			if (hr == S_OK)
			{
				bLoadedAtLeastOne = true;
				if (iResultSurfaceWidth < info.width) iResultSurfaceWidth = info.width;
				if (iResultSurfaceHeight < info.height) iResultSurfaceHeight = info.height;
			}

			// uncompress so we can use them
			images++;
			if (info.format >= DXGI_FORMAT_BC1_TYPELESS && info.format <= DXGI_FORMAT_BC5_SNORM)
			{
				// store uncompressed in loadedimages list
				scratchImages[contributiontex] = new DirectX::ScratchImage();
				hr = DirectX::Decompress(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DXGIFORMATR8G8B8A8UNORM, *scratchImages[contributiontex]);
				image->Release();
				delete image;
			}
			else
			{
				if (info.format != DXGIFORMATR8G8B8A8UNORM)
				{
					// must be in RGBA format, so convert
					scratchImages[contributiontex] = new DirectX::ScratchImage();
					hr = DirectX::Convert(image->GetImages(), image->GetImageCount(), image->GetMetadata(), DXGIFORMATR8G8B8A8UNORM, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, *scratchImages[contributiontex]);
					image->Release();
					delete image;
				}
				else
				{
					// store in loadedimages list
					scratchImages[contributiontex] = image;
				}
			}
		}
	}

	if (bLoadedAtLeastOne == false)
	{
		iResultSurfaceWidth = 32;
		iResultSurfaceHeight = 32;
	}

	// ensure all component images are resized to match 
	for (int i = 0; i < 3; i++)
	{
		// resize to match dimension
		if (!scratchImages[i]) continue;
		DirectX::ScratchImage* resizedImage = new DirectX::ScratchImage();
		hr = DirectX::Resize(scratchImages[i]->GetImages(), scratchImages[i]->GetImageCount(), scratchImages[i]->GetMetadata(), iResultSurfaceWidth, iResultSurfaceHeight, DirectX::TEX_FILTER_DITHER, *resizedImage);
		scratchImages[i]->Release();
		delete scratchImages[i];
		scratchImages[i] = resizedImage;
	}

	// create image 
	DirectX::ScratchImage result;
	hr = result.Initialize2D(DXGIFORMATR8G8B8A8UNORM, iResultSurfaceWidth, iResultSurfaceHeight, 1, 1, DirectX::DDS_FLAGS_NONE);

	// copy each texture to a channel in the normal map

	uint8_t* pDstPtr = result.GetPixels();
	uint8_t* pRPtr = scratchImages[0] ? scratchImages[0]->GetPixels() : 0;
	uint8_t* pGPtr = scratchImages[1] ? scratchImages[1]->GetPixels() : 0;
	uint8_t* pBPtr = scratchImages[2] ? scratchImages[2]->GetPixels() : 0;
	for (int y = 0; y < iResultSurfaceHeight; y++)
	{
		for (int x = 0; x < iResultSurfaceWidth; x++)
		{
			// Red channel
			if (pRPtr)
			{
				*(pDstPtr + 0) = *(pRPtr + iO);
				pRPtr += 4;
			}
			else *(pDstPtr + 0) = 255; 

			// Green channel
			if (pGPtr)
			{
				// Might not be the best way to invert the green channel. Tiny artefacts appear on the result.
				*(pDstPtr + 1) = 255 - *(pGPtr + iG);
				pGPtr += 4;
			}
			else *(pDstPtr + 1) = 128; 

			// Blue Channel
			if (pBPtr)
			{
				*(pDstPtr + 2) = *(pBPtr + iM);
				pBPtr += 4;
			}
			else *(pDstPtr + 2) = 0; // black if no metalness

			*(pDstPtr + 3) = 255;

			// next pixel please
			pDstPtr += 4;
		}
	}

	// create mipmaps for normal map
	DirectX::ScratchImage mipChain;
	hr = DirectX::GenerateMipMaps(result.GetImages(), result.GetImageCount(), result.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain);

	// compress result
	DirectX::ScratchImage compressedSurface;
	hr = DirectX::Compress(mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), DXGI_FORMAT_BC1_UNORM, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, compressedSurface);
	if (hr == S_OK)
	{
		// ensure destination folder exists (if using imported_models)
		char pPathToSaveTo[MAX_PATH];
		strcpy(pPathToSaveTo, pSurfaceToSave);
		for (int n = strlen(pPathToSaveTo) - 1; n > 0; n--)
		{
			if (pPathToSaveTo[n] == '\\' || pPathToSaveTo[n] == '/')
			{
				pPathToSaveTo[n] = 0;
				break;
			}
		}
		extern int PathExist(LPSTR pFilename);
		if (PathExist(pPathToSaveTo) == 0)
		{
			// create the imported_models folder
			LPSTR pOnFolderBack = strstr(pPathToSaveTo, "imported_models");
			if (pOnFolderBack != NULL)
			{
				*pOnFolderBack = 0;
				char pOldDir[MAX_PATH];
				GetCurrentDirectoryA(MAX_PATH, pOldDir);
				SetCurrentDirectoryA(pPathToSaveTo);
				CreateDirectoryA("imported_models", NULL);
				SetCurrentDirectoryA(pOldDir);
			}
		}
		
		// save the normal map as a DDS
		wchar_t wSaveSurfaceFile[512];
		MultiByteToWideChar(CP_ACP, 0, pSurfaceToSave, -1, wSaveSurfaceFile, sizeof(wSaveSurfaceFile));
		hr = DirectX::SaveToDDSFile(compressedSurface.GetImages(), compressedSurface.GetImageCount(), compressedSurface.GetMetadata(), DirectX::DDS_FLAGS_NONE, wSaveSurfaceFile);
		compressedSurface.Release();
	}

	// anything to free
	mipChain.Release();
	result.Release();
	for (int i = 0; i < 3; i++)
	{
		if (scratchImages[i])
		{
			scratchImages[i]->Release();
			delete scratchImages[i];
		}
	}
}

int ImageCreateTexturePlate(LPSTR pDestTerrainTextureFile, int iWhichTextureOver, LPSTR pTexFileToLoad, int iSeamlessMode, int iCompressIt)
{
	assert( 0 && "DX11 not in use" );
	return 0;
	/*
	// check if texture to load exists
	GGIMAGE_INFO finfo;
	HRESULT hr = D3DX11GetImageInfoFromFileA( pTexFileToLoad, NULL, &finfo, NULL );

	// real destination for image creation
	char pRealDestination[MAX_PATH];
	strcpy(pRealDestination, pDestTerrainTextureFile);
	GG_GetRealPath(pRealDestination, 1);

	// filenames to WCHAR
	wchar_t wFilenameInsert[512];
	wchar_t wFilenamePlate[512];
	std::string pPlateFilename = pRealDestination;
	MultiByteToWideChar(CP_ACP, 0, pTexFileToLoad, -1, wFilenameInsert, sizeof(wFilenameInsert));
	MultiByteToWideChar(CP_ACP, 0, pPlateFilename.c_str(), -1, wFilenamePlate, sizeof(wFilenamePlate));

	// is insert a DDS or other
	bool bInsertTextureIsDDS = false;
	if ( strnicmp ( pTexFileToLoad + strlen(pTexFileToLoad) - 4, ".dds", 4 ) == NULL )
		bInsertTextureIsDDS = true;

	// create and load the texture selected
	DirectX::ScratchImage imageTextureToInsert;
	DirectX::ScratchImage imageTexturePlate;
	DirectX::ScratchImage convertedTextureToInsert;
	DirectX::ScratchImage convertedTexturePlate;
	LPGGTEXTURE pLoadedTexSurface1024 = NULL;
	LPGGTEXTURE pLoadedTexSurface512 = NULL;
	LPGGTEXTURE pLoadedTexSurface256512 = NULL;
	LPGGTEXTURE pLoadedTexSurface512256 = NULL;
	LPGGTEXTURE pPlateSurface = NULL;
	if ( hr == S_OK )
	{
		// create/load texture to be inserted
		DirectX::TexMetadata insertdata;
		if ( bInsertTextureIsDDS == true )
		{
			hr = GetMetadataFromDDSFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromDDSFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		else
		{
			hr = GetMetadataFromWICFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, insertdata );			
			hr = LoadFromWICFile( wFilenameInsert, DirectX::DDS_FLAGS_NONE, &insertdata, imageTextureToInsert );
		}
		if ( SUCCEEDED(hr) )
		{
			// create/load texture of plate
			DirectX::TexMetadata platedata;
			hr = GetMetadataFromDDSFile( wFilenamePlate, DirectX::DDS_FLAGS_NONE, platedata );			
			if ( hr != S_OK )
			{
				// if plate file not exist, create one and provide dimensions
				finfo.Width = 4096;
				finfo.Height = 4096;

				// create the plate from fresh
				imageTexturePlate.Initialize2D (DXGIFORMATR8G8B8A8UNORM, finfo.Width, finfo.Height, 1, 1, 0 );
				platedata = imageTexturePlate.GetMetadata();
			}
			else
			{
				// existing plate exists, load it in
				hr = LoadFromDDSFile( wFilenamePlate, DirectX::DDS_FLAGS_NONE, &platedata, imageTexturePlate );
				if ( FAILED(hr) ) platedata.width = 0;
			}
			if ( platedata.width > 0 )
			{
				// ensure we convert compressed textures to uncompressed one, store as pWrkImage
				DirectX::ScratchImage* pWrkImage = &imageTextureToInsert;
				if ( imageTextureToInsert.GetMetadata().format >= DXGI_FORMAT_BC1_TYPELESS && imageTextureToInsert.GetMetadata().format <= DXGI_FORMAT_BC5_SNORM )
				{
					hr = Decompress(imageTextureToInsert.GetImages(), imageTextureToInsert.GetImageCount(), imageTextureToInsert.GetMetadata(),
						DXGI_FORMAT_B8G8R8A8_UNORM, convertedTextureToInsert);
					pWrkImage = &convertedTextureToInsert;
				}

				// resize
				DirectX::ScratchImage InsertImage1024;
				DirectX::ScratchImage InsertImage512;
				DirectX::ScratchImage InsertImage256512;
				DirectX::ScratchImage InsertImage512256;
				hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1024, 1024, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage1024 );
				CreateTexture(m_pD3D, InsertImage1024.GetImages(), InsertImage1024.GetImageCount(), InsertImage1024.GetMetadata(), &pLoadedTexSurface1024 );
				if ( iSeamlessMode == 0 )
				{
					// 512x512 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 512, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 256, 512, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage256512 );
					CreateTexture(m_pD3D, InsertImage256512.GetImages(), InsertImage256512.GetImageCount(), InsertImage256512.GetMetadata(), &pLoadedTexSurface256512 );
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 512, 256, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512256 );
					CreateTexture(m_pD3D, InsertImage512256.GetImages(), InsertImage512256.GetImageCount(), InsertImage512256.GetMetadata(), &pLoadedTexSurface512256 );
				}
				else
				{
					// 1022x1022 at center
					hr = Resize( pWrkImage->GetImages(), pWrkImage->GetImageCount(), pWrkImage->GetMetadata(), 1022, 1022, DirectX::TEX_FILTER_SEPARATE_ALPHA, InsertImage512 );
					hr = CreateTexture(m_pD3D, InsertImage512.GetImages(), InsertImage512.GetImageCount(), InsertImage512.GetMetadata(), &pLoadedTexSurface512 );
				}
				// texture plate always compressed
				hr = Decompress( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(),
					DXGI_FORMAT_B8G8R8A8_UNORM, convertedTexturePlate );
				if ( convertedTexturePlate.GetImageCount() == 0 )
				{
					CreateTexture(m_pD3D, imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				}
				else
				{
					platedata.format = convertedTexturePlate.GetImages()->format;
					CreateTexture(m_pD3D, convertedTexturePlate.GetImages(), convertedTexturePlate.GetImageCount(), platedata, &pPlateSurface );
				}
			}
		}

		// paste loaded texture into plate (60 in center of atlas texture area)
		if ( pLoadedTexSurface1024 && pPlateSurface ) 
		{
			// work out exact offset to slot position
			int iRow = iWhichTextureOver / 4;
			int iCol = iWhichTextureOver - (iRow*4);
			int iTexSlotOffsetX = iCol * 1024;
			int iTexSlotOffsetY = iRow * 1024;
			RECT rcPlate = RECT();

			// paste to fill 1024x1024 initially (to get at corners)
			rcPlate.left = iTexSlotOffsetX;
			rcPlate.top = iTexSlotOffsetY;
			rcPlate.right = iTexSlotOffsetX+1024;
			rcPlate.bottom = iTexSlotOffsetY+1024;
			m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface1024, 0, NULL );
			if ( iSeamlessMode == 0 )
			{
				// paste squashed 256x512 borders to help seamlessness
				// left
				rcPlate.left = iTexSlotOffsetX+0; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// right
				rcPlate.left = iTexSlotOffsetX+256+512; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+1024; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface256512, 0, NULL );
				// top
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+0; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// bottom
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256+512; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+1024;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512256, 0, NULL );
				// paste insert so smaller 512x512 terrain texture atlas can be seamless
				rcPlate.left = iTexSlotOffsetX+256; rcPlate.top = iTexSlotOffsetY+256; rcPlate.right = iTexSlotOffsetX+256+512; rcPlate.bottom = iTexSlotOffsetY+256+512;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}
			else
			{
				// paste 1022x1022 on borders for seamlessness
				int iX = 1, iY = 0;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 1, iY = 2;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 0, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				iX = 2, iY = 1;
				rcPlate.left = iTexSlotOffsetX+iX; rcPlate.top = iTexSlotOffsetY+iY; rcPlate.right = iTexSlotOffsetX+iX+1022; rcPlate.bottom = iTexSlotOffsetY+iY+1022;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
				// then paste into surface at 1022x1022 (so can have seamless textures within atlas)
				rcPlate.left = iTexSlotOffsetX+1; rcPlate.top = iTexSlotOffsetY+1;
				m_pImmediateContext->CopySubresourceRegion ( pPlateSurface, 0, (UINT)rcPlate.left, (UINT)rcPlate.top, 0, pLoadedTexSurface512, 0, NULL );
			}

			// replace imageTexturePlate with contents of pPlateSurface
			hr = CaptureTexture( m_pD3D, m_pImmediateContext, pPlateSurface, imageTexturePlate );
			if ( SUCCEEDED(hr) )
			{
				// create mipmaps
				DirectX::ScratchImage mipChain;
				hr = GenerateMipMaps( imageTexturePlate.GetImages(), imageTexturePlate.GetImageCount(), imageTexturePlate.GetMetadata(), DirectX::TEX_FILTER_SEPARATE_ALPHA, 0, mipChain );

				// compressed or not
				if ( iCompressIt == 0 )
				{
					// save new UNCOMPRESSED texture surface out
					const DirectX::Image* img = mipChain.GetImages();
					hr = SaveToDDSFile( img, mipChain.GetImageCount(), mipChain.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				}
				else
				{
					// compress to a DXT5 (BC3) texture
					hr = Compress( mipChain.GetImages(), mipChain.GetImageCount(), mipChain.GetMetadata(), 
						DXGI_FORMAT_BC3_UNORM, DirectX::TEX_COMPRESS_DEFAULT, DirectX::TEX_THRESHOLD_DEFAULT, convertedTexturePlate );

					// save new texture surface out
					const DirectX::Image* img = convertedTexturePlate.GetImages();
					hr = SaveToDDSFile( img, convertedTexturePlate.GetImageCount(), convertedTexturePlate.GetMetadata(), DirectX::DDS_FLAGS_NONE, wFilenamePlate );
				}
			}
		}

		// free temp surface captures
		SAFE_RELEASE(pPlateSurface);
		SAFE_RELEASE(pLoadedTexSurface1024);
		SAFE_RELEASE(pLoadedTexSurface512);
		SAFE_RELEASE(pLoadedTexSurface256512);
		SAFE_RELEASE(pLoadedTexSurface512256);
	}

	// success
	return 1;
	*/
}

LPGGTEXTURE ConvertBackBufferToNewFormat ( LPGGSURFACE pBackBuffer, GGFORMAT NewFormat)
{
	assert( 0 && "DX11 not in use" );
	return 0;
	/*
	// read data of backbuffer, copy into known format (as cannot use CopySubResource to do conversions)
	LPGGTEXTURE pCorrectBackBufferTexture = NULL;

	// read backbuffer surface into scratch image
	DirectX::ScratchImage imageBBSurface;
	HRESULT hr = DirectX::CaptureTexture( m_pD3D, m_pImmediateContext, pBackBuffer, imageBBSurface );
	if (SUCCEEDED(hr))
	{
		// convert to desired format
		DirectX::ScratchImage imageBBSurfaceConverted;
		
		hr = DirectX::Convert(imageBBSurface.GetImages(), imageBBSurface.GetImageCount(), imageBBSurface.GetMetadata(), NewFormat, DirectX::TEX_FILTER_DITHER, DirectX::TEX_THRESHOLD_DEFAULT, imageBBSurfaceConverted);
		if (SUCCEEDED(hr))
		{
			//if (0)
			//{
			//	// save something out as a test - yes, the backbuffer was captured fine :)
			//	wchar_t wFilenamePlate[512];
			//	MultiByteToWideChar(CP_ACP, 0, "leeleetestbackbufferconvert.png", -1, wFilenamePlate, sizeof(wFilenamePlate));
			//	const DirectX::Image* img = imageBBSurfaceConverted.GetImages();
			//	hr = SaveToWICFile(*img, DirectX::DDS_FLAGS_NONE, GUID_ContainerFormatPng, wFilenamePlate, NULL);
			//}
			// create new surface texture from new image
			hr = DirectX::CreateTexture(m_pD3D, imageBBSurfaceConverted.GetImages(), imageBBSurfaceConverted.GetImageCount(), imageBBSurfaceConverted.GetMetadata(), &pCorrectBackBufferTexture);
		}
	}

	// return newly minted texture (from which surface can be read later on)
	return pCorrectBackBufferTexture;
	*/
}

LPGGTEXTURE CreateCroppedTexture(LPGGTEXTURE pSourceTexture, D3D11_BOX rc)
{
	assert( 0 && "DX11 not in use" );
	return 0;
	/*
	// take a texture, take a crop from it and return that
	LPGGTEXTURE pCroppedTexture = NULL;

	// read backbuffer surface into scratch image
	DirectX::ScratchImage imageOrigTexture;
	HRESULT hr = DirectX::CaptureTexture( m_pD3D, m_pImmediateContext, pSourceTexture, imageOrigTexture );
	if (SUCCEEDED(hr))
	{
		// original image
		const DirectX::Image* img = imageOrigTexture.GetImages();

		// crop into new scratchimage
		size_t width = rc.right - rc.left;
		size_t height = rc.bottom - rc.top;
		DirectX::ScratchImage imageCroppedTexture;
		imageCroppedTexture.Initialize2D(img->format, width, height, 1, 1);
		const DirectX::Image* imgdest = imageCroppedTexture.GetImages();
		if (imgdest)
		{
			DirectX::Rect srcRect = { rc.left, rc.top, width, height };
			hr = DirectX::CopyRectangle(*img, srcRect, *imgdest, DirectX::TEX_FILTER_DEFAULT, 0, 0);

			// create new surface texture from new image
			hr = DirectX::CreateTexture(m_pD3D, imageCroppedTexture.GetImages(), imageCroppedTexture.GetImageCount(), imageCroppedTexture.GetMetadata(), &pCroppedTexture);
		}
	}

	// return newly cropped texture
	return pCroppedTexture;
	*/
}

DARKSDK unsigned char* DecompressImage ( LPGGSURFACE pTexSurface )
{
	assert( 0 && "DX11 not in use" );
	return 0;
	/*
	unsigned char* pixels = NULL;

	GGSURFACE_DESC srcddsd;
	if ( pTexSurface )
	{
		// determine size of surface
		HRESULT hRes;
		pTexSurface->GetDesc ( &srcddsd );

		// create and load the texture selected
		LPGGTEXTURE pPlateSurface = NULL;

		// load compressed texture to a plate
		DirectX::ScratchImage imageTexturePlate;
		HRESULT hr = CaptureTexture ( m_pD3D, m_pImmediateContext, pTexSurface, imageTexturePlate );
		if ( SUCCEEDED ( hr ) )
		{
			// decompress the plate
			DirectX::ScratchImage uncompressedTexturePlate;
			hr = Decompress ( imageTexturePlate.GetImages ( ), imageTexturePlate.GetImageCount ( ), imageTexturePlate.GetMetadata ( ),
				DXGIFORMATR8G8B8A8UNORM, uncompressedTexturePlate );
			if (FAILED(hr))
			{
				unsigned int iSize = imageTexturePlate.GetMetadata().width * imageTexturePlate.GetMetadata().height * 4;
				pixels = new unsigned char[iSize];
				memcpy(&pixels[0], imageTexturePlate.GetPixels(), iSize);
			}
			else
			{
				unsigned int iSize = uncompressedTexturePlate.GetMetadata().width * uncompressedTexturePlate.GetMetadata().height * 4;
				pixels = new unsigned char[iSize];
				memcpy(&pixels[0], uncompressedTexturePlate.GetPixels(), iSize);
			}

			// free surface and leave, completed save with DirectXTex functions (as needed format conversion)
			SAFE_RELEASE ( pTexSurface );
			return pixels;
		}
	}
	
	return NULL;
	*/
}

DARKSDK int ImageFormat(int iID)
{
	assert( 0 && "DX11 not in use" );
	return -1;
	/*
	// get the width of an image
	if (iID<1 || iID>MAXIMUMVALUE)
	{
		RunTimeError(RUNTIMEERROR_IMAGEILLEGALNUMBER);
		return false;
	}
	// update internal data
	if (!UpdatePtrImage(iID))
		return -1;

	LPGGSURFACE pTexSurface = NULL;
	GGSURFACE_DESC imgdesc;
	m_imgptr->lpTexture->QueryInterface<ID3D11Texture2D>(&pTexSurface);
	if (!pTexSurface) return -1;
	pTexSurface->GetDesc(&imgdesc);
	SAFE_RELEASE(pTexSurface);
	return imgdesc.Format;
	*/
}
