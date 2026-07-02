#ifndef SOLARSCAPE_SIMPLESPECIMENCOMPARATOR_H
#define SOLARSCAPE_SIMPLESPECIMENCOMPARATOR_H

#include "genetics/comparison/FitnessObjectiveComparator.h"

class SimpleSpecimenComparator final : public FitnessObjectiveComparator
{
public:
    std::size_t objectiveCount() const override;

    Real objectiveValue(
        const FitnessValue& fitness,
        std::size_t objective) const override;
};

#endif
