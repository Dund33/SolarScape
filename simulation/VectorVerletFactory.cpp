#include "simulation/VectorVerletFactory.h"

#include <memory>
#include <utility>

#include "simulation/VectorVerlet.h"

VectorVerletFactory::VectorVerletFactory(
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

std::size_t VectorVerletFactory::maxBatchSize() const
{
    return VectorVerlet::BatchWidth;
}

std::unique_ptr<VectorSimulation> VectorVerletFactory::create(
    std::vector<std::vector<Maneuver>> maneuverBatch) const
{
    return std::make_unique<VectorVerlet>(
        bodies,
        targetBody,
        probe,
        std::move(maneuverBatch),
        gravitationalConstant);
}
