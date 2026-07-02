#ifndef SOLARSCAPE_PARETO_RANKING_H
#define SOLARSCAPE_PARETO_RANKING_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/comparison/SpecimenRank.h"

struct ParetoRankedPopulation
{
    std::vector<std::vector<std::size_t>> fronts;
    std::vector<SpecimenRank> ranks;
};

class ParetoRanking final
{
public:
    static ParetoRankedPopulation rankPopulation(
        const std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator);

    static std::vector<std::size_t> sortedIndices(
        const std::vector<Specimen>& population,
        const ParetoRankedPopulation& rankedPopulation,
        const SpecimenComparator& specimenComparator);

private:
    static void calculateCrowdingDistance(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& front,
        const SpecimenComparator& specimenComparator,
        std::vector<SpecimenRank>& ranks);
};

#endif
