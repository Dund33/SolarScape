#include "RandomUniformMutation.h"

#include <algorithm>
#include <random>
#include <stdexcept>

#include "genetics/Specimen.h"

RandomUniformMutation::RandomUniformMutation(
    double mutationProbability,
    long double maxTimeOffset,
    long double maxDurationOffset,
    long double maxThrustOffset,
    const ProbeProperties& probeProperties
)
    : mutationProbability(mutationProbability),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxThrustOffset(maxThrustOffset),
      probeProperties(probeProperties)
{
    if (mutationProbability < 0.0 || mutationProbability > 1.0)
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

    std::uniform_real_distribution timeDelta(
        -maxTimeOffset,
         maxTimeOffset
    );

    std::uniform_real_distribution durationDelta(
        -maxDurationOffset,
         maxDurationOffset
    );

    std::uniform_real_distribution thrustDelta(
        -maxThrustOffset,
         maxThrustOffset
    );

    const long double maxPhysicalThrust =
        probeProperties.fuelFlow() * probeProperties.specificImpulse();

    for (std::size_t i = 0; i < specimen.size(); ++i)
    {
        Maneuver& maneuver = specimen[i];
        Vector3 throttleVector =
            maneuver.getThrustDirection() *
            maneuver.getThrottleValue();

        long double initDelay = maneuver.getInitDelay();
        long double duration = maneuver.getDuration();

        if (shouldMutate(rng))
        {
            initDelay += timeDelta(rng);
            initDelay = std::max(static_cast<long double>(0.0), initDelay);
        }

        if (shouldMutate(rng))
        {
            duration += durationDelta(rng);
            duration = std::max(static_cast<long double>(0.0), duration);
        }

        if (shouldMutate(rng))
        {
            const long double thrustScale =
                maxPhysicalThrust > 0.0L
                    ? maxPhysicalThrust
                    : 1.0L;

            throttleVector.x += thrustDelta(rng) / thrustScale;
            throttleVector.y += thrustDelta(rng) / thrustScale;
            throttleVector.z += thrustDelta(rng) / thrustScale;
        }

        const long double throttleNorm =
            std::clamp(
                throttleVector.norm(),
                0.0L,
                1.0L);

        if (throttleNorm <= 0.0L)
        {
            maneuver = Maneuver(Vector3{}, 0.0L, initDelay, duration);
            continue;
        }

        maneuver = Maneuver(
            throttleVector / throttleVector.norm(),
            throttleNorm,
            initDelay,
            duration);
    }

    specimen.clearFitness();
}
