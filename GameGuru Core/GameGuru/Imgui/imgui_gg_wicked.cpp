
#include "preprocessor-flags.h"
#include "preprocessor-moreflags.h"

// Includes 
#include "stdafx.h"

#include "imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "imgui_gg_extras.h"

#include "WickedEngine.h"

using namespace wiGraphics;
using namespace wiRenderer;

extern bool bImGuiReadyToRender;
extern bool bImGuiInTestGame;
extern bool bBlockImGuiUntilNewFrame;
extern bool bImGuiRenderWithNoCustomTextures;
extern bool bBlockImGuiUntilFurtherNotice;
extern ImVec2 fImGuiScissorTopLeft;
extern ImVec2 fImGuiScissorBottomRight;
extern bool bForceRenderEverywhere;
extern bool bDigAHoleToHWND;
extern RECT rDigAHole;

#ifdef WICKEDENGINE
	extern bool bRenderTabTab;
	extern bool bRenderNextFrame;
	extern std::vector<void*> lpBadTexture;
#endif

IMGUI_IMPL_API bool     ImGui_ImplWicked_Init();
IMGUI_IMPL_API void     ImGui_ImplWicked_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplWicked_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplWicked_Update( CommandList cmd );
//IMGUI_IMPL_API void     ImGui_ImplWicked_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
IMGUI_IMPL_API void     ImGui_ImplWicked_InvalidateDeviceObjects();
IMGUI_IMPL_API bool     ImGui_ImplWicked_CreateDeviceObjects();

extern "C" int GG_GetRealPath( char* fullPath, int create );

// DirectX data
static GPUBuffer         g_VertexBuffer;
static GPUBuffer         g_IndexBuffer;
static int               g_VertexBufferSize = 5000;
static int               g_IndexBufferSize = 10000;

static Shader            g_VertexShader;
static GPUBuffer         g_VertexConstantBuffer;

Shader imgui_vertexShader;
Shader imgui_pixelShader;
Shader imgui_pixelShaderNoAlpha;
Shader imgui_pixelShaderBoost25;
Shader imgui_pixelShaderBlur;

InputLayout imgui_inputLayout;

BlendState imgui_blendDesc;
RasterizerState imgui_rastDesc;
DepthStencilState imgui_depthDesc;

static PipelineState     g_PipelineState;
static PipelineState     g_PipelineStateBlur;
static PipelineState     g_PipelineStateNoAlpha;
static PipelineState     g_PipelineStateBoost25;

static Texture           g_FontTexture;
static Sampler           g_FontSampler;

struct VERTEX_CONSTANT_BUFFER
{
    float   mvp[4][4];
};

static void ImGui_ImplWicked_SetupRenderState( ImDrawData* draw_data, CommandList cmd )
{
	GraphicsDevice* device = GetDevice();

    // Setup viewport
    Viewport vp;
    vp.Width = draw_data->DisplaySize.x;
    vp.Height = draw_data->DisplaySize.y;
    vp.TopLeftX = 0;
	vp.TopLeftY = 0;
    device->BindViewports( 1, &vp, cmd );

    // Setup shader and vertex buffers
    unsigned int stride = sizeof(ImDrawVert);
    unsigned int offset = 0;
	GPUBuffer* pVBs[ 1 ] = { &g_VertexBuffer };
	device->BindVertexBuffers( pVBs, 0, 1, &stride, &offset, cmd );
    device->BindIndexBuffer( &g_IndexBuffer, sizeof(ImDrawIdx) == 2 ? INDEXFORMAT_16BIT : INDEXFORMAT_32BIT, 0, cmd );
	device->BindSampler( PS, &g_FontSampler, 0, cmd );
	device->BindPipelineState( &g_PipelineState, cmd );
	device->BindConstantBuffer( VS, &g_VertexConstantBuffer, 0, cmd );
}

