///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//  Based on "Adaptive Temporal Tone Mapping"
//	by ShaunDavidRamsey, J.ThomasJohnsonIII, CharlesHansen 
//  http://www.sci.utah.edu/~tjohnson/rp/attm.pdf
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PS_INPUT
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};
sampler sampler0	: register(s0);
Texture2D texture0	: register(t0);

cbuffer Constants : register(b1)
{
	float2 SrcTargetSize;
	float LumKey;
	float LumWhite;
};

static const int SampleCount = 4;
static const float2 SampleOfs[SampleCount] = {
	{ 0.0, 0.0 }, { 1.0, 0.0 },
	{ 0.0, 1.0 }, { 1.0, 1.0 }
};

float4 main(PS_INPUT input) : SV_Target
{
	float lumAvg = 0.0;
	[unroll] for (int i = 0; i < SampleCount; ++i)
	{
		float2 uv = input.uv + SampleOfs[i] / SrcTargetSize;
		float4 hdrVal = texture0.Sample(sampler0, uv);
		float Lp = 0.27 * hdrVal.r + 0.67 * hdrVal.g + 0.06 * hdrVal.b;
		lumAvg += Lp;
	}
	lumAvg = lumAvg / SampleCount;
	return lumAvg;
}