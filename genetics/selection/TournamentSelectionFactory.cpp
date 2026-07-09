#include "TournamentSelectionFactory.h"

#include <memory>

#include "genetics/selection/TournamentSelection.h"

TournamentSelectionFactory::TournamentSelectionFactory(std::size_t tournamentSizeValue) : tournamentSize(tournamentSizeValue) {}

std::unique_ptr<Selection> TournamentSelectionFactory::create() const
{
    return std::make_unique<TournamentSelection>(tournamentSize);
}
