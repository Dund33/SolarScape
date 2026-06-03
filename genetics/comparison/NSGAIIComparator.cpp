#include "NSGAIIComparator.h"

#include "genetics/Specimen.h"

std::partial_ordering NSGAIIComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const FitnessValue& lhsFitness = lhs.getFitness().value();
    const FitnessValue& rhsFitness = rhs.getFitness().value();

    return compareByPareto(
        lhsFitness,
        rhsFitness,
        fields);
}

bool NSGAIIComparator::isLess(
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

    return lexicographicallyLess(
        lhs.getFitness().value(),
        rhs.getFitness().value(),
        tieBreakerFields);
}

std::span<const FitnessField> NSGAIIComparator::objectiveFields() const
{
    return fields;
}
