#include "VerletFactory.h"

#include <memory>
#include <utility>

#include "simulation/Verlet.h"

VerletFactory::VerletFactory(
    std::vector<Body> bodies,
    Body targetBody,
    Probe probe)
    : bodies(std::move(bodies)),
      targetBody(std::move(targetBody)),
      probe(std::move(probe))
{
}

std::unique_ptr<Simulation> VerletFactory::create() const
{
    return std::make_unique<Verlet>(
        bodies,
        targetBody,
        probe);
}