void ImGui_ImplWicked_Update( CommandList cmd )
{
	GraphicsDevice* device = GetDevice();

	ImDrawData* draw_data = ImGui::GetDrawData();
	if ( !draw_data ) return;

	// Avoid rendering if no data
	if (draw_data == NULL)
		return;

	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	// Create and grow vertex/index buffers if needed
	if (!g_VertexBuffer.IsValid() || g_VertexBufferSize < draw_data->TotalVtxCount)
	{
		g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
		GPUBufferDesc desc;
		desc.Usage = USAGE_DYNAMIC;
		desc.ByteWidth = g_VertexBufferSize * sizeof(ImDrawVert);
		desc.BindFlags = BIND_VERTEX_BUFFER;
		desc.CPUAccessFlags = CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		if ( !device->CreateBuffer(&desc, NULL, &g_VertexBuffer) ) return;
	}
	if (!g_IndexBuffer.IsValid() || g_IndexBufferSize < draw_data->TotalIdxCount)
	{
		g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
		GPUBufferDesc desc;
		desc.Usage = USAGE_DYNAMIC;
		desc.ByteWidth = g_IndexBufferSize * sizeof(ImDrawIdx);
		desc.BindFlags = BIND_INDEX_BUFFER;
		desc.CPUAccessFlags = CPU_ACCESS_WRITE;
		if ( !device->CreateBuffer(&desc, NULL, &g_IndexBuffer) ) return;
	}

	uint32_t iVertexBufferTotalSize = 0;
	uint32_t iIndexBufferTotalSize = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		iVertexBufferTotalSize += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
		iIndexBufferTotalSize += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
	}

	uint8_t* pVertexData = new uint8_t[ iVertexBufferTotalSize ];
	uint8_t* pIndexData = new uint8_t[ iIndexBufferTotalSize ];
	ImDrawVert* vtx_dst = (ImDrawVert*)pVertexData;
	ImDrawIdx* idx_dst = (ImDrawIdx*)pIndexData;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.Size;
		idx_dst += cmd_list->IdxBuffer.Size;
	}

	device->UpdateBuffer( &g_VertexBuffer, pVertexData, cmd, iVertexBufferTotalSize );
	device->UpdateBuffer( &g_IndexBuffer, pIndexData, cmd, iIndexBufferTotalSize );

	delete [] pVertexData;
	delete [] pIndexData;

	// Setup orthographic projection matrix into our constant buffer
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	{
		float L = draw_data->DisplayPos.x;
		float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
		float T = draw_data->DisplayPos.y;
		float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
		float constant_buffer[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		device->UpdateBuffer( &g_VertexConstantBuffer, constant_buffer, cmd, sizeof(VERTEX_CONSTANT_BUFFER) );
	}
}

void ImGui_ImplWicked_RenderCall_Direct( CommandList cmd )
{
#ifdef WICKEDENGINE
	if (bBlockImGuiUntilNewFrame || bBlockImGuiUntilFurtherNotice)
		return;
	if (bRenderNextFrame)
	{
		//bRenderNextFrame = false;
	}
	else
	{
		if (bImGuiInTestGame && !bRenderTabTab)
			return;
		//	if (bRenderTabTab)
		//		bRenderTabTab = false;
	}
#endif

	GraphicsDevice* device = GetDevice();

	ImDrawData* draw_data = ImGui::GetDrawData();

	// Setup desired DX state
	ImGui_ImplWicked_SetupRenderState(draw_data, cmd);

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_idx_offset = 0;
	int global_vtx_offset = 0;
	ImVec2 clip_off = draw_data->DisplayPos;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			
			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)1)
			{
				//PE: Change shaders.
				// for now we ignore shader changes mid-processing, put back when we see something!
				//ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
				
				// never used
				assert(0);
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineStateNoAlpha, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				device->BindPipelineState( &g_PipelineStateBoost25, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				device->BindPipelineState( &g_PipelineStateBlur, cmd );
			}
			else if (pcmd->UserCallback != NULL)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				// for now we ignore shader changes mid-processing, put back when we see something!
				//if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
				//    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
				//else
				//    pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				// Apply scissor/clipping rectangle
				const Rect r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
				device->BindScissorRects( 1, &r, cmd );

				// Bind texture, Draw

				//Locate bad textures.
				bool bBadTexture = false;
			#ifdef WICKEDENGINE
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (pcmd->TextureId == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
			#endif
				if (!bBadTexture)
				{
					wiGraphics::Texture* pTexture = (wiGraphics::Texture*) pcmd->TextureId;
					device->BindResource( PS, pTexture, 0, cmd );
					
					device->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, cmd );
				}
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}

#ifdef WICKEDENGINE
	lpBadTexture.clear();
#endif
}

