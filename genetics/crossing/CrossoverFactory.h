#ifndef SOLARSCAPE_CROSSOVERFACTORY_H
#define SOLARSCAPE_CROSSOVERFACTORY_H

#include <memory>

#include "genetics/crossing/Crossover.h"

class CrossoverFactory
{
public:
    virtual ~CrossoverFactory() = default;

    virtual std::unique_ptr<Crossover> create() const = 0;
};

#endif
