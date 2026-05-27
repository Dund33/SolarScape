#include "VerletFactory.h"

#include <memory>
#include <utility>

#include "simulation/Verlet.h"

VerletFactory::VerletFactory(
    Real gravitationalConstant,
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe)
    : gravitationalConstant(gravitationalConstant),
      bodies(std::move(bodies)),
      targetBody(std::move(targetBody)),
      probe(std::move(probe))
{
}

std::unique_ptr<Simulation> VerletFactory::create(
    std::vector<Maneuver> maneuvers) const
{
    return std::make_unique<Verlet>(
        bodies,
        targetBody,
        probe,
        std::move(maneuvers),
        gravitationalConstant);
}