// New Render function which is now called from Wicked Engine LIB
void ImGui_ImplWicked_RenderCall( CommandList cmd )
{
	GraphicsDevice* device = GetDevice();

#ifdef WICKEDENGINE
	extern bool g_bNoGGUntilGameGuruMainCalled;
	if (g_bNoGGUntilGameGuruMainCalled==false)
		return;
	extern bool g_bNo2DRender;
	if (g_bNo2DRender)
		return;
	bool bSpecialNoCustomTextureRender = false;
	if (bBlockImGuiUntilNewFrame || bBlockImGuiUntilFurtherNotice)
	{
		if (bBlockImGuiUntilNewFrame && bImGuiRenderWithNoCustomTextures)
		{
			bSpecialNoCustomTextureRender = true;
		}
		else
		{
			return;
		}
	}

	if (bRenderNextFrame)
	{
		//PE: If we have zero sprites and if (bImGuiInTestGame && !bRenderTabTab) , we need to return.
		
		//bRenderNextFrame = false;
	} 
	else
	{
		if (bImGuiInTestGame && !bRenderTabTab)
			return;
		//	if (bRenderTabTab)
		//		bRenderTabTab = false;
	}
#endif

	// goes through the same sequence as 'ImGui_ImplDX11_RenderDrawData' but Wicked Friendly..

	ImDrawData* draw_data = ImGui::GetDrawData();

	// Avoid rendering if no data
	if (draw_data == NULL)
		return;

    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

	// Setup desired DX state
	ImGui_ImplWicked_SetupRenderState(draw_data, cmd);

	/*
	if (bDigAHoleToHWND)
	{
		//PE: Cant use it without a geometry shaders.
		//PE: Need 4 viewports.
		D3D11_VIEWPORT vp[4];
		for (int i = 0; i < 4; i++)
		{
			memset(&vp[i], 0, sizeof(D3D11_VIEWPORT));
			vp[i].Width = draw_data->DisplaySize.x;
			vp[i].Height = draw_data->DisplaySize.y;
			vp[i].MinDepth = 0.0f;
			vp[i].MaxDepth = 1.0f;
			vp[i].TopLeftX = vp[i].TopLeftY = 0;
		}
		ctx->RSSetViewports(4, &vp[0]);
	}
	*/

	ImGuiContext& g = *GImGui;

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback) 1)
			{
				//PE: Change shaders.
				// for now we ignore shader changes mid-processing, put back when we see something!
				//ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
				assert(0); // I don't think this is ever used
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineStateNoAlpha, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				// for now we ignore shader changes mid-processing, put back when we see something!
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				device->BindPipelineState( &g_PipelineStateBoost25, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				device->BindPipelineState( &g_PipelineStateBlur, cmd );
			}
			else if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				// for now we ignore shader changes mid-processing, put back when we see something!
                //if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                //    ImGui_ImplDX11_SetupRenderState(draw_data, ctx);
                //else
                //    pcmd->UserCallback(cmd_list, pcmd);
            }
			#ifdef WICKEDENGINE
			else if(bSpecialNoCustomTextureRender)
			{
				//PE: Only render eveything but no special textures.
				if (pcmd->TextureId == g.Font->ContainerAtlas->TexID)
				{
					// Apply scissor/clipping rectangle
					const Rect r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
					device->BindScissorRects( 1, &r, cmd );

					// Bind texture, Draw

					bool bBadTexture = false;
					#ifdef WICKEDENGINE
					//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
					if (lpBadTexture.size() > 0)
					{
						for (int i = 0; i < lpBadTexture.size(); i++)
						{
							if (pcmd->TextureId == lpBadTexture[i])
							{
								bBadTexture = true;
								break;
							}
						}
					}
					#endif

					if (!bBadTexture)
					{
						wiGraphics::Texture* pTexture = (wiGraphics::Texture*) pcmd->TextureId;
						device->BindResource( PS, pTexture, 0, cmd );

						device->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, cmd );
					}
				}
			}
			#endif
            else
            {
                // Apply scissor/clipping rectangle

				if (bDigAHoleToHWND && !bForceRenderEverywhere)
				{
					const Rect rAll = { (LONG)draw_data->DisplayPos.x - clip_off.x, draw_data->DisplayPos.y - clip_off.y, (LONG)draw_data->DisplayPos.x + draw_data->DisplaySize.x, (LONG)draw_data->DisplayPos.y + draw_data->DisplaySize.y };
					Rect r[4];
					for (int i = 0; i < 4; i++)
					{
						r[i] = rAll;
					}
					r[0].bottom = rDigAHole.top; //Rect 1
					r[1].top = rDigAHole.top; //Rect 2
					r[1].right = rDigAHole.left; //Rect 2
					r[2].top = rDigAHole.bottom; //Rect 3
					r[2].left = rDigAHole.left; //Rect 3
					r[3].top = rDigAHole.top; //Rect 4
					r[3].left = rDigAHole.right; //Rect 4
					r[3].bottom = rDigAHole.bottom; //Rect 4
					
					bool bBadTexture = false;
					#ifdef WICKEDENGINE
					//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
					if (lpBadTexture.size() > 0)
					{
						for (int i = 0; i < lpBadTexture.size(); i++)
						{
							if (pcmd->TextureId == lpBadTexture[i])
							{
								bBadTexture = true;
								break;
							}
						}
					}
					#endif
					if (!bBadTexture)
					{
						wiGraphics::Texture* pTexture = (wiGraphics::Texture*) pcmd->TextureId;
						
						for (int i = 0; i < 3; i++)
						{
							device->BindScissorRects(1, &r[i], cmd);
							device->BindResource( PS, pTexture, 0, cmd );
							device->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, cmd );
						}
					}
					device->BindScissorRects( 1, &r[3], cmd );
				}
				else
				{
					const Rect r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
					device->BindScissorRects( 1, &r, cmd );
				}

                // Bind texture, Draw
	            
				bool bBadTexture = false;
				#ifdef WICKEDENGINE
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (pcmd->TextureId == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
				#endif
				if (!bBadTexture)
				{
					Texture* pTexture = (wiGraphics::Texture*) pcmd->TextureId;
					device->BindResource( PS, pTexture, 0, cmd );
					
					device->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, cmd );
				}

            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

	#ifdef WICKEDENGINE
	lpBadTexture.clear();
	#endif

}

