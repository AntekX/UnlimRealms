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
	{ 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 }, { 0.0, 1.0 }
};

float4 main(GenericQuadVertex input) : SV_Target
{
	float lumAvg = 0.0;
	float2 samplePos = input.uv * (SrcTargetSize - 1.0);
	[unroll] for (int i = 0; i < SampleCount; ++i)
	{
		int2 samplePosCrnt = samplePos + SampleOfs[i];
		float lumVal = LumTexture.Load(int3(samplePosCrnt.xy, 0)).x;
		lumAvg += lumVal;
	}
	lumAvg = lumAvg / SampleCount;
	return lumAvg;
}