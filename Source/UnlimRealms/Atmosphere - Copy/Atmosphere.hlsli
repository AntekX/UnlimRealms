
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//cbuffer AtmosphereConstants
//{
//};
/*
static const int NumSamples = 5;
static const float NumSamplesInv = 1.0 / NumSamples;
static const float3 WavelengthInv = float3(
	1.0 / pow(0.650, 4.0),
	1.0 / pow(0.570, 4.0),
	1.0 / pow(0.475, 4.0));
static const float Pi = 3.14159;

static const float3 LightPos = float3(-1.0, 0.0, 0.0);
static const float OuterRadius = 10.25;
static const float InnerRadius = 10.0;
static const float ScaleDepth = 0.25;
static const float ScaleDepthInv = 1.0 / ScaleDepth;
static const float Scale = 1.0 / (OuterRadius - InnerRadius);
static const float ScaleOverScaleDepth = Scale / ScaleDepth;
static const float G = -0.98;
static const float G2 = 0.9604;
static const float Exposure = 0.5;
static const float Km = 0.0025;
static const float Kr = 0.0015;
static const float ESun = 10.0;
static const float KrESun = Kr * ESun;
static const float KmESun = Km * ESun;
static const float Kr4PI = Kr * 4.0 * Pi;
static const float Km4PI = Km * 4.0 * Pi;

float scale(float fCos)
{
	float x = 1.0 - fCos;
	return ScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float4 atmosphereScattering(const float3 vpos, const float3 cameraPos)
{
	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere) 
	float3 ray = vpos - cameraPos;
	float farDist = length(ray);
	ray /= farDist;

	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)	
	float nearDist = 0.0;
	float cameraHeight = length(cameraPos);
	float distToOuter = cameraHeight - OuterRadius;
	if (distToOuter > 0)
	{
		float B = 2.0 * dot(cameraPos, ray);
		float C = cameraHeight * cameraHeight - OuterRadius * OuterRadius;
		float Det = max(0.0, B*B - 4.0 * C);
		nearDist = 0.5 * (-B - sqrt(Det));
	}

	// Calculate the ray's starting position, then calculate its scattering offset	
	float3 rayStart = cameraPos + ray * nearDist;
	farDist -= nearDist;
	float startAngle = dot(ray, rayStart) / OuterRadius;
	float startDepth = exp(-1.0 / ScaleDepth);
	float startOffset = startDepth * scale(startAngle);

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
		float depth = exp(ScaleOverScaleDepth * (InnerRadius - height));
		float lightAngle = dot(LightPos, samplePoint) / height;
		float cameraAngle = dot(ray, samplePoint) / height;
		float scatter = (startOffset + depth * (scale(lightAngle) - scale(cameraAngle)));
		float3 attenuate = exp(-scatter * (WavelengthInv * Kr4PI + Km4PI));
		frontColor += attenuate * (depth * scaledLength);
		samplePoint += sampleRay;
	}

	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	float3 color = frontColor * (WavelengthInv * KrESun);
	float3 secondaryColor = frontColor * KmESun;
	float3 vecToCamera = cameraPos - vpos;
	float fCos = dot(LightPos, vecToCamera) / length(vecToCamera);
	float RayleighPhase = 0.75 * (1.0 + fCos*fCos);
	float MiePhase = 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + fCos*fCos) / pow(max(0.0, 1.0 + G2 - 2.0*G*fCos), 1.5);

	float4 output;
	output.rgb = 1.0 - exp(-Exposure * (RayleighPhase * color + MiePhase * secondaryColor));
	output.a = output.b;
	//output.rgb = RayleighPhase * color + MiePhase * secondaryColor;
	//output.a = output.b;

	return output;
}

float4 atmosphereScatteringSky(const float3 vpos, const float3 cameraPos)
{
	return atmosphereScattering(vpos, cameraPos);
}

float4 atmosphereScatteringGround(const float3 vpos, const float3 cameraPos)
{
	return atmosphereScattering(vpos, cameraPos);
}
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The number of sample points taken along the ray
static const float Pi = 3.14159;
static const float Exposure = 1.0;
static const int nSamples = 4;
static const float fSamples = 5.0f;
// Mie phase assymetry factor
static const float g = -0.98f;
static const float g2 = 0.9604f;
// Shader Constants
static const float3 v3LightPos = float3(-1, 0, 0);   // The direction vector to the light source
static const float3 v3InvWavelength = float3(1.0f / pow(0.650f, 4), 1.0f / pow(0.570f, 4), 1.0f / pow(0.475f, 4));  // 1 / pow(wavelength, 4) for the red, green, and blue channels
static const float fOuterRadius = 10.25;   // The outer (atmosphere) radius
static const float fOuterRadius2 = fOuterRadius*fOuterRadius;  // fOuterRadius^2
static const float fInnerRadius = 10.00;   // The inner (planetary) radius
static const float fInnerRadius2 = fInnerRadius*fInnerRadius;  // fInnerRadius^2
static const float Km = 0.0025f;
static const float Kr = 0.0015f;
static const float ESun = 10.0f;
static const float fKrESun = Kr * ESun;	// Kr * ESun
static const float fKmESun = Km * ESun;	// Km * ESun
static const float fKr4PI = Kr * 4.0f * Pi;	// Kr * 4 * PI
static const float fKm4PI = Km * 4.0f * Pi;	// Km * 4 * PI
static const float fScaleDepth = 0.25;   // The scale depth (the altitude at which the average atmospheric density is found)
static const float fInvScaleDepth = 1.0f / fScaleDepth;  // 1 / fScaleDepth
static const float fScale = 1.0f / (fOuterRadius - fInnerRadius);	// 1 / (fOuterRadius - fInnerRadius)
static const float fScaleOverScaleDepth = fScale / fScaleDepth; // fScale / fScaleDepth
							// The scale equation calculated by Vernier's Graphical Analysis
float scale(float fCos)
{
	float x = 1.0 - fCos;
	return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
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

float4 atmosphereScatteringGround(float3 v3Pos, float3 v3CameraPos)
{
	float fCameraHeight = length(v3CameraPos);  // The camera's current height
	float fCameraHeight2 = fCameraHeight * fCameraHeight;  // fCameraHeight^2

	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
	float3 v3Ray = v3Pos - v3CameraPos;
	v3Pos = normalize(v3Pos);
	float fFar = length(v3Ray);
	v3Ray /= fFar;
	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
	float fNear = getNearIntersection(v3CameraPos, v3Ray, fCameraHeight2, fOuterRadius2);
	// Calculate the ray's starting position, then calculate its scattering offset
	float3 v3Start = v3CameraPos + v3Ray * fNear;
	fFar -= fNear;
	float fDepth = exp((fInnerRadius - fOuterRadius) * fInvScaleDepth);
	float fCameraAngle = dot(-v3Ray, v3Pos);
	float fLightAngle = dot(v3LightPos, v3Pos);
	float fCameraScale = scale(fCameraAngle);
	float fLightScale = scale(fLightAngle);
	float fCameraOffset = fDepth*fCameraScale;
	float fTemp = (fLightScale + fCameraScale);
	// Initialize the scattering loop variables
	//gl_FrontColor = vec4(0.0, 0.0, 0.0, 0.0);
	float fSampleLength = fFar / fSamples;
	float fScaledLength = fSampleLength * fScale;
	float3 v3SampleRay = v3Ray * fSampleLength;
	float3 v3SamplePoint = v3Start + v3SampleRay * 0.5;
	// Now loop through the sample rays
	float3 v3FrontColor = float3(0.0, 0.0, 0.0);
	float3 v3Attenuate;
	for (int i = 0; i<nSamples; i++)
	{
		float fHeight = length(v3SamplePoint);
		float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
		float fScatter = fDepth*fTemp - fCameraOffset;
		v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
		v3SamplePoint += v3SampleRay;
	}
	float3 c0 = v3FrontColor * (v3InvWavelength * fKrESun + fKmESun);
	float3 c1 = v3Attenuate;
	float3 PlanetColor = float3(0.25, 0.25, 0.25);
	return float4(c0 + PlanetColor * c1, 1);
}

float4 atmosphereScatteringSky(const float3 v3Pos, const float3 v3CameraPos)
{
	float fCameraHeight = length(v3CameraPos);  // The camera's current height
	float fCameraHeight2 = fCameraHeight * fCameraHeight;  // fCameraHeight^2

	//return float4(0, 1, 0, 1);
	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
	float3 v3Ray = v3Pos - v3CameraPos;
	float fFar = length(v3Ray);
	v3Ray /= fFar;
	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)
	float fNear = getNearIntersection(v3CameraPos, v3Ray, fCameraHeight2, fOuterRadius2);
	// Calculate the ray's start and end positions in the atmosphere, then calculate its scattering offset
	float3 v3Start = v3CameraPos + v3Ray * fNear;
	fFar -= fNear;
	float fStartAngle = dot(v3Ray, v3Start) / fOuterRadius;
	float fStartDepth = exp(-fInvScaleDepth);
	float fStartOffset = fStartDepth*scale(fStartAngle);
	// Initialize the scattering loop variables
	float fSampleLength = fFar / fSamples;
	float fScaledLength = fSampleLength * fScale;
	float3 v3SampleRay = v3Ray * fSampleLength;
	float3 v3SamplePoint = v3Start + v3SampleRay * 0.5;
	// Now loop through the sample rays
	float3 v3FrontColor = float3(0.0, 0.0, 0.0);
	for (int i = 0; i<nSamples; i++)
	{
		float fHeight = length(v3SamplePoint);
		float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
		float fLightAngle = dot(v3LightPos, v3SamplePoint) / fHeight;
		float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
		float fScatter = (fStartOffset + fDepth*(scale(fLightAngle) - scale(fCameraAngle)));
		float3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
		v3SamplePoint += v3SampleRay;
	}
	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	float3 c0 = v3FrontColor * (v3InvWavelength * fKrESun);
	float3 c1 = v3FrontColor * fKmESun;
	float3 v3Direction = v3CameraPos - v3Pos;
	float fCos = dot(v3LightPos, v3Direction) / length(v3Direction);
	float fCos2 = fCos*fCos;
	float3 color = getRayleighPhase(fCos2) * c0 + getMiePhase(fCos, fCos2, g, g2) * c1;
	float4 AtmoColor = float4(color, color.b);

	return AtmoColor;
}
