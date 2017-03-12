#include "Isosurface.hlsli"

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;

	output.pos = mul(ViewProj, float4(input.pos.xyz, 1.0f));
	output.norm = input.norm;
	output.col = input.col;
	output.wpos.xyz = input.pos.xyz;
	output.wpos.w = output.pos.w;

	return output;
}