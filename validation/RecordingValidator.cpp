#include "RecordingValidator.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/Simulation.h"

namespace
{
    void appendStatuses(const Simulation& simulation, Real time, std::vector<Status>& recording)
    {
        const std::size_t bodyCount = simulation.bodyCount();

        for (std::size_t bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex)
        {
            recording.push_back(Status{bodyIndex, time, simulation.bodyPosition(bodyIndex), simulation.bodyVelocity(bodyIndex)});
        }

        const std::size_t probeIndex = bodyCount;
        recording.push_back(Status{probeIndex, time, simulation.probePosition(), simulation.probeVelocity()});
    }
} // namespace

RecordingValidator::RecordingValidator(const SimulationFactory& simulationFactory, Real timeStep, std::size_t steps)
    : simulationFactory(simulationFactory), timeStep(timeStep), steps(steps)
{
}

std::vector<Status> RecordingValidator::record() const
{
    if (timeStep <= 0.0)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
    }

    auto simulation = simulationFactory.create(std::vector<Maneuver>{});

    std::vector<Status> recording;
    recording.reserve((steps + 1) * (simulation->bodyCount() + 1));
    appendStatuses(*simulation, 0.0, recording);

    for (std::size_t step = 0; step < steps; ++step)
    {
        simulation->step(timeStep);

        appendStatuses(*simulation, static_cast<Real>(step + 1) * timeStep, recording);
    }

    return recording;
}
