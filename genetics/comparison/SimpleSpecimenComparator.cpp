#include "SimpleSpecimenComparator.h"

#include <cstddef>

#include "genetics/Specimen.h"
#include "genetics/comparison/FitnessObjectives.h"

namespace
{
    enum class DominanceRelation
    {
        lhsDominates,
        rhsDominates,
        equivalent,
        unordered
    };

    DominanceRelation compareByDominance(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        bool lhsStrictlyBetter = false;
        bool rhsStrictlyBetter = false;
        const auto lhsValues =
            fitnessObjectives::comparableValues(lhs);
        const auto rhsValues =
            fitnessObjectives::comparableValues(rhs);

        for (std::size_t i = 0; i < lhsValues.size(); ++i)
        {
            if (lhsValues[i] < rhsValues[i])
            {
                lhsStrictlyBetter = true;
            }

            if (rhsValues[i] < lhsValues[i])
            {
                rhsStrictlyBetter = true;
            }
        }

        if (lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return DominanceRelation::lhsDominates;
        }

        if (rhsStrictlyBetter && !lhsStrictlyBetter)
        {
            return DominanceRelation::rhsDominates;
        }

        if (!lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return DominanceRelation::equivalent;
        }

        return DominanceRelation::unordered;
    }

    bool lexicographicallyBetter(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        const auto lhsValues =
            fitnessObjectives::comparableValues(lhs);
        const auto rhsValues =
            fitnessObjectives::comparableValues(rhs);

        for (std::size_t i = 0; i < lhsValues.size(); ++i)
        {
            if (lhsValues[i] < rhsValues[i])
            {
                return true;
            }

            if (rhsValues[i] < lhsValues[i])
            {
                return false;
            }
        }

        return false;
    }
}

std::partial_ordering SimpleSpecimenComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    switch (compareByDominance(lhsFitness, rhsFitness))
    {
    case DominanceRelation::lhsDominates:
        return std::partial_ordering::less;
    case DominanceRelation::rhsDominates:
        return std::partial_ordering::greater;
    case DominanceRelation::equivalent:
        return std::partial_ordering::equivalent;
    case DominanceRelation::unordered:
        return std::partial_ordering::unordered;
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

    switch (compareByDominance(lhsFitness, rhsFitness))
    {
    case DominanceRelation::lhsDominates:
        return true;
    case DominanceRelation::rhsDominates:
        return false;
    case DominanceRelation::equivalent:
    case DominanceRelation::unordered:
        return lexicographicallyBetter(lhsFitness, rhsFitness);
    }

    return false;
}
