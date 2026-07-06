#include "TrajectorySpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/fitness/FitnessMetrics.h"

namespace
{
    bool isInsideTargetWindow(
        const FitnessValue& fitness)
    {
        return targetWindowViolation(fitness) <= 0.0;
    }

    Real fuelObjective(
        const FitnessValue& fitness)
    {
        return isInsideTargetWindow(fitness)
            ? fitness.fuelUsed
            : 0.0;
    }

    Real timeObjective(
        const FitnessValue& fitness)
    {
        return isInsideTargetWindow(fitness)
            ? fitness.minimumDistanceTime
            : 0.0;
    }
}

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
        return targetWindowViolation(
            fitness);
    case 1:
        return fuelObjective(fitness);
    case 2:
        return timeObjective(fitness);
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
    return 2;
}

Real TrajectorySpecimenComparator::tieBreakerValue(
    const FitnessValue& fitness,
    std::size_t tieBreaker) const
{
    switch (tieBreaker)
    {
    case 0:
        return fitness.fuelUsed;
    case 1:
        return fitness.minimumDistanceTime;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator tie-breaker index.");
}
