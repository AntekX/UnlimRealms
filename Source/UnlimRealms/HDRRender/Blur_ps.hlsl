///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"
#include "../GenericRender/Generic.hlsli"

sampler CommonSampler	: register(s0);
Texture2D SrcTexture	: register(t0);

static const int KernelSize = 7;
static const float KernelOfs[KernelSize] = { -3, -2, -1, 0, 1, 2, 3 };
//static const float BlurWeights[KernelSize] = { 0.064759, 0.120985, 0.176033, 0.199471, 0.176033, 0.120985, 0.064759 };
//static const float BlurWeights[KernelSize] = { 0.005980, 0.060626, 0.241843, 0.383103, 0.241843, 0.060626, 0.005980 }; // sigma = 1
static const float BlurWeights[KernelSize] = { 0.071303, 0.131514, 0.189879, 0.214607, 0.189879, 0.131514, 0.071303 }; // sigma = 2
//static const float BlurWeights[KernelSize] = { 0.106595, 0.140367, 0.165569, 0.174938, 0.165569, 0.140367, 0.106595 }; // sigma = 3
//static const float BlurWeights[KernelSize] = { 0.133142, 0.142694, 0.148751, 0.150826, 0.148751, 0.142694, 0.133142 }; // sigma = 6

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 val = 0.0;
	[unroll] for (int i = 0; i < KernelSize; ++i)
	{
		float2 uv = input.uv;
		uv.x += KernelOfs[i] / SrcTargetSize.x * (1.0 - BlurDirection);
		uv.y += KernelOfs[i] / SrcTargetSize.y * BlurDirection;
		val += SrcTexture.Sample(CommonSampler, uv) * BlurWeights[i];
	}
	return val;
}