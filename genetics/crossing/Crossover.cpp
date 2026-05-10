#include "Crossover.h"

#include <algorithm>
#include <limits>
#include <random>

std::pair<Specimen, Specimen> Crossover::cross(
    const Specimen& parent1,
    const Specimen& parent2
) const
{
    if (parent1.empty() || parent2.empty())
    {
        return {parent1, parent2};
    }

    static thread_local std::mt19937 rng(std::random_device{}());

    const std::size_t size1 = parent1.size();
    const std::size_t size2 = parent2.size();

    std::uniform_int_distribution<std::size_t> dist1(0, size1);
    std::uniform_int_distribution<std::size_t> dist2(0, size2);

    const std::size_t cut1 = dist1(rng);
    const std::size_t cut2 = dist2(rng);

    Specimen child1;
    Specimen child2;

    for (std::size_t i = 0; i < cut1; ++i)
    {
        child1.addManeuver(parent1[i]);
    }

    for (std::size_t i = cut2; i < size2; ++i)
    {
        child1.addManeuver(parent2[i]);
    }

    for (std::size_t i = 0; i < cut2; ++i)
    {
        child2.addManeuver(parent2[i]);
    }

    for (std::size_t i = cut1; i < size1; ++i)
    {
        child2.addManeuver(parent1[i]);
    }

    child1.setFitness(std::numeric_limits<double>::max());
    child2.setFitness(std::numeric_limits<double>::max());

    return {std::move(child1), std::move(child2)};
}