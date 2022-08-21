 struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR0;
	float2 uv  : TEXCOORD0;
};

Texture2D<float4> texture0 : register( t0 );
SamplerState sampler0 : register( s0 );

float4 main(PS_INPUT input) : SV_Target
{
	float4 out_col = input.col * texture0.Sample(sampler0, input.uv); 
	out_col.w = 1.0; 
	return out_col; 
}