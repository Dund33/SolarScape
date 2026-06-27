#include "Vector3.h"

#include <cmath>

Vector3::Vector3() : x(0.0L), y(0.0L), z(0.0L)
{
}

Vector3::Vector3(Real x, Real y, Real z) : x(x), y(y), z(z)
{
}

long double Vector3::norm() const
{
    return std::sqrt(
        static_cast<long double>(x) * static_cast<long double>(x) +
        static_cast<long double>(y) * static_cast<long double>(y) +
        static_cast<long double>(z) * static_cast<long double>(z)
    );
}

