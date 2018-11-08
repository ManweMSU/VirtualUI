#pragma once

#include "MathBase.h"
#include "Complex.h"

namespace Engine
{
	namespace Math
	{
		template <class F, int D> class Vector
		{
		public:
			union {
				F c[D];
				struct { F x, y, z, w; };
			};
			Vector(void) noexcept {};
			Vector(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };
			Vector & operator += (const Vector & a) noexcept { for (int i = 0; i < D; i++) c[i] += a.c[i]; return *this; };
			Vector & operator -= (const Vector & a) noexcept { for (int i = 0; i < D; i++) c[i] -= a.c[i]; return *this; };
			Vector & operator *= (F a) noexcept { for (int i = 0; i < D; i++) c[i] *= a; return *this; };
			Vector & operator /= (F a) noexcept { for (int i = 0; i < D; i++) c[i] /= a; return *this; };
			Vector operator - (void) const noexcept { Vector result; for (int i = 0; i < D; i++) result.c[i] = -c[i]; return result; };
			operator string (void) const noexcept { string result = L"{"; for (int i = 0; i < D; i++) result += string(c[i]) + (i == D - 1 ? L"}" : L"; "); return result; };
			F & operator [] (int i) noexcept { return c[i]; };
			const F & operator [] (int i) const noexcept { return c[i]; };
		};
		template <class F> class Vector<F, 4>
		{
		public:
			union {
				F c[4];
				struct { F x, y, z, w; };
			};
			Vector(void) noexcept {};
			Vector(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };
			Vector(F X, F Y, F Z, F W) noexcept : x(X), y(Y), z(Z), w(W) {};
			Vector & operator += (const Vector & a) noexcept { for (int i = 0; i < 4; i++) c[i] += a.c[i]; return *this; };
			Vector & operator -= (const Vector & a) noexcept { for (int i = 0; i < 4; i++) c[i] -= a.c[i]; return *this; };
			Vector & operator *= (F a) noexcept { for (int i = 0; i < 4; i++) c[i] *= a; return *this; };
			Vector & operator /= (F a) noexcept { for (int i = 0; i < 4; i++) c[i] /= a; return *this; };
			Vector operator - (void) const noexcept { Vector result; for (int i = 0; i < 4; i++) result.c[i] = -c[i]; return result; };
			operator string (void) const noexcept { string result = L"{"; for (int i = 0; i < 4; i++) result += string(c[i]) + (i == 3 ? L"}" : L"; "); return result; };
			F & operator [] (int i) noexcept { return c[i]; };
			const F & operator [] (int i) const noexcept { return c[i]; };
		};
		template <class F> class Vector<F, 3>
		{
		public:
			union {
				F c[3];
				struct { F x, y, z; };
			};
			Vector(void) noexcept {};
			Vector(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };
			Vector(F X, F Y, F Z) noexcept : x(X), y(Y), z(Z) {};
			Vector & operator += (const Vector & a) noexcept { for (int i = 0; i < 3; i++) c[i] += a.c[i]; return *this; };
			Vector & operator -= (const Vector & a) noexcept { for (int i = 0; i < 3; i++) c[i] -= a.c[i]; return *this; };
			Vector & operator *= (F a) noexcept { for (int i = 0; i < 3; i++) c[i] *= a; return *this; };
			Vector & operator /= (F a) noexcept { for (int i = 0; i < 3; i++) c[i] /= a; return *this; };
			Vector operator - (void) const noexcept { Vector result; for (int i = 0; i < 3; i++) result.c[i] = -c[i]; return result; };
			operator string (void) const noexcept { string result = L"{"; for (int i = 0; i < 3; i++) result += string(c[i]) + (i == 2 ? L"}" : L"; "); return result; };
			F & operator [] (int i) noexcept { return c[i]; };
			const F & operator [] (int i) const noexcept { return c[i]; };
		};
		template <class F> class Vector<F, 2>
		{
		public:
			union {
				F c[2];
				struct { F x, y; };
			};
			Vector(void) noexcept {};
			Vector(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };
			Vector(F X, F Y) noexcept : x(X), y(Y) {};
			Vector & operator += (const Vector & a) noexcept { for (int i = 0; i < 2; i++) c[i] += a.c[i]; return *this; };
			Vector & operator -= (const Vector & a) noexcept { for (int i = 0; i < 2; i++) c[i] -= a.c[i]; return *this; };
			Vector & operator *= (F a) noexcept { for (int i = 0; i < 2; i++) c[i] *= a; return *this; };
			Vector & operator /= (F a) noexcept { for (int i = 0; i < 2; i++) c[i] /= a; return *this; };
			Vector operator - (void) const noexcept { Vector result; for (int i = 0; i < 2; i++) result.c[i] = -c[i]; return result; };
			operator string (void) const noexcept { string result = L"{"; for (int i = 0; i < 2; i++) result += string(c[i]) + (i == 1 ? L"}" : L"; "); return result; };
			F & operator [] (int i) noexcept { return c[i]; };
			const F & operator [] (int i) const noexcept { return c[i]; };
		};
		template <class F> class Vector<F, 1>
		{
		public:
			union {
				F c[1];
				struct { F x; };
			};
			Vector(void) noexcept {};
			Vector(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };
			Vector(F X) noexcept : x(X) {};
			Vector & operator += (const Vector & a) noexcept { x += a.x; return *this; };
			Vector & operator -= (const Vector & a) noexcept { x -= a.x; return *this; };
			Vector & operator *= (F a) noexcept { x *= a; return *this; };
			Vector & operator /= (F a) noexcept { x /= a; return *this; };
			Vector operator - (void) const noexcept { return Vector<F, 1>(-x); };
			operator string (void) const noexcept { return L"{" + string(x) + L"}"; };
			F & operator [] (int i) noexcept { return c[i]; };
			const F & operator [] (int i) const noexcept { return c[i]; };
		};

