///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"

sampler CommonSampler	: register(s0);
Texture2D HDRTexture	: register(t0);

float4 main(GenericQuadVertex input) : SV_Target
{
	float2 uv = input.uv;
	float4 hdrVal = HDRTexture.Sample(CommonSampler, uv);
	float Lp = ComputeLuminance(hdrVal.rgb);
	float lum = Lp;
	if (LogLuminance) lum = log(lum + Eps);
	return lum;
}