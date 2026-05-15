//
// Created by Luke on 5/10/2026.
//

#include "TournamentSelection.h"

#include <functional>
#include <random>
#include <stdexcept>

TournamentSelection::TournamentSelection(std::size_t tournamentSize)
    : tournamentSize(tournamentSize)
{
    if (tournamentSize == 0)
    {
        throw std::invalid_argument("Tournament size must be greater than zero.");
    }
}

const Specimen& TournamentSelection::select(
    const std::vector<Specimen>& population
) const
{
    if (population.empty())
    {
        throw std::invalid_argument("Population cannot be empty.");
    }

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> dist(0, population.size() - 1);

    std::size_t bestIndex = dist(rng);

    // Operator < porównuje osobniki według dominacji FitnessResult.
    const std::less<Specimen> isBetter;
    for (std::size_t i = 1; i < tournamentSize; ++i)
    {
        std::size_t candidateIndex = dist(rng);

        if (isBetter(population[candidateIndex], population[bestIndex]))
        {
            bestIndex = candidateIndex;
        }
    }

    return population[bestIndex];
}
