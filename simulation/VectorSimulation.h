#ifndef SOLARSCAPE_VECTORSIMULATION_H
#define SOLARSCAPE_VECTORSIMULATION_H

#include <cstddef>

#include "math/Real.h"
#include "math/Vector3.h"

class VectorSimulation
{
public:
    virtual ~VectorSimulation() = default;

    virtual std::size_t batchSize() const = 0;
    virtual void step(Real timeStep) = 0;

    virtual Real requestedFuelUse(std::size_t laneIndex) const = 0;
    virtual Real initialProbeFuelMass(std::size_t laneIndex) const = 0;
    virtual Vector3 probePosition(std::size_t laneIndex) const = 0;
    virtual Vector3 targetBodyPosition(std::size_t laneIndex) const = 0;
};

#endif
