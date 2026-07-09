#ifndef SOLARSCAPE_FITNESSEVALUATIONUTILS_H
#define SOLARSCAPE_FITNESSEVALUATIONUTILS_H

#include <algorithm>
#include <stdexcept>

#include "math/Real.h"
#include "math/Vector3.h"

namespace FitnessEvaluationUtils
{
    inline void validateTiming(Real simulationTime, Real timeStep)
    {
        if (simulationTime < 0.0)
        {
            throw std::invalid_argument("simulationTime must be non-negative");
        }

        if (timeStep <= 0.0)
        {
            throw std::invalid_argument("timeStep must be greater than zero");
        }
    }

    inline Real nextStepTime(Real currentTime, Real simulationTime, Real timeStep)
    {
        return std::min(timeStep, simulationTime - currentTime);
    }

    inline Real distanceToTargetPoint(const Vector3& probePosition, const Vector3& targetBodyPosition,
                                      const Vector3& targetPointFromTargetBody)
    {
        return (probePosition - (targetBodyPosition + targetPointFromTargetBody)).length();
    }

    inline bool isBetterMinimumDistance(Real currentDistance, bool hasMinimumDistance, Real minimumDistance)
    {
        return !hasMinimumDistance || currentDistance < minimumDistance;
    }

    inline Real fuelConstraintViolation(Real requestedFuelUse, Real initialProbeFuelMass)
    {
        return std::max(0.0, requestedFuelUse - initialProbeFuelMass);
    }
} // namespace FitnessEvaluationUtils

#endif
