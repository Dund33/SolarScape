#include "ParetoFrontUtils.h"

#include <algorithm>
#include <iterator>
#include <ranges>

std::vector<Specimen> ParetoFrontUtils::frontFromIndices(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    std::vector<Specimen> front;
    front.reserve(
        frontIndices.size());

    std::ranges::transform(
        frontIndices,
        std::back_inserter(
            front),
        [&population](std::size_t specimenIndex)
        {
            return population[specimenIndex];
        });

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
