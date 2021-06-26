#pragma once

#include "Vector.h"
#include "../Streaming.h"

namespace Engine
{
	namespace Math
	{
		template <class F, int M, int N> class Matrix
		{
		public:
			Vector<F, N> row[M];

			Matrix(void) noexcept {};
			Matrix(const F * source) noexcept { MemoryCopy(this, source, sizeof(*this)); };

			operator Vector<F, M * N> & (void) noexcept { return *reinterpret_cast<Vector<F, M * N> *>(this); }
			operator const Vector<F, M * N> & (void) const noexcept { return *reinterpret_cast<const Vector<F, M * N> *>(this); }
			operator string (void) const noexcept { string result; for (int i = 0; i < M; i++) result += string(row[i]) + IO::LineFeedSequence; return result; };

			Vector<F, M * N> & VectorCast(void) noexcept { return *reinterpret_cast<Vector<F, M * N> *>(this); }
			const Vector<F, M * N> & VectorCast(void) const noexcept { return *reinterpret_cast<const Vector<F, M * N> *>(this); }
			static Matrix & MatrixCast(Vector<F, M * N> & a) noexcept { return reinterpret_cast<Matrix &>(a); }
			static const Matrix & MatrixCast(const Vector<F, M * N> & a) noexcept { return reinterpret_cast<const Matrix &>(a); }

			bool friend operator == (const Matrix & a, const Matrix & b) noexcept { return a.VectorCast() == b.VectorCast(); }
			bool friend operator != (const Matrix & a, const Matrix & b) noexcept { return a.VectorCast() != b.VectorCast(); }
			Matrix friend operator + (const Matrix & a, const Matrix & b) noexcept { return MatrixCast(a.VectorCast() + b.VectorCast()); }
			Matrix friend operator - (const Matrix & a, const Matrix & b) noexcept { return MatrixCast(a.VectorCast() - b.VectorCast()); }
			Matrix friend operator * (const Matrix & a, F b) noexcept { return MatrixCast(a.VectorCast() * b); }
			Matrix friend operator * (F b, const Matrix & a) noexcept { return MatrixCast(a.VectorCast() * b); }
			Matrix friend operator / (const Matrix & a, F b) noexcept { return MatrixCast(a.VectorCast() / b); }
			Matrix & operator += (const Matrix & a) noexcept { VectorCast() += a.VectorCast(); return *this; };
			Matrix & operator -= (const Matrix & a) noexcept { VectorCast() -= a.VectorCast(); return *this; };
			Matrix & operator *= (F a) noexcept { VectorCast() *= a; return *this; };
			Matrix & operator /= (F a) noexcept { VectorCast() /= a; return *this; };
			Matrix operator - (void) const noexcept { Matrix result; for (int i = 0; i < M; i++) result.row[i] = -row[i]; return result; };

			F & operator () (int i, int j) noexcept { return row[i].c[j]; }
			const F & operator () (int i, int j) const noexcept { return row[i].c[j]; }
		};

		template <class F, int N> Matrix<F, N, N> diag(const Vector<F, N> & v) noexcept
		{
			Matrix<F, N, N> result;
			ZeroVector(result.VectorCast());
			for (int i = 0; i < N; i++) result(i, i) = v[i];
			return result;
		}
		template <class F, int N> Matrix<F, N, N> diag(F v) noexcept
		{
			Matrix<F, N, N> result;
			ZeroVector(result.VectorCast());
			for (int i = 0; i < N; i++) result(i, i) = v;
			return result;
		}
		template <class F, int N> Matrix<F, N, N> identity(void) noexcept { return diag<F, N>(1.0); }

