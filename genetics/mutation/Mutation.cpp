//
// Created by Luke on 5/10/2026.
//

#include "Mutation.h"

#include <algorithm>
#include <random>
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

    static thread_local std::mt19937 rng(std::random_device{}());

    std::bernoulli_distribution shouldMutate(mutationProbability);

    std::uniform_real_distribution<long double> timeDelta(
        -maxTimeOffset,
         maxTimeOffset
    );

    std::uniform_real_distribution<long double> durationDelta(
        -maxDurationOffset,
         maxDurationOffset
    );

    std::uniform_real_distribution<long double> thrustDelta(
        -maxThrustOffset,
         maxThrustOffset
    );

    for (std::size_t i = 0; i < specimen.size(); ++i)
    {
        Maneuver& maneuver = specimen[i];

        Vector3 thrust = maneuver.getThrust();
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
            thrust.x += thrustDelta(rng);
            thrust.y += thrustDelta(rng);
            thrust.z += thrustDelta(rng);
        }

        maneuver = Maneuver(thrust, initTime, duration);
    }
}