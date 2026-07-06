#ifndef SOLARSCAPE_REFINEMENT_H
#define SOLARSCAPE_REFINEMENT_H

#include "config/consts.h"
#include "genetics/fitness/FitnessMetrics.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

namespace Refinement
{
    inline bool closeToTarget(
        const FitnessValue& fitness)
    {
        return targetWindowViolation(fitness) <=
            ALGO_CLOSE_TO_TARGET_WINDOW_VIOLATION;
    }

    inline Real mutationScale(
        bool closeToTargetValue)
    {
        return closeToTargetValue
            ? ALGO_CLOSE_TO_TARGET_MUTATION_SCALE
            : 1.0;
    }
}

#endif
