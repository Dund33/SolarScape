#ifndef SOLARSCAPE_CROSSOVER_H
#define SOLARSCAPE_CROSSOVER_H

#include <utility>

#include "genetics/Specimen.h"

class Crossover
{
public:
    virtual ~Crossover() = default;

    virtual std::pair<Specimen, Specimen> cross(
        const Specimen& parent1,
        const Specimen& parent2
    ) const = 0;
};

#endif
