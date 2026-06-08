#ifndef SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVER_H
#define SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVER_H

#include <utility>

#include "genetics/crossing/Crossover.h"
#include "math/Real.h"

class AlignedSimilarityCrossover final : public Crossover
{
public:
    explicit AlignedSimilarityCrossover(
        Real minPairSimilarity = 0.05L,
        Real timeScaleMultiplier = 1.0L,
        Real lengthReward = 0.5L);

    std::pair<Specimen, Specimen> cross(
        const Specimen& parent1,
        const Specimen& parent2
    ) const override;

private:
    Real minPairLogSimilarity;
    Real timeScaleMultiplier;
    Real lengthReward;
};

#endif
