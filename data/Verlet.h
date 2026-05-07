//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_VERLET_H
#define SOLARSCAPE_VERLET_H

#include <cmath>
#include <vector>

#include "Body.h"

namespace Verlet
{
    inline Vector3 add(const Vector3& left, const Vector3& right)
    {
        return Vector3(left.x + right.x, left.y + right.y, left.z + right.z);
    }

    inline Vector3 subtract(const Vector3& left, const Vector3& right)
    {
        return Vector3(left.x - right.x, left.y - right.y, left.z - right.z);
    }

    inline Vector3 multiply(const Vector3& vector, double scalar)
    {
        return Vector3(vector.x * scalar, vector.y * scalar, vector.z * scalar);
    }

    inline double lengthSquared(const Vector3& vector)
    {
        return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
    }

    inline std::vector<Vector3> calculateAccelerations(const std::vector<Body>& bodies, double gravitationalConstant)
    {
        std::vector<Vector3> accelerations(bodies.size());

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            Vector3 acceleration;

            for (std::size_t j = 0; j < bodies.size(); ++j)
            {
                if (i == j)
                {
                    continue;
                }

                const Vector3 direction = subtract(bodies[j].position, bodies[i].position);
                const double distanceSquared = lengthSquared(direction);

                if (distanceSquared == 0.0)
                {
                    continue;
                }

                const double distance = std::sqrt(distanceSquared);
                const double factor = gravitationalConstant * bodies[j].mass / (distanceSquared * distance);
                acceleration = add(acceleration, multiply(direction, factor));
            }

            accelerations[i] = acceleration;
        }

        return accelerations;
    }

    inline void step(std::vector<Body>& bodies, double timeStep, double gravitationalConstant)
    {
        const std::vector<Vector3> previousAccelerations = calculateAccelerations(bodies, gravitationalConstant);
        const double timeStepSquared = timeStep * timeStep;

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 velocityPart = multiply(bodies[i].velocity, timeStep);
            const Vector3 accelerationPart = multiply(previousAccelerations[i], 0.5 * timeStepSquared);
            bodies[i].position = add(bodies[i].position, add(velocityPart, accelerationPart));
        }

        const std::vector<Vector3> nextAccelerations = calculateAccelerations(bodies, gravitationalConstant);

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 averageAcceleration = multiply(add(previousAccelerations[i], nextAccelerations[i]), 0.5);
            bodies[i].velocity = add(bodies[i].velocity, multiply(averageAcceleration, timeStep));
        }
    }
}

#endif //SOLARSCAPE_VERLET_H
