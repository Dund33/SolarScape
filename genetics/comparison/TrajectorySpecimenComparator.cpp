#include "TrajectorySpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/fitness/FitnessMetrics.h"

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
        return targetWindowViolation(fitness);
    case 1:
        return fitness.fuelUsed;
    case 2:
        return fitness.minimumDistanceTime;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator objective index.");
}

bool TrajectorySpecimenComparator::prioritizesFuelConstraintViolation() const
{
    return true;
}

bool TrajectorySpecimenComparator::prioritizesTargetWindowViolation() const
{
    return true;
}

std::size_t TrajectorySpecimenComparator::tieBreakerCount() const
{
    return 5;
}

Real TrajectorySpecimenComparator::tieBreakerValue(
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
        return fitness.minimumDistance;
    case 4:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator tie-breaker index.");
}
