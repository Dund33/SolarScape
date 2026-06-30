#ifndef SOLARSCAPE_TRAJECTORYSPECIMENCOMPARATOR_H
#define SOLARSCAPE_TRAJECTORYSPECIMENCOMPARATOR_H

#include "genetics/comparison/SpecimenComparator.h"

class TrajectorySpecimenComparator final : public SpecimenComparator
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
