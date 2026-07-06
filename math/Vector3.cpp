#include "Vector3.h"

#include <cmath>

Vector3::Vector3() : x(0.0), y(0.0), z(0.0)
{
}

Vector3::Vector3(Real x, Real y, Real z) : x(x), y(y), z(z)
{
}

Real Vector3::norm() const
{
    return std::sqrt(
        static_cast<Real>(x) * static_cast<Real>(x) +
        static_cast<Real>(y) * static_cast<Real>(y) +
        static_cast<Real>(z) * static_cast<Real>(z)
    );
}

