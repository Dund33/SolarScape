#ifndef SOLARSCAPE_NSGAIICOMPARATOR_H
#define SOLARSCAPE_NSGAIICOMPARATOR_H

#include <array>
#include <span>

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

    std::span<const FitnessField> objectiveFields() const override;

private:
    const std::array<FitnessField, 2> fields{{
        {&FitnessValue::minimumDistanceFuelMass, -1.0L},
        {&FitnessValue::minimumDistance, 1.0L}}};
    const std::array<FitnessField, 2> tieBreakerFields{{
        {&FitnessValue::minimumDistanceTime, 1.0L},
        {&FitnessValue::fuelConstraintViolation, 1.0L}}};
};

#endif
