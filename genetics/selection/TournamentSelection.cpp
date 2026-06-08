#include "TournamentSelection.h"

#include <random>
#include <stdexcept>

#include "genetics/comparison/SpecimenComparator.h"

TournamentSelection::TournamentSelection(std::size_t tournamentSize)
    : tournamentSize(tournamentSize)
{
    if (tournamentSize == 0)
    {
        throw std::invalid_argument("Tournament size must be greater than zero.");
    }
}

const Specimen& TournamentSelection::select(
    const std::vector<Specimen>& population,
    const SpecimenComparator& specimenComparator
) const
{
    if (population.empty())
    {
        throw std::invalid_argument("Population cannot be empty.");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> dist(0, population.size() - 1);

    std::size_t bestIndex = dist(rng);

    for (std::size_t i = 1; i < tournamentSize; ++i)
    {
        std::size_t candidateIndex = dist(rng);

        if (specimenComparator.isLess(
            population[candidateIndex],
            population[bestIndex]))
        {
            bestIndex = candidateIndex;
        }
    }

    return population[bestIndex];
}
