#include "NSGAIIComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/fitness/FitnessMetrics.h"

std::size_t NSGAIIComparator::objectiveCount() const
{
    return 3;
}

Real NSGAIIComparator::objectiveValue(
    const FitnessValue& fitness,
    std::size_t objective) const
{
    switch (objective)
    {
    case 0:
        return targetWindowViolation(fitness);
    case 1:
        return fitness.fuelUsed;
    case 2:
        return fitness.minimumDistanceTime;
    }

    throw std::out_of_range("Invalid NSGA-II comparator objective index.");
}

bool NSGAIIComparator::prioritizesFuelConstraintViolation() const
{
    return true;
}

bool NSGAIIComparator::prioritizesTargetWindowViolation() const
{
    return true;
}

std::size_t NSGAIIComparator::tieBreakerCount() const
{
    return 4;
}

Real NSGAIIComparator::tieBreakerValue(
    const FitnessValue& fitness,
    std::size_t tieBreaker) const
{
    switch (tieBreaker)
    {
    case 0:
        return targetWindowViolation(fitness);
    case 1:
        return fitness.fuelUsed;
    case 2:
        return fitness.minimumDistanceTime;
    case 3:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range("Invalid NSGA-II comparator tie-breaker index.");
}
