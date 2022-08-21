#include "d3d11_3.h"
#include "d3dx11tex.h"
#include "d3d12.h"
#include "RendererFunctions.h"
#include "assert.h"
#include "WickedEngine.h"
#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"

using namespace wiGraphics;

wiGraphics::FORMAT ConvertDDSFormat( tinyddsloader::DDSFile::DXGIFormat format )
{
	switch (format)
	{
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float: return FORMAT_R32G32B32A32_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt: return FORMAT_R32G32B32A32_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt: return FORMAT_R32G32B32A32_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float: return FORMAT_R32G32B32_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt: return FORMAT_R32G32B32_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt: return FORMAT_R32G32B32_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float: return FORMAT_R16G16B16A16_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm: return FORMAT_R16G16B16A16_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt: return FORMAT_R16G16B16A16_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm: return FORMAT_R16G16B16A16_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt: return FORMAT_R16G16B16A16_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float: return FORMAT_R32G32_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt: return FORMAT_R32G32_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt: return FORMAT_R32G32_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UNorm: return FORMAT_R10G10B10A2_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UInt: return FORMAT_R10G10B10A2_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R11G11B10_Float: return FORMAT_R11G11B10_FLOAT; 
#ifdef GGREDUCED
		case tinyddsloader::DDSFile::DXGIFormat::B8G8R8X8_UNorm: return FORMAT_B8G8R8A8_UNORM; 
#endif
		case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm: return FORMAT_B8G8R8A8_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB: return FORMAT_B8G8R8A8_UNORM_SRGB; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm: return FORMAT_R8G8B8A8_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB: return FORMAT_R8G8B8A8_UNORM_SRGB; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt: return FORMAT_R8G8B8A8_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm: return FORMAT_R8G8B8A8_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt: return FORMAT_R8G8B8A8_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float: return FORMAT_R16G16_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm: return FORMAT_R16G16_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt: return FORMAT_R16G16_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm: return FORMAT_R16G16_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt: return FORMAT_R16G16_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::D32_Float: return FORMAT_D32_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32_Float: return FORMAT_R32_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32_UInt: return FORMAT_R32_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R32_SInt: return FORMAT_R32_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm: return FORMAT_R8G8_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt: return FORMAT_R8G8_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm: return FORMAT_R8G8_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt: return FORMAT_R8G8_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16_Float: return FORMAT_R16_FLOAT; 
		case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm: return FORMAT_D16_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm: return FORMAT_R16_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16_UInt: return FORMAT_R16_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm: return FORMAT_R16_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R16_SInt: return FORMAT_R16_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm: return FORMAT_R8_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8_UInt: return FORMAT_R8_UINT; 
		case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm: return FORMAT_R8_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::R8_SInt: return FORMAT_R8_SINT; 
		case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm: return FORMAT_BC1_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB: return FORMAT_BC1_UNORM_SRGB; 
		case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm: return FORMAT_BC2_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB: return FORMAT_BC2_UNORM_SRGB; 
		case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm: return FORMAT_BC3_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB: return FORMAT_BC3_UNORM_SRGB; 
		case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm: return FORMAT_BC4_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm: return FORMAT_BC4_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm: return FORMAT_BC5_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm: return FORMAT_BC5_SNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm: return FORMAT_BC7_UNORM; 
		case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB: return FORMAT_BC7_UNORM_SRGB; 
		default:
			assert(0); // incoming format is not supported 
			return FORMAT_UNKNOWN;
	}
}

