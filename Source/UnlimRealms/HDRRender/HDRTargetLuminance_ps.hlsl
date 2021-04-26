///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "../GenericRender/Generic.hlsli"
#include "../ShaderLib/Math.hlsli"
#include "HDRRender.hlsli"

sampler CommonSampler	: register(s0);
Texture2D HDRTexture	: register(t0);

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 hdrVal = HDRTexture.Sample(CommonSampler, input.uv);
	float lum = ComputeLuminance(hdrVal.rgb);
	//lum = max(lum, pow(saturate(lum / LumAdaptationMax), 0.5) * LumAdaptationMax);
	#if (LOG2_LUMINANCE_TEXTURE)
	lum = log2(lum + Eps);
	#endif
	return lum;
}