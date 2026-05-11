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

        for (std::size_t j = 0; j < bodies.size(); ++j)
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

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            accelerations.push_back(
                calculateAccelerationForBody(
                    bodies,
                    i,
                    gravitationalConstant));
        }

        return accelerations;
    }

    void step(
        std::vector<Body*>& bodies,
        Probe& probe,
        const std::optional<Maneuver>& maneuver,
        Real timeStep,
        Real gravitationalConstant)
    {
        if (std::ranges::any_of(
            bodies,
            [](const Body* body)
            {
                return body == nullptr;
            }))
        {
            throw std::invalid_argument("body pointer must not be null");
        }

        const std::vector<Vector3> previousAccelerations =
            calculateAccelerations(
                bodies,
                gravitationalConstant);

        const Real timeStepSquared =
            timeStep * timeStep;

        for (std::size_t i = 0; i < bodies.size(); ++i)
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

        for (std::size_t i = 0; i < bodies.size(); ++i)
        {
            Vector3 averageAcceleration =
            (previousAccelerations[i] +
                nextAccelerations[i]) * 0.5L;

            if (bodies[i] == &probe)
            {
                if (probe.fuelMass() > 0 && maneuver.has_value())
                {
                    const Real throttleValue =
                        std::clamp(
                            maneuver.value().getThrottleValue(),
                            0.0L,
                            1.0L);
                    const auto thrustDirection = maneuver.value().getThrustDirection();

                    const Real fuelNeeded =
                        probe.fuelFlow() * throttleValue * timeStep;
                    const Real fuelScale =
                        fuelNeeded > 0.0L
                            ? std::min(1.0L, probe.fuelMass() / fuelNeeded)
                            : 0.0L;
                    const Real effectiveThrottle = throttleValue * fuelScale;

                    Vector3 force =
                        thrustDirection *
                        effectiveThrottle *
                        probe.fuelFlow() *
                        probe.specificImpulse();

                    averageAcceleration += force / probe.mass();
                }
            }

            bodies[i]->velocity() +=
                averageAcceleration * timeStep;
        }

        if (maneuver.has_value())
        {
            const Real throttleValue =
                std::clamp(
                    maneuver.value().getThrottleValue(),
                    0.0L,
                    1.0L);

            probe.setFuelMass(
                std::max(
                    0.0L,
                    probe.fuelMass() -
                    probe.fuelFlow() * throttleValue * timeStep));
        }
    }
}
