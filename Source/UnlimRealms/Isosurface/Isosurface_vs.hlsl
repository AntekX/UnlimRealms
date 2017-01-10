#include "Isosurface.hlsli"

PS_INPUT main(VS_INPUT input)
{
	PS_INPUT output;

	output.pos = mul(ViewProj, float4(input.pos.xyz, 1.0f));
	output.norm = input.norm;
	output.col = input.col;
	output.wpos = input.pos.xyz;

	return output;
}