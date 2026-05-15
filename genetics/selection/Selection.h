#ifndef SOLARSCAPE_SELECTION_H
#define SOLARSCAPE_SELECTION_H

#include <vector>

#include "genetics/Specimen.h"

class Selection
{
public:
    virtual ~Selection() = default;

    virtual const Specimen& select(
        const std::vector<Specimen>& population
    ) const = 0;
};

#endif
