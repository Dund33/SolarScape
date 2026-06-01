#ifndef SOLARSCAPE_FITNESSOBJECTIVES_H
#define SOLARSCAPE_FITNESSOBJECTIVES_H

#include <array>
#include <cstddef>

#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

namespace fitnessObjectives
{
    inline constexpr std::size_t objectiveCount = 3;

    inline std::array<Real, objectiveCount> comparableValues(
        const FitnessValue& fitness)
    {
        return {
            fitness.minimumDistance,
            fitness.minimumDistanceTime,
            -fitness.minimumDistanceFuelMass};
    }
}

#endif
