//
// Created by Luke on 5/10/2026.
//

#include "Mutation.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <ranges>
#include <stdexcept>

Mutation::Mutation(
    double mutationProbability,
    long double maxTimeOffset,
    long double maxDurationOffset,
    long double maxThrustOffset
)
    : mutationProbability(mutationProbability),
      maxTimeOffset(maxTimeOffset),
      maxDurationOffset(maxDurationOffset),
      maxThrustOffset(maxThrustOffset)
{
    if (mutationProbability < 0.0 || mutationProbability > 1.0)
    {
        throw std::invalid_argument(
            "Mutation probability must be in range [0, 1]."
        );
    }
}

void Mutation::mutate(Specimen& specimen) const
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

    for (std::size_t i = 0; i < specimen.size(); ++i)
    {
        Maneuver& maneuver = specimen[i];
        const Probe* probe = specimen.getProbe();
        if (probe == nullptr)
        {
            throw std::invalid_argument("probe must not be null.");
        }

        const long double maxPhysicalThrust =
            probe->fuelFlow() * probe->specificImpulse();
        Vector3 throttleVector =
            maneuver.getThrustDirection() *
            maneuver.getThrottleValue();

        long double initTime = maneuver.getInitTime();
        long double duration = maneuver.getDuration();

        if (shouldMutate(rng))
        {
            initTime += timeDelta(rng);
            initTime = std::max(static_cast<long double>(0.0), initTime);
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
            maneuver = Maneuver(Vector3{}, 0.0L, initTime, duration);
            continue;
        }

        maneuver = Maneuver(
            throttleVector / throttleVector.norm(),
            throttleNorm,
            initTime,
            duration);
    }

    specimen.clearFitness();
}
