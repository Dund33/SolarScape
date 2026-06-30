#include "TrajectorySpecimenComparator.h"

#include <cstddef>
#include <stdexcept>

#include "genetics/Specimen.h"

std::partial_ordering TrajectorySpecimenComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    if (lhsFitness.fuelConstraintViolation <
        rhsFitness.fuelConstraintViolation)
    {
        return std::partial_ordering::less;
    }

    if (rhsFitness.fuelConstraintViolation <
        lhsFitness.fuelConstraintViolation)
    {
        return std::partial_ordering::greater;
    }

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

bool TrajectorySpecimenComparator::isLess(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
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

    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

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

    return lhsFitness.fuelConstraintViolation <
        rhsFitness.fuelConstraintViolation;
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
        return fitness.minimumDistance;
    case 1:
        return fitness.minimumDistanceTime;
    case 2:
        return -fitness.minimumDistanceFuelMass;
    }

    throw std::out_of_range(
        "Invalid trajectory comparator objective index.");
}
