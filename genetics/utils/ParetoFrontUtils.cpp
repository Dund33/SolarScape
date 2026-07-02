#include "ParetoFrontUtils.h"

#include <algorithm>
#include <compare>

#include "genetics/fitness/FitnessValue.h"

namespace
{
    void updateRange(
        Real& minValue,
        Real& maxValue,
        Real value)
    {
        minValue =
            std::min(
                minValue,
                value);
        maxValue =
            std::max(
                maxValue,
                value);
    }
}

std::vector<Specimen> ParetoFrontUtils::firstFront(
    const std::vector<Specimen>& population,
    const SpecimenComparator& specimenComparator)
{
    std::vector<Specimen> front;

    for (std::size_t candidateIndex = 0;
         candidateIndex < population.size();
         ++candidateIndex)
    {
        bool dominated = false;

        for (std::size_t otherIndex = 0;
             otherIndex < population.size();
             ++otherIndex)
        {
            if (candidateIndex == otherIndex)
            {
                continue;
            }

            if (specimenComparator.compare(
                population[otherIndex],
                population[candidateIndex]) ==
                std::partial_ordering::less)
            {
                dominated = true;
                break;
            }
        }

        if (!dominated)
        {
            front.push_back(
                population[candidateIndex]);
        }
    }

    return front;
}

std::vector<Specimen> ParetoFrontUtils::frontFromIndices(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    std::vector<Specimen> front;
    front.reserve(
        frontIndices.size());

    for (std::size_t specimenIndex : frontIndices)
    {
        front.push_back(
            population[specimenIndex]);
    }

    return front;
}

ParetoFrontStats ParetoFrontUtils::calculateStats(
    const std::vector<Specimen>& front)
{
    ParetoFrontStats stats;
    stats.size = front.size();

    if (front.empty())
    {
        return stats;
    }

    const FitnessValue& firstFitness =
        front.front().getFitness().value();

    stats.minDistance = firstFitness.minimumDistance;
    stats.maxDistance = firstFitness.minimumDistance;
    stats.minTime = firstFitness.minimumDistanceTime;
    stats.maxTime = firstFitness.minimumDistanceTime;
    stats.minFuel = firstFitness.minimumDistanceFuelMass;
    stats.maxFuel = firstFitness.minimumDistanceFuelMass;
    stats.minFuelViolation = firstFitness.fuelConstraintViolation;
    stats.maxFuelViolation = firstFitness.fuelConstraintViolation;

    for (const Specimen& specimen : front)
    {
        const FitnessValue& fitness =
            specimen.getFitness().value();

        if (fitness.fuelConstraintViolation <= 0.0L)
        {
            ++stats.fuelFeasibleCount;
        }

        updateRange(
            stats.minDistance,
            stats.maxDistance,
            fitness.minimumDistance);
        updateRange(
            stats.minTime,
            stats.maxTime,
            fitness.minimumDistanceTime);
        updateRange(
            stats.minFuel,
            stats.maxFuel,
            fitness.minimumDistanceFuelMass);
        updateRange(
            stats.minFuelViolation,
            stats.maxFuelViolation,
            fitness.fuelConstraintViolation);
    }

    return stats;
}

ParetoFrontStats ParetoFrontUtils::calculateStats(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    return calculateStats(
        frontFromIndices(
            population,
            frontIndices));
}
