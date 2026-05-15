#ifndef SOLARSCAPE_FITNESSEVALUATORFACTORY_H
#define SOLARSCAPE_FITNESSEVALUATORFACTORY_H

#include <memory>

#include "genetics/fitness/FitnessEvaluator.h"

class FitnessEvaluatorFactory
{
public:
    virtual ~FitnessEvaluatorFactory() = default;

    virtual std::unique_ptr<FitnessEvaluator> create() const = 0;
};

#endif
