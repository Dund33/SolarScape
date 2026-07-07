#ifndef SOLARSCAPE_TRAJECTORYSPECIMENCOMPARATOR_H
#define SOLARSCAPE_TRAJECTORYSPECIMENCOMPARATOR_H

#include "genetics/comparison/FitnessObjectiveComparator.h"

class TrajectorySpecimenComparator final : public FitnessObjectiveComparator
{
public:
    std::size_t objectiveCount() const override;

    Real objectiveValue(const FitnessValue& fitness, std::size_t objective) const override;

protected:
    bool prioritizesFuelConstraintViolation() const override;
    bool prioritizesTargetWindowViolation() const override;

    std::size_t tieBreakerCount() const override;

    Real tieBreakerValue(const FitnessValue& fitness, std::size_t tieBreaker) const override;
};

#endif
