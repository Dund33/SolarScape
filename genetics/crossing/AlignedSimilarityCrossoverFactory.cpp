#include "AlignedSimilarityCrossoverFactory.h"

#include <memory>
#include <utility>

#include "genetics/crossing/AlignedSimilarityCrossover.h"

AlignedSimilarityCrossoverFactory::AlignedSimilarityCrossoverFactory(
    Real minPairSimilarity,
    Real timeScaleMultiplier,
    Real lengthReward)
    : minPairSimilarity(minPairSimilarity),
      timeScaleMultiplier(timeScaleMultiplier),
      lengthReward(lengthReward)
{
}

std::unique_ptr<Crossover> AlignedSimilarityCrossoverFactory::create() const
{
    return std::make_unique<AlignedSimilarityCrossover>(
        minPairSimilarity,
        timeScaleMultiplier,
        lengthReward);
}
