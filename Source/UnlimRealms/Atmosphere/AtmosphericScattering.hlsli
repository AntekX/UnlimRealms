///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//  Resources:
//	http://publications.lib.chalmers.se/records/fulltext/203057/203057.pdf
//	http://www.vis.uni-stuttgart.de/~schafhts/HomePage/pubs/wscg07-schafhitzel.pdf
//  http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Math.hlsli"

// Configurable parameters
struct AtmosphereDesc
{
	float InnerRadius;
	float OuterRadius;
	float ScaleDepth;
	float G;
	float Km;
	float Kr;
	float D;
};

// todo: make sun light params configurable
static const float3 LightDirection = float3(1.0, 0.0, 0.0);
static const float3 LightIntensity = float3(1.0, 1.0, 1.0) * 200.0;
static const float3 LightWaveLength = float3(0.650, 0.570, 0.475);
static const float3 LightWaveLengthScatterConst = 1.0 / pow(LightWaveLength, 4.0); // precomputed constant from wavelength
static const int    IntergrationSteps = 8;

// Earth related physically based parameters for reference
static const float  EarthRadius = 6371.0e+3;
static const float  EarthAtmosphereHeight = 160.0e+3;
static const float  EarthHeightRayleigh = 8.0e+3;
static const float  EarthHeightMie = 1.2e+3;
static const float3 EarthScatterRayleigh = float3(6.55e-6, 1.73e-5, 2.30e-5); // precomputed scattering coefficient
static const float3 EarthScatterMie = float3(2.0e-6, 2.0e-6, 2.0e-6); // precomputed scattering coefficient
static const float3 ScatterRayleigh = EarthScatterRayleigh;
static const float3 ScatterMie = EarthScatterMie;
static const float3 ExtinctionRayleigh = ScatterRayleigh;
static const float3 ExtinctionMie = ScatterMie / 0.9;


float ScaledHeight(const AtmosphereDesc a, const float3 p)
{
	return saturate((length(p) - a.InnerRadius) / (a.OuterRadius - a.InnerRadius));
}

float DensityRayleigh(const AtmosphereDesc a, float h)
{
	return pow(max(0.0, a.D), -h / a.ScaleDepth);
}

float DensityMie(const AtmosphereDesc a, float h)
{
	return pow(max(0.0, a.D), -h / a.ScaleDepth);
}

float PhaseRayleigh(const AtmosphereDesc a, const float dirToCameraCos)
{
	return 0.75 * (1.0 + dirToCameraCos * dirToCameraCos);
}

float PhaseMie(const AtmosphereDesc a, const float dirToCameraCos)
{
	const float C = dirToCameraCos;
	const float C2 = C * C;
	float G2 = a.G * a.G;
	return 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + C2) / pow(max(0.0, 1.0 + G2 - 2.0 * a.G * C), 1.5);
}

// Transmittance (Attenuation / Out-Scattering)
float3 AtmosphericTransmittance(const AtmosphereDesc a, const float3 Pa, const float3 Pb)
{
	float3 dir = Pb - Pa;
	float dist = length(dir);
	dir /= dist;
	float stepSize = dist / IntergrationSteps;
	float3 stepVec = dir * stepSize;
	float totalDensityMie = 0.0;
	float totalDensityRayleigh = 0.0;
	float3 P = Pa + stepVec * 0.5;
	[unroll] for (int step = 0; step < IntergrationSteps; ++step)
	{
		float h = ScaledHeight(a, P);
		float crntDensityMie = DensityMie(a, h);
		float crntDensityRayleigh = DensityRayleigh(a, h);
		totalDensityMie += crntDensityMie * stepSize;
		totalDensityRayleigh += crntDensityRayleigh * stepSize;
		P += stepVec;
	}
	//float3 transmittance = exp(-(totalDensityRayleigh * ExtinctionRayleigh + totalDensityMie * ExtinctionMie));
	float3 transmittance = exp(-(totalDensityRayleigh * a.Kr * FourPi * LightWaveLengthScatterConst + totalDensityMie * a.Km * FourPi));

	return transmittance;
}

