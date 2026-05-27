#include "RecordingValidator.h"

#include <stdexcept>
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

std::vector<Status> RecordingValidator::record() const
{
    if (timeStep <= 0.0L)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
    }

    auto simulation =
        simulationFactory.create(
            std::vector<Maneuver>{});

    std::vector<Status> recording;
    recording.reserve(steps + 1);
    recording.push_back(
        Status{
            simulation->time(),
            simulation->probe().position(),
            simulation->probe().velocity()});

    for (std::size_t step = 0; step < steps; ++step)
    {
        simulation->step(
            timeStep);

        recording.push_back(
            Status{
                simulation->time(),
                simulation->probe().position(),
                simulation->probe().velocity()});
    }

    return recording;
}
