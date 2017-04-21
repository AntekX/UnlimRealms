
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
};

static const int SampleCount = 4;
static const float2 SampleOfs[SampleCount] = {
	{ 0.0, 0.0 }, { 1.0, 0.0 },
	{ 0.0, 1.0 }, { 1.0, 1.0 }
};

float4 main(PS_INPUT input) : SV_Target
{
	// log average luminance
	float lumAvg = 0.0;
	[unroll] for (int i = 0; i < SampleCount; ++i)
	{
		float2 uv = input.uv + SampleOfs[i] / SrcTargetSize;
		float4 srcVal = texture0.Sample(sampler0, uv);
		float Lp = 0.27 * srcVal.r + 0.67 * srcVal.g + 0.06 * srcVal.b;
		lumAvg += log(Lp);
	}
	lumAvg = exp(lumAvg / SampleCount);
	return lumAvg;
}