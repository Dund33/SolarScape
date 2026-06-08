#ifndef SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVERFACTORY_H
#define SOLARSCAPE_ALIGNEDSIMILARITYCROSSOVERFACTORY_H

#include "genetics/crossing/CrossoverFactory.h"
#include "math/Real.h"

class AlignedSimilarityCrossoverFactory final : public CrossoverFactory
{
public:
    explicit AlignedSimilarityCrossoverFactory(
        Real minPairSimilarity = 0.05L,
        Real timeScaleMultiplier = 1.0L,
        Real lengthReward = 0.5L);

    std::unique_ptr<Crossover> create() const override;

private:
    Real minPairSimilarity;
    Real timeScaleMultiplier;
    Real lengthReward;
};

#endif
