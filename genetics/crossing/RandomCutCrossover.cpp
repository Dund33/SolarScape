#include "RandomCutCrossover.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

std::pair<Specimen, Specimen> RandomCutCrossover::cross(
    const Specimen& parent1,
    const Specimen& parent2
) const
{
    if (parent1.empty() || parent2.empty())
    {
        return {
            Specimen(parent1.getManeuvers()),
            Specimen(parent2.getManeuvers())
        };
    }

    static thread_local std::mt19937 rng(std::random_device{}());

    const std::size_t size1 = parent1.size();
    const std::size_t size2 = parent2.size();

    std::uniform_int_distribution<std::size_t> dist1(0, size1);
    std::uniform_int_distribution<std::size_t> dist2(0, size2);

    const std::size_t cut1 = dist1(rng);
    const std::size_t cut2 = dist2(rng);

    std::vector<Maneuver> child1Maneuvers;
    child1Maneuvers.reserve(
        cut1 + size2 - cut2);
    std::vector<Maneuver> child2Maneuvers;
    child2Maneuvers.reserve(
        cut2 + size1 - cut1);

    std::ranges::copy(
        parent1.getManeuvers() | std::views::take(cut1),
        std::back_inserter(
            child1Maneuvers));
    std::ranges::copy(
        parent2.getManeuvers() | std::views::drop(cut2),
        std::back_inserter(
            child1Maneuvers));
    std::ranges::copy(
        parent2.getManeuvers() | std::views::take(cut2),
        std::back_inserter(
            child2Maneuvers));
    std::ranges::copy(
        parent1.getManeuvers() | std::views::drop(cut1),
        std::back_inserter(
            child2Maneuvers));

    return {
        Specimen(
            std::move(
                child1Maneuvers)),
        Specimen(
            std::move(
                child2Maneuvers))};
}
