//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_BODY_H
#define SOLARSCAPE_BODY_H

#include "Vector3.h"

class Body
{
public:
    Vector3 position;
    Vector3 velocity;
    double mass;

    Body();
    Body(const Vector3& position, const Vector3& velocity, double mass);
};


#endif //SOLARSCAPE_BODY_H
