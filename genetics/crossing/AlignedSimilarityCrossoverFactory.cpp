#include "AlignedSimilarityCrossoverFactory.h"

#include <memory>
#include <utility>

#include "genetics/crossing/AlignedSimilarityCrossover.h"

AlignedSimilarityCrossoverFactory::AlignedSimilarityCrossoverFactory(
    Real minRegionSimilarityValue,
    Real timeScaleMultiplierValue)
    : minRegionSimilarity(minRegionSimilarityValue),
      timeScaleMultiplier(timeScaleMultiplierValue)
{
}

std::unique_ptr<Crossover> AlignedSimilarityCrossoverFactory::create() const
{
    return std::make_unique<AlignedSimilarityCrossover>(
        minRegionSimilarity,
        timeScaleMultiplier);
}
