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
Texture2D hdrTexture	: register(t0);
Texture2D avgLumTexture	: register(t1);

cbuffer Constants : register(b1)
{
	float2 SrcTargetSize;
	float LumScale;
	float WhitePoint;
};

float4 main(PS_INPUT input) : SV_Target
{
	static const float Gamma = 2.2;
	static const float GammaRcp = 1.0 / Gamma;
	
	float4 hdrVal = hdrTexture.Sample(sampler0, input.uv);
	float frameLum = exp(log((avgLumTexture.Sample(sampler0, float2(0.0, 0.0)).x)));
	float Lp = 0.27 * hdrVal.r + 0.67 * hdrVal.g + 0.06 * hdrVal.b;
	float L = LumScale * Lp / frameLum;
	float Lt = L * (1.0 + L / (WhitePoint * WhitePoint)) / (1.0 + L);
	float4 ldrVal = saturate(hdrVal * Lt);
	float4 finalColor = pow(ldrVal, GammaRcp);
	
	return finalColor;
}