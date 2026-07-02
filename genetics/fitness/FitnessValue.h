#ifndef SOLARSCAPE_FITNESSVALUE_H
#define SOLARSCAPE_FITNESSVALUE_H

#include "math/Real.h"

struct FitnessValue
{
    Real minimumDistance{};
    Real minimumDistanceTime{};
    Real fuelUsed{};
    Real fuelConstraintViolation{};
};

#endif
