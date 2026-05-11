#include "Crossover.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <utility>

std::pair<Specimen, Specimen> Crossover::cross(
    const Specimen& parent1,
    const Specimen& parent2
) const
{
    if (parent1.empty() || parent2.empty())
    {
        return {
            Specimen(parent1.getManeuvers(), parent1.getProbe()),
            Specimen(parent2.getManeuvers(), parent2.getProbe())
        };
    }

    static thread_local std::mt19937 rng(std::random_device{}());

    const std::size_t size1 = parent1.size();
    const std::size_t size2 = parent2.size();

    std::uniform_int_distribution<std::size_t> dist1(0, size1);
    std::uniform_int_distribution<std::size_t> dist2(0, size2);

    const std::size_t cut1 = dist1(rng);
    const std::size_t cut2 = dist2(rng);

    Specimen child1(parent1.getProbe());
    Specimen child2(parent1.getProbe());

    auto appendTo = [](Specimen& child)
    {
        return [&child](const Maneuver& maneuver)
        {
            child.addManeuver(maneuver);
        };
    };

    std::ranges::for_each(
        parent1.getManeuvers() | std::views::take(cut1),
        appendTo(child1));
    std::ranges::for_each(
        parent2.getManeuvers() | std::views::drop(cut2),
        appendTo(child1));
    std::ranges::for_each(
        parent2.getManeuvers() | std::views::take(cut2),
        appendTo(child2));
    std::ranges::for_each(
        parent1.getManeuvers() | std::views::drop(cut1),
        appendTo(child2));

    return {std::move(child1), std::move(child2)};
}
