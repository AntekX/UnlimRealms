
static const int NumSamples = 4;
static const float NumSamplesInv = 1.0 / NumSamples;
static const float3 WavelengthInv = float3(
	1.0 / pow(0.650, 4.0),
	1.0 / pow(0.570, 4.0),
	1.0 / pow(0.475, 4.0));
static const float Pi = 3.14159;

static const float3 LightPos = float3(-1.0, 0.0, 0.0);
static const float OuterRadius = 18.5; // OuterRadius must 2.5% bigger then InnerRadius to make computation work properly
static const float InnerRadius = 14.5;
static const float ScaleDepth = 0.25;
static const float ScaleDepthInv = 1.0 / ScaleDepth;
static const float Scale = 1.0 / (OuterRadius - InnerRadius);
static const float ScaleOverScaleDepth = Scale / ScaleDepth;
static const float G = -0.9;
static const float G2 = G*G;// 0.9604;
static const float Exposure = 1.0;
static const float Km = 0.0025;
static const float Kr = 0.0015;
static const float3 ESunLight = float3(1.000, 0.877, 0.731);
static const float3 ESun = ESunLight * 188.0;
static const float3 KrESun = Kr * ESun;
static const float3 KmESun = Km * ESun;
static const float3 Kr4PI = Kr * 4.0 * Pi;
static const float3 Km4PI = Km * 4.0 * Pi;

struct Scattering
{
	float3 color;
	float3 secondaryColor;
};

float scale(float fCos)
{
	float x = 1.0 - fCos;
	return ScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
}

float4 atmosphericScatteringSky(const float3 vpos, const float3 cameraPos)
{
	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere) 
	float3 ray = vpos - cameraPos;
	float farDist = length(ray);
	ray /= farDist;

	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)	
	float cameraHeight = length(cameraPos);
	float B = 2.0 * dot(cameraPos, ray);
	float C = cameraHeight * cameraHeight - OuterRadius * OuterRadius;
	float Det = max(0.0, B*B - 4.0 * C);
	float nearDist = max(0.0, 0.5 * (-B - sqrt(Det)));

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
		//float cameraAngle = dot(ray, samplePoint) / height;
		//float scatter = (startOffset + depth * (scale(lightAngle) - scale(cameraAngle)));
		float scatter = depth * scale(lightAngle); // simplified scatter
		float3 attenuate = exp(-scatter * (WavelengthInv * Kr4PI + Km4PI));
		frontColor += attenuate * (depth * scaledLength);
		samplePoint += sampleRay;
	}

	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
	float3 color = frontColor * (WavelengthInv * KrESun);
	float3 secondaryColor = frontColor * KmESun;

	float4 output = 0.0;
	float3 vecToCamera = cameraPos - vpos;
	float fCos = dot(LightPos, vecToCamera) / length(vecToCamera);
	float RayleighPhase = 0.75 * (1.0 + fCos*fCos);
	float MiePhase = 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + fCos*fCos) / pow(max(0.0, 1.0 + G2 - 2.0*G*fCos), 1.5);
	output.rgb = 1.0 - exp(-Exposure * (RayleighPhase * color + MiePhase * secondaryColor));
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

float4 atmosphericScatteringSurface(float3 vpos, float3 cameraPos, float3 surfLight)
{
	float fSamples = 5.0f;
	float fOuterRadius2 = OuterRadius * OuterRadius;
	float fInvScaleDepth = 1.0f / ScaleDepth;  // 1 / fScaleDepth
	float fCameraHeight = length(cameraPos);  // The camera's current height
	float fCameraHeight2 = fCameraHeight * fCameraHeight;  // fCameraHeight^2

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
	float fDepth = exp((InnerRadius - OuterRadius) * fInvScaleDepth);
	float fLightAngle = dot(LightPos, vpos);
	//fLightAngle = (fLightAngle > 0 ? fLightAngle : lerp(0.0, -0.25, cos(Pi*0.5 + fLightAngle * Pi*0.5))); // negative attenuation wrap
	//float fCameraAngle = dot(-v3Ray, vpos); // simplified: removed camera angle from the equation
	float fCameraScale = 0.0;// scale(fCameraAngle);
	float fLightScale = scale(fLightAngle);
	float fCameraOffset = fDepth * fCameraScale;
	float fTemp = (fLightScale + fCameraScale);
	// Initialize the scattering loop variables
	//float fSampleLength = fFar / fSamples;
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
		float fDepth = exp(ScaleOverScaleDepth * (InnerRadius - fHeight));
		float fScatter = fDepth * fTemp - fCameraOffset;
		v3Attenuate = exp(-fScatter * (WavelengthInv * Kr4PI + Km4PI));
		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
		v3SamplePoint += v3SampleRay;
	}
	float3 c0 = v3FrontColor * (WavelengthInv * KrESun + KmESun);
	float3 c1 = max(ESun * 0.0025, v3Attenuate);
	return float4(1.0 - exp(-Exposure * (c0 + c1 * surfLight)), 1.0);
}