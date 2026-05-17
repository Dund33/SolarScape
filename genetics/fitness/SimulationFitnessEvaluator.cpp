#include "SimulationFitnessEvaluator.h"

#include <stdexcept>

#include "simulation/ManeuverSchedule.h"

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
    const std::vector<Maneuver> sortedManeuvers =
        sortManeuversByInitTime(maneuvers);

    const Body& simulatedTargetBody =
        simulation->targetBody();
    const Probe& simulatedProbe =
        simulation->probe();

    Real minimumDistance =
        distance(
            simulatedProbe.position(),
            absolutePointForBody(
                simulatedTargetBody,
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

        const auto maneuver =
            activeManeuverAt(
                sortedManeuvers,
                currentTime);

        simulation->step(
            maneuver,
            stepTime,
            gravitationalConstant);

        currentTime += stepTime;

        const Real currentDistance =
            distance(
                simulatedProbe.position(),
                absolutePointForBody(
                    simulatedTargetBody,
                    targetPointFromTargetBody));

        if (currentDistance < minimumDistance)
        {
            minimumDistance = currentDistance;
            minimumDistanceTime = currentTime;
            minimumDistanceFuelMass = simulatedProbe.fuelMass();
        }
    }

    return FitnessResult(
        minimumDistance,
        minimumDistanceTime,
        minimumDistanceFuelMass);
}
