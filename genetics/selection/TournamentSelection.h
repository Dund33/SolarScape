#ifndef SOLARSCAPE_TOURNAMENTSELECTION_H
#define SOLARSCAPE_TOURNAMENTSELECTION_H

#include <cstddef>

#include "genetics/selection/Selection.h"

class TournamentSelection final : public Selection
{
public:
    explicit TournamentSelection(std::size_t tournamentSize);

    const Specimen& select(
        const std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator
    ) const override;

private:
    std::size_t tournamentSize;
};

#endif
