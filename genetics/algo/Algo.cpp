#include "Algo.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <ranges>
#include <set>
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
    struct RunningStats
    {
        std::size_t count{};
        Real min{std::numeric_limits<Real>::max()};
        Real max{std::numeric_limits<Real>::lowest()};
        Real sum{};
        Real sumSquares{};

        void add(Real value)
        {
            ++count;
            min = std::min(min, value);
            max = std::max(max, value);
            sum += value;
            sumSquares += value * value;
        }

        Real mean() const
        {
            return count > 0
                ? sum / static_cast<Real>(count)
                : 0.0L;
        }

        Real stddev() const
        {
            if (count < 2)
            {
                return 0.0L;
            }

            const Real avg = mean();
            const Real variance =
                std::max(
                    0.0L,
                    sumSquares / static_cast<Real>(count) - avg * avg);

            return std::sqrt(variance);
        }
    };

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

    bool shouldReintroduceArchive(
        std::size_t generation)
    {
        return
            ALGO_ARCHIVE_REINTRODUCTION_INTERVAL > 0 &&
            ALGO_ARCHIVE_REINTRODUCTION_COUNT > 0 &&
            (generation + 1) % ALGO_ARCHIVE_REINTRODUCTION_INTERVAL == 0;
    }

    ParetoFront sortedArchiveByParetoRank(
        const ParetoFront& archive,
        const SpecimenComparator& specimenComparator)
    {
        ParetoFront sortedArchive = archive;

        sortPopulationByParetoRank(
            sortedArchive,
            specimenComparator);

        return sortedArchive;
    }

    void replaceTailWithArchiveSpecimens(
        std::vector<Specimen>& target,
        const ParetoFront& archive,
        std::size_t requestedCount,
        std::size_t startIndex,
        const SpecimenComparator& specimenComparator)
    {
        if (target.empty() || archive.empty() || requestedCount == 0)
        {
            return;
        }

        const std::size_t targetCount =
            std::min(requestedCount, target.size());
        std::size_t replacedCount = 0;
        std::size_t scannedCount = 0;

        while (
            replacedCount < targetCount &&
            scannedCount < archive.size())
        {
            const Specimen& candidate =
                archive[(startIndex + scannedCount) % archive.size()];
            ++scannedCount;

            if (
                containsSameObjectiveValues(
                    target,
                    candidate,
                    specimenComparator))
            {
                continue;
            }

            target[target.size() - 1 - replacedCount] =
                candidate;
            ++replacedCount;
        }
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

    Real normalizedDifference(
        Real lhs,
        Real rhs)
    {
        const Real scale =
            std::max({
                1.0L,
                std::abs(lhs),
                std::abs(rhs)});

        return std::min(
            1.0L,
            std::abs(lhs - rhs) / scale);
    }

    Real directionDistance(
        const Vector3& lhs,
        const Vector3& rhs)
    {
        return std::min(
            1.0L,
            (lhs - rhs).length() * 0.5L);
    }

    Real maneuverDistance(
        const Maneuver& lhs,
        const Maneuver& rhs)
    {
        return (
            std::abs(
                lhs.getThrottleValue() -
                rhs.getThrottleValue()) +
            directionDistance(
                lhs.getThrustDirection(),
                rhs.getThrustDirection()) +
            normalizedDifference(
                lhs.getInitDelay(),
                rhs.getInitDelay()) +
            normalizedDifference(
                lhs.getDuration(),
                rhs.getDuration())) *
            0.25L;
    }

    Real specimenDistance(
        const Specimen& lhs,
        const Specimen& rhs)
    {
        const std::size_t maxSize =
            std::max(lhs.size(), rhs.size());

        if (maxSize == 0)
        {
            return 0.0L;
        }

        Real distance = 0.0L;
        const std::size_t commonSize =
            std::min(lhs.size(), rhs.size());

        for (std::size_t i = 0; i < commonSize; ++i)
        {
            distance +=
                maneuverDistance(
                    lhs[i],
                    rhs[i]);
        }

        distance += static_cast<Real>(maxSize - commonSize);

        return distance / static_cast<Real>(maxSize);
    }

    Real averagePairwiseSpecimenDistance(
        const std::vector<const Specimen*>& population)
    {
        if (population.size() < 2)
        {
            return 0.0L;
        }

        Real distanceSum = 0.0L;
        std::size_t pairCount = 0;

        for (std::size_t i = 0; i < population.size(); ++i)
        {
            for (std::size_t j = i + 1; j < population.size(); ++j)
            {
                distanceSum +=
                    specimenDistance(
                        *population[i],
                        *population[j]);
                ++pairCount;
            }
        }

        return distanceSum / static_cast<Real>(pairCount);
    }

    std::string specimenKey(
        const Specimen& specimen)
    {
        std::ostringstream key;
        key << std::setprecision(
            std::numeric_limits<Real>::max_digits10);

        for (const Maneuver& maneuver : specimen.getManeuvers())
        {
            const Vector3& direction =
                maneuver.getThrustDirection();

            key
                << direction.x << ','
                << direction.y << ','
                << direction.z << ','
                << maneuver.getThrottleValue() << ','
                << maneuver.getInitDelay() << ','
                << maneuver.getDuration() << ';';
        }

        return key.str();
    }

    std::vector<const Specimen*> populationView(
        const std::vector<std::vector<Specimen>>& islands)
    {
        std::vector<const Specimen*> population;

        for (const Specimen& specimen : islands | std::views::join)
        {
            population.push_back(&specimen);
        }

        return population;
    }

    void printDiversityDiagnostics(
        std::size_t generation,
        const std::vector<std::vector<Specimen>>& islands,
        std::ostream& output)
    {
        const std::vector<const Specimen*> population =
            populationView(
                islands);

        RunningStats maneuverCounts;
        RunningStats distanceValues;
        RunningStats timeValues;
        RunningStats fuelValues;
        RunningStats fuelViolationValues;
        std::set<std::string> uniqueGenomes;

        for (const Specimen* specimen : population)
        {
            maneuverCounts.add(
                static_cast<Real>(specimen->size()));
            uniqueGenomes.insert(
                specimenKey(
                    *specimen));

            const FitnessValue& fitness =
                specimen->getFitness().value();
            distanceValues.add(
                fitness.minimumDistance);
            timeValues.add(
                fitness.minimumDistanceTime);
            fuelValues.add(
                fitness.fuelUsed);
            fuelViolationValues.add(
                fitness.fuelConstraintViolation);
        }

        output
            << "ALGO diversity generation " << generation
            << " | population_size=" << population.size()
            << " | unique_genomes=" << uniqueGenomes.size()
            << " | unique_ratio="
            << (
                population.empty()
                    ? 0.0L
                    : static_cast<Real>(uniqueGenomes.size()) /
                        static_cast<Real>(population.size()))
            << " | maneuver_count=["
            << maneuverCounts.min
            << ", " << maneuverCounts.max
            << "] avg=" << maneuverCounts.mean()
            << " stddev=" << maneuverCounts.stddev()
            << " | objective_stddev=[distance="
            << distanceValues.stddev()
            << ", time=" << timeValues.stddev()
            << ", fuel=" << fuelValues.stddev()
            << ", fuel_violation=" << fuelViolationValues.stddev()
            << "] | avg_pairwise_maneuver_distance="
            << averagePairwiseSpecimenDistance(
                population)
            << " | island_best_distance=[";

        for (std::size_t islandIndex = 0;
             islandIndex < islands.size();
             ++islandIndex)
        {
            if (islandIndex > 0)
            {
                output << ", ";
            }

            if (islands[islandIndex].empty())
            {
                output << "nan";
                continue;
            }

            output
                << islands[islandIndex]
                    .front()
                    .getFitness()
                    .value()
                    .minimumDistance;
        }

        output << "]\n";
    }
}

