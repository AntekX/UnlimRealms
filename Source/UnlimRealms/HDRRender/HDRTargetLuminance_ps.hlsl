///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"
#include "../GenericRender/Generic.hlsli"

sampler CommonSampler	: register(s0);
Texture2D HDRTexture	: register(t0);

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 hdrVal = HDRTexture.Sample(CommonSampler, input.uv);
	float lum = ComputeLuminance(hdrVal.rgb);
	if (LogLuminance) lum = log(lum + LumEps);
	return lum;
}