#include "ParetoFrontUtils.h"

#include <algorithm>
#include <ranges>

std::vector<Specimen> ParetoFrontUtils::frontFromIndices(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    std::vector<Specimen> front;
    front.reserve(frontIndices.size());

    const auto frontSpecimens =
        frontIndices |
        std::views::transform(
            [&population](std::size_t specimenIndex) -> const Specimen&
            {
                return population[specimenIndex];
            });

    for (const Specimen& specimen : frontSpecimens)
    {
        front.push_back(specimen);
    }

    return front;
}

ParetoFrontStats ParetoFrontUtils::calculateStats(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    return calculateStats(
        frontIndices |
        std::views::transform(
            [&population](std::size_t specimenIndex) -> const Specimen&
            {
                return population[specimenIndex];
            }));
}
