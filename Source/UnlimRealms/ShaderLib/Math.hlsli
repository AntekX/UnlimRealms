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

/***************************************************************/
// 3D value noise
// Ref: https://www.shadertoy.com/view/XsXfRH
// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
float hash(float3 p)
{
	p = frac(p*0.3183099 + .1);
	p *= 17.0;
	return frac(p.x*p.y*p.z*(p.x + p.y + p.z));
}

#endif