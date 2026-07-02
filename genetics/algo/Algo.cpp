#include "Algo.h"

#include <algorithm>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "config/consts.h"
#include "genetics/comparison/NSGAIIRankingComparator.h"
#include "genetics/crossing/Crossover.h"
#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/init/Initializer.h"
#include "genetics/mutation/Mutation.h"
#include "genetics/selection/Selection.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"
#include "genetics/utils/ParetoRanking.h"

namespace
{
    void sortPopulationByParetoRank(
        std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator)
    {
        if (population.size() < 2)
        {
            return;
        }

        const ParetoRankedPopulation rankedPopulation =
            ParetoRanking::rankPopulation(
                population,
                specimenComparator);
        const std::vector<std::size_t> sortedIndices =
            ParetoRanking::sortedIndices(
                population,
                rankedPopulation,
                specimenComparator);

        std::vector<Specimen> sortedPopulation;
        sortedPopulation.reserve(population.size());

        for (std::size_t specimenIndex : sortedIndices)
        {
            sortedPopulation.push_back(std::move(population[specimenIndex]));
        }

        population = std::move(sortedPopulation);
    }

    bool hasSameObjectiveValues(
        const Specimen& lhs,
        const Specimen& rhs,
        const SpecimenComparator& specimenComparator)
    {
        for (std::size_t objective = 0;
             objective < specimenComparator.objectiveCount();
             ++objective)
        {
            if (
                specimenComparator.objectiveValue(
                    lhs.getFitness().value(),
                    objective) !=
                specimenComparator.objectiveValue(
                    rhs.getFitness().value(),
                    objective))
            {
                return false;
            }
        }

        return true;
    }

    bool containsSameObjectiveValues(
        const ParetoFront& front,
        const Specimen& specimen,
        const SpecimenComparator& specimenComparator)
    {
        return std::ranges::any_of(
            front,
            [&](const Specimen& frontSpecimen)
            {
                return hasSameObjectiveValues(
                    frontSpecimen,
                    specimen,
                    specimenComparator);
            });
    }

    void appendDistinctByObjectiveValues(
        ParetoFront& target,
        ParetoFront& source,
        const SpecimenComparator& specimenComparator)
    {
        for (Specimen& specimen : source)
        {
            if (
                containsSameObjectiveValues(
                    target,
                    specimen,
                    specimenComparator))
            {
                continue;
            }

            target.push_back(std::move(specimen));
        }
    }

    void appendFirstSpecimens(
        std::vector<Specimen>& target,
        const std::vector<Specimen>& source,
        std::size_t requestedCount)
    {
        const std::size_t count = std::min(requestedCount, source.size());

        for (const Specimen& specimen : source | std::views::take(count))
        {
            target.push_back(specimen);
        }
    }

    std::vector<Specimen> copyFirstSpecimens(
        const std::vector<Specimen>& source,
        std::size_t requestedCount)
    {
        std::vector<Specimen> copiedSpecimens;
        copiedSpecimens.reserve(std::min(requestedCount, source.size()));

        appendFirstSpecimens(
            copiedSpecimens,
            source,
            requestedCount);

        return copiedSpecimens;
    }

    ParetoFront updateParetoArchive(
        ParetoFront archive,
        ParetoFront currentFront,
        const SpecimenComparator& specimenComparator)
    {
        ParetoFront candidates;
        candidates.reserve(archive.size() + currentFront.size());

        appendDistinctByObjectiveValues(
            candidates,
            archive,
            specimenComparator);
        appendDistinctByObjectiveValues(
            candidates,
            currentFront,
            specimenComparator);

        if (candidates.empty())
        {
            return {};
        }

        return ParetoFrontUtils::firstFront(
            candidates,
            specimenComparator);
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
    history.reserve(generations);
    ParetoFront archive =
        ParetoFrontUtils::firstFront(
            islands |
            std::views::join,
            specimenComparator);

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        Islands nextIslands;
        nextIslands.reserve(islands.size());

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
            ALGO_MIGRATION_INTERVAL > 0 &&
            (generation + 1) % ALGO_MIGRATION_INTERVAL == 0)
        {
            migrate(
                nextIslands);
        }

        ParetoFront currentFront =
            ParetoFrontUtils::firstFront(
                nextIslands |
                std::views::join,
                specimenComparator);
        archive = updateParetoArchive(
            std::move(archive),
            std::move(currentFront),
            specimenComparator);

        if (verbose)
        {
            printGenerationResult(
                generation,
                nextIslands,
                archive);
        }

        history.push_back(archive);
        islands = std::move(nextIslands);
    }

    return history;
}

Algo::Islands Algo::createIslands(
    Initializer& initializer) const
{
    const std::size_t islandCount =
        std::min(ALGO_TARGET_ISLAND_COUNT, populationSize);
    const std::size_t baseIslandSize =
        populationSize / islandCount;
    const std::size_t largerIslandCount =
        populationSize % islandCount;

    Islands islands;
    islands.reserve(islandCount);

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
    specimens.reserve(populationSize);

    for (Specimen& specimen : islands | std::views::join)
    {
        specimens.push_back(&specimen);
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
        sortPopulationByParetoRank(
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
    nextIsland.reserve(islandSize);

    appendFirstSpecimens(
        nextIsland,
        island,
        eliteCount);

    const ParetoRankedPopulation rankedIsland =
        ParetoRanking::rankPopulation(
            island,
            specimenComparator);
    const NSGAIIRankingComparator selectionComparator(
        island,
        rankedIsland.ranks,
        specimenComparator);

    appendChildren(
        island,
        nextIsland,
        islandSize,
        selectionComparator,
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
        std::max(ALGO_MIN_MIGRANT_COUNT, eliteCount);
    Islands migrants;
    migrants.reserve(islands.size());

    for (const auto& island : islands)
    {
        migrants.push_back(copyFirstSpecimens(island, migrantCount));
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
            std::min(sourceMigrants.size(), targetIsland.size());

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
        sortPopulationByParetoRank(
            island,
            specimenComparator);
    }
}

std::size_t Algo::immigrantCountForIsland(
    std::size_t islandSize) const
{
    const std::size_t islandEliteCount = std::min(eliteCount, islandSize);
    const std::size_t replaceableCount =
        islandSize - islandEliteCount;

    return std::min(
        replaceableCount,
        islandSize * immigrantCount / populationSize);
}
