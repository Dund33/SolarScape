#ifndef SOLARSCAPE_RANDOMUNIFORMMUTATION_H
#define SOLARSCAPE_RANDOMUNIFORMMUTATION_H

#include "genetics/mutation/Mutation.h"
#include "math/Real.h"

class RandomUniformMutation final : public Mutation
{
public:
    RandomUniformMutation(double mutationProbability, Real maxTimeOffset, Real maxDurationOffset, Real maxDirectionOffset,
                          Real maxThrottleOffset);

    void mutate(Specimen& specimen, bool closeToTarget = false) const override;

private:
    double mutationProbability;

    Real maxTimeOffset;
    Real maxDurationOffset;
    Real maxDirectionOffset;
    Real maxThrottleOffset;
};

#endif
