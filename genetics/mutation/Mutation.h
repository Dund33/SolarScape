//
// Created by Luke on 5/10/2026.
//

#ifndef SOLARSCAPE_MUTATION_H
#define SOLARSCAPE_MUTATION_H

#include "../Specimen.h"

class Mutation
{
public:
    Mutation(
        double mutationProbability,
        long double maxTimeOffset,
        long double maxDurationOffset,
        long double maxThrustOffset
    );

    void mutate(Specimen& specimen) const;

private:
    double mutationProbability;

    long double maxTimeOffset;
    long double maxDurationOffset;
    long double maxThrustOffset;
};

#endif // SOLARSCAPE_MUTATION_H