#include "RandomCutCrossover.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <utility>
#include <vector>

#include "config/consts.h"

namespace
{
    struct CutPoints
    {
        std::size_t first{};
        std::size_t second{};
    };

    std::size_t firstChildSize(
        std::size_t size2,
        CutPoints cuts)
    {
        return cuts.first + size2 - cuts.second;
    }

    std::size_t secondChildSize(
        std::size_t size1,
        CutPoints cuts)
    {
        return cuts.second + size1 - cuts.first;
    }

    std::vector<CutPoints> validCutPoints(
        std::size_t size1,
        std::size_t size2)
    {
        std::vector<CutPoints> cuts;
        cuts.reserve((size1 + 1) * (size2 + 1));

        for (std::size_t cut1 = 0; cut1 <= size1; ++cut1)
        {
            for (std::size_t cut2 = 0; cut2 <= size2; ++cut2)
            {
                const CutPoints cutPoints{cut1, cut2};

                if (firstChildSize(size2, cutPoints) >= MIN_MANEUVERS &&
                    secondChildSize(size1, cutPoints) >= MIN_MANEUVERS)
                {
                    cuts.push_back(cutPoints);
                }
            }
        }

        return cuts;
    }

    CutPoints randomCutPoints(
        std::size_t size1,
        std::size_t size2,
        std::mt19937& rng)
    {
        const std::vector<CutPoints> cuts =
            validCutPoints(
                size1,
                size2);

        if (!cuts.empty())
        {
            std::uniform_int_distribution<std::size_t> cutIndexDist(
                0,
                cuts.size() - 1);
            return cuts[cutIndexDist(rng)];
        }

        std::uniform_int_distribution<std::size_t> dist1(0, size1);
        std::uniform_int_distribution<std::size_t> dist2(0, size2);
        return {
            dist1(rng),
            dist2(rng)};
    }

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

    const CutPoints cuts =
        randomCutPoints(
            size1,
            size2,
            rng);

    std::vector<Maneuver> child1Maneuvers;
    child1Maneuvers.reserve(
        firstChildSize(
            size2,
            cuts));
    std::vector<Maneuver> child2Maneuvers;
    child2Maneuvers.reserve(
        secondChildSize(
            size1,
            cuts));

    appendManeuvers(
        child1Maneuvers,
        parent1.getManeuvers() | std::views::take(cuts.first));
    appendManeuvers(
        child1Maneuvers,
        parent2.getManeuvers() | std::views::drop(cuts.second));
    appendManeuvers(
        child2Maneuvers,
        parent2.getManeuvers() | std::views::take(cuts.second));
    appendManeuvers(
        child2Maneuvers,
        parent1.getManeuvers() | std::views::drop(cuts.first));

    return {
        Specimen(std::move(child1Maneuvers)),
        Specimen(std::move(child2Maneuvers))};
}
