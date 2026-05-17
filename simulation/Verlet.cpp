#include "simulation/Verlet.h"
#include "config/consts.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    Vector3 calculateManeuverAcceleration(
        const Probe& probe,
        const std::optional<Maneuver>& maneuver,
        Real timeStep)
    {
        if (probe.fuelMass() <= 0.0L || !maneuver.has_value())
        {
            return {};
        }

        const Maneuver& activeManeuver = maneuver.value();
        const Real throttleValue =
            std::clamp(
                activeManeuver.getThrottleValue(),
                0.0L,
                1.0L);

        const Real fuelNeeded =
            probe.fuelFlow() * throttleValue * timeStep;
        const Real fuelScale =
            fuelNeeded > 0.0L
                ? std::min(1.0L, probe.fuelMass() / fuelNeeded)
                : 0.0L;
        const Real effectiveThrottle = throttleValue * fuelScale;

        const Vector3 force =
            activeManeuver.getThrustDirection() *
            effectiveThrottle *
            probe.fuelFlow() *
            probe.specificImpulse() *
            STANDARD_GRAVITY;

        return force / probe.mass();
    }
}

Verlet::Verlet(
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe)
    : Simulation(
        std::move(bodies),
        std::move(targetBody),
        std::move(probe))
{
}

auto Verlet::calculateAccelerationForBody(
    const std::vector<Body*>& bodies,
    std::size_t bodyIndex,
    Real gravitationalConstant) -> Vector3
{
    Vector3 acceleration;
    const Body& body = *bodies[bodyIndex];

    for (const Body* otherBody : bodies)
    {
        if (otherBody == &body)
        {
            continue;
        }

        const Vector3 direction =
            otherBody->position() - body.position();

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
            otherBody->mass() /
            (distanceSquared * distance);

        acceleration += direction * factor;
    }

    return acceleration;
}

auto Verlet::calculateAccelerations(
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

void Verlet::step(
    const std::optional<Maneuver>& maneuver,
    Real timeStep,
    Real gravitationalConstant)
{
    std::vector<Body*> bodyPointers;
    bodyPointers.reserve(mutableBodies().size() + 1);

    for (Body& body : mutableBodies())
    {
        bodyPointers.push_back(&body);
    }

    Probe& simulationProbe = mutableProbe();
    bodyPointers.push_back(&simulationProbe);

    const std::vector<Vector3> previousAccelerations =
        calculateAccelerations(
            bodyPointers,
            gravitationalConstant);

    const Real timeStepSquared =
        timeStep * timeStep;

    for (std::size_t i = 0; i < bodyPointers.size(); ++i)
    {
        const Vector3 velocityPart =
            bodyPointers[i]->velocity() * timeStep;

        const Vector3 accelerationPart =
            previousAccelerations[i] *
            (0.5L * timeStepSquared);

        bodyPointers[i]->position() +=
            velocityPart + accelerationPart;
    }

    const std::vector<Vector3> nextAccelerations =
        calculateAccelerations(
            bodyPointers,
            gravitationalConstant);

    const Vector3 maneuverAcceleration =
        calculateManeuverAcceleration(
            simulationProbe,
            maneuver,
            timeStep);

    for (std::size_t i = 0; i < bodyPointers.size(); ++i)
    {
        Vector3 averageAcceleration =
            (previousAccelerations[i] +
                nextAccelerations[i]) * 0.5L;

        bodyPointers[i]->velocity() +=
            averageAcceleration * timeStep;
    }

    simulationProbe.velocity() += maneuverAcceleration * timeStep;

    if (maneuver.has_value())
    {
        const Real throttleValue =
            std::clamp(
                maneuver.value().getThrottleValue(),
                0.0L,
                1.0L);

        simulationProbe.setFuelMass(
            std::max(
                0.0L,
                simulationProbe.fuelMass() -
                simulationProbe.fuelFlow() * throttleValue * timeStep));
    }
}
