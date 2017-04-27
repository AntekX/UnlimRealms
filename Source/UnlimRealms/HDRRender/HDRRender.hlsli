///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// generic quad input
struct GenericQuadVertex
{
	float4 pos	: SV_POSITION;
	float4 col	: COLOR0;
	float2 uv	: TEXCOORD0;
};

cbuffer Constants : register(b1)
{
	float2 SrcTargetSize;
	float LumKey;
	float LumWhite;
	float BlurDirection;
};

static const float Gamma = 2.2;
static const float GammaRcp = 1.0 / Gamma;
static const float BloomLumThreshold = 1.0f;
static const float Eps = 1.0e-6;
static const bool LogLuminance = false;

float ComputeLuminance(float3 hdrVal)
{
	return (0.27 * hdrVal.r + 0.67 * hdrVal.g + 0.06 * hdrVal.b);
}

float4 LinearToGamma(float4 linearColor)
{
	return pow(linearColor, GammaRcp);
}

float4 GammaToLinear(float4 gammaColor)
{
	return pow(gammaColor, Gamma);
}