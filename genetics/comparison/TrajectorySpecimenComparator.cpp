#include "TrajectorySpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

std::size_t TrajectorySpecimenComparator::objectiveCount() const
{
    return 3;
}

Real TrajectorySpecimenComparator::objectiveValue(
    const FitnessValue& fitness,
    std::size_t objective) const
{
    switch (objective)
    {
    case 0:
        return fitness.minimumDistance;
    case 1:
        return fitness.minimumDistanceTime;
    case 2:
        return -fitness.minimumDistanceFuelMass;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator objective index.");
}

bool TrajectorySpecimenComparator::prioritizesFuelConstraintViolation() const
{
    return true;
}

std::size_t TrajectorySpecimenComparator::tieBreakerCount() const
{
    return 4;
}

Real TrajectorySpecimenComparator::tieBreakerValue(
    const FitnessValue& fitness,
    std::size_t tieBreaker) const
{
    switch (tieBreaker)
    {
    case 0:
        return fitness.minimumDistanceTime;
    case 1:
        return fitness.minimumDistance;
    case 2:
        return -fitness.minimumDistanceFuelMass;
    case 3:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator tie-breaker index.");
}
