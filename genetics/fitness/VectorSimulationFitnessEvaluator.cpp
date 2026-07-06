#include "genetics/fitness/VectorSimulationFitnessEvaluator.h"

#include <algorithm>
#include <execution>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace
{
    Real distance(
        const Vector3& left,
        const Vector3& right)
    {
        return (left - right).length();
    }

    Vector3 absolutePointForBody(
        const Vector3& targetBodyPosition,
        const Vector3& relativePoint)
    {
        return targetBodyPosition + relativePoint;
    }

    Real minimumDistanceStartTime(
        const std::vector<Maneuver>& maneuvers,
        Real simulationTime)
    {
        if (maneuvers.empty())
        {
            return simulationTime;
        }

        const Real firstManeuverEndTime =
            maneuvers.front().getInitDelay() +
            maneuvers.front().getDuration();

        return std::clamp(
            firstManeuverEndTime,
            0.0,
            simulationTime);
    }
}

VectorSimulationFitnessEvaluator::VectorSimulationFitnessEvaluator(
    Real timeStepValue,
    Real simulationTimeValue,
    Vector3 targetPointFromTargetBodyValue,
    const VectorSimulationFactory& simulationFactoryRef)
    : timeStep(timeStepValue),
      simulationTime(simulationTimeValue),
      targetPointFromTargetBody(targetPointFromTargetBodyValue),
      simulationFactory(simulationFactoryRef)
{
}

void VectorSimulationFitnessEvaluator::evaluate(
    Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    std::vector<Specimen*> specimens{&specimen};
    evaluateBatch(
        specimens);
}

void VectorSimulationFitnessEvaluator::evaluateBatch(
    std::vector<Specimen*>& specimens) const
{
    const std::size_t maxBatchSize =
        simulationFactory.maxBatchSize();

    if (maxBatchSize == 0)
    {
        throw std::invalid_argument(
            "Vector simulation max batch size must be greater than zero.");
    }

    std::vector<Specimen*> pendingSpecimens;
    pendingSpecimens.reserve(
        specimens.size());

    for (Specimen* specimen : specimens)
    {
        if (specimen != nullptr && !specimen->getFitness().has_value())
        {
            pendingSpecimens.push_back(
                specimen);
        }
    }

    using PendingSpecimensDifference =
        std::ranges::range_difference_t<decltype(pendingSpecimens)>;

    const auto specimenBatches =
        pendingSpecimens |
        std::views::chunk(
            static_cast<PendingSpecimensDifference>(maxBatchSize));

    std::for_each(
        std::execution::par,
        specimenBatches.begin(),
        specimenBatches.end(),
        [&](auto specimenBatch)
    {
        std::vector<Specimen*> batchSpecimens(
            specimenBatch.begin(),
            specimenBatch.end());

        std::vector<std::vector<Maneuver>> maneuverBatch;
        maneuverBatch.reserve(
            batchSpecimens.size());

        for (const Specimen* specimen : batchSpecimens)
        {
            maneuverBatch.push_back(
                specimen->getManeuvers());
        }

        const std::vector<FitnessValue> fitnessValues =
            calculateFitnessValues(
                std::move(maneuverBatch));

        for (std::size_t laneIndex = 0;
             laneIndex < fitnessValues.size();
             ++laneIndex)
        {
            batchSpecimens[laneIndex]->setFitness(
                fitnessValues[laneIndex]);
        }
    });
}

std::vector<FitnessValue> VectorSimulationFitnessEvaluator::calculateFitnessValues(
    std::vector<std::vector<Maneuver>> maneuverBatch) const
{
    if (simulationTime < 0.0)
    {
        throw std::invalid_argument(
            "simulationTime must be non-negative");
    }

    if (timeStep <= 0.0)
    {
        throw std::invalid_argument(
            "timeStep must be greater than zero");
    }

    std::vector<Real> distanceEvaluationStartTimes;
    distanceEvaluationStartTimes.reserve(
        maneuverBatch.size());

    for (const std::vector<Maneuver>& maneuvers : maneuverBatch)
    {
        distanceEvaluationStartTimes.push_back(
            minimumDistanceStartTime(
                maneuvers,
                simulationTime));
    }

    auto simulation =
        simulationFactory.create(
            std::move(maneuverBatch));

    const std::size_t batchSize =
        simulation->batchSize();

    std::vector<Real> minimumDistances(
        batchSize,
        std::numeric_limits<Real>::max());
    std::vector<Real> minimumDistanceTimes(
        distanceEvaluationStartTimes.begin(),
        distanceEvaluationStartTimes.end());
    std::vector<bool> hasMinimumDistance(
        batchSize,
        false);

    Real currentTime = 0.0;

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

        for (std::size_t laneIndex = 0;
             laneIndex < batchSize;
             ++laneIndex)
        {
            if (currentTime < distanceEvaluationStartTimes[laneIndex])
            {
                continue;
            }

            const Real currentDistance =
                distance(
                    simulation->probePosition(
                        laneIndex),
                    absolutePointForBody(
                        simulation->targetBodyPosition(
                            laneIndex),
                        targetPointFromTargetBody));

            if (!hasMinimumDistance[laneIndex] ||
                currentDistance < minimumDistances[laneIndex])
            {
                minimumDistances[laneIndex] =
                    currentDistance;
                minimumDistanceTimes[laneIndex] =
                    currentTime;
                hasMinimumDistance[laneIndex] =
                    true;
            }
        }
    }

    std::vector<FitnessValue> result;
    result.reserve(
        batchSize);

    for (std::size_t laneIndex = 0;
         laneIndex < batchSize;
         ++laneIndex)
    {
        if (!hasMinimumDistance[laneIndex])
        {
            minimumDistances[laneIndex] =
                distance(
                    simulation->probePosition(
                        laneIndex),
                    absolutePointForBody(
                        simulation->targetBodyPosition(
                            laneIndex),
                        targetPointFromTargetBody));
            minimumDistanceTimes[laneIndex] =
                currentTime;
        }

        const Real fuelUsed =
            simulation->requestedFuelUse(
                laneIndex);
        const Real fuelConstraintViolation =
            std::max(
                0.0,
                fuelUsed -
                    simulation->initialProbeFuelMass(
                        laneIndex));

        result.push_back(
            FitnessValue{
                minimumDistances[laneIndex],
                minimumDistanceTimes[laneIndex],
                fuelUsed,
                fuelConstraintViolation});
    }

    return result;
}
