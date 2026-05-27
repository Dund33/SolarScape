#include "RecordingValidator.h"

#include <stdexcept>
#include <utility>
#include <vector>

#include "simulation/Maneuver.h"

RecordingValidator::RecordingValidator(
    const SimulationFactory& simulationFactory,
    Real timeStep,
    std::size_t steps)
    : simulationFactory(simulationFactory),
      timeStep(timeStep),
      steps(steps)
{
}

std::vector<std::pair<Real, Vector3>> RecordingValidator::record() const
{
    if (timeStep <= 0.0L)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
    }

    auto simulation =
        simulationFactory.create(
            std::vector<Maneuver>{});

    std::vector<std::pair<Real, Vector3>> recording;
    recording.reserve(steps + 1);
    recording.emplace_back(
        0.0L,
        simulation->probe().position());

    for (std::size_t step = 0; step < steps; ++step)
    {
        simulation->step(
            timeStep);

        recording.emplace_back(
            static_cast<Real>(step + 1) * timeStep,
            simulation->probe().position());
    }

    return recording;
}
