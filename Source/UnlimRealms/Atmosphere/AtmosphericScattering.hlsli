///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//  Based on
//  Sean O'Neil Accurate Atmospheric Scattering
//  http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LIGHT_WRAP 1
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

struct AtmosphereDesc
{
	float InnerRadius;
	float OuterRadius;
	float ScaleDepth;
	float G;
	float Km;
	float Kr;
};


float scale(float scaleDepth, float fCos)
{
	float x = 1.0 - fCos;
	return scaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float4 atmosphericScatteringSky(const AtmosphereDesc a, const float3 vpos, const float3 cameraPos)
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
	output.a = max(max(output.r, output.r), output.b);

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
	//return 1.0;
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

float4 atmosphericScatteringSurface(const AtmosphereDesc a, float3 surfLight, float3 vpos, float3 cameraPos)
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