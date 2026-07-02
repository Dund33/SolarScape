#include "Algo.h"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "genetics/crossing/Crossover.h"
#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/init/Initializer.h"
#include "genetics/mutation/Mutation.h"
#include "genetics/selection/Selection.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"

namespace
{
    constexpr std::size_t TARGET_ISLAND_COUNT = 8;
    constexpr std::size_t MIGRATION_INTERVAL = 10;

    void sortPopulationByFitness(
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

    std::string islandSizesDetails(
        const std::vector<std::vector<Specimen>>& islands)
    {
        std::ostringstream details;
        details << "islands=[";

        for (std::size_t i = 0; i < islands.size(); ++i)
        {
            if (i > 0)
            {
                details << ", ";
            }

            details << islands[i].size();
        }

        details << ']';

        return details.str();
    }

    void printGenerationResult(
        std::size_t generation,
        const std::vector<std::vector<Specimen>>& islands,
        const ParetoFront& paretoFront)
    {
        GenerationProgressLogger::print(
            "ALGO",
            generation,
            ParetoFrontUtils::calculateStats(
                paretoFront),
            islandSizesDetails(
                islands));
    }
}

Algo::Algo(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t eliteCount,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories,
    bool verbose
)
    : populationSize(populationSize),
      generations(generations),
      eliteCount(eliteCount),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories),
      verbose(verbose)
{
    if (populationSize == 0)
    {
        throw std::invalid_argument(
            "Population size must be greater than zero.");
    }
}

ParetoFrontHistory Algo::run() const
{
    auto initializer =
        factories.initializerFactory.create();
    auto selection =
        factories.selectionFactory.create();
    auto crossover =
        factories.crossoverFactory.create();
    auto mutation =
        factories.mutationFactory.create();
    auto fitnessEvaluator =
        factories.fitnessEvaluatorFactory.create();

    Islands islands =
        createIslands(
            *initializer);

    evaluateIslands(
        islands,
        *fitnessEvaluator);
    sortIslands(
        islands);

    ParetoFrontHistory history;
    history.reserve(
        generations);

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        Islands nextIslands;
        nextIslands.reserve(
            islands.size());

        for (const auto& island : islands)
        {
            nextIslands.push_back(
                createNextIsland(
                    island,
                    *initializer,
                    *selection,
                    *crossover,
                    *mutation));
        }

        evaluateIslands(
            nextIslands,
            *fitnessEvaluator);
        sortIslands(
            nextIslands);

        if (
            MIGRATION_INTERVAL > 0 &&
            (generation + 1) % MIGRATION_INTERVAL == 0)
        {
            migrate(
                nextIslands);
        }

        ParetoFront paretoFront =
            ParetoFrontUtils::firstFront(
                nextIslands |
                std::views::join,
                specimenComparator);

        if (verbose)
        {
            printGenerationResult(
                generation,
                nextIslands,
                paretoFront);
        }

        history.push_back(
            std::move(
                paretoFront));
        islands =
            std::move(
                nextIslands);
    }

    return history;
}

Algo::Islands Algo::createIslands(
    Initializer& initializer) const
{
    const std::size_t islandCount =
        std::min(
            TARGET_ISLAND_COUNT,
            populationSize);
    const std::size_t baseIslandSize =
        populationSize / islandCount;
    const std::size_t largerIslandCount =
        populationSize % islandCount;

    Islands islands;
    islands.reserve(
        islandCount);

    for (std::size_t islandIndex = 0;
         islandIndex < islandCount;
         ++islandIndex)
    {
        const std::size_t islandSize =
            baseIslandSize +
            (islandIndex < largerIslandCount ? 1 : 0);

        islands.push_back(
            initializer.createPopulation(
                islandSize));
    }

    return islands;
}

void Algo::evaluateIslands(
    Islands& islands,
    const FitnessEvaluator& fitnessEvaluator) const
{
    std::vector<Specimen*> specimens;
    specimens.reserve(
        populationSize);

    for (Specimen& specimen : islands | std::views::join)
    {
        specimens.push_back(
            &specimen);
    }

    evaluateSpecimensUnsequenced(
        specimens,
        fitnessEvaluator);
}

void Algo::sortIslands(
    Islands& islands) const
{
    for (auto& island : islands)
    {
        sortPopulationByFitness(
            island,
            specimenComparator);
    }
}

auto Algo::createNextIsland(
    const std::vector<Specimen>& island,
    Initializer& initializer,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation) const -> std::vector<Specimen>
{
    const std::size_t islandSize =
        island.size();
    std::vector<Specimen> nextIsland;
    nextIsland.reserve(
        islandSize);

    std::ranges::copy(
        island |
        std::views::take(
            std::min(
                eliteCount,
                islandSize)),
        std::back_inserter(
            nextIsland));

    appendChildren(
        island,
        nextIsland,
        islandSize,
        specimenComparator,
        selection,
        crossover,
        mutation);

    replaceTailWithImmigrants(
        nextIsland,
        immigrantCountForIsland(
            islandSize),
        initializer);

    return nextIsland;
}

void Algo::migrate(
    Islands& islands) const
{
    if (islands.size() < 2 || eliteCount == 0)
    {
        return;
    }

    const std::size_t migrantCount =
        std::max<std::size_t>(
            1,
            eliteCount);
    Islands migrants;
    migrants.reserve(
        islands.size());

    for (const auto& island : islands)
    {
        std::vector<Specimen> islandMigrants;
        std::ranges::copy(
            island |
            std::views::take(
                std::min(
                    migrantCount,
                    island.size())),
            std::back_inserter(
                islandMigrants));
        migrants.push_back(
            std::move(
                islandMigrants));
    }

    for (std::size_t islandIndex = 0;
         islandIndex < islands.size();
         ++islandIndex)
    {
        std::vector<Specimen>& targetIsland =
            islands[(islandIndex + 1) % islands.size()];
        const std::vector<Specimen>& sourceMigrants =
            migrants[islandIndex];
        const std::size_t replacementCount =
            std::min(
                sourceMigrants.size(),
                targetIsland.size());

        for (std::size_t migrantIndex = 0;
             migrantIndex < replacementCount;
             ++migrantIndex)
        {
            targetIsland[targetIsland.size() - 1 - migrantIndex] =
                sourceMigrants[migrantIndex];
        }
    }

    for (auto& island : islands)
    {
        sortPopulationByFitness(
            island,
            specimenComparator);
    }
}

std::size_t Algo::immigrantCountForIsland(
    std::size_t islandSize) const
{
    const std::size_t islandEliteCount =
        std::min(
            eliteCount,
            islandSize);
    const std::size_t replaceableCount =
        islandSize - islandEliteCount;

    return std::min(
        replaceableCount,
        islandSize * immigrantCount / populationSize);
}
