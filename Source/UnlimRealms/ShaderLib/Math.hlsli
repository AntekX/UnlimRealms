///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
//	Math constants and calculation routines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MATH_HLSLI
#define MATH_HLSLI

static const float Pi = 3.14159;
static const float TwoPi = 6.28319;
static const float FourPi = 12.5664;
static const float HalfPi = 1.57080;
static const float OneOverPi = 0.31831;
static const float Exponent = 2.71828;
static const float3 WorldUp = float3(0.0, 1.0, 0.0);
static const float Eps = 1.0e-5;

//------------------------------------------------------------------------------
// Scalar operations
//------------------------------------------------------------------------------

float pow2(float x)
{
	float x2 = x * x;
	return x2;
}

float pow3(float x)
{
	float x2 = x * x;
	return x2 * x;
}

float pow4(float x)
{
	float x2 = x * x;
	return x2 * x2;
}

float pow5(float x)
{
	float x2 = x * x;
	return x2 * x2 * x;
}

float max3(const float3 v)
{
	return max(v.x, max(v.y, v.z));
}

float3 RayPlaneIntersection(float3 rayOrigin, float3 rayDirection, float3 planeOrigin, float3 planeNormal)
{
	float d = -dot(planeNormal, planeOrigin);
	float t = (-dot(planeNormal, rayOrigin) - d) / dot(planeNormal, rayDirection);
	return rayOrigin + t * rayDirection;
}

// Ray-Spehere intersection
// returns distances to intersection points
// where: x = near, y = far (-1 if no intersection)
float2 IntersectSphere(const float3 rayOrigin, const float3 rayDirection, const float3 sphereCenter, const float sphereRadius)
{
	float3 L = sphereCenter - rayOrigin;
	float L2 = dot(L, L);
	float sphereRadius2 = sphereRadius * sphereRadius;
	//[flatten] if (L2 <= sphereRadius2) near = 0.0; // inside sphere, near point = rayOrigin

	float tc = dot(L, rayDirection);
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

//------------------------------------------------------------------------------
// Depth Utils
//------------------------------------------------------------------------------

float ClipDepthToViewDepth(in float clipDepth, in float4x4 proj)
{
	return proj[3][2] / (clipDepth - proj[2][2]);
}

float3 ClipPosToWorldPos(in float3 clipPos, in float4x4 viewProjInv)
{
	float4 worldPos = mul(float4(clipPos, 1.0), viewProjInv);
	return (worldPos.xyz / worldPos.w);
}

float3 ImagePosToWorldPos(in int2 imagePos, in float2 imageToUvScale, in float clipDepth, in float4x4 viewProjInv)
{
	float2 uvPos = (float2(imagePos) + 0.5) * imageToUvScale;
	float3 clipPos = float3(float2(uvPos.x, 1.0 - uvPos.y) * 2.0 - 1.0, clipDepth);
	return ClipPosToWorldPos(clipPos, viewProjInv);
}

/***************************************************************/
// 3D value noise
// Ref: https://www.shadertoy.com/view/XsXfRH
// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
float HashFloat3(float3 p)
{
	p = frac(p*0.3183099 + .1);
	p *= 17.0;
	return frac(p.x*p.y*p.z*(p.x + p.y + p.z));
}

// Thomas Wang hash 
// Ref: http://www.burtleburtle.net/bob/hash/integer.html
uint HashUInt(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

// Xorshift algorithm from George Marsaglia's paper.
uint RandXorshift(uint state)
{
	state ^= (state << 13);
	state ^= (state >> 17);
	state ^= (state << 5);
	return state;
}

float2 Rand2D(const float2 p)
{
	float3 p3 = frac(float3(p.xyx) * float3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 19.19);
	return frac((p3.xx + p3.yz)*p3.zy);
}

float3 Rand3D(const float3 p)
{
	float3 p3 = frac(float3(p.xyz) * float3(.1031, .1030, .0973));
	p3 += dot(p3, p3.yzx + 19.19);
	return frac((p3.xxx + p3.yzx)*p3.xzy);
}

static const float2 BlueNoiseDiskLUT64[64] = {
	float2(0.478712, 0.875764),
	float2(-0.337956, -0.793959),
	float2(-0.955259, -0.028164),
	float2(0.864527, 0.325689),
	float2(0.209342, -0.395657),
	float2(-0.106779, 0.672585),
	float2(0.156213, 0.235113),
	float2(-0.413644, -0.082856),
	float2(-0.415667, 0.323909),
	float2(0.141896, -0.939980),
	float2(0.954932, -0.182516),
	float2(-0.766184, 0.410799),
	float2(-0.434912, -0.458845),
	float2(0.415242, -0.078724),
	float2(0.728335, -0.491777),
	float2(-0.058086, -0.066401),
	float2(0.202990, 0.686837),
	float2(-0.808362, -0.556402),
	float2(0.507386, -0.640839),
	float2(-0.723494, -0.229240),
	float2(0.489740, 0.317826),
	float2(-0.622663, 0.765301),
	float2(-0.010640, 0.929347),
	float2(0.663146, 0.647618),
	float2(-0.096674, -0.413835),
	float2(0.525945, -0.321063),
	float2(-0.122533, 0.366019),
	float2(0.195235, -0.687983),
	float2(-0.563203, 0.098748),
	float2(0.418563, 0.561335),
	float2(-0.378595, 0.800367),
	float2(0.826922, 0.001024),
	float2(-0.085372, -0.766651),
	float2(-0.921920, 0.183673),
	float2(-0.590008, -0.721799),
	float2(0.167751, -0.164393),
	float2(0.032961, -0.562530),
	float2(0.632900, -0.107059),
	float2(-0.464080, 0.569669),
	float2(-0.173676, -0.958758),
	float2(-0.242648, -0.234303),
	float2(-0.275362, 0.157163),
	float2(0.382295, -0.795131),
	float2(0.562955, 0.115562),
	float2(0.190586, 0.470121),
	float2(0.770764, -0.297576),
	float2(0.237281, 0.931050),
	float2(-0.666642, -0.455871),
	float2(-0.905649, -0.298379),
	float2(0.339520, 0.157829),
	float2(0.701438, -0.704100),
	float2(-0.062758, 0.160346),
	float2(-0.220674, 0.957141),
	float2(0.642692, 0.432706),
	float2(-0.773390, -0.015272),
	float2(-0.671467, 0.246880),
	float2(0.158051, 0.062859),
	float2(0.806009, 0.527232),
	float2(-0.057620, -0.247071),
	float2(0.333436, -0.516710),
	float2(-0.550658, -0.315773),
	float2(-0.652078, 0.589846),
	float2(0.008818, 0.530556),
	float2(-0.210004, 0.519896)
};

#endif // MATH_HLSLI