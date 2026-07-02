#include "SimpleSpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/fitness/FitnessMetrics.h"

std::size_t SimpleSpecimenComparator::objectiveCount() const
{
    return 4;
}

Real SimpleSpecimenComparator::objectiveValue(
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
    case 3:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range("Invalid simple comparator objective index.");
}

bool SimpleSpecimenComparator::prioritizesTargetWindowViolation() const
{
    return true;
}
