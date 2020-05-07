///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"
#include "../GenericRender/Generic.hlsli"

sampler PointSampler	: register(s0);
sampler LinearSampler	: register(s1);
Texture2D HDRTexture	: register(t0);
Texture2D LumTexture	: register(t1);
Texture2D BloomTexture	: register(t2);


float4 filmicF(float4 x)
{
	float a = 0.22;
	float b = 0.30;
	float c = 0.10;
	float d = 0.20;
	float e = 0.01;
	float f = 0.30;
	return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

float4 filmicACES(float4 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return ((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 finalColor = 0.0;

	float4 hdrVal = HDRTexture.Sample(PointSampler, input.uv);
	float4 lumData = LumTexture.Sample(PointSampler, input.uv);
	float4 bloom = pow(BloomTexture.Sample(LinearSampler, input.uv) * BloomIntensity, 0.66);
	
#if LIGHT_SHAFTS
	// use max() fn to avoid overbloom (light shafts sample same bloom RT)
	hdrVal += max(bloom, LightShaftsScreenSpace(input.uv));
#else
	hdrVal += bloom;
#endif
	
	float Lf = lumData.x + Eps;
	if (LogLuminance) Lf = exp(Lf) / (SrcTargetSize.x * SrcTargetSize.y);
	Lf = clamp(Lf, LumAdaptationMin, LumAdaptationMax);

#if 0

	// Reinhard
	float Lp = ComputeLuminance(hdrVal.rgb);
	float L = LumKey / Lf * Lp ;
	float Lt = L * (1.0 + L / (LumWhite * LumWhite)) / (1.0 + L);
	finalColor = saturate(hdrVal * Lt);

#elif 0

	// Reinhard (RGB to luminance, color, chromacity)
	
	// RGB -> XYZ conversion
	const float3x3 RGB2XYZ = {
		0.5141364, 0.3238786,  0.16036376,
		0.265068,  0.67023428, 0.06409157,
		0.0241188, 0.1228178,  0.84442666
	};
	float3 XYZ = mul(RGB2XYZ, hdrVal.xyz);

	// XYZ -> Yxy conversion
	float3 Yxy;
	Yxy.r = XYZ.g;                 // copy luminance Y
	Yxy.g = XYZ.r / dot(XYZ, 1.0); // x = X / (X + Y + Z)
	Yxy.b = XYZ.g / dot(XYZ, 1.0); // y = Y / (X + Y + Z)
	
	// (Lp) Map average luminance to the middlegrey zone by scaling pixel luminance
	float Lp = Yxy.r * LumKey / Lf;

	// (Ld) Scale all luminance within a displayable range of 0 to 1
	Yxy.r = (Lp * (1.0 + Lp / (LumWhite * LumWhite))) / (1.0 + Lp);

	// Yxy -> XYZ conversion
	XYZ.r = Yxy.r * Yxy.g / Yxy.b;               // X = Y * x / y
	XYZ.g = Yxy.r;                               // copy luminance Y
	XYZ.b = Yxy.r * (1 - Yxy.g - Yxy.b) / Yxy.b; // Z = Y * (1-x-y) / y
												 
	// XYZ -> RGB conversion
	const float3x3 XYZ2RGB = {
		2.5651, -1.1665,-0.3986,
		-1.0217, 1.9777, 0.0439,
		0.0753, -0.2543, 1.1892
	};
	finalColor.xyz = mul(XYZ2RGB, XYZ);

#elif 1

	// ACES filmic
	float4 L = hdrVal * LumKey / Lf;
	finalColor = filmicACES(L) / filmicACES(LumWhite);

#elif 0

	// Filmic Uncharted2
	float4 L = hdrVal * LumKey / Lf;
	finalColor = filmicF(L) / filmicF(LumWhite);

#elif 1

	// Exponential
	float T = LumKey / Lf;
	finalColor = saturate(1.0 - exp(-T * hdrVal));

#elif 1

	// Simple
	float4 L = hdrVal * LumKey / Lf;
	finalColor = L / (L + 1.0);

#endif

	finalColor = LinearToGamma(finalColor);

	return float4(finalColor.xyz, hdrVal.w);
}