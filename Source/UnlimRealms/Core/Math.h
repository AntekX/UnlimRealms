///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	UnlimRealms
//	Author: Anatole Kuzub
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common.h"
#include "Core/BaseTypes.h"

namespace UnlimRealms
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Constants
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	struct MathConst
	{
		static const T Pi;
		static const T PiHalf;
		static const T PiRcp;
		static const T Exp;
	};
	template <typename T>
	const T MathConst<T>::Pi		= (T)3.141592653589793238463;
	template <typename T>
	const T MathConst<T>::PiHalf	= (T)1.57079632679489661923;
	template <typename T>
	const T MathConst<T>::PiRcp		= (T)0.318309886183790671538;
	template <typename T>
	const T MathConst<T>::Exp		= (T)2.71828182845904523536;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	2d vector template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TVector2
	{
		T x;
		T y;

		TVector2()
		{
		}

		TVector2(T _v)
		{
			this->x = _v;
			this->y = _v;
		}

		TVector2(T _x, T _y)
		{
			this->x = _x;
			this->y = _y;
		}

		TVector2(const TVector2<T> &v)
		{
			this->x = v.x;
			this->y = v.y;
		}

		static const TVector2<T> Zero;

		TVector2<T> operator+ (const TVector2<T> &v) const
		{
			TVector2 r;
			r.x = x + v.x;
			r.y = y + v.y;
			return r;
		}

		TVector2<T> operator- (const TVector2<T> &v) const
		{
			TVector2 r;
			r.x = x - v.x;
			r.y = y - v.y;
			return r;
		}

		TVector2<T> operator* (const TVector2<T> &v) const
		{
			TVector2 r;
			r.x = x * v.x;
			r.y = y * v.y;
			return r;
		}

		TVector2<T> operator/ (const TVector2<T> v) const
		{
			TVector2 r;
			r.x = x / v.x;
			r.y = y / v.y;
			return r;
		}

		TVector2<T>& operator+= (const TVector2<T> &v)
		{
			this->x += v.x;
			this->y += v.y;
			return *this;
		}

		TVector2<T>& operator-= (const TVector2<T> &v)
		{
			this->x -= v.x;
			this->y -= v.y;
			return *this;
		}

		TVector2<T>& operator*= (const TVector2<T> &v)
		{
			this->x *= v.x;
			this->y *= v.y;
			return *this;
		}

		TVector2<T>& operator/= (const TVector2<T> &v)
		{
			this->x /= v.x;
			this->y /= v.y;
			return *this;
		}

		bool operator== (const TVector2<T> &v)
		{
			return (x == v.x && y == v.y);
		}

		bool operator!= (const TVector2<T> &v)
		{
			return (x != v.x || y != v.y);
		}

		T& operator [] (const ur_uint idx)
		{
			return *(&this->x + idx);
		}

		const T& operator [] (const ur_uint idx) const
		{
			return *(&this->x + idx);
		}

		void SetMin(const TVector2<T> &v)
		{
			this->x = std::min(this->x, v.x);
			this->y = std::min(this->y, v.y);
		}

		static TVector2<T> Min(const TVector2<T> &v1, const TVector2<T> &v2)
		{
			TVector2<T> r = v1;
			r.SetMin(v2);
			return r;
		}

		void SetMax(const TVector2<T> &v)
		{
			this->x = std::max(this->x, v.x);
			this->y = std::max(this->y, v.y);
		}

		static TVector2<T> Max(const TVector2<T> &v1, const TVector2<T> &v2)
		{
			TVector2<T> r = v1;
			r.SetMax(v2);
			return r;
		}

		T LengthSquared() const
		{
			return 0;
		}

		T Length() const
		{
			return 0;
		}

		void Normalize()
		{
			T l = this->Length();
			if (l > 0)
			{
				this->x /= l;
				this->y /= l;
			}
		}

		static TVector2<T> Normalize(const TVector2<T> &v)
		{
			TVector2<T> r = v;
			r.Normalize();
			return r;
		}

		static void Lerp(TVector2<T> &output, const TVector2<T> &v0, const TVector2<T> &v1, T s)
		{
			output.x = v0.x * (T(1) - s) + v1.x * s;
			output.y = v0.y * (T(1) - s) + v1.y * s;
		}

		static TVector2<T> Lerp(const TVector2<T> &v0, const TVector2<T> &v1, T s)
		{
			TVector2<T> r;
			Lerp(r, v0, v1, s);
			return r;
		}
	};

	float TVector2<float>::LengthSquared() const
	{
		return (this->x * this->x + this->y * this->y);
	}

	double TVector2<double>::LengthSquared() const
	{
		return (this->x * this->x + this->y * this->y);
	}

	float TVector2<float>::Length() const
	{
		return sqrtf(this->LengthSquared());
	}

	double TVector2<double>::Length() const
	{
		return sqrt(this->LengthSquared());
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	3d vector template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TVector3
	{
		T x;
		T y;
		T z;

		TVector3()
		{
		}

		TVector3(T _v)
		{
			this->x = _v;
			this->y = _v;
			this->z = _v;
		}

		TVector3(T _x, T _y, T _z)
		{
			this->x = _x;
			this->y = _y;
			this->z = _z;
		}

		TVector3(const TVector3<T> &v)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
		}

		static const TVector3<T> Zero;

		TVector3<T> operator+ (const TVector3<T> &v) const
		{
			TVector3 r;
			r.x = x + v.x;
			r.y = y + v.y;
			r.z = z + v.z;
			return r;
		}

		TVector3<T> operator- (const TVector3<T> &v) const
		{
			TVector3 r;
			r.x = x - v.x;
			r.y = y - v.y;
			r.z = z - v.z;
			return r;
		}

		TVector3<T> operator* (const TVector3<T> &v) const
		{
			TVector3 r;
			r.x = x * v.x;
			r.y = y * v.y;
			r.z = z * v.z;
			return r;
		}

		TVector3<T> operator/ (const TVector3<T> v) const
		{
			TVector3 r;
			r.x = x / v.x;
			r.y = y / v.y;
			r.z = z / v.z;
			return r;
		}

		TVector3<T>& operator+= (const TVector3<T> &v)
		{
			this->x += v.x;
			this->y += v.y;
			this->z += v.z;
			return *this;
		}

		TVector3<T>& operator-= (const TVector3<T> &v)
		{
			this->x -= v.x;
			this->y -= v.y;
			this->z -= v.z;
			return *this;
		}

		TVector3<T>& operator*= (const TVector3<T> &v)
		{
			this->x *= v.x;
			this->y *= v.y;
			this->z *= v.z;
			return *this;
		}

		TVector3<T>& operator/= (const TVector3<T> &v)
		{
			this->x /= v.x;
			this->y /= v.y;
			this->z /= v.z;
			return *this;
		}

		bool operator== (const TVector3<T> &v) const
		{
			return (this->x == v.x && this->y == v.y && this->z == v.z);
		}

		bool operator!= (const TVector3<T> &v) const
		{
			return (this->x != v.x || this->y != v.y || this->z != v.z);
		}

		TVector3<T>& operator= (const T* v)
		{
			this->x = v[0];
			this->y = v[1];
			this->z = v[2];
			return *this;
		}

		T& operator [] (const ur_uint idx)
		{
			return *(&this->x + idx);
		}

		const T& operator [] (const ur_uint idx) const
		{
			return *(&this->x + idx);
		}

		bool Any()
		{
			return (this->x != 0 || this->y != 0 || this->z != 0);
		}

		bool All()
		{
			return !(this->x == 0 || this->y == 0 || this->z == 0);
		}

		void SetMin(const TVector3<T> &v)
		{
			this->x = std::min(this->x, v.x);
			this->y = std::min(this->y, v.y);
			this->z = std::min(this->z, v.z);
		}

		static TVector3<T> Min(const TVector3<T> &v1, const TVector3<T> &v2)
		{
			TVector3<T> r = v1;
			r.SetMin(v2);
			return r;
		}

		void SetMax(const TVector3<T> &v)
		{
			this->x = std::max(this->x, v.x);
			this->y = std::max(this->y, v.y);
			this->z = std::max(this->z, v.z);
		}

		static TVector3<T> Max(const TVector3<T> &v1, const TVector3<T> &v2)
		{
			TVector3<T> r = v1;
			r.SetMax(v2);
			return r;
		}

		T GetMaxValue()
		{
			return std::max(std::max(this->x, this->y), this->z);
		}

		T GetMinValue()
		{
			return std::min(std::min(this->x, this->y), this->z);
		}

		T LengthSquared() const
		{
			return 0;
		}

		T Length() const
		{
			return 0;
		}

		TVector3<T>& Normalize()
		{
			T l = this->Length();
			if (l > 0)
			{
				this->x /= l;
				this->y /= l;
				this->z /= l;
			}
			return *this;
		}

		static TVector3<T> Normalize(const TVector3<T> &v)
		{
			TVector3<T> r = v;
			r.Normalize();
			return r;
		}

		static void Lerp(TVector3<T> &output, const TVector3<T> &v0, const TVector3<T> &v1, T s)
		{
			output.x = v0.x * (T(1) - s) + v1.x * s;
			output.y = v0.y * (T(1) - s) + v1.y * s;
			output.z = v0.z * (T(1) - s) + v1.z * s;
		}

		static void Lerp(TVector3<T> &output, const TVector3<T> &v0, const TVector3<T> &v1, const TVector3<T> &s)
		{
			output.x = v0.x * (T(1) - s) + v1.x * s.x;
			output.y = v0.y * (T(1) - s) + v1.y * s.y;
			output.z = v0.z * (T(1) - s) + v1.z * s.z;
		}

		static TVector3<T> Lerp(const TVector3<T> &v0, const TVector3<T> &v1, T s)
		{
			TVector3<T> r;
			Lerp(r, v0, v1, s);
			return r;
		}

		static TVector3<T> Lerp(const TVector3<T> &v0, const TVector3<T> &v1, const TVector3<T> &s)
		{
			TVector3<T> r;
			Lerp(r, v0, v1, s);
			return r;
		}

		static T Dot(const TVector3<T> &v1, const TVector3<T> &v2)
		{
			return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
		}

		static void Cross(TVector3<T> &output, const TVector3<T> &v1, const TVector3<T> &v2)
		{
			output.x = v1.y * v2.z - v1.z * v2.y;
			output.y = v1.z * v2.x - v1.x * v2.z;
			output.z = v1.x * v2.y - v1.y * v2.x;
		}

		static TVector3<T> Cross(const TVector3<T> &v1, const TVector3<T> &v2)
		{
			TVector3<T> r;
			Cross(r, v1, v2);
			return r;
		}

		static const TVector3<T> I;
		static const TVector3<T> J;
		static const TVector3<T> K;
	};

	template <class T>
	const TVector3<T> TVector3<T>::I = { 1, 0, 0 };

	template <class T>
	const TVector3<T> TVector3<T>::J = { 0, 1, 0 };

	template <class T>
	const TVector3<T> TVector3<T>::K = { 0, 0, 1 };

	float TVector3<float>::LengthSquared() const
	{
		return (this->x * this->x + this->y * this->y + this->z * this->z);
	}

	double TVector3<double>::LengthSquared() const
	{
		return (this->x * this->x + this->y * this->y + this->z * this->z);
	}

	float TVector3<float>::Length() const
	{
		return sqrtf(this->LengthSquared());
	}

	double TVector3<double>::Length() const
	{
		return sqrt(this->LengthSquared());
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	4d vector template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TVector4
	{
		T x;
		T y;
		T z;
		T w;

		TVector4()
		{
		}

		TVector4(T _v)
		{
			this->x = _v;
			this->y = _v;
			this->z = _v;
			this->w = _v;
		}

		TVector4(T _x, T _y, T _z, T _w)
		{
			this->x = _x;
			this->y = _y;
			this->z = _z;
			this->w = _w;
		}

		TVector4(const TVector4<T> &v)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			this->w = v.w;
		}

		TVector4(const TVector3<T> &v, T _w)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			this->w = _w;
		}

		static const TVector4<T> Zero;

		TVector4<T>& operator= (const TVector3<T> &v)
		{
			this->x = v.x;
			this->y = v.y;
			this->z = v.z;
			return *this;
		}

		TVector4<T> operator+ (const TVector4<T> &v) const
		{
			TVector r;
			r.x = x + v.x;
			r.y = y + v.y;
			r.z = z + v.z;
			r.w = w + v.w;
			return r;
		}

		TVector4<T> operator- (const TVector4<T> &v) const
		{
			TVector4 r;
			r.x = x - v.x;
			r.y = y - v.y;
			r.z = z - v.z;
			r.w = w - v.w;
			return r;
		}

		TVector4<T> operator* (const TVector4<T> &v) const
		{
			TVector4<T> r;
			r.x = x * v.x;
			r.y = y * v.y;
			r.z = z * v.z;
			r.w = w * v.w;
			return r;
		}

		TVector4<T> operator/ (const TVector4<T> v) const
		{
			TVector4<T> r;
			r.x = x / v.x;
			r.y = y / v.y;
			r.z = z / v.z;
			r.w = w / v.w;
			return r;
		}

		TVector4<T>& operator+= (const TVector4<T> &v)
		{
			this->x += v.x;
			this->y += v.y;
			this->z += v.z;
			this->w += v.w;
			return *this;
		}

		TVector4<T>& operator-= (const TVector4<T> &v)
		{
			this->x -= v.x;
			this->y -= v.y;
			this->z -= v.z;
			this->w -= v.w;
			return *this;
		}

		TVector4<T>& operator*= (const TVector4<T> &v)
		{
			this->x *= v.x;
			this->y *= v.y;
			this->z *= v.z;
			this->w *= v.w;
			return *this;
		}

		TVector4<T>& operator/= (const TVector4<T> &v)
		{
			this->x /= v.x;
			this->y /= v.y;
			this->z /= v.z;
			this->w /= v.w;
			return *this;
		}

		bool operator== (const TVector4<T> &v)
		{
			return (x == v.x && y == v.y && z == v.z && w == v.w);
		}

		bool operator!= (const TVector4<T> &v)
		{
			return (x != v.x || y != v.y || z != v.z || w != v.w);
		}

		T& operator [] (const ur_uint idx)
		{
			return *(&this->x + idx);
		}

		const T& operator [] (const ur_uint idx) const
		{
			return *(&this->x + idx);
		}

		operator TVector3<T>& ()
		{
			return *(TVector3<T>*)this;
		}

		static void Lerp(TVector4<T> &output, const TVector4<T> &v0, const TVector4<T> &v1, T s)
		{
			output.x = v0.x * (T(1) - s) + v1.x * s;
			output.y = v0.y * (T(1) - s) + v1.y * s;
			output.z = v0.z * (T(1) - s) + v1.z * s;
			output.w = v0.w * (T(1) - s) + v1.w * s;
		}

		static TVector4<T> Lerp(const TVector4<T> &v0, const TVector4<T> &v1, T s)
		{
			TVector4<T> r;
			Lerp(r, v0, v1, s);
			return r;
		}

		void SetMin(const TVector4<T> &v)
		{
			this->x = std::min(this->x, v.x);
			this->y = std::min(this->y, v.y);
			this->z = std::min(this->z, v.z);
			this->w = std::min(this->w, v.w);
		}

		static TVector4<T> Min(const TVector4<T> &v1, const TVector4<T> &v2)
		{
			TVector4<T> r = v1;
			r.SetMin(v2);
			return r;
		}

		void SetMax(const TVector4<T> &v)
		{
			this->x = std::max(this->x, v.x);
			this->y = std::max(this->y, v.y);
			this->z = std::max(this->z, v.z);
			this->w = std::max(this->w, v.w);
		}

		static TVector4<T> Max(const TVector4<T> &v1, const TVector4<T> &v2)
		{
			TVector4<T> r = v1;
			r.SetMax(v2);
			return r;
		}

		T GetMaxValue()
		{
			return std::max(std::max(std::max(this->x, this->y), this->z), this->w);
		}

		T GetMinValue()
		{
			return std::min(std::min(std::min(this->x, this->y), this->z), this->w);
		}

		void SetQuaternion(const TVector3<T> &axis, const T &angle)
		{
			T a = angle / 2;
			T s = sin(a);
			this->x = axis.x * s;
			this->y = axis.y * s;
			this->z = axis.z * s;
			this->w = cos(a);
		}

		static TVector4<T> Quaternion(const TVector3<T> &axis, const T &angle)
		{
			TVector4<T> q;
			q.SetQuaternion(axis, angle);
			return q;
		}
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	4x4 matrix template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TMatrix
	{
		TVector4<T> r[4];

		TMatrix()
		{
		}

		TMatrix(const TVector4<T> &r0, const TVector4<T> &r1, const TVector4<T> &r2, const TVector4<T> &r3)
		{
			r[0] = r0;
			r[1] = r1;
			r[2] = r2;
			r[3] = r3;
		}

		TMatrix(
			T m00, T m01, T m02, T m03,
			T m10, T m11, T m12, T m13,
			T m20, T m21, T m22, T m23,
			T m30, T m31, T m32, T m33)
		{
			r[0].x = m00; r[0].y = m01; r[0].z = m02; r[0].w = m03;
			r[1].x = m10; r[1].y = m11; r[1].z = m12; r[1].w = m13;
			r[2].x = m20; r[2].y = m21; r[2].z = m22; r[2].w = m23;
			r[3].x = m30; r[3].y = m31; r[3].z = m32; r[3].w = m33;
		}

		TMatrix(const T* a)
		{
			r[0].x = a + 0;  r[0].y = a + 1;  r[0].z = a + 2;  r[0].w = a + 3;
			r[1].x = a + 4;  r[1].y = a + 5;  r[1].z = a + 6;  r[1].w = a + 7;
			r[2].x = a + 8;  r[2].y = a + 9;  r[2].z = a + 10; r[2].w = a + 11;
			r[3].x = a + 12; r[3].y = a + 13; r[3].z = a + 14; r[3].w = a + 15;
		}

		static const TMatrix<T> Zero;

		static const TMatrix<T> Identity;

		void SetZero()
		{
			*this = Zero;
		}

		void SetIdentity()
		{
			*this = Identity;
		}

		static TMatrix<T>& Transpose(TMatrix<T> &m)
		{
			T t;
			t = m.r[0][1]; m.r[0][1] = m.r[1][0]; m.r[1][0] = t;
			t = m.r[0][2]; m.r[0][2] = m.r[2][0]; m.r[2][0] = t;
			t = m.r[0][3]; m.r[0][3] = m.r[3][0]; m.r[3][0] = t;
			t = m.r[1][2]; m.r[1][2] = m.r[2][1]; m.r[2][1] = t;
			t = m.r[1][3]; m.r[1][3] = m.r[3][1]; m.r[3][1] = t;
			t = m.r[2][3]; m.r[2][3] = m.r[3][2]; m.r[3][2] = t;
			return m;
		}

		TMatrix<T>& Transpose()
		{
			return Transpose(*this);
		}

		static TMatrix<T>& Inverse(TMatrix<T>& result, const TMatrix<T> &m)
		{
			T A2323 = m.r[2][2] * m.r[3][3] - m.r[2][3] * m.r[3][2];
			T A1323 = m.r[2][1] * m.r[3][3] - m.r[2][3] * m.r[3][1];
			T A1223 = m.r[2][1] * m.r[3][2] - m.r[2][2] * m.r[3][1];
			T A0323 = m.r[2][0] * m.r[3][3] - m.r[2][3] * m.r[3][0];
			T A0223 = m.r[2][0] * m.r[3][2] - m.r[2][2] * m.r[3][0];
			T A0123 = m.r[2][0] * m.r[3][1] - m.r[2][1] * m.r[3][0];
			T A2313 = m.r[1][2] * m.r[3][3] - m.r[1][3] * m.r[3][2];
			T A1313 = m.r[1][1] * m.r[3][3] - m.r[1][3] * m.r[3][1];
			T A1213 = m.r[1][1] * m.r[3][2] - m.r[1][2] * m.r[3][1];
			T A2312 = m.r[1][2] * m.r[2][3] - m.r[1][3] * m.r[2][2];
			T A1312 = m.r[1][1] * m.r[2][3] - m.r[1][3] * m.r[2][1];
			T A1212 = m.r[1][1] * m.r[2][2] - m.r[1][2] * m.r[2][1];
			T A0313 = m.r[1][0] * m.r[3][3] - m.r[1][3] * m.r[3][0];
			T A0213 = m.r[1][0] * m.r[3][2] - m.r[1][2] * m.r[3][0];
			T A0312 = m.r[1][0] * m.r[2][3] - m.r[1][3] * m.r[2][0];
			T A0212 = m.r[1][0] * m.r[2][2] - m.r[1][2] * m.r[2][0];
			T A0113 = m.r[1][0] * m.r[3][1] - m.r[1][1] * m.r[3][0];
			T A0112 = m.r[1][0] * m.r[2][1] - m.r[1][1] * m.r[2][0];

			T det = m.r[0][0] * (m.r[1][1] * A2323 - m.r[1][2] * A1323 + m.r[1][3] * A1223)
				- m.r[0][1] * (m.r[1][0] * A2323 - m.r[1][2] * A0323 + m.r[1][3] * A0223)
				+ m.r[0][2] * (m.r[1][0] * A1323 - m.r[1][1] * A0323 + m.r[1][3] * A0123)
				- m.r[0][3] * (m.r[1][0] * A1223 - m.r[1][1] * A0223 + m.r[1][2] * A0123);
			det = 1 / det;

			result.r[0][0] = det * (m.r[1][1] * A2323 - m.r[1][2] * A1323 + m.r[1][3] * A1223);
			result.r[0][1] = det * -(m.r[0][1] * A2323 - m.r[0][2] * A1323 + m.r[0][3] * A1223);
			result.r[0][2] = det * (m.r[0][1] * A2313 - m.r[0][2] * A1313 + m.r[0][3] * A1213);
			result.r[0][3] = det * -(m.r[0][1] * A2312 - m.r[0][2] * A1312 + m.r[0][3] * A1212);
			result.r[1][0] = det * -(m.r[1][0] * A2323 - m.r[1][2] * A0323 + m.r[1][3] * A0223);
			result.r[1][1] = det * (m.r[0][0] * A2323 - m.r[0][2] * A0323 + m.r[0][3] * A0223);
			result.r[1][2] = det * -(m.r[0][0] * A2313 - m.r[0][2] * A0313 + m.r[0][3] * A0213);
			result.r[1][3] = det * (m.r[0][0] * A2312 - m.r[0][2] * A0312 + m.r[0][3] * A0212);
			result.r[2][0] = det * (m.r[1][0] * A1323 - m.r[1][1] * A0323 + m.r[1][3] * A0123);
			result.r[2][1] = det * -(m.r[0][0] * A1323 - m.r[0][1] * A0323 + m.r[0][3] * A0123);
			result.r[2][2] = det * (m.r[0][0] * A1313 - m.r[0][1] * A0313 + m.r[0][3] * A0113);
			result.r[2][3] = det * -(m.r[0][0] * A1312 - m.r[0][1] * A0312 + m.r[0][3] * A0112);
			result.r[3][0] = det * -(m.r[1][0] * A1223 - m.r[1][1] * A0223 + m.r[1][2] * A0123);
			result.r[3][1] = det * (m.r[0][0] * A1223 - m.r[0][1] * A0223 + m.r[0][2] * A0123);
			result.r[3][2] = det * -(m.r[0][0] * A1213 - m.r[0][1] * A0213 + m.r[0][2] * A0113);
			result.r[3][3] = det * (m.r[0][0] * A1212 - m.r[0][1] * A0212 + m.r[0][2] * A0112);

			return result;
		}

		TMatrix<T>& Inverse()
		{
			*this = Inverse(*this);
			return *this;
		}

		static TMatrix<T>& Translation(TMatrix<T> &m, T x, T y, T z)
		{
			m.r[0].x = 1; m.r[0].y = 0; m.r[0].z = 0; m.r[0].w = 0;
			m.r[1].x = 0; m.r[1].y = 1; m.r[1].z = 0; m.r[1].w = 0;
			m.r[2].x = 0; m.r[2].y = 0; m.r[2].z = 1; m.r[2].w = 0;
			m.r[3].x = x; m.r[3].y = y; m.r[3].z = z; m.r[3].w = 1;
			return m;
		}

		static TMatrix<T> Translation(T x, T y, T z)
		{
			TMatrix<T> m;
			TMatrix<T>::Translation(m, x, y, z);
			return m;
		}

		static TMatrix<T>& Scaling(TMatrix<T> &m, T x, T y, T z)
		{
			m.r[0].x = x; m.r[0].y = 0; m.r[0].z = 0; m.r[0].w = 0;
			m.r[1].x = 0; m.r[1].y = y; m.r[1].z = 0; m.r[1].w = 0;
			m.r[2].x = 0; m.r[2].y = 0; m.r[2].z = z; m.r[2].w = 0;
			m.r[3].x = 0; m.r[3].y = 0; m.r[3].z = 0; m.r[3].w = 1;
			return m;
		}

		static TMatrix<T> Scaling(T x, T y, T z)
		{
			TMatrix<T> m;
			TMatrix<T>::Scaling(m, x, y, z);
			return m;
		}

		static TMatrix<T>& RotationAxis(TMatrix<T> &m, const TVector3<T> &axis, const T &angle)
		{
			const TVector3<T> &r = axis;
			const T &a = angle;
			T c = cos(a);
			T s = sin(a);
			T t = 1 - c;
			m.r[0].x = t * r.x * r.x + c;
			m.r[1].x = t * r.x * r.y - s * r.z;
			m.r[2].x = t * r.x * r.z + s * r.y;
			m.r[3].x = 0;
			m.r[0].y = t * r.x * r.y + s * r.z;
			m.r[1].y = t * r.y * r.y + c;
			m.r[2].y = t * r.y * r.z - s * r.x;
			m.r[3].y = 0;
			m.r[0].z = t * r.x * r.z - s * r.y;
			m.r[1].z = t * r.y * r.z + s * r.x;
			m.r[2].z = t * r.z * r.z + c;
			m.r[3].z = 0;
			m.r[0].w = 0;
			m.r[1].w = 0;
			m.r[2].w = 0;
			m.r[3].w = 1;
			return m;
		}

		static TMatrix<T> RotationAxis(const TVector3<T> &axis, const T &angle)
		{
			TMatrix<T> m;
			TMatrix<T>::RotationAxis(m, axis, angle);
			return m;
		}

		static TMatrix<T>& RotationQuaternion(TMatrix<T> &m, const TVector4<T> &q)
		{
			T s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
			s = 2 / (q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
			xs = s*q.x;		ys = s*q.y;		zs = s*q.z;
			wx = q.w*xs;	wy = q.w*ys;	wz = q.w*zs;
			xx = q.x*xs;	xy = q.x*ys;	xz = q.x*zs;
			yy = q.y*ys;	yz = q.y*zs;	zz = q.z*zs;
			m.r[0].x = 1 - (yy + zz);
			m.r[1].x = xy - wz;
			m.r[2].x = xz + wy;
			m.r[3].x = 0;
			m.r[0].y = xy + wz;
			m.r[1].y = 1 - (xx + zz);
			m.r[2].y = yz - wx;
			m.r[3].y = 0;
			m.r[0].z = xz - wy;
			m.r[1].z = yz + wx;
			m.r[2].z = 1 - (xx + yy);
			m.r[3].z = 0;
			m.r[0].w = 0;
			m.r[1].w = 0;
			m.r[2].w = 0;
			m.r[3].w = 1;
			return m;
		}

		static TMatrix<T> RotationQuaternion(const TVector4<T> &q)
		{
			TMatrix<T> m;
			TMatrix<T>::RotationQuaternion(m, q);
			return m;
		}

		static TMatrix<T>& Multiply(TMatrix<T> &rm, const TMatrix<T> &m1, const TMatrix<T> &m2)
		{
			rm.r[0].x = m2.r[0].x*m1.r[0].x + m2.r[1].x*m1.r[0].y + m2.r[2].x*m1.r[0].z + m2.r[3].x*m1.r[0].w;
			rm.r[0].y = m2.r[0].y*m1.r[0].x + m2.r[1].y*m1.r[0].y + m2.r[2].y*m1.r[0].z + m2.r[3].y*m1.r[0].w;
			rm.r[0].z = m2.r[0].z*m1.r[0].x + m2.r[1].z*m1.r[0].y + m2.r[2].z*m1.r[0].z + m2.r[3].z*m1.r[0].w;
			rm.r[0].w = m2.r[0].w*m1.r[0].x + m2.r[1].w*m1.r[0].y + m2.r[2].w*m1.r[0].z + m2.r[3].w*m1.r[0].w;
			
			rm.r[1].x = m2.r[0].x*m1.r[1].x + m2.r[1].x*m1.r[1].y + m2.r[2].x*m1.r[1].z + m2.r[3].x*m1.r[1].w;
			rm.r[1].y = m2.r[0].y*m1.r[1].x + m2.r[1].y*m1.r[1].y + m2.r[2].y*m1.r[1].z + m2.r[3].y*m1.r[1].w;
			rm.r[1].z = m2.r[0].z*m1.r[1].x + m2.r[1].z*m1.r[1].y + m2.r[2].z*m1.r[1].z + m2.r[3].z*m1.r[1].w;
			rm.r[1].w = m2.r[0].w*m1.r[1].x + m2.r[1].w*m1.r[1].y + m2.r[2].w*m1.r[1].z + m2.r[3].w*m1.r[1].w;
			
			rm.r[2].x = m2.r[0].x*m1.r[2].x + m2.r[1].x*m1.r[2].y + m2.r[2].x*m1.r[2].z + m2.r[3].x*m1.r[2].w;
			rm.r[2].y = m2.r[0].y*m1.r[2].x + m2.r[1].y*m1.r[2].y + m2.r[2].y*m1.r[2].z + m2.r[3].y*m1.r[2].w;
			rm.r[2].z = m2.r[0].z*m1.r[2].x + m2.r[1].z*m1.r[2].y + m2.r[2].z*m1.r[2].z + m2.r[3].z*m1.r[2].w;
			rm.r[2].w = m2.r[0].w*m1.r[2].x + m2.r[1].w*m1.r[2].y + m2.r[2].w*m1.r[2].z + m2.r[3].w*m1.r[2].w;
			
			rm.r[3].x = m2.r[0].x*m1.r[3].x + m2.r[1].x*m1.r[3].y + m2.r[2].x*m1.r[3].z + m2.r[3].x*m1.r[3].w;
			rm.r[3].y = m2.r[0].y*m1.r[3].x + m2.r[1].y*m1.r[3].y + m2.r[2].y*m1.r[3].z + m2.r[3].y*m1.r[3].w;
			rm.r[3].z = m2.r[0].z*m1.r[3].x + m2.r[1].z*m1.r[3].y + m2.r[2].z*m1.r[3].z + m2.r[3].z*m1.r[3].w;
			rm.r[3].w = m2.r[0].w*m1.r[3].x + m2.r[1].w*m1.r[3].y + m2.r[2].w*m1.r[3].z + m2.r[3].w*m1.r[3].w;

			return rm;
		}

		static TMatrix<T> Multiply(const TMatrix<T> &m1, const TMatrix<T> &m2)
		{
			TMatrix<T> m;
			TMatrix<T>::Multiply(m, m1, m2);
			return m;
		}

		TMatrix<T>& Multiply(const TMatrix<T> &m)
		{
			return TMatrix<T>::Multiply(*this, *this, m);
		}

		static TVector4<T>& Multiply(TVector4<T> &rv, const TMatrix<T> &m, const TVector4<T> &v)
		{
			rv.x = m.r[0].x * v.x + m.r[1].x * v.y + m.r[2].x * v.z + m.r[3].x * v.w;
			rv.y = m.r[0].y * v.x + m.r[1].y * v.y + m.r[2].y * v.z + m.r[3].y * v.w;
			rv.z = m.r[0].z * v.x + m.r[1].z * v.y + m.r[2].z * v.z + m.r[3].z * v.w;
			rv.w = m.r[0].w * v.x + m.r[1].w * v.y + m.r[2].w * v.z + m.r[3].w * v.w;
			return rv;
		}

		static TVector4<T> Multiply(const TMatrix<T> &m, const TVector4<T> &v)
		{
			TVector4<T> rv;
			TMatrix<T>::Multiply(rv, m, v);
			return rv;
		}

		TVector4<T> Multiply(const TVector4<T> &v) const
		{
			return TMatrix<T>::Multiply(*this, v);
		}

		static TVector3<T>& TransformCoord(TVector3<T>& rc, const TMatrix<T> &m, const TVector3<T> &c)
		{
			rc.x = m.r[0].x * c.x + m.r[1].x * c.y + m.r[2].x * c.z + m.r[3].x;
			rc.y = m.r[0].y * c.x + m.r[1].y * c.y + m.r[2].y * c.z + m.r[3].y;
			rc.z = m.r[0].z * c.x + m.r[1].z * c.y + m.r[2].z * c.z + m.r[3].z;
			return rc;
		}

		static TVector3<T> TransformCoord(const TMatrix<T> &m, const TVector3<T> &c)
		{
			TVector3<T> rc;
			TMatrix<T>::TransformCoord(rc, m, c);
			return rc;
		}

		TVector3<T> TransformCoord(const TVector3<T> &c) const
		{
			return TMatrix<T>::TransformCoord(*this, c);
		}

		static TMatrix<T> View(const TVector3<T> &eye, const TVector3<T> &right, const TVector3<T> &up, const TVector3<T> &ahead)
		{
			TMatrix<T> rm(
				right.x,	up.x,	ahead.x,	0,
				right.y,	up.y,	ahead.y,	0,
				right.z,	up.z,	ahead.z,	0,
				0,			0,		0,			1
			);
			TVector3<T> eyeInv = TMatrix::TransformCoord(rm, eye) * -1;
			rm.r[3].x = eyeInv.x;
			rm.r[3].y = eyeInv.y;
			rm.r[3].z = eyeInv.z;
			return rm;
		}

		static TMatrix<T> LookAt(const TVector3<T> &eye, const TVector3<T> &lookAt, const TVector3<T> &up)
		{
			TVector3<T> ahead = lookAt - eye;
			ahead.Normalize();
			TVector3<T> right = TVector3<T>::Cross(up, ahead);
			right.Normalize();
			TVector3<T> viewUp = TVector3<T>::Cross(ahead, right);
			return TMatrix<T>::View(eye, right, viewUp, ahead);
		}

		static TMatrix<T> Perspective(T width, T height, T nearPlane, T farPlane)
		{
			TMatrix<T> rm(
				nearPlane / width * 2, 0, 0, 0,
				0, nearPlane / height * 2, 0, 0,
				0, 0, farPlane / (farPlane - nearPlane), 1,
				0, 0, nearPlane * farPlane / (nearPlane - farPlane), 0
			);
			return rm;
		}

		static TMatrix<T> PerspectiveFov(T fieldOfView, T aspectRatio, T nearPlane, T farPlane)
		{
			T sy = (T)1.0 / std::tan(fieldOfView / 2);
			T sx = sy / aspectRatio;
			TMatrix<T> rm(
				sx, 0, 0, 0,
				0, sy, 0, 0,
				0, 0, farPlane / (farPlane - nearPlane), 1,
				0, 0, nearPlane * farPlane / (nearPlane - farPlane), 0
				);
			return rm;
		}

		static void FrustumPlanes(const TMatrix<T> &m, TVector4<T> (&planes)[6], bool normalize)
		{
			planes[0].x = m.r[0][3] + m.r[0][0]; planes[0].y = m.r[1][3] + m.r[1][0]; planes[0].z = m.r[2][3] + m.r[2][0]; planes[0].w = m.r[3][3] + m.r[3][0];
			planes[1].x = m.r[0][3] - m.r[0][0]; planes[1].y = m.r[1][3] - m.r[1][0]; planes[1].z = m.r[2][3] - m.r[2][0]; planes[1].w = m.r[3][3] - m.r[3][0];
			planes[2].x = m.r[0][3] - m.r[0][1]; planes[2].y = m.r[1][3] - m.r[1][1]; planes[2].z = m.r[2][3] - m.r[2][1]; planes[2].w = m.r[3][3] - m.r[3][1];
			planes[3].x = m.r[0][3] + m.r[0][1]; planes[3].y = m.r[1][3] + m.r[1][1]; planes[3].z = m.r[2][3] + m.r[2][1]; planes[3].w = m.r[3][3] + m.r[3][1];
			planes[4].x = m.r[0][2];  planes[4].y = m.r[1][2]; planes[4].z = m.r[2][2]; planes[4].w = m.r[3][2];
			planes[5].x = m.r[0][3] - m.r[0][2]; planes[5].y = m.r[1][3] - m.r[1][2]; planes[5].z = m.r[2][3] - m.r[2][2]; planes[5].w = m.r[3][3] - m.r[3][2];

			if (normalize)
			{
				for (int i = 0; i < 6; ++i)
				{
					float mag = sqrtf(planes[i].x * planes[i].x + planes[i].y * planes[i].y + planes[i].z * planes[i].z);
					planes[i] = planes[i] / mag;
				}
			}
		}

		void FrustumPlanes(TVector4<T>(&planes)[6], bool normalize) const
		{
			this->FrustumPlanes(*this, planes, normalize);
		}
	};

	template <class T>
	const TVector2<T> TVector2<T>::Zero = { 0, 0 };

	template <class T>
	const TVector3<T> TVector3<T>::Zero = { 0, 0, 0 };

	template <class T>
	const TVector4<T> TVector4<T>::Zero = { 0, 0, 0, 0 };

	template <class T>
	const TMatrix<T> TMatrix<T>::Zero = {
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0
	};

	template <class T>
	const TMatrix<T> TMatrix<T>::Identity = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

#ifdef DIRECTX

	static XMMATRIX GetXMMatrix(const TMatrix<float> &m)
	{
		XMMATRIX xm = XMMatrixSet(
			m.r[0].x, m.r[0].y, m.r[0].z, m.r[0].w,
			m.r[1].x, m.r[1].y, m.r[1].z, m.r[1].w,
			m.r[2].x, m.r[2].y, m.r[2].z, m.r[2].w,
			m.r[3].x, m.r[3].y, m.r[3].z, m.r[3].w);
		return xm;
	}

	static void SetXMMatrix(TMatrix<float> &m, const XMMATRIX &xm)
	{
		m.r[0].x = XMVectorGetX(xm.r[0]);
		m.r[0].y = XMVectorGetY(xm.r[0]);
		m.r[0].z = XMVectorGetZ(xm.r[0]);
		m.r[0].w = XMVectorGetW(xm.r[0]);
		m.r[1].x = XMVectorGetX(xm.r[1]);
		m.r[1].y = XMVectorGetY(xm.r[1]);
		m.r[1].z = XMVectorGetZ(xm.r[1]);
		m.r[1].w = XMVectorGetW(xm.r[1]);
		m.r[2].x = XMVectorGetX(xm.r[2]);
		m.r[2].y = XMVectorGetY(xm.r[2]);
		m.r[2].z = XMVectorGetZ(xm.r[2]);
		m.r[2].w = XMVectorGetW(xm.r[2]);
		m.r[3].x = XMVectorGetX(xm.r[3]);
		m.r[3].y = XMVectorGetY(xm.r[3]);
		m.r[3].z = XMVectorGetZ(xm.r[3]);
		m.r[3].w = XMVectorGetW(xm.r[3]);
	}

	TMatrix<float> TMatrix<float>::RotationAxis(const TVector3<float> &axis, const float &angle)
	{
		TMatrix<float> rm;
		XMMATRIX xm = XMMatrixRotationNormal(XMLoadFloat3((XMFLOAT3*)&axis), angle);
		SetXMMatrix(rm, xm);
		return rm;
	}

	TMatrix<float> TMatrix<float>::Multiply(const TMatrix<float> &m1, const TMatrix<float> &m2)
	{
		TMatrix<float> rm;
		XMMATRIX xm = XMMatrixMultiply(GetXMMatrix(m1), GetXMMatrix(m2));
		SetXMMatrix(rm, xm);
		return rm;
	}

	TVector3<float> TMatrix<float>::TransformCoord(const TMatrix<float> &m, const TVector3<float> &c)
	{
		XMVECTOR xv = XMVector3TransformCoord(XMLoadFloat3((XMFLOAT3*)&c), GetXMMatrix(m));
		TVector3<float> rc(XMVectorGetX(xv), XMVectorGetY(xv), XMVectorGetZ(xv));
		return rc;
	}

#endif


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Bounding sphere template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TBoundingSphere
	{
		TVector3<T> Center;
		T Radius;

		TBoundingSphere() : Center(0, 0, 0), Radius(0) {}

		TBoundingSphere(const TVector3<T> &center, T radius) : Center(center), Radius(radius) {}

		TBoundingSphere(const TBoundingSphere &sp) : Center(sp.Center), Radius(sp.Radius) {}

		inline bool IsZeroSize() const { return (this->Radius == 0); }
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Axis aligned rectangle template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TRect
	{
		TVector2<T> Min;
		TVector2<T> Max;

		TRect() : Min(std::numeric_limits<T>::max()), Max(-std::numeric_limits<T>::max()) {}

		TRect(const TRect<T> &r) : Min(r.Min), Max(r.Max) {}

		TRect(const TVector2<T> &_min, const TVector2<T> &_max) : Min(_min), Max(_max) {}

		TRect(const T _minX, const T _minY, const T _maxX, const T _maxY) : Min(_minX, _minY), Max(_maxX, _maxY) {}

		T Width() const
		{
			return (Max.x - Min.x);
		}

		T Height() const
		{
			return (Max.y - Min.y);
		}

		T Area() const
		{
			return this->Width() * this->Height();
		}

		bool IsInsideOut() const
		{
			return (this->Width() < 0 || this->Height() < 0);
		}

		void Move(T x, T y)
		{
			int dx = x - Min.x;
			int dy = y - Min.y;
			Min.x += dx;
			Min.y += dy;
			Max.x += dx;
			Max.y += dy;
		}

		void Resize(T width, T height)
		{
			Max.x = Min.x + width;
			Max.y = Min.y + height;
		}

		bool Intersects(const TRect<T> &r)
		{
			return !(r.Min.x > this->Max.x || r.Max.x < this->Min.x ||
				r.Min.y > this->Max.y || r.Max.y < this->Min.y);
		}

		bool Intersection(const TRect<T> &r, TRect<T> &interection)
		{
			if (!Intersects(r))
				return false;
			interection.Min.x = std::max(r.Min.x, this->Min.x);
			interection.Max.x = std::min(r.Max.x, this->Max.x);
			interection.Min.y = std::max(r.Min.y, this->Min.y);
			interection.Max.y = std::min(r.Max.y, this->Max.y);
			return true;
		}		
	};


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	Axis aligned bounding box template class
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct TBoundingBox
	{
		TVector3<T> Min;
		TVector3<T> Max;

		TBoundingBox() : Min(std::numeric_limits<T>::max()), Max(-std::numeric_limits<T>::max()) {}

		TBoundingBox(const TVector3<T> &_min, const TVector3<T> &_max) : Min(_min), Max(_max) {}

		TBoundingBox(const TBoundingBox<T> &bb) : Min(bb.Min), Max(bb.Max) {}

		TVector3<T> Size() const
		{
			return this->Max - this->Min;
		}

		T SizeX() const
		{
			return this->Max.x - this->Min.x;
		}

		T SizeY() const
		{
			return this->Max.y - this->Min.y;
		}

		T SizeZ() const
		{
			return this->Max.z - this->Min.z;
		}

		T SizeMin() const
		{
			return std::min(std::min(SizeX(), SizeY()), SizeZ());
		}

		T SizeMax() const
		{
			return std::max(std::max(SizeX(), SizeY()), SizeZ());
		}

		bool IsInsideOut() const
		{
			return (this->SizeX() < 0 || this->SizeY() < 0 || this->SizeZ() < 0);
		}

		TVector3<T> Center() const
		{
			return (this->Min / 2 + this->Max / 2);
		}

		bool Intersects(const TVector3<T> &v) const
		{
			bool isOutside =
				v.x < this->Min.x || v.x > this->Max.x ||
				v.y < this->Min.y || v.y > this->Max.y ||
				v.z < this->Min.z || v.z > this->Max.z;
			return !isOutside;
		}

		bool Intersects(const TBoundingBox<T> &bb) const
		{
			bool isOutside =
				bb.Max.x < this->Min.x || bb.Min.x > this->Max.x ||
				bb.Max.y < this->Min.y || bb.Min.y > this->Max.y ||
				bb.Max.z < this->Min.z || bb.Min.z > this->Max.z;
			return !isOutside;
		}

		bool Intersects(const TVector4<T>(&frustumPlanes)[6]) const
		{
			bool culled = false;
			for (const Vector4 &plane : frustumPlanes)
			{
				culled |= (
					(plane.x * this->Min.x + plane.y * this->Min.y + plane.z * this->Min.z + plane.w < 0) &&
					(plane.x * this->Max.x + plane.y * this->Min.y + plane.z * this->Min.z + plane.w < 0) &&
					(plane.x * this->Min.x + plane.y * this->Min.y + plane.z * this->Max.z + plane.w < 0) &&
					(plane.x * this->Max.x + plane.y * this->Min.y + plane.z * this->Max.z + plane.w < 0) &&
					(plane.x * this->Min.x + plane.y * this->Max.y + plane.z * this->Min.z + plane.w < 0) &&
					(plane.x * this->Max.x + plane.y * this->Max.y + plane.z * this->Min.z + plane.w < 0) &&
					(plane.x * this->Min.x + plane.y * this->Max.y + plane.z * this->Max.z + plane.w < 0) &&
					(plane.x * this->Max.x + plane.y * this->Max.y + plane.z * this->Max.z + plane.w < 0));
				if (culled) break;
			}
			return !culled;
		}

		bool Contains(const TBoundingBox<T> &bb) const
		{
			return (
				bb.Min.x >= this->Min.x && bb.Min.y >= this->Min.y && bb.Min.z >= this->Min.z &&
				bb.Max.x <= this->Max.x && bb.Max.y <= this->Max.y && bb.Max.z <= this->Max.z);
		}

		void ToPointArray(TVector3<T> *pa) const
		{
			if (ur_null == pa)
				return;

			pa[0].x = this->Min.x; pa[0].x = this->Min.y; pa[0].x = this->Min.z;
			pa[1].x = this->Max.x; pa[1].x = this->Min.y; pa[1].x = this->Min.z;
			pa[2].x = this->Min.x; pa[2].x = this->Max.y; pa[2].x = this->Min.z;
			pa[3].x = this->Max.x; pa[3].x = this->Max.y; pa[3].x = this->Min.z;
			pa[4].x = this->Min.x; pa[4].x = this->Min.y; pa[4].x = this->Max.z;
			pa[5].x = this->Max.x; pa[5].x = this->Min.y; pa[5].x = this->Max.z;
			pa[6].x = this->Min.x; pa[6].x = this->Max.y; pa[6].x = this->Max.z;
			pa[7].x = this->Max.x; pa[7].x = this->Max.y; pa[7].x = this->Max.z;
		}

		T Distance(const TVector3<T> &v) const
		{
			TVector3<T> d;
			d.x = std::max(std::max(this->Min.x - v.x, v.x - this->Max.x), (T)0);
			d.y = std::max(std::max(this->Min.y - v.y, v.y - this->Max.y), (T)0);
			d.z = std::max(std::max(this->Min.z - v.z, v.z - this->Max.z), (T)0);
			return d.Length();
		}

		void Expand(const TVector3<T> &v)
		{
			this->Min.SetMin(v);
			this->Max.SetMax(v);
		}

		void Merge(const TBoundingBox<T> &bb)
		{
			this->Min.SetMin(bb.Min);
			this->Max.SetMax(bb.Max);
		}

		static TBoundingBox<T> Merge(const TBoundingBox<T> &bb1, const TBoundingBox<T> &bb2)
		{
			TBoundingBox<T> r = bb1;
			r.Merge(bb2);
			return r;
		}

		void CalculateProjRect(const TMatrix<T> & viewProjection, TRect<T> &projRect) const
		{
			projRect.Min = std::numeric_limits<T>::max();
			projRect.Max = -std::numeric_limits<T>::max();

			TVector4<T> wpos(0.0f, 0.0f, 0.0f, 1.0f);
			TVector4<T> ppos;
			auto projectPoint = [&](const ur_float &x, const ur_float &y, const ur_float &z) -> void {
				wpos.x = x; wpos.y = y; wpos.z = z;
				TMatrix<T>::Multiply(ppos, viewProjection, wpos);
				ppos.x = ppos.x / ppos.w;
				ppos.y = ppos.y / ppos.w;
				projRect.Min.x = min(projRect.Min.x, ppos.x);
				projRect.Min.y = min(projRect.Min.y, ppos.y);
				projRect.Max.x = max(projRect.Max.x, ppos.x);
				projRect.Max.y = max(projRect.Max.y, ppos.y);
			};

			projectPoint(this->Min.x, this->Min.y, this->Min.z);
			projectPoint(this->Max.x, this->Min.y, this->Min.z);
			projectPoint(this->Min.x, this->Min.y, this->Max.z);
			projectPoint(this->Max.x, this->Min.y, this->Max.z);
			projectPoint(this->Min.x, this->Max.y, this->Min.z);
			projectPoint(this->Max.x, this->Max.y, this->Min.z);
			projectPoint(this->Min.x, this->Max.y, this->Max.z);
			projectPoint(this->Max.x, this->Max.y, this->Max.z);

			return projRect;
		}

		bool operator== (const TBoundingBox<T> &v) const
		{
			return !(this->Min != v.Min || this->Max != v.Max);
		}

		static const TBoundingBox<T> Zero;
	};

	template <class T>
	const TBoundingBox<T> TBoundingBox<T>::Zero = {
		{ 0, 0, 0 },
		{ 0, 0, 0 }
	};



	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Common template classes
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	typedef TVector2<ur_float>			Vector2;
	typedef TVector3<ur_float>			Vector3;
	typedef TVector4<ur_float>			Vector4;
	typedef TMatrix<ur_float>			Matrix;
	typedef TVector4<ur_float>			Color;
	typedef TRect<ur_float>				RectF;
	typedef TRect<ur_double>			RectD;
	typedef TRect<ur_int>				RectI;
	typedef TBoundingSphere<ur_float>	BoundingSphere;
	typedef TBoundingBox<ur_float>		BoundingBox;
	typedef TBoundingBox<ur_int>		BoxI;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Extended numeric types
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	typedef TVector2<ur_float>			ur_float2;
	typedef TVector2<ur_double>			ur_double2;
	typedef TVector2<ur_int>			ur_int2;
	typedef TVector2<ur_uint>			ur_uint2;
	typedef TVector2<ur_byte>			ur_byte2;
	
	typedef TVector3<ur_float>			ur_float3;
	typedef TVector3<ur_double>			ur_double3;
	typedef TVector3<ur_int>			ur_int3;
	typedef TVector3<ur_uint>			ur_uint3;
	typedef TVector3<ur_byte>			ur_byte3;

	typedef TVector4<ur_float>			ur_float4;
	typedef TVector4<ur_double>			ur_double4;
	typedef TVector4<ur_int>			ur_int4;
	typedef TVector4<ur_uint>			ur_uint4;
	typedef TVector4<ur_byte>			ur_byte4;

	typedef TMatrix<ur_float>			ur_float4x4;
	typedef TMatrix<ur_double>			ur_double4x4;


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Basic routine functions
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	template <typename T>
	inline T clamp(T v, T vmin, T vmax)
	{
		return std::max(vmin, std::min(vmax, v));
	}

	template <typename T>
	inline T saturate(T v)
	{
		return clamp<T>(v, 0.0, 1.0);
	}

	template <typename T>
	inline T lerp(T x, T y, T s)
	{
		return (x + s * (y - x));
	}

	template <typename T>
	inline ur_uint32 Vector4ToRGBA32(T v)
	{
		ur_uint32 u = 0x0;
		u |= ur_uint32(v.x * 0xff) << 0;
		u |= ur_uint32(v.y * 0xff) << 8;
		u |= ur_uint32(v.z * 0xff) << 16;
		u |= ur_uint32(v.w * 0xff) << 24;
		return u;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Fowler–Noll–Vo hash function
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	inline ur_size ComputeHash(const ur_byte *buffer, const ur_size sizeInBytes)
	{
		#ifdef UR_PLATFORM_x64
		const ur_size fnv_offset_basis = 14695981039346656037;
		const ur_size fnv_prime = 1099511628211;
		#else
		const ur_size fnv_offset_basis = 2166136261;
		const ur_size fnv_prime = 16777619;
		#endif
		ur_size hash = fnv_offset_basis;
		const ur_byte *p_octet = buffer;
		const ur_byte *p_end = buffer + sizeInBytes;
		for (; p_octet != p_end; ++p_octet)
		{
			hash ^= *p_octet;
			hash *= fnv_prime;
		}
		return hash;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Intersection test functions
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	inline bool IntersectRayPlaneDistance(T &dist, const TVector3<T> &rayPos, const TVector3<T> &rayDir,
		const TVector3<T> &planePoint, const TVector3<T> &planeNormal)
	{
		T denom = TVector3<T>::Dot(rayDir, planeNormal);
		if (denom != 0)
		{
			T num = TVector3<T>::Dot(planePoint - rayPos, planeNormal);
			dist = num / denom;
		}
		return false;
	}

	template <typename T>
	inline bool IntersectRayPlane(TVector3<T> *result, const TVector3<T> &rayPos, const TVector3<T> &rayDir,
		const TVector3<T> &planePoint, const TVector3<T> &planeNormal)
	{
		float d;
		if (IntersectRayPlaneDistance(d, rayPos, rayDir, planePoint, planeNormal))
		{
			if (d >= 0)
			{
				if (result)
				{
					*result = rayPos + rayDir * d;
				}
				return true;
			}
		}
		return false;
	}

#ifdef DIRECTX
	inline bool IntersectRayPlane(TVector3<float> *result, const TVector3<float> &rayPos, const TVector3<float> &rayDir,
		const TVector3<float> &planePoint, const TVector3<float> &planeNormal)
	{
		XMVECTOR pp = XMVectorSet(planePoint.x, planePoint.y, planePoint.z, 0.0f);
		XMVECTOR pn = XMVectorSet(planeNormal.x, planeNormal.y, planeNormal.z, 0.0f);
		XMVECTOR p = XMVectorSet(planeNormal.x, planeNormal.y, planeNormal.z, XMVectorGetX(XMVector3Dot(p, pn)) * -1.0f);
		XMVECTOR l0 = XMVectorSet(rayPos.x, rayPos.y, rayPos.z, 0.0f);
		XMVECTOR l1 = XMVectorSet(rayDir.x, rayDir.y, rayDir.z, 0.0f) + l0;
		XMVECTOR r = XMPlaneIntersectLine(p, l0, l1);
		if (result)
		{
			result->x = XMVectorGetX(r);
			result->y = XMVectorGetY(r);
			result->z = XMVectorGetZ(r);
		}
		return (XMVector3IsNaN(r) == TRUE);
	}
#endif

} // end namespace UnlimRealms