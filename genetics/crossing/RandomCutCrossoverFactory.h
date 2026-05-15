#ifndef SOLARSCAPE_RANDOMCUTCROSSOVERFACTORY_H
#define SOLARSCAPE_RANDOMCUTCROSSOVERFACTORY_H

#include "genetics/crossing/CrossoverFactory.h"

class RandomCutCrossoverFactory final : public CrossoverFactory
{
public:
    std::unique_ptr<Crossover> create() const override;
};

#endif
