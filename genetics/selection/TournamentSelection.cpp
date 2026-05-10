//
// Created by Luke on 5/10/2026.
//

#include "TournamentSelection.h"

#include <stdexcept>
#include <random>

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
    double bestFitness = population[bestIndex].getFitness();

    // Mniejszy fitness = lepszy osobnik
    for (std::size_t i = 1; i < tournamentSize; ++i)
    {
        std::size_t candidateIndex = dist(rng);
        double candidateFitness = population[candidateIndex].getFitness();

        if (candidateFitness < bestFitness)
        {
            bestFitness = candidateFitness;
            bestIndex = candidateIndex;
        }
    }

    return population[bestIndex];
}