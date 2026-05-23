#include "Simulation.h"

#include <utility>

Simulation::Simulation(
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe,
    SimulationContext context,
    Real gravitationalConstant)
    : bodies_(std::move(bodies)),
      probe_(std::move(probe)),
      context_(std::move(context)),
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

std::vector<Body>& Simulation::mutableBodies()
{
    return bodies_;
}

Probe& Simulation::mutableProbe()
{
    return probe_;
}

const SimulationContext& Simulation::context() const
{
    return context_;
}

Real Simulation::gravitationalConstant() const
{
    return gravitationalConstant_;
}

void Simulation::advanceTime(Real timeStep)
{
    time_ += timeStep;
}
