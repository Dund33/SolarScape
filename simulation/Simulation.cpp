#include "Simulation.h"

#include <utility>

Simulation::Simulation(
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe)
    : bodies_(std::move(bodies)),
      probe_(std::move(probe))
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

std::vector<Body>& Simulation::mutableBodies()
{
    return bodies_;
}

Probe& Simulation::mutableProbe()
{
    return probe_;
}
