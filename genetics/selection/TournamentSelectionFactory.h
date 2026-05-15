#ifndef SOLARSCAPE_TOURNAMENTSELECTIONFACTORY_H
#define SOLARSCAPE_TOURNAMENTSELECTIONFACTORY_H

#include <cstddef>

#include "genetics/selection/SelectionFactory.h"

class TournamentSelectionFactory final : public SelectionFactory
{
public:
    explicit TournamentSelectionFactory(std::size_t tournamentSize);

    std::unique_ptr<Selection> create() const override;

private:
    std::size_t tournamentSize;
};

#endif
