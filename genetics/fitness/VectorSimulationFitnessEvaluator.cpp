#include "genetics/fitness/VectorSimulationFitnessEvaluator.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include "genetics/fitness/FitnessEvaluationUtils.h"

VectorSimulationFitnessEvaluator::VectorSimulationFitnessEvaluator(Real timeStepValue, Real simulationTimeValue,
                                                                   Vector3 targetPointFromTargetBodyValue,
                                                                   const VectorSimulationFactory& simulationFactoryRef)
    : timeStep(timeStepValue), simulationTime(simulationTimeValue), targetPointFromTargetBody(targetPointFromTargetBodyValue),
      simulationFactory(simulationFactoryRef)
{
}

void VectorSimulationFitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    std::vector<Specimen*> specimens{&specimen};
    evaluateBatch(specimens);
}

void VectorSimulationFitnessEvaluator::evaluateBatch(std::vector<Specimen*>& specimens) const
{
    const std::size_t maxBatchSize = simulationFactory.maxBatchSize();

    if (maxBatchSize == 0)
    {
        throw std::invalid_argument("Vector simulation max batch size must be greater than zero.");
    }

    std::vector<Specimen*> pendingSpecimens;
    pendingSpecimens.reserve(specimens.size());

    const auto needsEvaluation = [](const Specimen* specimen) { return specimen != nullptr && !specimen->getFitness().has_value(); };

    std::ranges::copy(specimens | std::views::filter(needsEvaluation), std::back_inserter(pendingSpecimens));

    if (pendingSpecimens.empty())
    {
        return;
    }

    const std::size_t batchCount = (pendingSpecimens.size() + maxBatchSize - 1) / maxBatchSize;

    const auto evaluateBatchAt = [&](std::size_t batchIndex) {
        const std::size_t batchStart = batchIndex * maxBatchSize;
        const std::size_t batchEnd = std::min(pendingSpecimens.size(), batchStart + maxBatchSize);

        const std::span<Specimen*> batchSpecimens(pendingSpecimens.data() + batchStart, batchEnd - batchStart);

        std::vector<std::vector<Maneuver>> maneuverBatch;
        maneuverBatch.reserve(batchSpecimens.size());

        std::ranges::transform(batchSpecimens, std::back_inserter(maneuverBatch),
                               [](const Specimen* specimen) { return specimen->getManeuvers(); });

        const std::vector<FitnessValue> fitnessValues = calculateFitnessValues(std::move(maneuverBatch));

        for (const std::size_t laneIndex : std::views::iota(std::size_t{0}, fitnessValues.size()))
        {
            batchSpecimens[laneIndex]->setFitness(fitnessValues[laneIndex]);
        }
    };

    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, batchCount), [&](const tbb::blocked_range<std::size_t>& batchRange) {
        for (std::size_t batchIndex = batchRange.begin(); batchIndex != batchRange.end(); ++batchIndex)
        {
            evaluateBatchAt(batchIndex);
        }
    });
}

std::vector<FitnessValue> VectorSimulationFitnessEvaluator::calculateFitnessValues(std::vector<std::vector<Maneuver>> maneuverBatch) const
{
    FitnessEvaluationUtils::validateTiming(simulationTime, timeStep);

    auto simulation = simulationFactory.create(std::move(maneuverBatch));

    const std::size_t batchSize = simulation->batchSize();

    std::vector<Real> minimumDistances(batchSize, std::numeric_limits<Real>::max());
    std::vector<Real> minimumDistanceTimes(batchSize, 0.0);
    std::vector<bool> hasMinimumDistance(batchSize, false);

    Real currentTime = 0.0;

    while (currentTime < simulationTime)
    {
        const Real stepTime = FitnessEvaluationUtils::nextStepTime(currentTime, simulationTime, timeStep);

        simulation->step(stepTime);

        currentTime += stepTime;

        for (const std::size_t laneIndex : std::views::iota(std::size_t{0}, batchSize))
        {
            const Real currentDistance = FitnessEvaluationUtils::distanceToTargetPoint(
                simulation->probePosition(laneIndex), simulation->targetBodyPosition(laneIndex), targetPointFromTargetBody);

            if (FitnessEvaluationUtils::isBetterMinimumDistance(currentDistance, hasMinimumDistance[laneIndex],
                                                                minimumDistances[laneIndex]))
            {
                minimumDistances[laneIndex] = currentDistance;
                minimumDistanceTimes[laneIndex] = currentTime;
                hasMinimumDistance[laneIndex] = true;
            }
        }
    }

    std::vector<FitnessValue> result;
    result.reserve(batchSize);

    for (const std::size_t laneIndex : std::views::iota(std::size_t{0}, batchSize))
    {
        if (!hasMinimumDistance[laneIndex])
        {
            minimumDistances[laneIndex] = FitnessEvaluationUtils::distanceToTargetPoint(
                simulation->probePosition(laneIndex), simulation->targetBodyPosition(laneIndex), targetPointFromTargetBody);
            minimumDistanceTimes[laneIndex] = currentTime;
        }

        const Real fuelUsed = simulation->requestedFuelUse(laneIndex);
        const Real fuelConstraintViolation =
            FitnessEvaluationUtils::fuelConstraintViolation(fuelUsed, simulation->initialProbeFuelMass(laneIndex));

        result.push_back(FitnessValue{minimumDistances[laneIndex], minimumDistanceTimes[laneIndex], fuelUsed, fuelConstraintViolation});
    }

    return result;
}