/*
// Render function
// (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
void ImGui_ImplWicked_RenderDrawData( ImDrawData* draw_data, CommandList cmd )
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

	GraphicsDevice* device = GetDevice();

	// Setup desired DX state
	ImGui_ImplWicked_SetupRenderState(draw_data, cmd);

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_idx_offset = 0;
    int global_vtx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback == (ImDrawCallback)10)
			{
				bForceRenderEverywhere = true;
			}
			else if (pcmd->UserCallback == (ImDrawCallback)11)
			{
				bForceRenderEverywhere = false;
			}
			else if (pcmd->UserCallback == (ImDrawCallback) 1)
			{
				//PE: Change shaders.
				//ctx->PSSetShader(g_pPixelShaderNoWhite, NULL, 0);
				assert(0); // I don't think this is used
			}
			else if (pcmd->UserCallback == (ImDrawCallback)2)
			{
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)3)
			{
				//NoAlpha.
				device->BindPipelineState( &g_PipelineStateNoAlpha, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)4)
			{
				//Alpha.
				device->BindPipelineState( &g_PipelineState, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)5)
			{
				//boost colors 25 percent.
				device->BindPipelineState( &g_PipelineStateBoost25, cmd );
			}
			else if (pcmd->UserCallback == (ImDrawCallback)6)
			{
				//Blur
				device->BindPipelineState( &g_PipelineStateBlur, cmd );
			}
			else if (pcmd->UserCallback != NULL)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplWicked_SetupRenderState(draw_data, cmd);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Apply scissor/clipping rectangle
                const Rect r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                device->BindScissorRects( 1, &r, cmd );

                // Bind texture, Draw
                
				bool bBadTexture = false;
				#ifdef WICKEDENGINE
				//PE: Prevent imgui from crashing when rendering using a deleted ID3D11ShaderResourceView.
				if (lpBadTexture.size() > 0)
				{
					for (int i = 0; i < lpBadTexture.size(); i++)
					{
						if (pcmd->TextureId == lpBadTexture[i])
						{
							bBadTexture = true;
							break;
						}
					}
				}
				#endif
				
				if (!bBadTexture)
				{
					Texture* pTexture = (wiGraphics::Texture*) pcmd->TextureId;
					device->BindResource( PS, pTexture, 0, cmd );
					
					device->DrawIndexed( pcmd->ElemCount, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, cmd );
				}
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

	#ifdef WICKEDENGINE
	lpBadTexture.clear();
	#endif
}
*/

