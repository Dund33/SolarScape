#ifndef SOLARSCAPE_NSGAIICOMPARATOR_H
#define SOLARSCAPE_NSGAIICOMPARATOR_H

#include "genetics/comparison/SpecimenComparator.h"

class NSGAIIComparator final : public SpecimenComparator
{
public:
    std::partial_ordering compare(
        const Specimen& lhs,
        const Specimen& rhs
    ) const override;

    bool isLess(
        const Specimen& lhs,
        const Specimen& rhs
    ) const override;

    std::size_t objectiveCount() const override;

    Real objectiveValue(
        const FitnessValue& fitness,
        std::size_t objective) const override;
};

#endif
