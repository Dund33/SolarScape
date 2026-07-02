#include "RandomCutCrossover.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

namespace
{
    template <std::ranges::input_range ManeuverRange>
    void appendManeuvers(
        std::vector<Maneuver>& target,
        ManeuverRange&& maneuvers)
    {
        for (const Maneuver& maneuver : maneuvers)
        {
            target.push_back(maneuver);
        }
    }
}

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
    child1Maneuvers.reserve(cut1 + size2 - cut2);
    std::vector<Maneuver> child2Maneuvers;
    child2Maneuvers.reserve(cut2 + size1 - cut1);

    appendManeuvers(
        child1Maneuvers,
        parent1.getManeuvers() | std::views::take(cut1));
    appendManeuvers(
        child1Maneuvers,
        parent2.getManeuvers() | std::views::drop(cut2));
    appendManeuvers(
        child2Maneuvers,
        parent2.getManeuvers() | std::views::take(cut2));
    appendManeuvers(
        child2Maneuvers,
        parent1.getManeuvers() | std::views::drop(cut1));

    return {
        Specimen(std::move(child1Maneuvers)),
        Specimen(std::move(child2Maneuvers))};
}