static void ImGui_ImplWicked_CreateFontsTexture()
{
	GraphicsDevice* device = GetDevice();
	
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    // Upload texture to graphics system
    {
		TextureDesc desc;
		desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = FORMAT_R8G8B8A8_UNORM;
        desc.Usage = USAGE_DEFAULT;
        desc.BindFlags = BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

		SubresourceData subResource;
        subResource.pSysMem = pixels;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        device->CreateTexture( &desc, &subResource, &g_FontTexture );
    }

    // Store our identifier
    io.Fonts->TexID = (ImTextureID)&g_FontTexture;

    // Create texture sampler
    {
		SamplerDesc desc;
        desc.Filter = FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = TEXTURE_ADDRESS_WRAP;
        desc.AddressV = TEXTURE_ADDRESS_WRAP;
        desc.AddressW = TEXTURE_ADDRESS_WRAP;
        desc.MipLODBias = 0.f;
        desc.ComparisonFunc = COMPARISON_ALWAYS;
        desc.MinLOD = 0.f;
        desc.MaxLOD = 0.f;
        device->CreateSampler( &desc, &g_FontSampler );
    }
}

bool ImGui_ImplWicked_CreateDeviceObjects()
{
	GraphicsDevice* device = GetDevice();
	
    if ( g_FontSampler.IsValid() )
	{
        ImGui_ImplWicked_InvalidateDeviceObjects();
	}
        
    PipelineStateDesc pipelineDesc;

	// Create the constant buffer
	{
		GPUBufferDesc desc;
		desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
		desc.Usage = USAGE_DEFAULT;
		desc.BindFlags = BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;
		device->CreateBuffer( &desc, NULL, &g_VertexConstantBuffer );
	}
    
    // Create the input layout
	imgui_inputLayout.elements =
	{
		{ "POSITION", 0, FORMAT_R32G32_FLOAT,   0, 0,  INPUT_PER_VERTEX_DATA },
		{ "TEXCOORD", 0, FORMAT_R32G32_FLOAT,   0, 8,  INPUT_PER_VERTEX_DATA },
		{ "COLOR",    0, FORMAT_R8G8B8A8_UNORM, 0, 16, INPUT_PER_VERTEX_DATA },
	};

	std::vector<uint8_t> buffer;
	char filePath[ MAX_PATH ];
	
	strcpy_s( filePath, MAX_PATH, "../shaders/imguiVertex.cso" );
	GG_GetRealPath( filePath, 0 );
	if ( !wiHelper::FileRead(filePath, buffer) ) return false;
	device->CreateShader( VS, buffer.data(), buffer.size(), &imgui_vertexShader );

	strcpy_s( filePath, MAX_PATH, "../shaders/imguiPixel.cso" );
	GG_GetRealPath( filePath, 0 );
	if ( !wiHelper::FileRead(filePath, buffer) ) return false;
	device->CreateShader( PS, buffer.data(), buffer.size(), &imgui_pixelShader );

	strcpy_s( filePath, MAX_PATH, "../shaders/imguiPixelNoAlpha.cso" );
	GG_GetRealPath( filePath, 0 );
	if ( !wiHelper::FileRead(filePath, buffer) ) return false;
	device->CreateShader( PS, buffer.data(), buffer.size(), &imgui_pixelShaderNoAlpha );

	strcpy_s( filePath, MAX_PATH, "../shaders/imguiPixelBlur.cso" );
	GG_GetRealPath( filePath, 0 );
	if ( !wiHelper::FileRead(filePath, buffer) ) return false;
	device->CreateShader( PS, buffer.data(), buffer.size(), &imgui_pixelShaderBlur );

	strcpy_s( filePath, MAX_PATH, "../shaders/imguiPixelBoost25.cso" );
	GG_GetRealPath( filePath, 0 );
	if ( !wiHelper::FileRead(filePath, buffer) ) return false;
	device->CreateShader( PS, buffer.data(), buffer.size(), &imgui_pixelShaderBoost25 );

    // Create the blending setup
	imgui_blendDesc.AlphaToCoverageEnable = false;
	imgui_blendDesc.RenderTarget[0].BlendEnable = true;
	imgui_blendDesc.RenderTarget[0].SrcBlend = BLEND_SRC_ALPHA;
	imgui_blendDesc.RenderTarget[0].DestBlend = BLEND_INV_SRC_ALPHA;
	imgui_blendDesc.RenderTarget[0].BlendOp = BLEND_OP_ADD;
	imgui_blendDesc.RenderTarget[0].SrcBlendAlpha = BLEND_INV_SRC_ALPHA;
	imgui_blendDesc.RenderTarget[0].DestBlendAlpha = BLEND_ZERO;
	imgui_blendDesc.RenderTarget[0].BlendOpAlpha = BLEND_OP_ADD;
	imgui_blendDesc.RenderTarget[0].RenderTargetWriteMask = COLOR_WRITE_ENABLE_ALL;

    // Create the rasterizer state
	imgui_rastDesc.FillMode = FILL_SOLID;
	imgui_rastDesc.CullMode = CULL_NONE;
	imgui_rastDesc.DepthClipEnable = true;

    // Create depth-stencil State
	imgui_depthDesc.DepthEnable = false;
	imgui_depthDesc.DepthWriteMask = DEPTH_WRITE_MASK_ALL;
	imgui_depthDesc.DepthFunc = COMPARISON_ALWAYS;
	imgui_depthDesc.StencilEnable = false;
	imgui_depthDesc.FrontFace.StencilFailOp = STENCIL_OP_KEEP;
	imgui_depthDesc.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
	imgui_depthDesc.FrontFace.StencilPassOp = STENCIL_OP_KEEP;
	imgui_depthDesc.FrontFace.StencilFunc = COMPARISON_ALWAYS;
	imgui_depthDesc.BackFace = imgui_depthDesc.FrontFace;

	PipelineStateDesc desc = {};
	desc.vs = &imgui_vertexShader;
	desc.ps = &imgui_pixelShader;

	desc.il = &imgui_inputLayout;
	desc.pt = TRIANGLELIST;
	desc.rs = &imgui_rastDesc;
	desc.dss = &imgui_depthDesc;
	desc.bs = &imgui_blendDesc;
	device->CreatePipelineState( &desc, &g_PipelineState );

	desc.ps = &imgui_pixelShaderNoAlpha;
	device->CreatePipelineState( &desc, &g_PipelineStateNoAlpha );

	desc.ps = &imgui_pixelShaderBlur;
	device->CreatePipelineState( &desc, &g_PipelineStateBlur );

	desc.ps = &imgui_pixelShaderBoost25;
	device->CreatePipelineState( &desc, &g_PipelineStateBoost25 );
	
    ImGui_ImplWicked_CreateFontsTexture();

    return true;
}

