#include "ExtensiveMutationFactory.h"

#include <memory>

#include "genetics/mutation/ExtensiveMutation.h"

ExtensiveMutationFactory::ExtensiveMutationFactory(
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
    Real maxThrottleOffset)
    : mutationProbability(mutationProbability),
      addProbability(addProbability),
      removeProbability(removeProbability),
      minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitDelay(minInitDelay),
      maxInitDelay(maxInitDelay),
      minDuration(minDuration),
      maxDuration(maxDuration),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxDirectionOffset(maxDirectionOffset),
      maxThrottleOffset(maxThrottleOffset)
{
}

std::unique_ptr<Mutation> ExtensiveMutationFactory::create() const
{
    return std::make_unique<ExtensiveMutation>(
        mutationProbability,
        addProbability,
        removeProbability,
        minManeuvers,
        maxManeuvers,
        minInitDelay,
        maxInitDelay,
        minDuration,
        maxDuration,
        maxTimeOffset,
        maxDurationOffset,
        maxDirectionOffset,
        maxThrottleOffset);
}
