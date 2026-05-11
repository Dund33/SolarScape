//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_MANEUVER_H
#define SOLARSCAPE_MANEUVER_H
#include "math/Vector3.h"


class Maneuver
{
public:
    Maneuver(
        const Vector3& thrustDirection,
        Real thrustValue,
        long double initTime,
        long double duration)
        : thrustDirection(normalized(thrustDirection)),
          throttleValue(thrustValue),
          initTime(initTime),
          duration(duration)
    {}

    const Vector3& getThrustDirection() const { return thrustDirection; }
    Real getThrottleValue() const { return throttleValue; }
    long double getInitTime() const { return initTime; }
    long double getDuration() const { return duration; }

private:
    static Vector3 normalized(const Vector3& vector)
    {
        const Real length = vector.length();

        if (length <= 0.0L)
        {
            return {};
        }

        return vector / length;
    }

    Vector3 thrustDirection;
    Real throttleValue;
    long double initTime;
    long double duration;
};


#endif //SOLARSCAPE_MANEUVER_H
