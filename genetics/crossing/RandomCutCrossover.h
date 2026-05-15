#ifndef SOLARSCAPE_RANDOMCUTCROSSOVER_H
#define SOLARSCAPE_RANDOMCUTCROSSOVER_H

#include "genetics/crossing/Crossover.h"

class RandomCutCrossover final : public Crossover
{
public:
    std::pair<Specimen, Specimen> cross(
        const Specimen& parent1,
        const Specimen& parent2
    ) const override;
};

#endif // SOLARSCAPE_RANDOMCUTCROSSOVER_H
