#include "SimulationFitnessEvaluator.h"

#include <algorithm>
#include <execution>
#include <limits>
#include <utility>

#include "genetics/fitness/FitnessEvaluationUtils.h"

SimulationFitnessEvaluator::SimulationFitnessEvaluator(Real timeStepValue, Real simulationTimeValue, Vector3 targetPointFromTargetBodyValue,
                                                       const SimulationFactory& simulationFactoryRef)
    : timeStep(timeStepValue), simulationTime(simulationTimeValue), targetPointFromTargetBody(targetPointFromTargetBodyValue),
      simulationFactory(simulationFactoryRef)
{
}

void SimulationFitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    const FitnessValue fitnessValue = calculateFitnessValue(specimen.getManeuvers());

    specimen.setFitness(fitnessValue);
}

void SimulationFitnessEvaluator::evaluateBatch(std::vector<Specimen*>& specimens) const
{
    std::for_each(std::execution::par, specimens.begin(), specimens.end(), [this](Specimen* specimen) {
        if (specimen != nullptr)
        {
            evaluate(*specimen);
        }
    });
}

FitnessValue SimulationFitnessEvaluator::calculateFitnessValue(std::vector<Maneuver> maneuvers) const
{
    FitnessEvaluationUtils::validateTiming(simulationTime, timeStep);

    auto simulation = simulationFactory.create(std::move(maneuvers));
    Real currentTime = 0.0;

    const Real fuelUsed = simulation->requestedFuelUse();

    const Real fuelConstraintViolation = FitnessEvaluationUtils::fuelConstraintViolation(fuelUsed, simulation->initialProbeFuelMass());

    Real minimumDistance = std::numeric_limits<Real>::max();
    Real minimumDistanceTime = 0.0;
    bool hasMinimumDistance = false;

    while (currentTime < simulationTime)
    {
        const Real stepTime = FitnessEvaluationUtils::nextStepTime(currentTime, simulationTime, timeStep);

        simulation->step(stepTime);

        currentTime += stepTime;

        const Real currentDistance = FitnessEvaluationUtils::distanceToTargetPoint(
            simulation->probePosition(), simulation->targetBodyPosition(), targetPointFromTargetBody);

        if (FitnessEvaluationUtils::isBetterMinimumDistance(currentDistance, hasMinimumDistance, minimumDistance))
        {
            minimumDistance = currentDistance;
            minimumDistanceTime = currentTime;
            hasMinimumDistance = true;
        }
    }

    if (!hasMinimumDistance)
    {
        minimumDistance = FitnessEvaluationUtils::distanceToTargetPoint(simulation->probePosition(), simulation->targetBodyPosition(),
                                                                        targetPointFromTargetBody);
        minimumDistanceTime = currentTime;
    }

    return {minimumDistance, minimumDistanceTime, fuelUsed, fuelConstraintViolation};
}
