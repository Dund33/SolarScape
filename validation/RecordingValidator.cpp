#include "RecordingValidator.h"

#include <ranges>
#include <stdexcept>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/Simulation.h"

namespace
{
    void appendStatuses(
        const Simulation& simulation,
        std::vector<Status>& recording)
    {
        const std::vector<Body>& bodies =
            simulation.bodies();

        for (const auto [bodyIndex, body] : std::views::enumerate(bodies))
        {
            recording.push_back(
                Status{
                    static_cast<std::size_t>(bodyIndex),
                    simulation.time(),
                    body.position(),
                    body.velocity()});
        }

        const std::size_t probeIndex =
            bodies.size();
        recording.push_back(
            Status{
                probeIndex,
                simulation.time(),
                simulation.probe().position(),
                simulation.probe().velocity()});
    }
}

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
    if (timeStep <= 0.0)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
    }

    auto simulation =
        simulationFactory.create(
            std::vector<Maneuver>{});

    std::vector<Status> recording;
    recording.reserve((steps + 1) * (simulation->bodies().size() + 1));
    appendStatuses(
        *simulation,
        recording);

    for (std::size_t step = 0; step < steps; ++step)
    {
        simulation->step(
            timeStep);

        appendStatuses(
            *simulation,
            recording);
    }

    return recording;
}
