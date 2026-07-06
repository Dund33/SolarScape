#ifndef SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVER_H
#define SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVER_H

#include <utility>

#include "config/consts.h"
#include "genetics/crossing/Crossover.h"
#include "math/Real.h"

class AlignedSimilarityCrossover final : public Crossover
{
public:
    explicit AlignedSimilarityCrossover(
        Real minRegionSimilarity = ALIGNED_SIMILARITY_CROSSOVER_MIN_REGION_SIMILARITY,
        Real timeScaleMultiplier = 1.0);

    std::pair<Specimen, Specimen> cross(
        const Specimen& parent1,
        const Specimen& parent2
    ) const override;

private:
    Real minRegionLogSimilarity;
    Real timeScaleMultiplier;
};

#endif
