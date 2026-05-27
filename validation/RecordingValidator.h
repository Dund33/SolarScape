#ifndef SOLARSCAPE_RECORDINGVALIDATOR_H
#define SOLARSCAPE_RECORDINGVALIDATOR_H

#include <cstddef>
#include <utility>
#include <vector>

#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/SimulationFactory.h"

class RecordingValidator
{
public:
    RecordingValidator(
        const SimulationFactory& simulationFactory,
        Real timeStep,
        std::size_t steps);

    std::vector<std::pair<Real, Vector3>> record() const;

private:
    const SimulationFactory& simulationFactory;
    Real timeStep;
    std::size_t steps;
};

#endif
