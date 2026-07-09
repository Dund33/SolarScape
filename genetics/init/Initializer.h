#ifndef SOLARSCAPE_INITIALIZER_H
#define SOLARSCAPE_INITIALIZER_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"

class Initializer
{
public:
    virtual ~Initializer() = default;

    virtual Specimen create() const = 0;
    virtual std::vector<Specimen> createPopulation(std::size_t populationSize) const = 0;
};

#endif
