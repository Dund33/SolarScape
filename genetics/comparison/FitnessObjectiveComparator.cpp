#include "FitnessObjectiveComparator.h"

#include <compare>
#include <cstddef>

#include "genetics/Specimen.h"

std::partial_ordering FitnessObjectiveComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness =
        lhs.getFitness().value();
    const FitnessValue& rhsFitness =
        rhs.getFitness().value();

    if (prioritizesFuelConstraintViolation())
    {
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
    }

    bool lhsStrictlyBetter = false;
    bool rhsStrictlyBetter = false;

    for (std::size_t objective = 0; objective < objectiveCount(); ++objective)
    {
        const Real lhsValue =
            objectiveValue(
                lhsFitness,
                objective);
        const Real rhsValue =
            objectiveValue(
                rhsFitness,
                objective);

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

bool FitnessObjectiveComparator::isLess(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const std::partial_ordering result =
        compare(
            lhs,
            rhs);

    if (result == std::partial_ordering::less)
    {
        return true;
    }

    if (result == std::partial_ordering::greater)
    {
        return false;
    }

    const FitnessValue& lhsFitness =
        lhs.getFitness().value();
    const FitnessValue& rhsFitness =
        rhs.getFitness().value();

    for (std::size_t tieBreaker = 0;
         tieBreaker < tieBreakerCount();
         ++tieBreaker)
    {
        const Real lhsValue =
            tieBreakerValue(
                lhsFitness,
                tieBreaker);
        const Real rhsValue =
            tieBreakerValue(
                rhsFitness,
                tieBreaker);

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

bool FitnessObjectiveComparator::prioritizesFuelConstraintViolation() const
{
    return false;
}

std::size_t FitnessObjectiveComparator::tieBreakerCount() const
{
    return objectiveCount();
}

Real FitnessObjectiveComparator::tieBreakerValue(
    const FitnessValue& fitness,
    std::size_t tieBreaker) const
{
    return objectiveValue(
        fitness,
        tieBreaker);
}