		template <class F, int M, int N> Matrix<F, M, N> TensorProduct(const Vector<F, M> & a, const Vector<F, N> & b) noexcept
		{
			Matrix<F, M, N> result;
			for (int i = 0; i < M; i++) for (int j = 0; j < N; j++) result.row[i].c[j] = a.c[i] * b.c[j];
			return result;
		}
		template <class F, int M> Matrix<F, M, 1> VectorColumn(const Vector<F, M> & a) noexcept
		{
			Matrix<F, M, 1> result;
			for (int i = 0; i < M; i++) result.row[i].c[0] = a.c[i];
			return result;
		}
		template <class F, int N> Matrix<F, 1, N> VectorRow(const Vector<F, N> & a) noexcept
		{
			Matrix<F, 1, N> result;
			for (int i = 0; i < N; i++) result.row[0].c[i] = a.c[i];
			return result;
		}
		template <class F, int M> Matrix<F, M, 1> & VectorColumnCast(Vector<F, M> & a) noexcept { return reinterpret_cast<Matrix<F, M, 1> &>(a); }
		template <class F, int N> Matrix<F, 1, N> & VectorRowCast(Vector<F, N> & a) noexcept { return reinterpret_cast<Matrix<F, 1, N> &>(a); }
		template <class F, int M> const Matrix<F, M, 1> & VectorColumnCast(const Vector<F, M> & a) noexcept { return reinterpret_cast<const Matrix<F, M, 1> &>(a); }
		template <class F, int N> const Matrix<F, 1, N> & VectorRowCast(const Vector<F, N> & a) noexcept { return reinterpret_cast<const Matrix<F, 1, N> &>(a); }
		template <class F, int M, int N> void ZeroMatrix(Matrix<F, M, N> & a) noexcept { ZeroVector(a.VectorCast()); }
		template <class F, int M, int N> Matrix<F, M, N> & ArrayToMatrixCast(F * data) noexcept { return *reinterpret_cast<Matrix<F, M, N> *>(data); }
		template <class F, int M, int N> const Matrix<F, M, N> & ArrayToMatrixCast(const F * data) noexcept { return *reinterpret_cast<const Matrix<F, M, N> *>(data); }

