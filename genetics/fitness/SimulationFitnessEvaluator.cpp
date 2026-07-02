#include "SimulationFitnessEvaluator.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

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
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const SimulationFactory& simulationFactory
)
    : timeStep(timeStep),
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

    const FitnessValue fitnessValue =
        calculateFitnessValue(
            specimen.getManeuvers());

    specimen.setFitness(fitnessValue);
}

FitnessValue SimulationFitnessEvaluator::calculateFitnessValue(
    std::vector<Maneuver> maneuvers) const
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
        simulationFactory.create(
            std::move(maneuvers));

    Real currentTime = 0.0L;

    const Body& simulatedTargetBody =
        simulation->targetBody();
    const Probe& simulatedProbe =
        simulation->probe();
    const Real fuelUsed =
        simulation->requestedFuelUse();

    const Real fuelConstraintViolation =
        std::max(0.0L, fuelUsed - simulatedProbe.fuelMass());

    Real minimumDistance =
        distance(
            simulatedProbe.position(),
            absolutePointForBody(
                simulatedTargetBody,
                targetPointFromTargetBody));

    Real minimumDistanceTime = currentTime;
    while (currentTime < simulationTime)
    {
        const Real remainingTime =
            simulationTime - currentTime;

        const Real stepTime =
            remainingTime < timeStep
                ? remainingTime
                : timeStep;

        simulation->step(
            stepTime);

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
        }
    }

    return {
        minimumDistance,
        minimumDistanceTime,
        fuelUsed,
        fuelConstraintViolation};
}
