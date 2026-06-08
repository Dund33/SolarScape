#ifndef SOLARSCAPE_SPECIMENRANK_H
#define SOLARSCAPE_SPECIMENRANK_H

#include <cstddef>

#include "math/Real.h"

struct SpecimenRank
{
    std::size_t rank{};
    Real crowdingDistance{};
};

#endif
