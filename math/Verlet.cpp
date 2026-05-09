//
// Created by Luke on 5/7/2026.
//

#include "Verlet.h"

#include <cmath>

namespace Verlet
{
    auto calculateAccelerationForBody(
        const std::vector<Body>& bodies,
        std::size_t bodyIndex,
        Real gravitationalConstant) -> Vector3
    {
        Vector3 acceleration;

        for (std::size_t j = 0; j < bodies.size(); ++j)
        {
            if (bodyIndex == j)
            {
                continue;
            }

            const Vector3 direction =
                bodies[j].position - bodies[bodyIndex].position;

            const Real distanceSquared =
                direction.lengthSquared();

            if (distanceSquared == 0.0L)
            {
                continue;
            }

            const Real distance =
                std::sqrt(distanceSquared);

            const Real factor =
                gravitationalConstant *
                bodies[j].mass /
                (distanceSquared * distance);

            acceleration += direction * factor;
        }

        return acceleration;
    }

    auto calculateAccelerations(
        const std::vector<Body>& bodies,
        Real gravitationalConstant) -> std::vector<Vector3>
    {
        std::vector<Vector3> accelerations(bodies.size());

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            accelerations[i] =
                calculateAccelerationForBody(
                    bodies,
                    i,
                    gravitationalConstant);
        }

        return accelerations;
    }

    void step(
        std::vector<Body>& bodies,
        Real timeStep,
        Real gravitationalConstant)
    {
        const std::vector<Vector3> previousAccelerations =
            calculateAccelerations(
                bodies,
                gravitationalConstant);

        const Real timeStepSquared =
            timeStep * timeStep;

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 velocityPart =
                bodies[i].velocity * timeStep;

            const Vector3 accelerationPart =
                previousAccelerations[i] *
                (0.5L * timeStepSquared);

            bodies[i].position +=
                velocityPart + accelerationPart;
        }

        const std::vector<Vector3> nextAccelerations =
            calculateAccelerations(
                bodies,
                gravitationalConstant);

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            const Vector3 averageAcceleration =
            (previousAccelerations[i] +
                nextAccelerations[i]) * 0.5L;

            bodies[i].velocity +=
                averageAcceleration * timeStep;
        }
    }
}