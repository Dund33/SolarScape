#ifndef SOLARSCAPE_MANEUVERMUTATIONUTILS_H
#define SOLARSCAPE_MANEUVERMUTATIONUTILS_H

#include <algorithm>
#include <random>

#include "config/consts.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"

namespace ManeuverMutationUtils
{
    inline Vector3 validDirectionOrDefault(
        const Vector3& direction)
    {
        return direction.norm() > 0.0
            ? direction
            : Vector3(1.0, 0.0, 0.0);
    }

    inline Maneuver mutateUniformly(
        const Maneuver& maneuver,
        std::bernoulli_distribution& shouldMutate,
        std::uniform_real_distribution<Real>& timeDelta,
        std::uniform_real_distribution<Real>& durationDelta,
        std::uniform_real_distribution<Real>& directionDelta,
        std::uniform_real_distribution<Real>& throttleDelta,
        std::mt19937& rng)
    {
        Vector3 thrustDirection = maneuver.getThrustDirection();
        Real throttleValue = maneuver.getThrottleValue();
        Real initDelay = maneuver.getInitDelay();
        Real duration = maneuver.getDuration();

        if (shouldMutate(rng))
        {
            initDelay += timeDelta(rng);
            initDelay = std::max(0.0, initDelay);
        }

        if (shouldMutate(rng))
        {
            duration += durationDelta(rng);
            duration = std::max(MIN_MANEUVER_DURATION, duration);
        }

        if (shouldMutate(rng))
        {
            thrustDirection.x += directionDelta(rng);
            thrustDirection.y += directionDelta(rng);
            thrustDirection.z += directionDelta(rng);
        }

        if (shouldMutate(rng))
        {
            throttleValue = std::clamp(
                throttleValue + throttleDelta(rng),
                MIN_MANEUVER_THROTTLE,
                1.0);
        }

        return Maneuver(
            validDirectionOrDefault(
                thrustDirection),
            std::clamp(
                throttleValue,
                MIN_MANEUVER_THROTTLE,
                1.0),
            initDelay,
            duration);
    }
}

#endif
