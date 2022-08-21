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
	float texelSize = 1.0f / 512.0f;
	float4 out_col = texture0.Sample(sampler0, input.uv); 
	out_col += texture0.Sample(sampler0, input.uv + float2(0.0f,texelSize) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,texelSize) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,0.0f) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(0.0f,-texelSize) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(-texelSize,-texelSize) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(-texelSize,0.0f) ); 
	out_col += texture0.Sample(sampler0, input.uv + float2(texelSize,-texelSize) ); 
	out_col *= 0.125f;
	out_col = input.col * out_col; 

	return out_col; 
}