Algo::Algo(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t eliteCount,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories,
    bool verbose,
    std::ostream* diversityLog
)
    : populationSize(populationSize),
      generations(generations),
      eliteCount(eliteCount),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories),
      verbose(verbose),
      diversityLog(diversityLog)
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

        if (shouldReintroduceArchive(generation))
        {
            reintroduceArchive(
                nextIslands,
                archive,
                generation);
        }

        if (verbose)
        {
            printGenerationResult(
                generation,
                nextIslands,
                archive);
        }

        if (diversityLog != nullptr)
        {
            printDiversityDiagnostics(
                generation,
                nextIslands,
                *diversityLog);
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

void Algo::reintroduceArchive(
    Islands& islands,
    const ParetoFront& archive,
    std::size_t generation) const
{
    if (islands.empty() || archive.empty())
    {
        return;
    }

    const ParetoFront sortedArchive =
        sortedArchiveByParetoRank(
            archive,
            specimenComparator);
    std::size_t archiveIndex =
        generation * ALGO_ARCHIVE_REINTRODUCTION_COUNT;

    for (std::vector<Specimen>& island : islands)
    {
        const std::size_t replacementCount =
            archiveReintroductionCountForIsland(
                island.size());

        replaceTailWithArchiveSpecimens(
            island,
            sortedArchive,
            replacementCount,
            archiveIndex,
            specimenComparator);

        archiveIndex += replacementCount;

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

std::size_t Algo::archiveReintroductionCountForIsland(
    std::size_t islandSize) const
{
    const std::size_t islandEliteCount = std::min(eliteCount, islandSize);
    const std::size_t replaceableCount =
        islandSize - islandEliteCount;

    return std::min(
        replaceableCount,
        islandSize * ALGO_ARCHIVE_REINTRODUCTION_COUNT / populationSize);
}
