#ifndef SOLARSCAPE_MANEUVER_H
#define SOLARSCAPE_MANEUVER_H

#include "math/Vector3.h"

class Maneuver
{
public:
    Maneuver(
        const Vector3& thrustDirectionValue,
        Real thrustValue,
        Real initDelayValue,
        Real durationValue)
        : thrustDirection(normalized(thrustDirectionValue)),
          throttleValue(thrustValue),
          initDelay(initDelayValue),
          duration(durationValue)
    {}

    const Vector3& getThrustDirection() const { return thrustDirection; }
    Real getThrottleValue() const { return throttleValue; }
    Real getInitDelay() const { return initDelay; }
    Real getDuration() const { return duration; }

private:
    static Vector3 normalized(const Vector3& vector)
    {
        const Real length = vector.length();

        if (length <= 0.0)
        {
            return {};
        }

        return vector / length;
    }

    Vector3 thrustDirection;
    Real throttleValue;
    Real initDelay;
    Real duration;
};


#endif
