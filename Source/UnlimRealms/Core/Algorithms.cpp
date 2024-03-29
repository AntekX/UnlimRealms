///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Core/Algorithms.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Noise generators
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	const ur_int64 PerlinNoise::perm[512] = {
		151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
		140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
		247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
		57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
		74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
		60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
		65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
		200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
		52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
		207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
		119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
		218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
		81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
		184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
		222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
		151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
		140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
		247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
		57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
		74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
		60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
		65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
		200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
		52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
		207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
		119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
		129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
		218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
		81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
		184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
		222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
	};

	const ur_int64 PerlinNoise::grad[12][3] = {
		{ 1,1,0 },{ -1,1,0 },{ 1,-1,0 },{ -1,-1,0 },
		{ 1,0,1 },{ -1,0,1 },{ 1,0,-1 },{ -1,0,-1 },
		{ 0,1,1 },{ 0,-1,1 },{ 0,1,-1 },{ 0,-1,-1 }
	};

	PerlinNoise::PerlinNoise()
	{
	}

	PerlinNoise::~PerlinNoise()
	{
	}

	ur_double PerlinNoise::Noise(ur_double x, ur_double y, ur_double z)
	{
		ur_int64 ix = (ur_int64)floor(x) & 255;
		ur_int64 iy = (ur_int64)floor(y) & 255;
		ur_int64 iz = (ur_int64)floor(z) & 255;
		x -= floor(x);
		y -= floor(y);
		z -= floor(z);
		ur_double u = fade(x);
		ur_double v = fade(y);
		ur_double w = fade(z);
		ur_int64 a = perm[ix] + iy;
		ur_int64 aa = perm[a] + iz;
		ur_int64 ab = perm[a + 1] + iz;
		ur_int64 b = perm[ix + 1] + iy;
		ur_int64 ba = perm[b] + iz;
		ur_int64 bb = perm[b + 1] + iz;

		return lerp(
			lerp(
				lerp(grad_fn(perm[aa], x, y, z), grad_fn(perm[ba], x - 1, y, z), u),
				lerp(grad_fn(perm[ab], x, y - 1, z), grad_fn(perm[bb], x - 1, y - 1, z), u),
				v),
			lerp(
				lerp(grad_fn(perm[aa + 1], x, y, z - 1), grad_fn(perm[ba + 1], x - 1, y, z - 1), u),
				lerp(grad_fn(perm[ab + 1], x, y - 1, z - 1), grad_fn(perm[bb + 1], x - 1, y - 1, z - 1), u),
				v),
			w);


		return lerp(lerp(lerp(u, grad_fn(perm[aa], x, y, z), grad_fn(perm[ba], x - 1, y, z)),
			lerp(u, grad_fn(perm[ab], x, y - 1, z), grad_fn(perm[bb], x - 1, y - 1, z)), v),
			lerp(lerp(u, grad_fn(perm[aa + 1], x, y, z - 1), grad_fn(perm[ba + 1], x - 1, y, z - 1)),
				lerp(u, grad_fn(perm[ab + 1], x, y - 1, z - 1), grad_fn(perm[bb + 1], x - 1, y - 1, z - 1)), v), w);
	}


	SimplexNoise::SimplexNoise()
	{
	}

	SimplexNoise::~SimplexNoise()
	{
	}

	ur_double SimplexNoise::Noise(ur_double xin, ur_double yin)
	{
		ur_double n0, n1, n2;
		ur_double F2 = 0.5 * (sqrt(3.0) - 1.0);
		ur_double s = (xin + yin) * F2;
		ur_int64 i = fastfloor(xin + s);
		ur_int64 j = fastfloor(yin + s);
		ur_double G2 = (3.0 - sqrt(3.0)) / 6.0;
		ur_double t = (i + j) * G2;
		ur_double X0 = i - t;
		ur_double Y0 = j - t;
		ur_double x0 = xin - X0;
		ur_double y0 = yin - Y0;
		ur_int64 i1, j1;
		if (x0 > y0) { i1 = 1; j1 = 0; }
		else { i1 = 0; j1 = 1; }
		ur_double x1 = x0 - i1 + G2;
		ur_double y1 = y0 - j1 + G2;
		ur_double x2 = x0 - 1.0 + 2.0 * G2;
		ur_double y2 = y0 - 1.0 + 2.0 * G2;
		ur_int64 ii = i & 255;
		ur_int64 jj = j & 255;
		ur_int64 gi0 = PerlinNoise::perm[ii + PerlinNoise::perm[jj]] % 12;
		ur_int64 gi1 = PerlinNoise::perm[ii + i1 + PerlinNoise::perm[jj + j1]] % 12;
		ur_int64 gi2 = PerlinNoise::perm[ii + 1 + PerlinNoise::perm[jj + 1]] % 12;
		ur_double t0 = 0.5 - x0 * x0 - y0 * y0;
		if (t0 < 0) n0 = 0.0;
		else
		{
			t0 *= t0;
			n0 = t0 * t0 * dot(PerlinNoise::grad[gi0], x0, y0);
		}
		ur_double t1 = 0.5 - x1 * x1 - y1 * y1;
		if (t1 < 0) n1 = 0.0;
		else
		{
			t1 *= t1;
			n1 = t1 * t1 * dot(PerlinNoise::grad[gi1], x1, y1);
		}
		ur_double t2 = 0.5 - x2 * x2 - y2 * y2;
		if (t2 < 0) n2 = 0.0;
		else
		{
			t2 *= t2;
			n2 = t2 * t2 * dot(PerlinNoise::grad[gi2], x2, y2);
		}
		return 70.0 * (n0 + n1 + n2);
	}

	ur_double SimplexNoise::Noise(ur_double xin, ur_double yin, ur_double zin)
	{
		ur_double n0, n1, n2, n3;
		ur_double F3 = 1.0 / 3.0;
		ur_double s = (xin + yin + zin) * F3;
		ur_int64 i = fastfloor(xin + s);
		ur_int64 j = fastfloor(yin + s);
		ur_int64 k = fastfloor(zin + s);
		ur_double G3 = 1.0 / 6.0;
		ur_double t = (i + j + k) * G3;
		ur_double X0 = i - t;
		ur_double Y0 = j - t;
		ur_double Z0 = k - t;
		ur_double x0 = xin - X0;
		ur_double y0 = yin - Y0;
		ur_double z0 = zin - Z0;
		ur_int64 i1, j1, k1;
		ur_int64 i2, j2, k2;
		if (x0 >= y0)
		{
			if (y0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
			else if (x0 >= z0) { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
			else { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
		}
		else
		{
			if (y0 < z0) { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
			else if (x0 < z0) { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
			else { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
		}
		ur_double x1 = x0 - i1 + G3;
		ur_double y1 = y0 - j1 + G3;
		ur_double z1 = z0 - k1 + G3;
		ur_double x2 = x0 - i2 + 2.0*G3;
		ur_double y2 = y0 - j2 + 2.0*G3;
		ur_double z2 = z0 - k2 + 2.0*G3;
		ur_double x3 = x0 - 1.0 + 3.0*G3;
		ur_double y3 = y0 - 1.0 + 3.0*G3;
		ur_double z3 = z0 - 1.0 + 3.0*G3;
		ur_int64 ii = i & 255;
		ur_int64 jj = j & 255;
		ur_int64 kk = k & 255;
		ur_int64 gi0 = PerlinNoise::perm[ii + PerlinNoise::perm[jj + PerlinNoise::perm[kk]]] % 12;
		ur_int64 gi1 = PerlinNoise::perm[ii + i1 + PerlinNoise::perm[jj + j1 + PerlinNoise::perm[kk + k1]]] % 12;
		ur_int64 gi2 = PerlinNoise::perm[ii + i2 + PerlinNoise::perm[jj + j2 + PerlinNoise::perm[kk + k2]]] % 12;
		ur_int64 gi3 = PerlinNoise::perm[ii + 1 + PerlinNoise::perm[jj + 1 + PerlinNoise::perm[kk + 1]]] % 12;
		ur_double t0 = 0.5 - x0*x0 - y0*y0 - z0*z0;
		if (t0 < 0) n0 = 0.0;
		else
		{
			t0 *= t0;
			n0 = t0 * t0 * dot(PerlinNoise::grad[gi0], x0, y0, z0);
		}
		ur_double t1 = 0.5 - x1*x1 - y1*y1 - z1*z1;
		if (t1 < 0) n1 = 0.0;
		else
		{
			t1 *= t1;
			n1 = t1 * t1 * dot(PerlinNoise::grad[gi1], x1, y1, z1);
		}
		ur_double t2 = 0.5 - x2*x2 - y2*y2 - z2*z2;
		if (t2 < 0) n2 = 0.0;
		else
		{
			t2 *= t2;
			n2 = t2 * t2 * dot(PerlinNoise::grad[gi2], x2, y2, z2);
		}
		ur_double t3 = 0.5 - x3*x3 - y3*y3 - z3*z3;
		if (t3 < 0) n3 = 0.0;
		else
		{
			t3 *= t3;
			n3 = t3 * t3 * dot(PerlinNoise::grad[gi3], x3, y3, z3);
		}
		return 32.0 * (n0 + n1 + n2 + n3);
	}

} // end namespace UnlimRealms