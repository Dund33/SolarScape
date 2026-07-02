#include "Simulation.h"

#include <algorithm>
#include <numeric>
#include <utility>

Simulation::Simulation(
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe,
    std::vector<Maneuver> maneuvers,
    Real gravitationalConstant)
    : bodies_(std::move(bodies)),
      probe_(std::move(probe)),
      maneuvers_(std::move(maneuvers)),
      gravitationalConstant_(gravitationalConstant)
{
    bodies_.push_back(std::move(targetBody));
    targetBody_ = &bodies_.back();
}

const std::vector<Body>& Simulation::bodies() const
{
    return bodies_;
}

const Probe& Simulation::probe() const
{
    return probe_;
}

const Body& Simulation::targetBody() const
{
    return *targetBody_;
}

Real Simulation::time() const
{
    return time_;
}

Real Simulation::requestedFuelUse() const
{
    return std::accumulate(
        maneuvers_.begin(),
        maneuvers_.end(),
        0.0L,
        [this](Real totalFuelUse, const Maneuver& maneuver)
        {
            return totalFuelUse +
                probe_.fuelFlow() *
                maneuver.getThrottleValue() *
                maneuver.getDuration();
        });
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

void Simulation::advanceTime(Real timeStep)
{
    time_ += timeStep;
}
