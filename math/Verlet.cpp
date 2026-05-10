//
// Created by Luke on 5/7/2026.
//

#include "Verlet.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <ranges>
#include <stdexcept>

namespace Verlet
{
    auto calculateAccelerationForBody(
        const std::vector<Body*>& bodies,
        std::size_t bodyIndex,
        Real gravitationalConstant) -> Vector3
    {
        if (bodyIndex >= bodies.size() || bodies[bodyIndex] == nullptr)
        {
            throw std::invalid_argument("body pointer must not be null");
        }

        Vector3 acceleration;

        for (const std::size_t j : std::views::iota(std::size_t{0}, bodies.size()))
        {
            if (bodies[j] == nullptr)
            {
                throw std::invalid_argument("body pointer must not be null");
            }

            if (bodyIndex == j)
            {
                continue;
            }

            const Vector3 direction =
                bodies[j]->position() - bodies[bodyIndex]->position();

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
                bodies[j]->mass() /
                (distanceSquared * distance);

            acceleration += direction * factor;
        }

        return acceleration;
    }

    auto calculateAccelerations(
        const std::vector<Body*>& bodies,
        Real gravitationalConstant) -> std::vector<Vector3>
    {
        std::vector<Vector3> accelerations;
        accelerations.reserve(bodies.size());

        std::ranges::transform(
            std::views::iota(std::size_t{0}, bodies.size()),
            std::back_inserter(accelerations),
            [&bodies, gravitationalConstant](std::size_t i)
            {
                return calculateAccelerationForBody(
                    bodies,
                    i,
                    gravitationalConstant);
            });

        return accelerations;
    }

    void step(
        std::vector<Body*>& bodies,
        Probe* probe,
        Real throttleValue,
        const Vector3& thrustDirection,
        Real timeStep,
        Real gravitationalConstant)
    {
        if (probe == nullptr)
        {
            throw std::invalid_argument("probe must not be null");
        }

        if (std::ranges::any_of(
            bodies,
            [](const Body* body)
            {
                return body == nullptr;
            }))
        {
            throw std::invalid_argument("body pointer must not be null");
        }

        if (std::ranges::find(bodies, static_cast<Body*>(probe)) == bodies.end())
        {
            throw std::invalid_argument("probe must be part of bodies");
        }

        const std::vector<Vector3> previousAccelerations =
            calculateAccelerations(
                bodies,
                gravitationalConstant);

        const Real timeStepSquared =
            timeStep * timeStep;

        for (const std::size_t i : std::views::iota(std::size_t{0}, bodies.size()))
        {
            const Vector3 velocityPart =
                bodies[i]->velocity() * timeStep;

            const Vector3 accelerationPart =
                previousAccelerations[i] *
                (0.5L * timeStepSquared);

            bodies[i]->position() +=
                velocityPart + accelerationPart;
        }

        const std::vector<Vector3> nextAccelerations =
            calculateAccelerations(
                bodies,
                gravitationalConstant);

        for (const std::size_t i : std::views::iota(std::size_t{0}, bodies.size()))
        {
            Vector3 averageAcceleration =
            (previousAccelerations[i] +
                nextAccelerations[i]) * 0.5L;

            if (bodies[i] == probe)
            {
                Vector3 force {0,0,0};

                if (probe->fuelMass() > 0)
                    force =
                        thrustDirection *
                        throttleValue *
                        probe->fuelFlow() *
                        probe->specificImpulse();

                averageAcceleration += force / probe->mass();
            }

            bodies[i]->velocity() +=
                averageAcceleration * timeStep;
        }

        probe->setFuelMass(
            std::max(
                0.0L,
                probe->fuelMass() - probe->fuelFlow() * timeStep));
    }
}
