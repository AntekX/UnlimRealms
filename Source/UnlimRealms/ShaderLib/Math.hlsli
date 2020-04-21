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
static const float Exponent = 0.31831;
static const float3 WorldUp = float3(0.0, 1.0, 0.0);

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

// Ray-Spehere intersection
// returns distances to intersection points
// where: x = near, y = far (-1 if no intersection)
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

#endif