float3 AtmosphericSingleScattering(const AtmosphereDesc a, const float3 vpos, const float3 cameraPos)
{
	float3 totalInscatteringMie = 0.0;
	float3 totalInscatteringRayleigh = 0.0;

	float3 aPos = float3(0.0, 0.0, 0.0); // vpos & cameraPos are expected to be in atmosphere local coords
	float3 dir = normalize(vpos - cameraPos);
	float2 Ad = IntersectSphere(cameraPos, dir, aPos, a.OuterRadius);
	float3 Pa = cameraPos + dir * Ad.x;
	float3 Pb = cameraPos + dir * Ad.y;

	float dist = Ad.y - Ad.x;
	float stepSize = dist / IntergrationSteps;
	float3 stepVec = dir * stepSize;
	float3 P = Pa + stepVec * 0.5;
	[unroll] for (int step = 0; step < IntergrationSteps; ++step)
	{
		float3 Pc = P - IntersectSphere(P, -LightDirection, aPos, a.OuterRadius).y * LightDirection;
		float3 transmittance = AtmosphericTransmittance(a, Pa, P) * AtmosphericTransmittance(a, P, Pc);
		float h = ScaledHeight(a, P);
		float3 crntInscatteringMie = DensityMie(a, h) * transmittance;
		float3 crntInscatteringRayleigh = DensityRayleigh(a, h) * transmittance;
		totalInscatteringMie += crntInscatteringMie * stepSize;
		totalInscatteringRayleigh += crntInscatteringRayleigh * stepSize;
		P += stepVec;
	}

	float dirToCameraCos = dot(dir, LightDirection);
	totalInscatteringMie *= PhaseMie(a, dirToCameraCos) * a.Km;//ScatterMie / (Pi * 4.0);
	totalInscatteringRayleigh *= PhaseRayleigh(a, dirToCameraCos) * a.Kr * LightWaveLengthScatterConst;//ScatterRayleigh / (Pi * 4.0);
	
	return LightIntensity * (totalInscatteringMie + totalInscatteringRayleigh);
}

float4 AtmosphericScatteringSky(const AtmosphereDesc a, const float3 vpos, const float3 cameraPos)
{
	float3 light = AtmosphericSingleScattering(a, vpos, cameraPos);
	float alpha = max(max(light.r, light.g), light.b);
	return float4(light.rgb, min(alpha, 1.0));
}

float4 AtmosphericScatteringSurface(const AtmosphereDesc a, float3 surfLight, float3 vpos, float3 cameraPos)
{
	float3 totalInscatteringRayleigh = 0.0;

	float3 aPos = float3(0.0, 0.0, 0.0); // vpos & cameraPos are expected to be in atmosphere local coords
	float3 dir = normalize(vpos - cameraPos);
	float2 Ad = IntersectSphere(cameraPos, dir, aPos, a.OuterRadius);
	float3 Pa = cameraPos + dir * Ad.x;
	float3 Pb = vpos;

	float dist = length(Pb - Pa);
	float stepSize = dist / IntergrationSteps;
	float3 stepVec = dir * stepSize;
	float3 transmittance = 0.0;
	float3 P = Pa + stepVec * 0.5;
	[unroll] for (int step = 0; step < IntergrationSteps; ++step)
	{
		float3 Pc = P - IntersectSphere(P, -LightDirection, aPos, a.OuterRadius).y * LightDirection;
		transmittance = AtmosphericTransmittance(a, Pa, P) * AtmosphericTransmittance(a, P, Pc);
		float h = ScaledHeight(a, P);
		float3 crntInscatteringRayleigh = DensityRayleigh(a, h) * transmittance;
		totalInscatteringRayleigh += crntInscatteringRayleigh * stepSize;
		P += stepVec;
	}

	// real light intensity leads to super dense (foggy) atmospere look due to the planetoid small size
	// multiplying it here by a factor to get more pleasant surface rendering
	float3 lightIntensity = LightIntensity * 0.15;
	float3 scatteredLight = lightIntensity * totalInscatteringRayleigh * a.Kr * LightWaveLengthScatterConst;
	float3 light = scatteredLight + surfLight * transmittance;

	return float4(light.rgb, 1.0);
}