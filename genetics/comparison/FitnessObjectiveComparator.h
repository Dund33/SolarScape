#ifndef SOLARSCAPE_FITNESSOBJECTIVECOMPARATOR_H
#define SOLARSCAPE_FITNESSOBJECTIVECOMPARATOR_H

#include "genetics/comparison/SpecimenComparator.h"

class FitnessObjectiveComparator : public SpecimenComparator
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

protected:
    virtual bool prioritizesFuelConstraintViolation() const;

    virtual std::size_t tieBreakerCount() const;

    virtual Real tieBreakerValue(
        const FitnessValue& fitness,
        std::size_t tieBreaker) const;
};

#endif
