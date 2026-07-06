#include "genetics/fitness/VectorSimulationFitnessEvaluator.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

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

    if (pendingSpecimens.empty())
    {
        return;
    }

    const std::size_t batchCount =
        (pendingSpecimens.size() + maxBatchSize - 1) / maxBatchSize;

    const auto evaluateBatchAt =
        [&](std::size_t batchIndex)
    {
        const std::size_t batchStart =
            batchIndex * maxBatchSize;
        const std::size_t batchEnd =
            std::min(
                pendingSpecimens.size(),
                batchStart + maxBatchSize);

        std::vector<Specimen*> batchSpecimens;
        batchSpecimens.reserve(
            batchEnd - batchStart);

        for (std::size_t specimenIndex = batchStart;
             specimenIndex < batchEnd;
             ++specimenIndex)
        {
            batchSpecimens.push_back(
                pendingSpecimens[specimenIndex]);
        }

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
    };

    tbb::parallel_for(
        tbb::blocked_range<std::size_t>(
            0,
            batchCount),
        [&](const tbb::blocked_range<std::size_t>& batchRange)
        {
            for (std::size_t batchIndex = batchRange.begin();
                 batchIndex != batchRange.end();
                 ++batchIndex)
            {
                evaluateBatchAt(
                    batchIndex);
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

    auto simulation =
        simulationFactory.create(
            std::move(maneuverBatch));

    const std::size_t batchSize =
        simulation->batchSize();

    std::vector<Real> minimumDistances(
        batchSize,
        std::numeric_limits<Real>::max());
    std::vector<Real> minimumDistanceTimes(
        batchSize,
        0.0);
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
