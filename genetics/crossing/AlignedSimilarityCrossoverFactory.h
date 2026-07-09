#ifndef SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVERFACTORY_H
#define SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVERFACTORY_H

#include "config/consts.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "math/Real.h"

class AlignedSimilarityCrossoverFactory final : public CrossoverFactory
{
public:
    explicit AlignedSimilarityCrossoverFactory(Real minRegionSimilarity = ALIGNED_SIMILARITY_CROSSOVER_MIN_REGION_SIMILARITY,
                                               Real timeScaleMultiplier = 1.0);

    std::unique_ptr<Crossover> create() const override;

private:
    Real minRegionSimilarity;
    Real timeScaleMultiplier;
};

#endif
