///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "HDRRender.hlsli"

sampler PointSampler	: register(s0);
sampler LinearSampler	: register(s1);
Texture2D HDRTexture	: register(t0);
Texture2D LumTexture	: register(t1);
Texture2D BloomTexture	: register(t2);

float4 main(GenericQuadVertex input) : SV_Target
{
	float4 finalColor = 0.0;

	float4 hdrVal = HDRTexture.Sample(PointSampler, input.uv);
	float4 lumData = LumTexture.Sample(PointSampler, float2(0.0, 0.0));
	float4 bloom = BloomTexture.Sample(LinearSampler, input.uv);
	
	float Lf = (lumData.x + Eps);
	if (LogLuminance) Lf = exp(Lf);
	Lf = max(LumAdaptationMin, Lf);

#if 0

	// Reinhard
	hdrVal += bloom;
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
	float3 XYZ = mul(RGB2XYZ, hdrVal.xyz + bloom.xyz);

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

#elif 0

	// ACES filmic
	float4 x = hdrVal + bloom;
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	finalColor = saturate((x * (a * x + b)) / (x * (c * x + d) + e));

#elif 1

	// Exponential
	//float T = pow(Lf, -1);
	float T = LumKey / Lf;
	finalColor = saturate(1.0 - exp(-T * (hdrVal + bloom)));

#elif 1

	// Simple
	finalColor = (hdrVal + bloom) / (hdrVal + 1.0);

#endif

	//finalColor = saturate(finalColor + bloom);
	finalColor = LinearToGamma(finalColor);
	
	return finalColor;
}