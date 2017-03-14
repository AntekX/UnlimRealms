
// SkyFromSpaceVert.glsl 
/*
attribute vec3 vertex;

uniform mat4 viewMatrix;

uniform mat4 projectionMatrix;

uniform vec3 v3CameraPos;		// The camera's current position	

uniform vec3 v3LightPos;		// The direction vector to the light source	

uniform vec3 v3InvWavelength;	// 1 / pow(wavelength, 4) for the red, green, and blue channels	

uniform float fCameraHeight2;	// fCameraHeight^2	

uniform float fOuterRadius;		// The outer (atmosphere) radius	

uniform float fOuterRadius2;	// fOuterRadius^2	

uniform float fInnerRadius;		// The inner (planetary) radius	

uniform float fInnerRadius2;	// fInnerRadius^2	

uniform float fKrESun;			// Kr * ESun	

uniform float fKmESun;			// Km * ESun	

uniform float fKr4PI;			// Kr * 4 * PI	

uniform float fKm4PI;			// Km * 4 * PI	

uniform float fScale;			// 1 / (fOuterRadius - fInnerRadius)	

uniform float fScaleDepth;		// The scale depth (i.e. the altitude at which the atmosphere's average density is found)	 

uniform float fScaleOverScaleDepth;	// fScale / fScaleDepth		

const int iSamples = 4;

const float fInvSamples = 0.25;



varying vec2 texCoord;

varying vec3 color;

varying vec3 secondaryColor;

varying vec3 v3Direction;





float scale(float fCos)

{

	float x = 1.0 - fCos;

	return fScaleDepth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));

}



void main(void)

{

	// Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere) 

	vec3 v3Pos = vertex.xyz;

	vec3 v3Ray = v3Pos - v3CameraPos;

	float fFar = length(v3Ray);

	v3Ray /= fFar;



	// Calculate the closest intersection of the ray with the outer atmosphere (which is the near point of the ray passing through the atmosphere)	

	float B = 2.0 * dot(v3CameraPos, v3Ray);

	float C = fCameraHeight2 - fOuterRadius2;

	float fDet = max(0.0, B*B - 4.0 * C);

	float fNear = 0.5 * (-B - sqrt(fDet));



	// Calculate the ray's starting position, then calculate its scattering offset	

	vec3 v3Start = v3CameraPos + v3Ray * fNear;

	fFar -= fNear;

	float fStartAngle = dot(v3Ray, v3Start) / fOuterRadius;

	float fStartDepth = exp(-1.0 / fScaleDepth);

	float fStartOffset = fStartDepth*scale(fStartAngle);



	// Initialize the scattering loop variables		

	float fSampleLength = fFar * fInvSamples;

	float fScaledLength = fSampleLength * fScale;

	vec3 v3SampleRay = v3Ray * fSampleLength;

	vec3 v3SamplePoint = v3Start + v3SampleRay * 0.5;



	// Now loop through the sample rays	

	vec3 v3FrontColor = vec3(0.0, 0.0, 0.0);

	for (int i = 0; i<iSamples; i++)

	{

		float fHeight = length(v3SamplePoint);

		float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));

		float fLightAngle = dot(v3LightPos, v3SamplePoint) / fHeight;

		float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;

		float fScatter = (fStartOffset + fDepth*(scale(fLightAngle) - scale(fCameraAngle)));

		vec3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));

		v3FrontColor += v3Attenuate * (fDepth * fScaledLength);

		v3SamplePoint += v3SampleRay;

	}



	// Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader	

	secondaryColor = v3FrontColor * fKmESun;

	color = v3FrontColor * (v3InvWavelength * fKrESun);

	gl_Position = projectionMatrix * viewMatrix * vec4(vertex, 1.0);

	v3Direction = v3CameraPos - v3Pos;

}*/

// SkyFrag.glsl

/*varying vec3 color;

varying vec3 secondaryColor;



uniform vec3 v3LightPos;

uniform float g;

uniform float g2;

uniform float fExposure;

varying vec3 v3Direction;





void main(void)

{

	float fCos = dot(v3LightPos, v3Direction) / length(v3Direction);

	float fRayleighPhase = 0.75 * (1.0 + fCos*fCos);

	float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);

	gl_FragColor.rgb = 1.0 - exp(-fExposure * (fRayleighPhase * color + fMiePhase * secondaryColor));

	gl_FragColor.a = 1.0;

	//gl_FragColor = fRayleighPhase * color + fMiePhase * secondaryColor;

	//gl_FragColor.a = gl_FragColor.b;

}*/

//cbuffer AtmosphereConstants
//{
//};

static const int NumSamples = 5;
static const float NumSamplesInv = 1.0 / NumSamples;
static const float3 WavelengthInv = float3(
	1.0 / pow(0.650, 4.0),
	1.0 / pow(0.570, 4.0),
	1.0 / pow(0.475, 4.0));
static const float Pi = 3.14159;

static const float3 LightPos = float3(-1.0, 0.0, 0.0);
static const float OuterRadius = 16.0 * 1.1;
static const float InnerRadius = 16.0 * 0.9;
static const float ScaleDepth = 0.25;
static const float ScaleDepthInv = 1.0 / ScaleDepth;
static const float Scale = 1.0 / (OuterRadius - InnerRadius);
static const float ScaleOverScaleDepth = Scale / ScaleDepth;
static const float G = -0.98;
static const float G2 = 0.9604;
static const float Exposure = 1.0;
static const float Km = 0.0025;
static const float Kr = 0.0015;
static const float ESun = 500.0;
static const float KrESun = Kr * ESun;
static const float KmESun = Km * ESun;
static const float Kr4PI = Kr * 4.0 * Pi;
static const float Km4PI = Km * 4.0 * Pi;

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

Scattering computeScattering(const float3 vpos, const float3 cameraPos)
{
	Scattering output = (Scattering)0;
	
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
	output.color = frontColor * (WavelengthInv * KrESun);
	output.secondaryColor = frontColor * KmESun;

	return output;
}

float4 computeScatteringColor(const Scattering scattering, const float3 vecToCamera)
{
	float4 output = 0.0;

	float fCos = dot(LightPos, vecToCamera) / length(vecToCamera);
	float RayleighPhase = 0.75 * (1.0 + fCos*fCos);
	float MiePhase = 1.5 * ((1.0 - G2) / (2.0 + G2)) * (1.0 + fCos*fCos) / pow(max(0.0, 1.0 + G2 - 2.0*G*fCos), 1.5);
	output.rgb = 1.0 - exp(-Exposure * (RayleighPhase * scattering.color + MiePhase * scattering.secondaryColor));
	output.a = output.b;
	//output.rgb = RayleighPhase * scattering.color + MiePhase * scattering.secondaryColor;
	//output.a = output.b;

	return output;
}