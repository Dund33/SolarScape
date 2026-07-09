#ifndef SOLARSCAPE_REFERENCE_DIRECTIONS_H
#define SOLARSCAPE_REFERENCE_DIRECTIONS_H

#include <cstddef>
#include <vector>

#include "math/Real.h"

namespace ReferenceDirections
{
    using Direction = std::vector<Real>;

    std::vector<Direction> generate(std::size_t directionCount, std::size_t objectiveCount);
}

#endif
