#include "RandomCutCrossoverFactory.h"

#include <memory>

#include "genetics/crossing/RandomCutCrossover.h"

std::unique_ptr<Crossover> RandomCutCrossoverFactory::create() const
{
    return std::make_unique<RandomCutCrossover>();
}
