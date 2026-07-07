#include "simulation/Verlet.h"
#include "config/consts.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace
{
    std::optional<Maneuver> activeManeuverAt(const std::vector<Maneuver>& maneuvers, Real time)
    {
        Real previousManeuverEndTime = 0.0;

        for (const Maneuver& maneuver : maneuvers)
        {
            const Real maneuverStartTime = previousManeuverEndTime + maneuver.getInitDelay();
            const Real maneuverEndTime = maneuverStartTime + maneuver.getDuration();

            if (maneuverStartTime <= time && time < maneuverEndTime)
            {
                return maneuver;
            }

            previousManeuverEndTime = maneuverEndTime;
        }

        return std::nullopt;
    }

    Vector3 calculateManeuverAcceleration(const Probe& probe, const std::optional<Maneuver>& maneuver, Real timeStep)
    {
        if (probe.fuelMass() <= 0.0 || !maneuver.has_value())
        {
            return {};
        }

        const Maneuver& activeManeuver = maneuver.value();
        const Real throttleValue = std::clamp(activeManeuver.getThrottleValue(), 0.0, 1.0);

        const Real fuelNeeded = probe.fuelFlow() * throttleValue * timeStep;
        const Real fuelScale = fuelNeeded > 0.0 ? std::min(1.0, probe.fuelMass() / fuelNeeded) : 0.0;
        const Real effectiveThrottle = throttleValue * fuelScale;

        const Vector3 force =
            activeManeuver.getThrustDirection() * effectiveThrottle * probe.fuelFlow() * probe.specificImpulse() * STANDARD_GRAVITY;

        return force / probe.mass();
    }
} // namespace

Verlet::Verlet(std::vector<Body> bodies, Body targetBody, Probe probe, std::vector<Maneuver> maneuvers, Real gravitationalConstant)
    : Simulation(std::move(bodies), std::move(targetBody), std::move(probe), std::move(maneuvers), gravitationalConstant)
{
    const std::size_t reserveSize = mutableBodies().size() + 1;
    bodyPointers_.reserve(reserveSize);
    previousAccelerations_.reserve(reserveSize);
    nextAccelerations_.reserve(reserveSize);
}

auto Verlet::calculateAccelerationForBody(const std::vector<Body*>& bodies, std::size_t bodyIndex, Real gravitationalConstant) -> Vector3
{
    Vector3 acceleration;
    const Body& body = *bodies[bodyIndex];

    for (const Body* otherBody : bodies)
    {
        if (otherBody == &body)
        {
            continue;
        }

        const Vector3 direction = otherBody->position() - body.position();

        const Real distanceSquared = direction.lengthSquared();

        if (distanceSquared == 0.0)
        {
            continue;
        }

        const Real distance = std::sqrt(distanceSquared);

        const Real factor = gravitationalConstant * otherBody->mass() / (distanceSquared * distance);

        acceleration += direction * factor;
    }

    return acceleration;
}

auto Verlet::calculateAccelerations(const std::vector<Body*>& bodies, Real gravitationalConstant, std::vector<Vector3>& accelerations)
    -> void
{
    accelerations.resize(bodies.size());

    for (std::size_t i = 0; i < bodies.size(); ++i)
    {
        accelerations[i] = calculateAccelerationForBody(bodies, i, gravitationalConstant);
    }
}

void Verlet::step(Real timeStep)
{
    const auto maneuver = activeManeuverAt(maneuvers(), currentTime());

    bodyPointers_.clear();
    bodyPointers_.reserve(mutableBodies().size() + 1);

    for (Body& body : mutableBodies())
    {
        bodyPointers_.push_back(&body);
    }

    Probe& simulationProbe = mutableProbe();
    bodyPointers_.push_back(&simulationProbe);

    calculateAccelerations(bodyPointers_, gravitationalConstant(), previousAccelerations_);

    const Real timeStepSquared = timeStep * timeStep;

    for (std::size_t i = 0; i < bodyPointers_.size(); ++i)
    {
        const Vector3 velocityPart = bodyPointers_[i]->velocity() * timeStep;

        const Vector3 accelerationPart = previousAccelerations_[i] * (0.5 * timeStepSquared);

        bodyPointers_[i]->position() += velocityPart + accelerationPart;
    }

    calculateAccelerations(bodyPointers_, gravitationalConstant(), nextAccelerations_);

    const Vector3 maneuverAcceleration = calculateManeuverAcceleration(simulationProbe, maneuver, timeStep);

    for (std::size_t i = 0; i < bodyPointers_.size(); ++i)
    {
        Vector3 averageAcceleration = (previousAccelerations_[i] + nextAccelerations_[i]) * 0.5;

        bodyPointers_[i]->velocity() += averageAcceleration * timeStep;
    }

    simulationProbe.velocity() += maneuverAcceleration * timeStep;

    if (maneuver.has_value())
    {
        const Real throttleValue = std::clamp(maneuver.value().getThrottleValue(), 0.0, 1.0);

        simulationProbe.setFuelMass(std::max(0.0, simulationProbe.fuelMass() - simulationProbe.fuelFlow() * throttleValue * timeStep));
    }

    advanceTime(timeStep);
}
