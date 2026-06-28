#include "PopulationPyramid.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace
{
    std::vector<std::size_t> calculateLevelSizes(
        std::size_t populationSize)
    {
        if (populationSize == 0)
        {
            throw std::invalid_argument(
                "Population size must be greater than zero.");
        }

        std::vector<std::size_t> levelSizes;
        std::size_t remaining = populationSize;

        while (remaining > 0)
        {
            const std::size_t levelSize =
                (remaining + 1) / 2;

            levelSizes.push_back(levelSize);
            remaining -= levelSize;
        }

        return levelSizes;
    }

    void sortPopulation(
        std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator)
    {
        std::ranges::sort(
            population,
            [&specimenComparator](
                const Specimen& lhs,
                const Specimen& rhs)
            {
                return specimenComparator.isLess(
                    lhs,
                    rhs);
            });
    }
}

PopulationPyramid::PopulationPyramid(
    std::vector<std::vector<Specimen>> levels)
    : levels_(std::move(levels))
{
}

PopulationPyramid PopulationPyramid::create(
    std::size_t populationSize,
    Initializer& initializer)
{
    std::vector<std::vector<Specimen>> levels;

    for (std::size_t levelSize :
         calculateLevelSizes(populationSize))
    {
        levels.push_back(
            initializer.createPopulation(
                levelSize));
    }

    return PopulationPyramid(
        std::move(levels));
}

std::vector<std::vector<Specimen>>& PopulationPyramid::levels()
{
    return levels_;
}

const std::vector<std::vector<Specimen>>& PopulationPyramid::levels() const
{
    return levels_;
}

void PopulationPyramid::promoteElite(
    std::size_t eliteCount,
    const SpecimenComparator& specimenComparator)
{
    if (eliteCount == 0)
    {
        return;
    }

    for (std::size_t level = 0;
         level + 1 < levels_.size();
         ++level)
    {
        const std::vector<Specimen>& source = levels_[level];
        std::vector<Specimen>& target = levels_[level + 1];

        if (source.empty() || target.empty())
        {
            continue;
        }

        sortPopulation(
            target,
            specimenComparator);

        const std::size_t promotionCount =
            std::min({
                eliteCount,
                source.size(),
                target.size()});

        for (std::size_t i = 0; i < promotionCount; ++i)
        {
            const std::size_t targetIndex =
                target.size() - 1 - i;

            if (specimenComparator.isLess(
                source[i],
                target[targetIndex]))
            {
                target[targetIndex] = source[i];
            }
        }

        sortPopulation(
            target,
            specimenComparator);
    }
}

std::vector<Specimen> PopulationPyramid::flatten() const
{
    const std::size_t specimenCount =
        std::accumulate(
            levels_.begin(),
            levels_.end(),
            std::size_t{0},
            [](std::size_t total, const auto& level)
            {
                return total + level.size();
            });

    std::vector<Specimen> population;
    population.reserve(specimenCount);

    for (const auto& level : levels_)
    {
        std::ranges::copy(
            level,
            std::back_inserter(population));
    }

    return population;
}
