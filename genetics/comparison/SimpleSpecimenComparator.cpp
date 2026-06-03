#include "SimpleSpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/Specimen.h"

std::partial_ordering SimpleSpecimenComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    bool lhsStrictlyBetter = false;
    bool rhsStrictlyBetter = false;

    for (std::size_t objective = 0; objective < objectiveCount(); ++objective)
    {
        const Real lhsValue =
            objectiveValue(lhsFitness, objective);
        const Real rhsValue =
            objectiveValue(rhsFitness, objective);

        if (lhsValue < rhsValue)
        {
            lhsStrictlyBetter = true;
        }

        if (rhsValue < lhsValue)
        {
            rhsStrictlyBetter = true;
        }
    }

    if (lhsStrictlyBetter && !rhsStrictlyBetter)
    {
        return std::partial_ordering::less;
    }

    if (rhsStrictlyBetter && !lhsStrictlyBetter)
    {
        return std::partial_ordering::greater;
    }

    if (!lhsStrictlyBetter && !rhsStrictlyBetter)
    {
        return std::partial_ordering::equivalent;
    }

    return std::partial_ordering::unordered;
}

bool SimpleSpecimenComparator::isLess(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    const std::partial_ordering result =
        compare(lhs, rhs);

    if (result == std::partial_ordering::less)
    {
        return true;
    }

    if (result == std::partial_ordering::greater)
    {
        return false;
    }

    for (std::size_t objective = 0; objective < objectiveCount(); ++objective)
    {
        const Real lhsValue =
            objectiveValue(lhsFitness, objective);
        const Real rhsValue =
            objectiveValue(rhsFitness, objective);

        if (lhsValue < rhsValue)
        {
            return true;
        }

        if (rhsValue < lhsValue)
        {
            return false;
        }
    }

    return false;
}

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
