#ifndef SOLARSCAPE_MANEUVERMUTATIONUTILS_H
#define SOLARSCAPE_MANEUVERMUTATIONUTILS_H

#include <algorithm>
#include <random>

#include "math/ProbeProperties.h"
#include "simulation/Maneuver.h"

namespace ManeuverMutationUtils
{
    inline Maneuver mutateUniformly(
        const Maneuver& maneuver,
        std::bernoulli_distribution& shouldMutate,
        std::uniform_real_distribution<Real>& timeDelta,
        std::uniform_real_distribution<Real>& durationDelta,
        std::uniform_real_distribution<Real>& thrustDelta,
        std::mt19937& rng,
        const ProbeProperties& probeProperties)
    {
        Vector3 throttleVector =
            maneuver.getThrustDirection() *
            maneuver.getThrottleValue();

        Real initDelay = maneuver.getInitDelay();
        Real duration = maneuver.getDuration();

        if (shouldMutate(rng))
        {
            initDelay += timeDelta(rng);
            initDelay = std::max(0.0L, initDelay);
        }

        if (shouldMutate(rng))
        {
            duration += durationDelta(rng);
            duration = std::max(0.0L, duration);
        }

        if (shouldMutate(rng))
        {
            const Real maxPhysicalThrust =
                probeProperties.fuelFlow() *
                probeProperties.specificImpulse();
            const Real thrustScale =
                maxPhysicalThrust > 0.0L
                    ? maxPhysicalThrust
                    : 1.0L;

            throttleVector.x += thrustDelta(rng) / thrustScale;
            throttleVector.y += thrustDelta(rng) / thrustScale;
            throttleVector.z += thrustDelta(rng) / thrustScale;
        }

        const Real throttleNorm =
            std::clamp(
                throttleVector.norm(),
                0.0L,
                1.0L);

        if (throttleNorm <= 0.0L)
        {
            return Maneuver(Vector3{}, 0.0L, initDelay, duration);
        }

        return Maneuver(
            throttleVector / throttleVector.norm(),
            throttleNorm,
            initDelay,
            duration);
    }
}

#endif
