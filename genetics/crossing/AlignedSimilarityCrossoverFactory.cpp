#include "AlignedSimilarityCrossoverFactory.h"

#include <memory>
#include <utility>

#include "genetics/crossing/AlignedSimilarityCrossover.h"

AlignedSimilarityCrossoverFactory::AlignedSimilarityCrossoverFactory(
    Real minRegionSimilarity,
    Real timeScaleMultiplier)
    : minRegionSimilarity(minRegionSimilarity),
      timeScaleMultiplier(timeScaleMultiplier)
{
}

std::unique_ptr<Crossover> AlignedSimilarityCrossoverFactory::create() const
{
    return std::make_unique<AlignedSimilarityCrossover>(
        minRegionSimilarity,
        timeScaleMultiplier);
}
