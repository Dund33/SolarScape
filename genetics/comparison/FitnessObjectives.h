#ifndef SOLARSCAPE_FITNESSOBJECTIVES_H
#define SOLARSCAPE_FITNESSOBJECTIVES_H

#include <array>
#include <cstddef>

#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

namespace fitnessObjectives
{
    inline constexpr std::size_t objectiveCount = 1;
    inline constexpr std::size_t tieBreakValueCount = 4;

    inline bool satisfiesFuelConstraint(
        const FitnessValue& fitness)
    {
        return fitness.fuelConstraintViolation <= 0.0L;
    }

    inline Real fuelConstraintViolation(
        const FitnessValue& fitness)
    {
        return fitness.fuelConstraintViolation;
    }

    inline std::array<Real, objectiveCount> comparableValues(
        const FitnessValue& fitness)
    {
        return {
            fitness.minimumDistance};
    }

    inline std::array<Real, tieBreakValueCount> tieBreakValues(
        const FitnessValue& fitness)
    {
        return {
            fitness.fuelConstraintViolation,
            fitness.minimumDistance,
            fitness.minimumDistanceTime,
            -fitness.minimumDistanceFuelMass};
    }
}

#endif
