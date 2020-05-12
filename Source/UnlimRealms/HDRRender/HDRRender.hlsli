///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cbuffer Constants : register(b1)
{
	float2 SrcTargetSize;
	float BlurDirection;
	float LumKey;
	float LumWhite;
	float LumAdaptationMin;
	float LumAdaptationMax;
	float BloomThreshold;
	float BloomIntensity;
};

static const float Gamma = 2.2;
static const float GammaRcp = 1.0 / Gamma;
static const float LumEps = 1.0e-4;
static const bool LogLuminance = false;

float ComputeLuminance(float3 hdrVal)
{
	//return (0.27 * hdrVal.r + 0.67 * hdrVal.g + 0.06 * hdrVal.b);
	return (0.2126 * hdrVal.r + 0.7152 * hdrVal.g + 0.0722 * hdrVal.b);
}

float4 LinearToGamma(float4 linearColor)
{
	return pow(abs(linearColor), GammaRcp);
}

float4 GammaToLinear(float4 gammaColor)
{
	return pow(abs(gammaColor), Gamma);
}