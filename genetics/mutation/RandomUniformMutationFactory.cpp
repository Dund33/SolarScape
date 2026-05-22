#include "RandomUniformMutationFactory.h"

#include <memory>

#include "genetics/mutation/RandomUniformMutation.h"

RandomUniformMutationFactory::RandomUniformMutationFactory(
    double mutationProbability,
    Real maxTimeOffset,
    Real maxDurationOffset,
    Real maxThrustOffset,
    const ProbeProperties& probeProperties,
    const RepairFactory& repairFactory
)
    : MutationFactory(repairFactory),
      mutationProbability(mutationProbability),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxThrustOffset(maxThrustOffset),
      probeProperties(probeProperties)
{
}

std::unique_ptr<Mutation> RandomUniformMutationFactory::create() const
{
    return std::make_unique<RandomUniformMutation>(
        mutationProbability,
        maxTimeOffset,
        maxDurationOffset,
        maxThrustOffset,
        probeProperties);
}
