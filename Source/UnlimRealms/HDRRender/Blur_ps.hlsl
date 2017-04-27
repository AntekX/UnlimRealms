///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"

sampler CommonSampler	: register(s0);
Texture2D SrcTexture	: register(t0);

static const int KernelSize = 7;
static const float KernelOfs[KernelSize] = { -3, -2, -1, 0, 1, 2, 3 };
static const float BlurWeights[KernelSize] = { 0.064759, 0.120985, 0.176033, 0.199471, 0.176033, 0.120985, 0.064759 };

float4 main(GenericQuadVertex input) : SV_Target
{
	float val = 0.0;
	[unroll] for (int i = 0; i < KernelSize; ++i)
	{
		float2 uv = input.uv;
		uv.x += KernelOfs[i] / SrcTargetSize.x * float(BlurDirection < 0.5);
		uv.y += KernelOfs[i] / SrcTargetSize.y * float(BlurDirection > 0.5);
		val += SrcTexture.Sample(CommonSampler, uv).x * BlurWeights[i];
	}
	return val;
}