		template <class F, int D> bool operator == (const Vector<F, D> & a, const Vector<F, D> & b) noexcept { for (int i = 0; i < D; i++) if (a.c[i] != b.c[i]) return false; return true; }
		template <class F, int D> bool operator != (const Vector<F, D> & a, const Vector<F, D> & b) noexcept { for (int i = 0; i < D; i++) if (a.c[i] != b.c[i]) return true; return false; }
		template <class F, int D> Vector<F, D> operator + (const Vector<F, D> & a, const Vector<F, D> & b) noexcept { Vector<F, D> result; for (int i = 0; i < D; i++) result.c[i] = a.c[i] + b.c[i]; return result; }
		template <class F, int D> Vector<F, D> operator - (const Vector<F, D> & a, const Vector<F, D> & b) noexcept { Vector<F, D> result; for (int i = 0; i < D; i++) result.c[i] = a.c[i] - b.c[i]; return result; }
		template <class F, int D> Vector<F, D> operator * (const Vector<F, D> & a, F b) noexcept { Vector<F, D> result; for (int i = 0; i < D; i++) result.c[i] = a.c[i] * b; return result; }
		template <class F, int D> Vector<F, D> operator * (F b, const Vector<F, D> & a) noexcept { Vector<F, D> result; for (int i = 0; i < D; i++) result.c[i] = a.c[i] * b; return result; }
		template <class F, int D> Vector<F, D> operator / (const Vector<F, D> & a, F b) noexcept { Vector<F, D> result; for (int i = 0; i < D; i++) result.c[i] = a.c[i] / b; return result; }
		template <int D> Real length(const Vector<Real, D> & a) noexcept { Real s = 0.0; for (int i = 0; i < D; i++) s += a.c[i] * a.c[i]; return sqrt(s); }
		template <int D> Real length(const Vector<Complex, D> & a) noexcept { Real s = 0.0; for (int i = 0; i < D; i++) s += (a.c[i] * conjugate(a.c[i])).re; return sqrt(s); }
		template <class F, int D> Vector<F, D> normalize(const Vector<F, D> & a) noexcept { return a / length(a); }
		template <int D> Real dot(const Vector<Real, D> & a, const Vector<Real, D> & b) noexcept { Real s = 0.0; for (int i = 0; i < D; i++) s += a.c[i] * b.c[i]; return s; }
		template <int D> Complex dot(const Vector<Complex, D> & a, const Vector<Complex, D> & b) noexcept { Complex s = 0.0; for (int i = 0; i < D; i++) s += a.c[i] * conjugate(b.c[i]); return s; }
		template <class F, int D> F operator * (const Vector<F, D> & a, const Vector<F, D> & b) noexcept { return dot(a, b); }
		Vector<Real, 3> cross(const Vector<Real, 3> & a, const Vector<Real, 3> & b) noexcept;

		typedef Vector<Real, 4> Vector4;
		typedef Vector<Real, 3> Vector3;
		typedef Vector<Real, 2> Vector2;

		template <class F, int D> void ZeroVector(Vector<F, D> & a) { ZeroMemory(&a, sizeof(a)); }
	}
}