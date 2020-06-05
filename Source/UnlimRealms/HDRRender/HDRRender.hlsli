///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//	Reference:
//	"Moving Frostbite to Physically Based Rendering" (Sebastien Lagarde, Charles de Rousiers)
//	https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
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

#define LOG2_LUMINANCE_TEXTURE 1
#define SRGB_LINEAR_PRECISE 1

float ComputeLuminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 SRGBToLinearFast(in float3 sRGBCol)
{
	return pow(sRGBCol, 2.2);
}

float3 LinearToSRGBFast(in float3 linearCol)
{
	return pow(linearCol, 1.0 / 2.2);
}

float3 SRGBToLinearPrecise(in float3 sRGBCol)
{
	float3 linearRGBLo = sRGBCol / 12.92;
	float3 linearRGBHi = pow((sRGBCol + 0.055) / 1.055, 2.4);
	float3 linearRGB = (sRGBCol <= 0.04045) ? linearRGBLo : linearRGBHi;
	return linearRGB;
}

float3 LinearToSRGBPrecise(in float3 linearCol)
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	float3 sRGB = (linearCol <= 0.0031308) ? sRGBLo : sRGBHi;
	return sRGB;
}

float3 SRGBToLinear(in float3 sRGBCol)
{
	#if (SRGB_LINEAR_PRECISE)
	return SRGBToLinearPrecise(sRGBCol);
	#else
	return SRGBToLinearFast(sRGBCol);
	#endif
}

float3 LinearToSRGB(in float3 linearCol)
{
	#if (SRGB_LINEAR_PRECISE)
	return LinearToSRGBPrecise(linearCol);
	#else
	return LinearToSRGBFast(linearCol);
	#endif
}

float ComputeEV100(float aperture, float shutterTime, float ISO)
{
	// EV number is defined as:
	// 2^ EV_s = N^2 / t and EV_s = EV_100 + log2 (S /100)
	// This gives
	// EV_s = log2 (N^2 / t)
	// EV_100 + log2 (S /100) = log2 (N^2 / t)
	// EV_100 = log2 (N^2 / t) - log2 (S /100)
	// EV_100 = log2 (N^2 / t . 100 / S)
	return log2((aperture * aperture) / shutterTime * 100 / ISO);
}

float ComputeEV100FromAvgLuminance(float avgLuminance)
{
	// We later use the middle gray at 12.7% in order to have
	// a middle gray at 18% with a sqrt (2) room for specular highlights
	// But here we deal with the spot meter measuring the middle gray
	// which is fixed at 12.5 for matching standard camera
	// constructor settings (i.e. calibration constant K = 12.5)
	// Reference : https://en.wikipedia.org/wiki/Film_speed
	return log2(avgLuminance * 100.0 / 12.5);
}

float ConvertEV100ToExposure(float EV100)
{
	// Compute the maximum luminance possible with H_sbs sensitivity
	// maxLum = 78 / ( S * q ) * N^2 / t
	// = 78 / ( S * q ) * 2^ EV_100
	// = 78 / (100 * 0.65) * 2^ EV_100
	// = 1.2 * 2^ EV
	// Reference : https://en.wikipedia.org/wiki/Film_speed
	float maxLuminance = 1.2 * pow(2.0, EV100);	return 1.0 / maxLuminance;}