void ImGui_ImplWicked_InvalidateDeviceObjects()
{
	/*
    if (g_pFontSampler) { g_pFontSampler->Release(); g_pFontSampler = NULL; }
    if (g_pFontTextureView) { g_pFontTextureView->Release(); g_pFontTextureView = NULL; ImGui::GetIO().Fonts->TexID = NULL; } // We copied g_pFontTextureView to io.Fonts->TexID so let's clear that as well.
    if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
    if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }

    if (g_pBlendState) { g_pBlendState->Release(); g_pBlendState = NULL; }
    if (g_pDepthStencilState) { g_pDepthStencilState->Release(); g_pDepthStencilState = NULL; }
    if (g_pRasterizerState) { g_pRasterizerState->Release(); g_pRasterizerState = NULL; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = NULL; }
	if (g_pPixelShaderBlur) { g_pPixelShaderBlur->Release(); g_pPixelShaderBlur = NULL; }
	if (g_ppixelShaderBoost25) { g_ppixelShaderBoost25->Release(); g_ppixelShaderBoost25 = NULL; }
	if (g_pPixelShaderNoAlpha) { g_pPixelShaderNoAlpha->Release(); g_pPixelShaderNoAlpha = NULL; }

    if (g_pVertexConstantBuffer) { g_pVertexConstantBuffer->Release(); g_pVertexConstantBuffer = NULL; }
    if (g_pInputLayout) { g_pInputLayout->Release(); g_pInputLayout = NULL; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = NULL; }
    */
}

bool ImGui_ImplWicked_Init()
{
    // Setup back-end capabilities flags
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_wicked";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

    return true;
}

void ImGui_ImplWicked_Shutdown()
{
    ImGui_ImplWicked_InvalidateDeviceObjects();
}

void ImGui_ImplWicked_NewFrame()
{
    if ( !g_FontSampler.IsValid() )
	{
        ImGui_ImplWicked_CreateDeviceObjects();
	}
}
