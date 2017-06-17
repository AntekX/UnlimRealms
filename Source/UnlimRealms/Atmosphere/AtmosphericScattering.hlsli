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


//------------------------------------------------------------------

#define LIGHT_WRAP 0
#define APPLY_EXPOSURE 0
static const int NumSamples = 4;
static const float NumSamplesInv = 1.0 / NumSamples;
static const float Pi = 3.14159;
static const float SunIntensity = 100.0;
static const float3 SunLight = float3(1.0, 1.0, 1.0);
static const float3 SunDirection = float3(1.0, 0.0, 0.0);
static const float3 WaveLength = float3(0.650, 0.570, 0.475);
static const float3 WaveLengthInv = 1.0 / pow(WaveLength, 4.0);
static const float Exposure = 1.0;

float scale(float scaleDepth, float fCos)
{
	float x = 1.0 - fCos;
	return scaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float4 __atmosphericScatteringSky(const AtmosphereDesc a, const float3 vpos, const float3 cameraPos)
{
	// precomputed derivatives
	const float ScaleDepthInv = 1.0f / a.ScaleDepth;
	const float Scale = 1.0f / (a.OuterRadius - a.InnerRadius);
	const float ScaleOverScaleDepth = Scale / a.ScaleDepth;
	const float G2 = a.G * a.G;
	const float Kr4PI = a.Kr * Pi * 4.0f;
	const float Km4PI = a.Km * Pi * 4.0f;
	const float3 ESun = SunIntensity * SunLight / (dot(SunLight, 1.0) / 3.0);
	const float3 KrESun = a.Kr * ESun;
	const float3 KmESun = a.Km * ESun;

	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere) 
	float3 ray = vpos - cameraPos;
	float farDist = length(ray);
	ray /= farDist;

	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)	
	float cameraHeight = length(cameraPos);
	float B = 2.0 * dot(cameraPos, ray);
	float C = cameraHeight * cameraHeight - a.OuterRadius * a.OuterRadius;
	float Det = max(0.0, B*B - 4.0 * C);
	float nearDist = max(0.0, 0.5 * (-B - sqrt(Det)));

	// Calculate the ray's starting position, then calculate its scattering offset	
	float3 rayStart = cameraPos + ray * nearDist;
	farDist -= nearDist;
	float startAngle = dot(ray, rayStart) / a.OuterRadius;
	float startDepth = exp(-1.0 / a.ScaleDepth);
	float startOffset = startDepth * scale(a.ScaleDepth, startAngle);

	// Initialize the scattering loop variables		
	float sampleLength = farDist * NumSamplesInv;
	float scaledLength = sampleLength * Scale;
	float3 sampleRay = ray * sampleLength;
	float3 samplePoint = rayStart + sampleRay * 0.5;

	// Now loop through the sample rays	
	float3 frontColor = float3(0.0, 0.0, 0.0);
	[unroll] for (int i = 0; i < NumSamples; ++i)
	{
		float height = length(samplePoint);
		float depth = exp(ScaleOverScaleDepth * (a.InnerRadius - height));
		#if LIGHT_WRAP
		float lightWrap = 2.0;
		float lightAngle = ((dot(-SunDirection, samplePoint) + lightWrap) / (1.0 + lightWrap)) / height;
		#else
		float lightAngle = dot(-SunDirection, samplePoint) / height;
		#endif
		//float cameraAngle = dot(ray, samplePoint) / height;
		//float scatter = (startOffset + depth * (scale(lightAngle) - scale(cameraAngle)));
		float scatter = depth * scale(a.ScaleDepth, lightAngle); // simplified scatter
		float3 attenuate = exp(-scatter * (WaveLengthInv * Kr4PI + Km4PI));
		frontColor += attenuate * (depth * scaledLength);
		samplePoint += sampleRay;
	}

	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	float3 color = frontColor * (WaveLengthInv * KrESun);
	float3 secondaryColor = frontColor * KmESun;

	float4 output = 0.0;
	float3 vecToCamera = cameraPos - vpos;
	float fCos = dot(-SunDirection, vecToCamera) / length(vecToCamera);
	float RayleighPhase = 0.75 * (1.0 + fCos*fCos);
	float MiePhase = 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + fCos*fCos) / pow(max(0.0, 1.0 + G2 - 2.0*a.G*fCos), 1.5);
	output.rgb = (RayleighPhase * color + MiePhase * secondaryColor);
	#if APPLY_EXPOSURE
	output.rgb = 1.0 - exp(-Exposure * output.rgb);
	#endif
	//output.a = output.b;
	output.a = max(max(output.r, output.g), output.b);

	return output;
}