int RendererLoadTexture2D( void* pDevice, const char* filename, void* desc, void** pOutTexture )
{
	wiGraphics::GraphicsDevice* device = wiRenderer::GetDevice();

	wiGraphics::Texture** pTexture = (wiGraphics::Texture**) pOutTexture;
	if ( *pTexture ) delete *pTexture;
	*pTexture = new wiGraphics::Texture();

	FILE *pFile = 0;
	int err = fopen_s( &pFile, filename, "rb" );
	if ( err != 0 ) return -1;

	fseek( pFile, 0, SEEK_END );
	uint32_t size = ftell( pFile );
	fseek( pFile, 0, SEEK_SET );

	unsigned char* fileData = new unsigned char[ size ];
	fread( fileData, 1, size, pFile );
	fclose( pFile );

	uint32_t fileFormat = ((uint32_t*)fileData)[ 0 ];

	//const char* szExt = strrchr( filename, '.' );
	//if ( _stricmp(szExt, ".dds") == 0 )
	if ( fileFormat == ' SDD' ) // DDS
	{
		tinyddsloader::DDSFile dds;
		auto result = dds.Load( fileData, size );
		delete [] fileData;

		if (result != tinyddsloader::Result::Success) return -1;

		TextureDesc desc;
		desc.ArraySize = 1;
		desc.BindFlags = BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.Width = dds.GetWidth();
		desc.Height = dds.GetHeight();
		desc.Depth = dds.GetDepth();
		desc.MipLevels = dds.GetMipCount();
		desc.ArraySize = dds.GetArraySize();
		desc.MiscFlags = 0;
		desc.Usage = USAGE_IMMUTABLE;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		desc.Format = ConvertDDSFormat( dds.GetFormat() );

		std::vector<SubresourceData> InitData;
		for (uint32_t arrayIndex = 0; arrayIndex < desc.ArraySize; ++arrayIndex)
		{
			for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
			{
				auto imageData = dds.GetImageData(mip, arrayIndex);
				SubresourceData subresourceData;
				subresourceData.pSysMem = imageData->m_mem;
				subresourceData.SysMemPitch = imageData->m_memPitch;
				subresourceData.SysMemSlicePitch = imageData->m_memSlicePitch;
				InitData.push_back(subresourceData);
			}
		}

		auto dim = dds.GetTextureDimension();
		switch (dim)
		{
			case tinyddsloader::DDSFile::TextureDimension::Texture1D: desc.type = TextureDesc::TEXTURE_1D; break;
			case tinyddsloader::DDSFile::TextureDimension::Texture2D: desc.type = TextureDesc::TEXTURE_2D; break;
			case tinyddsloader::DDSFile::TextureDimension::Texture3D: desc.type = TextureDesc::TEXTURE_3D;break;
			default: assert(0); break;
		}
		
		device->CreateTexture( &desc, InitData.data(), *pTexture );
		device->SetName( *pTexture, filename );
	}
	else
	{
		int width = 0;
		int height = 0;
		int bpp = 0;
		unsigned char* rgb = stbi_load_from_memory(fileData, (int)size, &width, &height, &bpp, 4);
		delete [] fileData;
		if ( !rgb ) return -1;

// would like mipmaps but can't do it without a CommandList
//#define CAN_GEN_MIPMAPS

		TextureDesc desc;
		desc.Height = uint32_t(height);
		desc.Width = uint32_t(width);
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;

		desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
		desc.CPUAccessFlags = 0;
		desc.Format = FORMAT_R8G8B8A8_UNORM;
		desc.MiscFlags = 0;
		desc.Usage = USAGE_DEFAULT;
		desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;
		
#ifdef CAN_GEN_MIPMAPS
		desc.MipLevels = 0;

		device->CreateTexture(&desc, nullptr, *pTexture);
		device->SetName(*pTexture, filename);

		// fill mip 0 with data
		uint32_t rowStride = width * 4;
		wiRenderer::GetDevice()->UpdateTexture( *pTexture, 0, 0, 0, rgb, rowStride, cmd ); 
		
		// generate other mip levels from mip 0
		wiRenderer::GetDevice()->GenerateMipmaps( *pTexture, cmd );
#else
		// no mipmaps, only the full res image
		desc.MipLevels = 1;

		SubresourceData subresourceData;
		subresourceData.pSysMem = rgb;
		subresourceData.SysMemPitch = width * 4;
		subresourceData.SysMemSlicePitch = 0;

		device->CreateTexture(&desc, &subresourceData, *pTexture);
		device->SetName(*pTexture, filename);
#endif
		
		stbi_image_free( rgb );
	}

	return 0;
}