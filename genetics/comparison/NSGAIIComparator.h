#ifndef SOLARSCAPE_NSGAIICOMPARATOR_H
#define SOLARSCAPE_NSGAIICOMPARATOR_H

#include "genetics/comparison/FitnessObjectiveComparator.h"

class NSGAIIComparator final : public FitnessObjectiveComparator
{
public:
    std::size_t objectiveCount() const override;

    Real objectiveValue(
        const FitnessValue& fitness,
        std::size_t objective) const override;

protected:
    bool prioritizesFuelConstraintViolation() const override;
    bool prioritizesTargetWindowViolation() const override;

    std::size_t tieBreakerCount() const override;

    Real tieBreakerValue(
        const FitnessValue& fitness,
        std::size_t tieBreaker) const override;
};

#endif