// Calculates the Mie phase function
float getMiePhase(float fCos, float fCos2, float g, float g2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos2) / pow(abs(1.0 + g2 - 2.0*g*fCos), 1.5);
}
// Calculates the Rayleigh phase function
float getRayleighPhase(float fCos2)
{
	return 0.75 + 0.75*fCos2;
}
// Returns the near intersection point of a line and a sphere
float getNearIntersection(float3 v3Pos, float3 v3Ray, float fDistance2, float fRadius2)
{
	float B = 2.0 * dot(v3Pos, v3Ray);
	float C = fDistance2 - fRadius2;
	float fDet = max(0.0, B*B - 4.0 * C);
	return 0.5 * (-B - sqrt(fDet));
}
// Returns the far intersection point of a line and a sphere
float getFarIntersection(float3 v3Pos, float3 v3Ray, float fDistance2, float fRadius2)
{
	float B = 2.0 * dot(v3Pos, v3Ray);
	float C = fDistance2 - fRadius2;
	float fDet = max(0.0, B*B - 4.0 * C);
	return 0.5 * (-B + sqrt(fDet));
}

float4 __atmosphericScatteringSurface(const AtmosphereDesc a, float3 surfLight, float3 vpos, float3 cameraPos)
{
	// precomputed derivatives
	const float ScaleDepthInv = 1.0f / a.ScaleDepth;
	const float Scale = 1.0f / (a.OuterRadius - a.InnerRadius);
	const float ScaleOverScaleDepth = Scale / a.ScaleDepth;
	const float G2 = a.G * a.G;
	const float Kr4PI = a.Kr * Pi * 4.0f;
	const float Km4PI = a.Km * Pi * 4.0f;
	const float3 ESun = SunIntensity * SunLight / (dot(SunLight, 1.0) / 3.0);
	const float3 KrESun = a.Kr * ESun;
	const float3 KmESun = a.Km * ESun;
	const float fOuterRadius2 = a.OuterRadius * a.OuterRadius;
	const float fCameraHeight = length(cameraPos);
	const float fCameraHeight2 = fCameraHeight * fCameraHeight;

	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
	float3 v3Ray = vpos - cameraPos;
	vpos = normalize(vpos);
	float fFar = length(v3Ray);
	v3Ray /= fFar;
	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
	float fNear = max(0.0, getNearIntersection(cameraPos, v3Ray, fCameraHeight2, fOuterRadius2));
	// Calculate the ray's starting position, then calculate its scattering offset
	float3 v3Start = cameraPos + v3Ray * fNear;
	fFar -= fNear;
	float fDepth = exp((a.InnerRadius - a.OuterRadius) * ScaleDepthInv);
	float fLightAngle = dot(-SunDirection, vpos);
	#if LIGHT_WRAP
	fLightAngle = (fLightAngle > 0 ? fLightAngle : lerp(0.0, -0.25, cos(Pi*0.5 + fLightAngle * Pi*0.5))); // negative attenuation wrap
	#endif
	//float fCameraAngle = dot(-v3Ray, vpos); // simplified: removed camera angle from the equation
	float fCameraScale = 0.0;// scale(fCameraAngle);
	float fLightScale = scale(a.ScaleDepth, fLightAngle);
	float fCameraOffset = fDepth * fCameraScale;
	float fTemp = (fLightScale + fCameraScale);
	// Initialize the scattering loop variables
	float fSampleLength = fFar * NumSamplesInv;
	float fScaledLength = fSampleLength * Scale;
	float3 v3SampleRay = v3Ray * fSampleLength;
	float3 v3SamplePoint = v3Start + v3SampleRay * 0.5;
	// Now loop through the sample rays
	float3 v3FrontColor = float3(0.0, 0.0, 0.0);
	float3 v3Attenuate;
	for (int i = 0; i<NumSamples; i++)
	{
		float fHeight = length(v3SamplePoint);
		float fDepth = exp(ScaleOverScaleDepth * (a.InnerRadius - fHeight));
		float fScatter = fDepth * fTemp - fCameraOffset;
		v3Attenuate = exp(-fScatter * (WaveLengthInv * Kr4PI + Km4PI));
		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
		v3SamplePoint += v3SampleRay;
	}
	float3 c0 = v3FrontColor * (WaveLengthInv * KrESun + KmESun);
	float3 c1 = max(ESun * 0.0025, v3Attenuate);
	float3 c = (c0 + c1 * surfLight);
	#if APPLY_EXPOSURE
	c = 1.0 - exp(-Exposure * c);
	#endif
	return float4(c, 1.0);
}


//-----------------------------------

