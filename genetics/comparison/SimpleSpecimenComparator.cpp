#include "SimpleSpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

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
        return fitness.minimumDistance;
    case 1:
        return fitness.minimumDistanceTime;
    case 2:
        return -fitness.minimumDistanceFuelMass;
    case 3:
        return fitness.fuelConstraintViolation;
    }

    throw std::out_of_range("Invalid simple comparator objective index.");
}
