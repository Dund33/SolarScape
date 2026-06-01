#ifndef SOLARSCAPE_EXTENSIVEMUTATIONFACTORY_H
#define SOLARSCAPE_EXTENSIVEMUTATIONFACTORY_H

#include <cstddef>

#include "genetics/mutation/MutationFactory.h"
#include "math/ProbeProperties.h"
#include "math/Real.h"

class ExtensiveMutationFactory final : public MutationFactory
{
public:
    ExtensiveMutationFactory(
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
        Real maxThrustOffset,
        const ProbeProperties& probeProperties);

    std::unique_ptr<Mutation> create() const override;

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
    Real maxThrustOffset;
    ProbeProperties probeProperties;
};

#endif
