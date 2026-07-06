#ifndef SOLARSCAPE_EXTENSIVEMUTATION_H
#define SOLARSCAPE_EXTENSIVEMUTATION_H

#include <cstddef>

#include "genetics/mutation/Mutation.h"
#include "math/Real.h"

class ExtensiveMutation final : public Mutation
{
public:
    ExtensiveMutation(
        double mutationProbability,
        double addProbability,
        double removeProbability,
        std::size_t minManeuvers,
        std::size_t maxManeuvers,
        Real minInitDelay,
        Real maxInitDelay,
        Real minDuration,
        Real maxDuration,
        Real maxTimeOffset,
        Real maxDurationOffset,
        Real maxDirectionOffset,
        Real maxThrottleOffset);

    void mutate(
        Specimen& specimen,
        bool closeToTarget = false) const override;

private:
    double mutationProbability;
    double addProbability;
    double removeProbability;
    std::size_t minManeuvers;
    std::size_t maxManeuvers;
    Real minInitDelay;
    Real maxInitDelay;
    Real minDuration;
    Real maxDuration;
    Real maxTimeOffset;
    Real maxDurationOffset;
    Real maxDirectionOffset;
    Real maxThrottleOffset;
};

#endif
