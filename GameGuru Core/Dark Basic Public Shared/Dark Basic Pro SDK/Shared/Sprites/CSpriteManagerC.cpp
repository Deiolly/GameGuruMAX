// GG flag header for preprocessor defines
#include "..\..\..\..\GameGuru\Include\preprocessor-flags.h"

#include "cspritemanagerc.h"
#include <algorithm>
#include "CImageC.h"
#include "CGfxC.h"
#include "CCameraC.h"
#include ".\..\..\Shared\Objects\CommonC.h"
#include ".\..\..\Shared\camera\ccameradatac.h"

#ifdef ENABLEIMGUI
//PE: GameGuru IMGUI.
#include "..\..\..\..\GameGuru\Imgui\imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "..\..\..\..\GameGuru\Imgui\imgui_internal.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_impl_win32.h"
#include "..\..\..\..\GameGuru\Imgui\imgui_gg_extras.h"
#endif

#undef DARKSDK
#define DARKSDK

// External Data from rest of code
extern LPGGDEVICE				m_pD3D;
extern LPGGIMMEDIATECONTEXT		m_pImmediateContext;
extern LPGGSURFACE				g_pBackBuffer;
extern int						g_iFilterMode;

GGVECTOR3						CSpriteManager::vecDisplayMin;
GGVECTOR3						CSpriteManager::vecDisplayMax;
std::vector<tagSpriteData*>     CSpriteManager::m_SpriteDrawList;
CSpriteManager::t_SpriteList    CSpriteManager::m_SortedSpriteList;

#ifdef DX11
struct CBPerFrame
{
	KMaths::Matrix mWorld;
	KMaths::Matrix mView;
	KMaths::Matrix mProjection;
};				
struct SpriteVertex
{
	GGVECTOR4 PositionRHW;
	DWORD dwColor;
	GGVECTOR2 UV;
};
void*				g_pCBPerFrame						= NULL;
void*				g_pSpriteVertexDeclaration			= NULL;
void*				g_pSpriteVertexBuffer				= NULL;
#endif

struct CSpriteManager::PrioritiseSprite
{
    int m_id;
    tagSpriteData* m_ptr;

    PrioritiseSprite ( SpritePtr pItem ) 
        : m_id( pItem->first), m_ptr( pItem->second )
    {}

    PrioritiseSprite ( SpritePtrConst pItem ) 
        : m_id( pItem->first), m_ptr( pItem->second )
    {}

    bool operator < (const PrioritiseSprite& Other) const
    {
        // Sort by Priority
        if (m_ptr->iPriority < Other.m_ptr->iPriority)        return true;
        if (m_ptr->iPriority > Other.m_ptr->iPriority)        return false;
        
        // For matching priorities, sort by the image id
        // to try and avoid texture lookups and changes.
        if (m_ptr->iImage < Other.m_ptr->iImage)              return true;
        if (m_ptr->iImage > Other.m_ptr->iImage)              return false;

        // For matching image ids, sort by transparency
        // to try and avoid transparency mode changes.
        if (m_ptr->bTransparent < Other.m_ptr->bTransparent)  return true;
        if (m_ptr->bTransparent > Other.m_ptr->bTransparent)  return false;

        // Where everything else matches, sort by the sprite id
        // to give a unique order to all sprites
        return (m_id < Other.m_id);
    }
};



////////////////////////////
// Class member functions //
////////////////////////////



CSpriteManager::CSpriteManager ( void )
{
    m_CurrentId = 0;
    m_CurrentPtr = 0;
    m_FilterMode = 0;
	m_TempDisableDraw = false;
	m_bSpriteBatcherActive = false;
}


CSpriteManager::~CSpriteManager ( void )
{
}


bool CSpriteManager::Add ( tagSpriteData* pData, int iID )
{
    tagSpriteData* ptr = 0;

    SpritePtr p = m_List.find( iID );
    if (p == m_List.end())
    {
        // If doesn't exist, then create one and add to the list
	    ptr = new tagSpriteData;
        m_List.insert( std::make_pair(iID, ptr) );
    }
    else
    {
        ptr = p->second;
    }

    // Update the sprite data
    *ptr = *pData;

    m_CurrentId = iID;
    m_CurrentPtr = ptr;

	return false;
}


