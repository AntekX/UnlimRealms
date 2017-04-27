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
	float4 hdrVal = HDRTexture.Sample(CommonSampler, input.uv);
	float Lp = ComputeLuminance(hdrVal.rgb);
	float bloom = max(0.0, Lp - BloomLumThreshold);
	return bloom;
}