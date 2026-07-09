#ifndef SOLARSCAPE_RANDOMUNIFORMMUTATIONFACTORY_H
#define SOLARSCAPE_RANDOMUNIFORMMUTATIONFACTORY_H

#include "genetics/mutation/MutationFactory.h"
#include "math/Real.h"

class RandomUniformMutationFactory final : public MutationFactory
{
public:
    RandomUniformMutationFactory(double mutationProbability, Real maxTimeOffset, Real maxDurationOffset, Real maxDirectionOffset,
                                 Real maxThrottleOffset);

    std::unique_ptr<Mutation> create() const override;

private:
    double mutationProbability;
    Real maxTimeOffset;
    Real maxDurationOffset;
    Real maxDirectionOffset;
    Real maxThrottleOffset;
};

#endif
