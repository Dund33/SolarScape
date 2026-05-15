#ifndef SOLARSCAPE_RANDOMUNIFORMMUTATION_H
#define SOLARSCAPE_RANDOMUNIFORMMUTATION_H

#include "genetics/mutation/Mutation.h"

class RandomUniformMutation final : public Mutation
{
public:
    RandomUniformMutation(
        double mutationProbability,
        long double maxTimeOffset,
        long double maxDurationOffset,
        long double maxThrustOffset
    );

    void mutate(Specimen& specimen) const override;

private:
    double mutationProbability;

    long double maxTimeOffset;
    long double maxDurationOffset;
    long double maxThrustOffset;
};

#endif // SOLARSCAPE_RANDOMUNIFORMMUTATION_H