static const int IntergrationSteps = 5;
static const float EarthRadius = 6371.0e+3;
static const float EarthAtmosphereHeight = 160.0e+3;
static const float EarthHeightRayleigh = 8.0e+3;
static const float EarthHeightMie = 1.2e+3;
static const float3 EarthScatterRayleigh = float3(6.55e-6, 1.73e-5, 2.30e-5); // precomputed scattering coefficient
static const float3 EarthScatterMie = float3(2.0e-6, 2.0e-6, 2.0e-6); // precomputed scattering coefficient
static const float EarthScatterScale = 1e+3; // must be applied to real scattering params to immitate Earth atmosphere on a small scale planetoid
static const float3 LightWaveLength = float3(0.650, 0.570, 0.475);
static const float3 ScatterLightWaveLength = 1.0 / pow(LightWaveLength, 4.0); // precomputed constant from wavelength
static const float3 ScatterRayleigh = EarthScatterRayleigh;
static const float3 ScatterMie = EarthScatterMie;
static const float3 ExtinctionRayleigh = ScatterRayleigh;
static const float3 ExtinctionMie = ScatterMie / 0.9;
static const float3 LightIntensity = float3(1.0, 1.0, 1.0) * 200.0;
static const float HeightScaleRayleigh = 0.25;
static const float HeightScaleMie = HeightScaleRayleigh * 0.15;

float2 IntersectSphere(const float3 rayOrigin, const float3 rayDir, const float3 sphereCenter, const float sphereRadius)
{
	float3 L = sphereCenter - rayOrigin;
	float L2 = dot(L, L);
	float sphereRadius2 = sphereRadius * sphereRadius;
	//[flatten] if (L2 <= sphereRadius2) near = 0.0; // inside sphere, near point = rayOrigin

	float tc = dot(L, rayDir);
	//[flatten] if (tc < 0.0) return -1.0; // we are not inside and sphere center is behind - no intersection

	float d2 = L2 - tc * tc;
	[flatten] if (d2 > sphereRadius2) return -1.0; // ray misses the sphere

	float tcd = sqrt(sphereRadius2 - d2);
	float d0 = (tc - tcd);
	float d1 = (tc + tcd);
	float near = max(0.0, min(d0, d1));
	float far = max(d0, d1);

	return float2(near, far);
}

float ScaledHeight(const AtmosphereDesc a, const float3 p)
{
	return saturate((length(p) - a.InnerRadius) / (a.OuterRadius - a.InnerRadius));
}

float DensityRayleigh(const AtmosphereDesc a, float h)
{
	//return exp(-h / /*HeightScaleRayleigh*/a.ScaleDepth);
	return pow(max(0.0, a.D), -h / a.ScaleDepth);
}

float DensityMie(const AtmosphereDesc a, float h)
{
	//return exp(-h / /*HeightScaleMie*/a.ScaleDepth);
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
	float3 transmittance = exp(-(totalDensityRayleigh * a.Kr*4.0*Pi*ScatterLightWaveLength + totalDensityMie*a.Km*4.0*Pi));

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
		float3 Pc = P - IntersectSphere(P, -SunDirection, aPos, a.OuterRadius).y * SunDirection;
		float3 transmittance = AtmosphericTransmittance(a, Pa, P) * AtmosphericTransmittance(a, P, Pc);
		float h = ScaledHeight(a, P);
		float3 crntInscatteringMie = DensityMie(a, h) * transmittance;
		float3 crntInscatteringRayleigh = DensityRayleigh(a, h) * transmittance;
		totalInscatteringMie += crntInscatteringMie * stepSize;
		totalInscatteringRayleigh += crntInscatteringRayleigh * stepSize;
		P += stepVec;
	}

	float dirToCameraCos = dot(dir, SunDirection);
	totalInscatteringMie *= PhaseMie(a, dirToCameraCos) * a.Km;//ScatterMie / (Pi * 4.0);
	totalInscatteringRayleigh *= PhaseRayleigh(a, dirToCameraCos) * a.Kr * ScatterLightWaveLength;//ScatterRayleigh / (Pi * 4.0);
	
	return LightIntensity * (totalInscatteringMie + totalInscatteringRayleigh);
}

float4 atmosphericScatteringSky(const AtmosphereDesc a, const float3 vpos, const float3 cameraPos)
{
	float3 light = AtmosphericSingleScattering(a, vpos, cameraPos);
	float alpha = max(max(light.r, light.g), light.b);
	return float4(light.rgb, min(alpha, 1.0));
}

float4 atmosphericScatteringSurface(const AtmosphereDesc a, float3 surfLight, float3 vpos, float3 cameraPos)
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
		float3 Pc = P - IntersectSphere(P, -SunDirection, aPos, a.OuterRadius).y * SunDirection;
		transmittance = AtmosphericTransmittance(a, Pa, P) * AtmosphericTransmittance(a, P, Pc);
		float h = ScaledHeight(a, P);
		float3 crntInscatteringRayleigh = DensityRayleigh(a, h) * transmittance;
		totalInscatteringRayleigh += crntInscatteringRayleigh * stepSize;
		P += stepVec;
	}

	// real light intensity leads to super dense (foggy) atmospere look due to the planetoid small size
	// multiplying it here by a factor to get more pleasant surface rendering
	float3 lightIntensity = LightIntensity * 0.15;
	float3 scatteredLight = lightIntensity * totalInscatteringRayleigh * a.Kr * ScatterLightWaveLength;
	float3 light = scatteredLight + surfLight * transmittance;

	return float4(light.rgb, 1.0);
}