		template <class F, int M, int N, int K> Matrix<F, M, K> operator * (const Matrix<F, M, N> & a, const Matrix<F, N, K> & b) noexcept
		{
			Matrix<F, M, K> result;
			for (int i = 0; i < M; i++) for (int j = 0; j < K; j++) {
				F summ = 0.0;
				for (int k = 0; k < N; k++) summ += a.row[i].c[k] * b.row[k].c[j];
				result.row[i].c[j] = summ;
			}
			return result;
		}
		template <class F, int M, int N> Vector<F, M> operator * (const Matrix<F, M, N> & a, const Vector<F, N> & b) noexcept { return (a * VectorColumnCast(b)).VectorCast(); }
		template <class F, int M, int N> Matrix<F, N, M> transpone(const Matrix<F, M, N> & a) noexcept
		{
			Matrix<F, N, M> result;
			for (int i = 0; i < N; i++) for (int j = 0; j < M; j++) {
				result.row[i].c[j] = a.row[j].c[i];
			}
			return result;
		}
		template <class F, int N> void GaussianEliminationMethod(Matrix<F, N, N> & A, Vector<F, N> & f, Vector<F, N> & r) noexcept
		{
			int * Reorder = new (std::nothrow) int[N];
			if (!Reorder) return;
			for (int j = 0; j < N; j++) Reorder[j] = j;
			// Direct Flow
			for (int r = 0; r < N; r++) {
				// Searching main element
				real max = abs(A(r, Reorder[r]));
				int max_c = r;
				for (int c = r + 1; c < N; c++) if (abs(A(r, Reorder[c])) > max) { max = abs(A(r, Reorder[c])); max_c = c; }
				if (max_c != r) swap(Reorder[r], Reorder[max_c]);
				// Normalizing row
				F div = inverse(A(r, Reorder[r]));
				for (int c = r + 1; c < N; c++) A(r, Reorder[c]) *= div;
				f[r] *= div;
				A(r, Reorder[r]) = 1.0;
				// Subtracting
				for (int rs = r + 1; rs < N; rs++) {
					F ac = -A(rs, Reorder[r]);
					A(rs, Reorder[r]) = 0.0;
					for (int c = r + 1; c < N; c++) A(rs, Reorder[c]) += A(r, Reorder[c]) * ac;
					f[rs] += f[r] * ac;
				}
			}
			// Reverse Flow
			for (int w = N - 1; w >= 0; w--) {
				r[Reorder[w]] = f[w];
				for (int c = w + 1; c < N; c++) {
					r[Reorder[w]] -= r[Reorder[c]] * A(w, Reorder[c]);
				}
			}
			delete[] Reorder;
		}
		template <class F, int N> F det(const Matrix<F, N, N> & B) noexcept
		{
			auto A = B;
			int * Reorder = new (std::nothrow) int[N];
			if (!Reorder) return 0.0;
			for (int j = 0; j < N; j++) Reorder[j] = j;
			// Direct Flow
			for (int r = 0; r < N; r++) {
				// Searching main element
				int nz_c = r;
				for (int c = r; c < N; c++) if (A(r, Reorder[c]) != 0.0) { nz_c = c; break; }
				if (nz_c != r) swap(Reorder[r], Reorder[nz_c]);
				// Subtracting
				for (int rs = r + 1; rs < N; rs++) {
					F ac = -A(rs, Reorder[r]) / A(r, Reorder[r]);
					A(rs, Reorder[r]) = 0.0;
					for (int c = r + 1; c < N; c++) A(rs, Reorder[c]) += A(r, Reorder[c]) * ac;
				}
			}
			F prod = 1.0;
			for (int i = 0; i < N; i++) prod *= A(i, Reorder[i]);
			delete[] Reorder;
			return prod;
		}
		template <class F, int N> F tr(const Matrix<F, N, N> & A) noexcept
		{
			F summ = 0.0;
			for (int i = 0; i < N; i++) summ += A(i, i);
			return summ;
		}
		template <class F, int N> Matrix<F, N, N> inverse(const Matrix<F, N, N> & B) noexcept
		{
			Matrix<F, N, N> I;
			for (int k = 0; k < N; k++) {
				auto A = B;
				Vector<F, N> f;
				for (int i = 0; i < N; i++) f[i] = (i == k) ? 1.0 : 0.0;
				GaussianEliminationMethod(A, f, I.row[k]);
			}
			return transpone(I);
			//auto A = B;
			//auto I = identity<F, N>();
			//int * Reorder = new (std::nothrow) int[N];
			//if (!Reorder) return diag<F, N>(0.0);
			//for (int j = 0; j < N; j++) Reorder[j] = j;
			//// Direct Flow
			//for (int r = 0; r < N; r++) {
			//	// Searching main element
			//	int nz_c = r;
			//	for (int c = r; c < N; c++) if (A(r, Reorder[c]) != 0.0) { nz_c = c; break; }
			//	if (nz_c != r) swap(Reorder[r], Reorder[nz_c]);
			//	// Normalizing row
			//	F mul = A(r, Reorder[r]);
			//	F div = inverse(mul);
			//	for (int c = r + 1; c < N; c++) A(r, Reorder[c]) *= div;
			//	for (int c = 0; c < N; c++) I(r, Reorder[c]) *= div;
			//	A(r, Reorder[r]) = 1.0;
			//	// Subtracting
			//	for (int rs = r + 1; rs < N; rs++) {
			//		F ac = -A(rs, Reorder[r]);
			//		A(rs, Reorder[r]) = 0.0;
			//		for (int c = r + 1; c < N; c++) A(rs, Reorder[c]) += A(r, Reorder[c]) * ac;
			//		for (int c = 0; c < N; c++) I(rs, Reorder[c]) += I(r, Reorder[c]) * ac;
			//	}
			//}
			//// Reverse Flow
			//for (int c = N - 1; c > 0; c--) {
			//	for (int r = c - 1; r >= 0; r--) {
			//		F mul = A(r, Reorder[c]);
			//		for (int l = 0; l < N; l++) I(r, Reorder[l]) -= mul * I(c, Reorder[l]);
			//	}
			//}
			//delete[] Reorder;
			//return I;
		}

		typedef Matrix<Real, 2, 2> Matrix2x2;
		typedef Matrix<Real, 3, 3> Matrix3x3;
		typedef Matrix<Real, 4, 4> Matrix4x4;

		typedef Matrix<ShortReal, 2, 2> Matrix2x2f;
		typedef Matrix<ShortReal, 3, 3> Matrix3x3f;
		typedef Matrix<ShortReal, 4, 4> Matrix4x4f;

		typedef Matrix<Complex, 2, 2> Matrix2x2c;
		typedef Matrix<Complex, 3, 3> Matrix3x3c;
		typedef Matrix<Complex, 4, 4> Matrix4x4c;
	}
}