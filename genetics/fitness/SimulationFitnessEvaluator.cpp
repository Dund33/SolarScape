#include "SimulationFitnessEvaluator.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>

namespace
{
    Real distance(
        const Vector3& left,
        const Vector3& right)
    {
        return (left - right).length();
    }

    Vector3 absolutePointForBody(
        const Body& targetBody,
        const Vector3& relativePoint)
    {
        return targetBody.position() + relativePoint;
    }
}

SimulationFitnessEvaluator::SimulationFitnessEvaluator(
    Real gravitationalConstant,
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const SimulationFactory& simulationFactory
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      simulationFactory(simulationFactory)
{
}

void SimulationFitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    const FitnessResult fitnessResult =
        calculateFitnessResult(specimen.getManeuvers());

    specimen.setFitness(fitnessResult);
}

FitnessResult SimulationFitnessEvaluator::calculateFitnessResult(
    const std::vector<Maneuver>& maneuvers) const
{
    if (simulationTime < 0.0L)
    {
        throw std::invalid_argument("simulationTime must be non-negative");
    }

    if (timeStep <= 0.0L)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
    }

    auto simulation =
        simulationFactory.create();

    Real currentTime = 0.0L;
    std::vector<Maneuver> sortedManeuvers = maneuvers;
    std::ranges::sort(
        sortedManeuvers,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    const Probe& simulatedProbe =
        simulation->probe();

    Real minimumDistance =
        distance(
            simulatedProbe.position(),
            absolutePointForBody(
                simulation->targetBody(),
                targetPointFromTargetBody));

    Real minimumDistanceTime = currentTime;
    Real minimumDistanceFuelMass = simulatedProbe.fuelMass();

    while (currentTime < simulationTime)
    {
        const Real remainingTime =
            simulationTime - currentTime;

        const Real stepTime =
            remainingTime < timeStep
                ? remainingTime
                : timeStep;

        std::optional<Maneuver> maneuver;
        auto maneuverIt = std::ranges::upper_bound(
            sortedManeuvers,
            currentTime,
            std::less<>{},
            [](const Maneuver& candidate)
            {
                return candidate.getInitTime();
            });

        if (maneuverIt != sortedManeuvers.begin())
        {
            --maneuverIt;

            const Real maneuverStartTime =
                maneuverIt->getInitTime();
            const Real maneuverEndTime =
                maneuverStartTime + maneuverIt->getDuration();

            if (currentTime < maneuverEndTime)
            {
                maneuver = *maneuverIt;
            }
        }

        simulation->step(
            maneuver,
            stepTime,
            gravitationalConstant);

        currentTime += stepTime;

        const Real currentDistance =
            distance(
                simulation->probe().position(),
                absolutePointForBody(
                    simulation->targetBody(),
                    targetPointFromTargetBody));

        if (currentDistance < minimumDistance)
        {
            minimumDistance = currentDistance;
            minimumDistanceTime = currentTime;
            minimumDistanceFuelMass = simulation->probe().fuelMass();
        }
    }

    return FitnessResult(
        minimumDistance,
        minimumDistanceTime,
        minimumDistanceFuelMass);
}
