#include "NSGAIIComparator.h"

#include <cstddef>
#include <stdexcept>

std::size_t NSGAIIComparator::objectiveCount() const
{
    return 2;
}

Real NSGAIIComparator::objectiveValue(
    const FitnessValue& fitness,
    std::size_t objective) const
{
    switch (objective)
    {
    case 0:
        return fitness.fuelUsed;
    case 1:
        return fitness.minimumDistance;
    }

    throw std::out_of_range("Invalid NSGA-II comparator objective index.");
}

bool NSGAIIComparator::prioritizesFuelConstraintViolation() const
{
    return true;
}

std::size_t NSGAIIComparator::tieBreakerCount() const
{
    return 2;
}

Real NSGAIIComparator::tieBreakerValue(
    const FitnessValue& fitness,
    std::size_t tieBreaker) const
{
    switch (tieBreaker)
    {
    case 0:
        return fitness.minimumDistanceTime;
    case 1:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range("Invalid NSGA-II comparator tie-breaker index.");
}
