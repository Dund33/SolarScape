#include "VerletFactory.h"

#include <memory>
#include <utility>

#include "simulation/Verlet.h"

VerletFactory::VerletFactory(
    Real gravitationalConstantValue,
    std::vector<Body> bodyValues,
    Body targetBodyValue,
    Probe probeValue)
    : gravitationalConstant(gravitationalConstantValue),
      bodies(std::move(bodyValues)),
      targetBody(std::move(targetBodyValue)),
      probe(std::move(probeValue))
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
