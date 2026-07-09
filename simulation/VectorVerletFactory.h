#ifndef SOLARSCAPE_VECTORVERLETFACTORY_H
#define SOLARSCAPE_VECTORVERLETFACTORY_H

#include <cstddef>
#include <memory>
#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/Maneuver.h"
#include "simulation/VectorSimulationFactory.h"

class VectorVerletFactory final : public VectorSimulationFactory
{
public:
    VectorVerletFactory(Real gravitationalConstant, std::vector<Body> bodies, Body targetBody, Probe probe);

    std::size_t maxBatchSize() const override;

    std::unique_ptr<VectorSimulation> create(std::vector<std::vector<Maneuver>> maneuverBatch) const override;

private:
    Real gravitationalConstant;
    std::vector<Body> bodies;
    Body targetBody;
    Probe probe;
};

#endif
