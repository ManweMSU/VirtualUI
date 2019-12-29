#include "Vector.h"

namespace Engine
{
	namespace Math
	{
		Vector<Real, 3> cross(const Vector<Real, 3>& a, const Vector<Real, 3>& b) noexcept { return Vector<Real, 3>(a.y * b.z - b.y * a.z, b.x * a.z - a.x * b.z, a.x * b.y - b.x * a.y); }
		Vector<ShortReal, 3> cross(const Vector<ShortReal, 3> & a, const Vector<ShortReal, 3> & b) noexcept { return Vector<ShortReal, 3>(a.y * b.z - b.y * a.z, b.x * a.z - a.x * b.z, a.x * b.y - b.x * a.y); }
	}
}