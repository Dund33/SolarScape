#ifndef SOLARSCAPE_VECTORSIMULATIONFACTORY_H
#define SOLARSCAPE_VECTORSIMULATIONFACTORY_H

#include <cstddef>
#include <memory>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/VectorSimulation.h"

class VectorSimulationFactory
{
public:
    virtual ~VectorSimulationFactory() = default;

    virtual std::size_t maxBatchSize() const = 0;

    virtual std::unique_ptr<VectorSimulation> create(
        std::vector<std::vector<Maneuver>> maneuverBatch) const = 0;
};

#endif
