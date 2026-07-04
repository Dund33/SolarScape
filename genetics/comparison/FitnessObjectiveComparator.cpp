#include "FitnessObjectiveComparator.h"

#include <compare>
#include <cstddef>

#include "genetics/Specimen.h"
#include "genetics/fitness/FitnessMetrics.h"

namespace
{
    std::partial_ordering compareMinimizedValues(
        Real lhs,
        Real rhs)
    {
        if (lhs < rhs)
        {
            return std::partial_ordering::less;
        }

        if (rhs < lhs)
        {
            return std::partial_ordering::greater;
        }

        return std::partial_ordering::equivalent;
    }
}

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
        const std::partial_ordering comparison =
            compareMinimizedValues(
                lhsFitness.fuelConstraintViolation,
                rhsFitness.fuelConstraintViolation);

        if (comparison != std::partial_ordering::equivalent)
        {
            return comparison;
        }
    }

    if (prioritizesTargetWindowViolation())
    {
        const std::partial_ordering comparison =
            compareMinimizedValues(
                targetWindowViolation(lhsFitness),
                targetWindowViolation(rhsFitness));

        if (comparison != std::partial_ordering::equivalent)
        {
            return comparison;
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
        const std::partial_ordering comparison =
            compareMinimizedValues(
                lhsValue,
                rhsValue);

        if (comparison == std::partial_ordering::less)
        {
            lhsStrictlyBetter = true;
        }

        if (comparison == std::partial_ordering::greater)
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
        const std::partial_ordering comparison =
            compareMinimizedValues(
                lhsValue,
                rhsValue);

        if (comparison == std::partial_ordering::less)
        {
            return true;
        }

        if (comparison == std::partial_ordering::greater)
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

bool FitnessObjectiveComparator::prioritizesTargetWindowViolation() const
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
