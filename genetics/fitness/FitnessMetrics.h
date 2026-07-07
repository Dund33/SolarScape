#ifndef SOLARSCAPE_FITNESSMETRICS_H
#define SOLARSCAPE_FITNESSMETRICS_H

#include <algorithm>

#include "config/consts.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

inline Real targetWindowViolation(const FitnessValue& fitness)
{
    return std::max(0.0, fitness.minimumDistance - TARGET_WINDOW_DISTANCE);
}

#endif
