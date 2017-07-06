///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "AtmosphericScattering.hlsli"
#include "../GenericRender/Generic.hlsli"

cbuffer Constants : register(b1)
{
	float4x4 CameraViewProj;
	float3 CameraPos;
	AtmosphereDesc Atmosphere;
};

sampler LinearSampler	: register(s0);
Texture2D HDRTexture	: register(t0);

#define LIGHT_SHAFTS 1
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch13.html
float4 LightShaftsScreenSpace(const float2 screenUV)
{
	const int NUM_SAMPLES = 40;
	const float Density = 0.5;
	const float Weight = 1.0;
	const float Decay = 0.85;
	const float Exposure = 0.2;
	const float3 LightDirection = float3(1.0, 0.0, 0.0);
	const float3 LightPos = (-LightDirection) * 1e+5;

	float4 ScreenLightPos = mul(CameraViewProj, float4(LightPos, 1.0));
	ScreenLightPos.xyz /= ScreenLightPos.w;
	ScreenLightPos.xy = (ScreenLightPos.xy + 1.0) * 0.5;
	ScreenLightPos.y = 1.0 - ScreenLightPos.y;
	if (ScreenLightPos.w < 0.0)
		return 0.0;

	float2 offscreen;
	offscreen.x = max(ScreenLightPos.x - 1.0, 0.0 - ScreenLightPos.x);
	offscreen.y = max(ScreenLightPos.y - 1.0, 0.0 - ScreenLightPos.y);
	float intensity = saturate(1.0 - max(offscreen.x, offscreen.y) / 1.0);
	if (intensity <= 0.0)
		return 0.0;

	// Calculate vector from pixel to light source in screen space.
	float2 texCoord = screenUV;
	float2 deltaTexCoord = (texCoord - ScreenLightPos.xy);
	// Divide by number of samples and scale by control factor.
	deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;
	// Set up illumination decay factor.
	float illuminationDecay = 1.0f;
	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
	float4 scatteredLight = 0.0;
	for (int i = 0; i < NUM_SAMPLES; i++)
	{
		// Step sample location along ray.
		texCoord -= deltaTexCoord;
		// Retrieve sample at new location.
		// TEMP: use bloom texture as occlusion map for a test
		float4 hdrVal = HDRTexture.Sample(LinearSampler, texCoord);
		// Apply sample attenuation scale/decay factors.
		hdrVal *= illuminationDecay * Weight;
		// Accumulate combined color.
		scatteredLight += hdrVal;
		// Update exponential decay factor.
		illuminationDecay *= Decay;
	}
	// Output final color with a further scale control factor.
	return scatteredLight * Exposure * intensity;
}

float4 main(GenericQuadVertex input) : SV_TARGET
{
	return LightShaftsScreenSpace(input.uv);
}