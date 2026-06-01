#include "SimpleSpecimenComparator.h"

#include <array>
#include <cstddef>

#include "genetics/Specimen.h"

namespace
{
    enum class DominanceRelation
    {
        lhsDominates,
        rhsDominates,
        equivalent,
        unordered
    };

    std::array<Real, 3> comparableFitnessValues(
        const FitnessValue& fitness)
    {
        return {
            fitness.minimumDistance,
            fitness.minimumDistanceTime,
            -fitness.minimumDistanceFuelMass};
    }

    DominanceRelation compareByDominance(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        bool lhsStrictlyBetter = false;
        bool rhsStrictlyBetter = false;
        const std::array<Real, 3> lhsValues =
            comparableFitnessValues(lhs);
        const std::array<Real, 3> rhsValues =
            comparableFitnessValues(rhs);

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
        const std::array<Real, 3> lhsValues =
            comparableFitnessValues(lhs);
        const std::array<Real, 3> rhsValues =
            comparableFitnessValues(rhs);

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
