///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"
#include "../GenericRender/Generic.hlsli"

sampler CommonSampler	: register(s0);
Texture2D LumTexture	: register(t0);

static const int SampleCount = 4;
static const float2 SampleOfs[SampleCount] = {
	{-0.5,-0.5 }, { 0.5,-0.5 },
	{-0.5, 0.5 }, { 0.5, 0.5 }
};

float4 main(GenericQuadVertex input) : SV_Target
{
	float lumAvg = 0.0;
	[unroll] for (int i = 0; i < SampleCount; ++i)
	{
		float2 uv = input.uv + SampleOfs[i] / SrcTargetSize;
		float lumVal = LumTexture.Sample(CommonSampler, uv).x;
		lumAvg += lumVal;
	}
	if (!LogLuminance) lumAvg = lumAvg / SampleCount;
	return lumAvg;
}