bool CSpriteManager::Delete ( int iID )
{
    SpritePtr p = m_List.find( iID );
    if (p == m_List.end())
		return false;

    DeleteJustOne( p->second );
    m_List.erase( p );
	
    if (m_CurrentId == iID)
    {
        m_CurrentId = 0;
        m_CurrentPtr = 0;
    }

	return true;
}

int g_iStoreSpriteBatchCount = 0;
int g_iStoreSpriteBatchMax = 10000;
tagSpriteData* g_pStoreSpriteBatch[10000];

void CSpriteManager::DrawImmediate ( tagSpriteData* pData )
{
	// determine if immediate or batcher in effect
	if ( m_bSpriteBatcherActive == true )
	{
		// store sprite data for later batch rendering
		if ( g_iStoreSpriteBatchCount < g_iStoreSpriteBatchMax )
		{
			g_pStoreSpriteBatch[g_iStoreSpriteBatchCount] = new tagSpriteData;
			memcpy ( g_pStoreSpriteBatch[g_iStoreSpriteBatchCount], pData, sizeof(tagSpriteData) );
			g_iStoreSpriteBatchCount++;
		}
	}
	else
	{
		// Build a render list containing just one sprite
		tagSpriteData* pSpriteList[1] = { pData };

		// Render it
		RenderDrawList ( pSpriteList, 1, m_FilterMode );
	}
}

void CSpriteManager::DrawBatchImmediate ( void )
{
	if ( g_iStoreSpriteBatchCount > 0 )
	{
		// draw all sprites added during the batching
		RenderDrawList ( g_pStoreSpriteBatch, g_iStoreSpriteBatchCount, m_FilterMode );

		// delete stored batch items and reset draw count index for next time
		for ( int iI = 0; iI < g_iStoreSpriteBatchCount; iI++ )
		{
			SAFE_DELETE ( g_pStoreSpriteBatch[iI] );
		}
		g_iStoreSpriteBatchCount = 0;
	}
}

tagSpriteData* CSpriteManager::GetData ( int iID ) const
{
    if (iID != m_CurrentId)
    {
        SpritePtrConst p = m_List.find( iID );
        if (p == m_List.end())
            return 0;

        m_CurrentId = iID;
        m_CurrentPtr = p->second;
    }
    return m_CurrentPtr;
}


