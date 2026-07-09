#ifndef SOLARSCAPE_RECORDINGVALIDATOR_H
#define SOLARSCAPE_RECORDINGVALIDATOR_H

#include <cstddef>
#include <vector>

#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/SimulationFactory.h"

struct Status
{
    std::size_t bodyId{};
    Real time{};
    Vector3 position;
    Vector3 velocity;
};

class RecordingValidator
{
public:
    RecordingValidator(const SimulationFactory& simulationFactory, Real timeStep, std::size_t steps);

    std::vector<Status> record() const;

private:
    const SimulationFactory& simulationFactory;
    Real timeStep;
    std::size_t steps;
};

#endif
