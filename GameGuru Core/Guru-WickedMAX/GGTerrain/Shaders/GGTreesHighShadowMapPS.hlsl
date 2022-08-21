Texture2D<float> texNoise : register( t51 );

SamplerState samplerBilinearWrap : register( s0 );
SamplerState samplerTrilinearWrap : register( s2 );

#include "GGTreesConstants.hlsli"

struct PixelIn
{
    float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float2 uv : TEXCOORD1;
	uint data : TEXCOORD2;
};

float4 main( PixelIn IN ) : SV_TARGET
{
	uint treeType = GetTreeType( IN.data );
	uint index = GetTreeVariation( IN.data );

	float3 diff = tree_playerPos - IN.worldPos;
	float sqrDist = dot( diff, diff );

	float noise = texNoise.Sample( samplerBilinearWrap, IN.uv*3 );
	float limit = (noise * GGTREES_LOD_SHADOW_TRANSITION) + tree_lodDistShadow + GGTREES_LOD_SHADOW_TRANSITION;
	if( sqrDist > limit*limit ) discard;

	return float4( 1, 1, 1, 1 );
}