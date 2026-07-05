#include "RandomUniformMutationFactory.h"

#include <memory>

#include "genetics/mutation/RandomUniformMutation.h"

RandomUniformMutationFactory::RandomUniformMutationFactory(
    double mutationProbability,
    Real maxTimeOffset,
    Real maxDurationOffset,
    Real maxDirectionOffset,
    Real maxThrottleOffset
)
    : mutationProbability(mutationProbability),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxDirectionOffset(maxDirectionOffset),
      maxThrottleOffset(maxThrottleOffset)
{
}

std::unique_ptr<Mutation> RandomUniformMutationFactory::create() const
{
    return std::make_unique<RandomUniformMutation>(
        mutationProbability,
        maxTimeOffset,
        maxDurationOffset,
        maxDirectionOffset,
        maxThrottleOffset);
}
