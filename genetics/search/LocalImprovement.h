#ifndef SOLARSCAPE_LOCALIMPROVEMENT_H
#define SOLARSCAPE_LOCALIMPROVEMENT_H

#include "genetics/Specimen.h"
#include "genetics/fitness/FitnessEvaluator.h"

class SpecimenComparator;

class LocalImprovement
{
public:
    virtual ~LocalImprovement() = default;

    virtual void improve(
        Specimen& specimen,
        const FitnessEvaluator& fitnessEvaluator,
        const SpecimenComparator& specimenComparator) const = 0;
};

#endif
