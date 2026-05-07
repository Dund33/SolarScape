//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_BODY_H
#define SOLARSCAPE_BODY_H


struct Vector3
{
    double x;
    double y;
    double z;

    Vector3();
    Vector3(double x, double y, double z);
};

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
