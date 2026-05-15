#ifndef SOLARSCAPE_FITNESSEVALUATOR_H
#define SOLARSCAPE_FITNESSEVALUATOR_H

#include "genetics/Specimen.h"

class FitnessEvaluator
{
public:
    virtual ~FitnessEvaluator() = default;

    virtual void evaluate(Specimen& specimen) const = 0;
};

#endif
