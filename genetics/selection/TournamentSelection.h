//
// Created by Luke on 5/10/2026.
//

#ifndef SOLARSCAPE_TOURNAMENTSELECTION_H
#define SOLARSCAPE_TOURNAMENTSELECTION_H

#include <cstddef>

#include "genetics/selection/Selection.h"

class TournamentSelection final : public Selection
{
public:
    explicit TournamentSelection(std::size_t tournamentSize);

    // Zwraca najlepszego osobnika spośród losowo wybranej grupy.
    // Zakładamy, że mniejsza wartość fitness oznacza lepsze rozwiązanie.
    const Specimen& select(
        const std::vector<Specimen>& population
    ) const override;

private:
    std::size_t tournamentSize;
};

#endif // SOLARSCAPE_TOURNAMENTSELECTION_H
