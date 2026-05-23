#include "VerletFactory.h"

#include <memory>
#include <utility>

#include "simulation/Verlet.h"

VerletFactory::VerletFactory(
    std::vector<Body> bodies,
    Body targetBody,
    Vector3 probePosition,
    Vector3 probeVelocity,
    ProbeFactory probeFactory)
    : bodies(std::move(bodies)),
      targetBody(std::move(targetBody)),
      probePosition(probePosition),
      probeVelocity(probeVelocity),
      probeFactory(std::move(probeFactory))
{
}

std::unique_ptr<Simulation> VerletFactory::create(
    SimulationContext context) const
{
    return std::make_unique<Verlet>(
        bodies,
        targetBody,
        probeFactory.create(
            probePosition,
            probeVelocity),
        std::move(context));
}