int CSpriteManager::Update ( void ) const
{
    // Clear the old list and resize to hold up to the actual number of sprites.
    m_SortedSpriteList.clear();
    m_SortedSpriteList.reserve(m_List.size());

    // Update our copy of the display size, for culling purposes
    GetDisplaySize ();

	// 050416 - can disable sprite drawing for a while (for screen blanking)
	if ( m_TempDisableDraw == false )
	{
		// Build the sorted list of sprites
		// Dump all visible sprites into a vector
		for (SpritePtrConst pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
		{
			if (pCheck->second)
			{
				PrioritiseSprite s(pCheck);
				if (s.m_ptr->iImage > 0 && s.m_ptr->bVisible && IsSpriteInDisplay ( s.m_ptr ))
				{
					m_SortedSpriteList.push_back( s );
				}
			}
		}

		if ( ! m_SortedSpriteList.empty() )
		{
			// Sort the vector
			std::sort( m_SortedSpriteList.begin(), m_SortedSpriteList.end() );

			// Prepare the draw list
			m_SpriteDrawList.clear();
			m_SpriteDrawList.reserve( m_SortedSpriteList.size() );

			// Build the draw list from the sorted list
			t_SpriteListPtr Last = m_SortedSpriteList.end();
			for (t_SpriteListPtr Current = m_SortedSpriteList.begin(); Current != Last; ++Current)
			{
				m_SpriteDrawList.push_back( Current->m_ptr );
			}

			// Render the draw list
			RenderDrawList ( &m_SpriteDrawList[0], m_SpriteDrawList.size(), m_FilterMode );
		}
		else
		{
			#ifdef WICKEDENGINE
			int get_gameisexe(void);
			if (get_gameisexe() == 1)
			{
				//PE: We still need to render background if needed.
				RenderDrawList(NULL, 0, m_FilterMode);
			}
			#endif
		}
	}
	#ifdef WICKEDENGINE
	extern bool bRenderNextFrame;
	bRenderNextFrame = false;
	#endif

    return m_SortedSpriteList.size();
}


void CSpriteManager::DeleteAll ( void )
{
    for (SpritePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagSpriteData* ptr = pCheck->second;

	    // release the sprite
		if ( ptr )
		    DeleteJustOne(ptr);
    }

    m_List.clear();

    m_CurrentId = 0;
    m_CurrentPtr = 0;
	#ifdef WICKEDENGINE
	extern bool bRenderNextFrame;
	bRenderNextFrame = false;
	#endif

}


void CSpriteManager::HideAll ( void )
{
    for (SpritePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagSpriteData* ptr = pCheck->second;

		// if the data doesn't exist then skip it
		if ( ptr )
        {
		    // run through all sprites and set their visible property
		    ptr->bVisible = false;
        }
    }
	#ifdef WICKEDENGINE
	extern bool bRenderNextFrame;
	bRenderNextFrame = false;
	#endif

}


void CSpriteManager::ShowAll ( void )
{
    for (SpritePtr pCheck = m_List.begin(); pCheck != m_List.end(); ++pCheck)
	{
		tagSpriteData* ptr = pCheck->second;

		// if the data doesn't exist then skip it
		if ( ptr )
        {
		    // run through all sprites and set their visible property
		    ptr->bVisible = true;
        }
    }
}

void CSpriteManager::DisableAll ( void )
{
	m_TempDisableDraw = true;
	#ifdef WICKEDENGINE
	extern bool bRenderNextFrame;
	bRenderNextFrame = false;
	#endif
}

void CSpriteManager::EnableAll ( void )
{
	m_TempDisableDraw = false;
}



///////////////////////////////////
// Class static member functions //
///////////////////////////////////



void CSpriteManager::DeleteJustOne(tagSpriteData* ptr)
{
	// remove reference to image (do not delete image!)
	ptr->lpTexture = NULL;

	// free up the allocated memory block
	delete ptr;
}

#ifdef WICKEDENGINE
#define TESTIMGUI
#endif

bool bSpriteWinVisible = false;
extern bool bImGuiFrameState;
#ifdef TESTIMGUI
int iKeepBackgroundForFrames = 0;
void CSpriteManager::RenderDrawList(tagSpriteData** pList, int iListSize, int iFilterMode)
{
	ImGuiContext& g = *GImGui;
	// externs used
	extern bool bImGuiInTestGame;
	extern bool bRenderTabTab;
	extern bool bBlockImGuiUntilNewFrame;
	extern bool bImGuiRenderWithNoCustomTextures;
	extern bool g_bNoGGUntilGameGuruMainCalled;
	if (!g_bNoGGUntilGameGuruMainCalled)
		return;
	if (!g.Font->IsLoaded())
		return;
	// override
	bool bNeed2DUniversally = true;

	// leave if not in imgui test game
	if (!bImGuiInTestGame && bNeed2DUniversally==false)
		return;

	// new frame if about to use imgui to render this sprite draw list
	if ((bImGuiInTestGame || bNeed2DUniversally) && !bRenderTabTab && !bImGuiFrameState)
	{
		//We need a new frame.
		ImGui_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		bSpriteWinVisible = false;
		bRenderTabTab = true;
		bBlockImGuiUntilNewFrame = false;
		bImGuiRenderWithNoCustomTextures = false;
	}

	// Wicked will have its own way to render batched sprites
	if (g.FrameCount > 0) 
	{
		ImGuiWindow* gotwindow = ImGui::FindWindowByName("GGClassicSprite");
		if (!bSpriteWinVisible)	gotwindow = NULL;
		ImGuiWindow* window = NULL;
		if (!gotwindow)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(ImVec2(-100.0, 1) + ImGui::GetMainViewport()->Pos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(0.1, 0.1), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::Begin("GGClassicSprite", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
			window = ImGui::GetCurrentWindow();
			ImVec2 renderTargetSize = ImGui::GetContentRegionAvail();
			ImVec2 renderTargetPos = ImGui::GetWindowPos();
		}
		else 
		{
			window = gotwindow;
		}

		if (!gotwindow)
		{
			bSpriteWinVisible = true;
			ImGui::End();
			#ifdef STORYBOARD
			extern DWORD g_dwSyncMaskOverride;
			extern bool bJustRederedScreenEditor;
			extern bool bMainLoopRunning;
			extern int g_iInGameMenuState;
			//if (g_dwSyncMaskOverride == 0 && !bJustRederedScreenEditor)
			if (!bMainLoopRunning)
					iKeepBackgroundForFrames = 10;
			if (g_iInGameMenuState != 1 && iKeepBackgroundForFrames > 0 && !bJustRederedScreenEditor)
			{
				int get_gameisexe(void);
				if (get_gameisexe() == 1)
				{
					//Fill background so no 3D can be seen.
					ImVec4 monitor_col = ImVec4(0.0, 0, 0, 1.0); //Black for now.
					window->DrawList->AddRectFilled(ImVec2(-1, -1), ImGui::GetMainViewport()->Size + ImVec2(1, 1), ImGui::GetColorU32(monitor_col));
				}
			}
			if (iKeepBackgroundForFrames > 0) iKeepBackgroundForFrames--;
			#endif
		}
		if (pList == NULL || iListSize == 0) return;
		//PE: Moved drawing outside window so we always goes to the main viewport.
		for (int iTemp = 0; iTemp < iListSize; iTemp++)
		{
			// this sprite
			tagSpriteData* pCurrent = pList[iTemp];
			ImVec4 drawCol_normal = ImColor(pCurrent->iRed, pCurrent->iGreen, pCurrent->iBlue, pCurrent->iAlpha);
			bool bVisible = pCurrent->bVisible;

			if (bVisible)
			{
				//PE: We need a way to disable tabtab drawing if we have no sprites. so:
				extern bool bRenderNextFrame;
				bRenderNextFrame = true;
				void* lpTexture = pCurrent->lpTexture;
				if (pCurrent->iImage > 0)
				{
					lpTexture = GetImagePointer(pCurrent->iImage);
				}
				//LB: Allow for rotated sprites
				//window->DrawList->AddImage((ImTextureID)lpTexture, ImVec2(fX, fY), (ImVec2(fX, fY) + ImVec2(iWidth, iHeight)), UV_Min, UV_Max, ImGui::GetColorU32(drawCol_normal));
				ImVec2 p0 = ImVec2(pCurrent->lpVertices[0].x, pCurrent->lpVertices[0].y);
				ImVec2 p1 = ImVec2(pCurrent->lpVertices[1].x, pCurrent->lpVertices[1].y);
				ImVec2 p2 = ImVec2(pCurrent->lpVertices[3].x, pCurrent->lpVertices[3].y);
				ImVec2 p3 = ImVec2(pCurrent->lpVertices[2].x, pCurrent->lpVertices[2].y);
				ImVec2 uv0 = ImVec2(pCurrent->lpVertices[0].tu, pCurrent->lpVertices[0].tv);
				ImVec2 uv1 = ImVec2(pCurrent->lpVertices[1].tu, pCurrent->lpVertices[1].tv);
				ImVec2 uv2 = ImVec2(pCurrent->lpVertices[3].tu, pCurrent->lpVertices[3].tv);
				ImVec2 uv3 = ImVec2(pCurrent->lpVertices[2].tu, pCurrent->lpVertices[2].tv);

				if (lpTexture)
				{
					//PE: We need to render above everything else.
					ImDrawList* dl = ImGui::GetForegroundDrawList();
					if (dl)
					{
						dl->AddImageQuad((ImTextureID)lpTexture, p0, p1, p2, p3, uv0, uv1, uv2, uv3, ImGui::GetColorU32(drawCol_normal));
					}
					else
					{
						window->DrawList->AddImageQuad((ImTextureID)lpTexture, p0, p1, p2, p3, uv0, uv1, uv2, uv3, ImGui::GetColorU32(drawCol_normal));
					}
				}
			}
		}
	}
}
#endif

void CSpriteManager::GetDisplaySize()
{
	#ifdef WICKEDENGINE
	// get clipping area for sprite culling
	vecDisplayMin = GGVECTOR3((float)0, (float)0, 0);
	vecDisplayMax = GGVECTOR3((float)GetDisplayWidth() , (float)GetDisplayHeight(), 1);
	#else
	if ( g_pBackBuffer == NULL ) return;
    // Build display size from the viewport rather than the resolution...
	#ifdef DX11
	#ifdef ENABLEIMGUI
	#ifndef USEOLDIDE
	//PE: Sprites get clipped, as the backbuffer can be smaller when we use a render target.
	vecDisplayMin = GGVECTOR3((float)0, (float)0, 0);
	vecDisplayMax = GGVECTOR3((float)GetDisplayWidth() , (float)GetDisplayHeight(), 1);
	return;
	#endif
	#endif
	D3D11_TEXTURE2D_DESC ddsd;
	//PE: Got a g_pBackBuffer=NULL here and "D3D11: Removing Device." caused by decals. Nothing we can do if g_pBackBuffer=0.
	//PE: Seams like g_pBackBuffer=0 if "D3D11: Removing Device.", so if a future device restore is attempted this could be checked.
	g_pBackBuffer->GetDesc ( &ddsd );
    vecDisplayMin = GGVECTOR3 ( (float)0, (float)0, 0 );
    vecDisplayMax = GGVECTOR3 ( (float)ddsd.Width, (float)ddsd.Height, 1);
	#else
    GGVIEWPORT SaveViewport;
    m_pD3D->GetViewport(&SaveViewport);
    vecDisplayMin = GGVECTOR3 ( 
        (float)SaveViewport.X, 
        (float)SaveViewport.Y,
        SaveViewport.MinZ );
    vecDisplayMax = GGVECTOR3 (
        (float)(SaveViewport.X + SaveViewport.Width),
        (float)(SaveViewport.Y + SaveViewport.Height),
        SaveViewport.MaxZ );
	#endif
	#endif
}


bool CSpriteManager::IsSpriteInDisplay(tagSpriteData* m_ptr)
{
    // Determine min/max of the sprite's draw area from its vertices
    GGVECTOR3 vecMin = GGVECTOR3 ( m_ptr->lpVertices[0].x, m_ptr->lpVertices[0].y, m_ptr->lpVertices[0].z );
    GGVECTOR3 vecMax = vecMin;

    for ( int iVertex = 1; iVertex < 4; iVertex++ ) 
    {
	    if ( m_ptr->lpVertices [ iVertex ].x < vecMin.x ) vecMin.x = m_ptr->lpVertices [ iVertex ].x;
	    if ( m_ptr->lpVertices [ iVertex ].y < vecMin.y ) vecMin.y = m_ptr->lpVertices [ iVertex ].y;
	    if ( m_ptr->lpVertices [ iVertex ].z < vecMin.z ) vecMin.z = m_ptr->lpVertices [ iVertex ].z;
	    if ( m_ptr->lpVertices [ iVertex ].x > vecMax.x ) vecMax.x = m_ptr->lpVertices [ iVertex ].x;
	    if ( m_ptr->lpVertices [ iVertex ].y > vecMax.y ) vecMax.y = m_ptr->lpVertices [ iVertex ].y;
	    if ( m_ptr->lpVertices [ iVertex ].z > vecMax.z ) vecMax.z = m_ptr->lpVertices [ iVertex ].z;
    }

    // Check box intersection
    return (vecDisplayMax.x >= vecMin.x && vecDisplayMin.x <= vecMax.x &&
		    vecDisplayMax.y >= vecMin.y && vecDisplayMin.y <= vecMax.y );
}

