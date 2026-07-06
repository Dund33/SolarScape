#include "RandomUniformMutation.h"

#include <random>
#include <stdexcept>

#include "genetics/mutation/ManeuverMutationUtils.h"
#include "genetics/Specimen.h"

RandomUniformMutation::RandomUniformMutation(
    double mutationProbabilityValue,
    Real maxTimeOffsetValue,
    Real maxDurationOffsetValue,
    Real maxDirectionOffsetValue,
    Real maxThrottleOffsetValue
)
    : mutationProbability(mutationProbabilityValue),
      maxTimeOffset(maxTimeOffsetValue),
      maxDurationOffset(maxDurationOffsetValue),
      maxDirectionOffset(maxDirectionOffsetValue),
      maxThrottleOffset(maxThrottleOffsetValue)
{
    if (mutationProbabilityValue < 0.0 || mutationProbabilityValue > 1.0)
    {
        throw std::invalid_argument(
            "Mutation probability must be in range [0, 1]."
        );
    }
}

void RandomUniformMutation::mutate(Specimen& specimen) const
{
    if (specimen.empty())
    {
        return;
    }

    thread_local std::mt19937 rng(std::random_device{}());

    std::bernoulli_distribution shouldMutate(mutationProbability);

    std::uniform_real_distribution<Real> timeDelta(
        -maxTimeOffset,
         maxTimeOffset
    );

    std::uniform_real_distribution<Real> durationDelta(
        -maxDurationOffset,
         maxDurationOffset
    );

    std::uniform_real_distribution<Real> directionDelta(
        -maxDirectionOffset,
         maxDirectionOffset
    );

    std::uniform_real_distribution<Real> throttleDelta(
        -maxThrottleOffset,
         maxThrottleOffset
    );

    for (std::size_t i = 0; i < specimen.size(); ++i)
    {
        specimen[i] =
            ManeuverMutationUtils::mutateUniformly(
                specimen[i],
                shouldMutate,
                timeDelta,
                durationDelta,
                directionDelta,
                throttleDelta,
                rng);
    }

    specimen.clearFitness();
}
