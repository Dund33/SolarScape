#include "Simulation.h"

#include <numeric>
#include <utility>

Simulation::Simulation(std::vector<Body> bodies, Body targetBody, Probe probe, std::vector<Maneuver> maneuvers, Real gravitationalConstant)
    : bodies_(std::move(bodies)), probe_(std::move(probe)), maneuvers_(std::move(maneuvers)), gravitationalConstant_(gravitationalConstant),
      initialProbeFuelMass_(probe_.fuelMass())
{
    bodies_.push_back(std::move(targetBody));
    targetBody_ = &bodies_.back();
}

std::size_t Simulation::bodyCount() const
{
    return bodies_.size();
}

Vector3 Simulation::bodyPosition(std::size_t bodyIndex) const
{
    return bodies_.at(bodyIndex).position();
}

Vector3 Simulation::bodyVelocity(std::size_t bodyIndex) const
{
    return bodies_.at(bodyIndex).velocity();
}

Real Simulation::bodyMass(std::size_t bodyIndex) const
{
    return bodies_.at(bodyIndex).mass();
}

Vector3 Simulation::probePosition() const
{
    return probe_.position();
}

Vector3 Simulation::probeVelocity() const
{
    return probe_.velocity();
}

Real Simulation::probeMass() const
{
    return probe_.mass();
}

Vector3 Simulation::targetBodyPosition() const
{
    return targetBody_->position();
}

Real Simulation::requestedFuelUse() const
{
    return std::accumulate(maneuvers_.begin(), maneuvers_.end(), 0.0, [this](Real totalFuelUse, const Maneuver& maneuver) {
        return totalFuelUse + probe_.fuelFlow() * maneuver.getThrottleValue() * maneuver.getDuration();
    });
}

Real Simulation::initialProbeFuelMass() const
{
    return initialProbeFuelMass_;
}

std::vector<Body>& Simulation::mutableBodies()
{
    return bodies_;
}

Probe& Simulation::mutableProbe()
{
    return probe_;
}

const std::vector<Maneuver>& Simulation::maneuvers() const
{
    return maneuvers_;
}

Real Simulation::gravitationalConstant() const
{
    return gravitationalConstant_;
}

Real Simulation::currentTime() const
{
    return time_;
}

void Simulation::advanceTime(Real timeStep)
{
    time_ += timeStep;
}
