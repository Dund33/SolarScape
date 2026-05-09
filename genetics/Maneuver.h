//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_MANEUVER_H
#define SOLARSCAPE_MANEUVER_H
#include "math/Vector3.h"


class Maneuver
{
public:
    Maneuver(const Vector3& thrust, long double initTime, long double duration)
        : thrust(thrust), initTime(initTime), duration(duration)
    {}

    const Vector3& getThrust() const { return thrust; }
    long double getInitTime() const { return initTime; }
    long double getDuration() const { return duration; }

private:
    Vector3 thrust;
    long double initTime;
    long double duration;
};


#endif //SOLARSCAPE_MANEUVER_H
