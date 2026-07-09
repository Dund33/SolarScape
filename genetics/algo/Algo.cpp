#include "Algo.h"

#include <algorithm>
#include <cmath>
#include <compare>
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
#include "genetics/utils/Refinement.h"

namespace
{
    constexpr Real DirectionDistanceScale = 0.5;
    constexpr Real ManeuverDistanceComponentWeight = 0.25;

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
            return count > 0 ? sum / static_cast<Real>(count) : 0.0;
        }

        Real stddev() const
        {
            if (count < 2)
            {
                return 0.0;
            }

            const Real avg = mean();
            const Real variance = std::max(0.0, sumSquares / static_cast<Real>(count) - avg * avg);

            return std::sqrt(variance);
        }
    };

    void sortByRankAndCrowding(std::vector<Specimen>& population, const SpecimenComparator& specimenComparator)
    {
        if (population.size() < 2)
        {
            return;
        }

        const ParetoRankedPopulation rankedPopulation = ParetoRanking::rankPopulation(population, specimenComparator);
        const std::vector<std::size_t> sortedIndices = ParetoRanking::sortedIndices(population, rankedPopulation, specimenComparator);

        std::vector<Specimen> sortedPopulation;
        sortedPopulation.reserve(population.size());

        for (std::size_t specimenIndex : sortedIndices)
        {
            sortedPopulation.push_back(std::move(population[specimenIndex]));
        }

        population = std::move(sortedPopulation);
    }

    auto specimensIn(auto& islands)
    {
        return islands | std::views::transform([](auto& island) -> decltype(auto) { return (island.specimens); }) | std::views::join;
    }

    bool isEquivalent(const Specimen& lhs, const Specimen& rhs, const SpecimenComparator& specimenComparator)
    {
        return specimenComparator.compare(lhs, rhs) == std::partial_ordering::equivalent;
    }

    bool containsEquivalentSpecimen(const ParetoFront& front, const Specimen& specimen, const SpecimenComparator& specimenComparator)
    {
        return std::ranges::any_of(
            front, [&](const Specimen& frontSpecimen) { return isEquivalent(frontSpecimen, specimen, specimenComparator); });
    }

    void appendDistinctSpecimens(ParetoFront& target, ParetoFront& source, const SpecimenComparator& specimenComparator)
    {
        for (Specimen& specimen : source)
        {
            if (containsEquivalentSpecimen(target, specimen, specimenComparator))
            {
                continue;
            }

            target.push_back(std::move(specimen));
        }
    }

    void appendTopRankedSpecimens(std::vector<Specimen>& target, const std::vector<Specimen>& source,
                                  const std::vector<std::size_t>& sortedIndices, std::size_t requestedCount)
    {
        for (std::size_t specimenIndex : sortedIndices | std::views::take(requestedCount))
        {
            target.push_back(source[specimenIndex]);
        }
    }

    bool shouldReintroduceArchive(std::size_t generation)
    {
        return ALGO_ARCHIVE_REINTRODUCTION_INTERVAL > 0 && ALGO_ARCHIVE_REINTRODUCTION_COUNT > 0 &&
               (generation + 1) % ALGO_ARCHIVE_REINTRODUCTION_INTERVAL == 0;
    }

    void replaceWorstWithArchiveSpecimens(std::vector<Specimen>& target, const std::vector<std::size_t>& sortedTargetIndices,
                                          const ParetoFront& archive, std::size_t requestedCount, std::size_t startIndex,
                                          const SpecimenComparator& specimenComparator)
    {
        if (target.empty() || sortedTargetIndices.empty() || archive.empty() || requestedCount == 0)
        {
            return;
        }

        const std::size_t targetCount = std::min(requestedCount, sortedTargetIndices.size());
        std::size_t replacedCount = 0;
        std::size_t scannedCount = 0;

        while (replacedCount < targetCount && scannedCount < archive.size())
        {
            const Specimen& candidate = archive[(startIndex + scannedCount) % archive.size()];
            ++scannedCount;

            if (containsEquivalentSpecimen(target, candidate, specimenComparator))
            {
                continue;
            }

            const std::size_t replacementIndex = sortedTargetIndices[sortedTargetIndices.size() - 1 - replacedCount];

            target[replacementIndex] = candidate;
            ++replacedCount;
        }
    }

    ParetoFront firstFrontOf(auto& islands, const SpecimenComparator& specimenComparator)
    {
        return ParetoFrontUtils::firstFront(specimensIn(islands), specimenComparator);
    }

    ParetoFront updateParetoArchive(ParetoFront archive, ParetoFront newFront, const SpecimenComparator& specimenComparator)
    {
        ParetoFront candidates;
        candidates.reserve(archive.size() + newFront.size());

        appendDistinctSpecimens(candidates, archive, specimenComparator);
        appendDistinctSpecimens(candidates, newFront, specimenComparator);

        if (candidates.empty())
        {
            return {};
        }

        return ParetoFrontUtils::firstFront(candidates, specimenComparator);
    }

    std::string islandSizesDetails(const auto& islands)
    {
        std::ostringstream details;
        details << "islands=[";

        for (std::size_t i = 0; i < islands.size(); ++i)
        {
            if (i > 0)
            {
                details << ", ";
            }

            details << islands[i].specimens.size();
        }

        details << ']';

        return details.str();
    }

    void printGenerationResult(std::size_t generation, const auto& islands, const ParetoFront& paretoFront)
    {
        GenerationProgressLogger::print("ALGO", generation, ParetoFrontUtils::calculateStats(paretoFront), islandSizesDetails(islands));
    }

    Real normalizedDifference(Real lhs, Real rhs)
    {
        const Real scale = std::max({1.0, std::abs(lhs), std::abs(rhs)});

        return std::min(1.0, std::abs(lhs - rhs) / scale);
    }

    Real directionDistance(const Vector3& lhs, const Vector3& rhs)
    {
        return std::min(1.0, (lhs - rhs).length() * DirectionDistanceScale);
    }

    Real maneuverDistance(const Maneuver& lhs, const Maneuver& rhs)
    {
        return (std::abs(lhs.getThrottleValue() - rhs.getThrottleValue()) +
                directionDistance(lhs.getThrustDirection(), rhs.getThrustDirection()) +
                normalizedDifference(lhs.getInitDelay(), rhs.getInitDelay()) + normalizedDifference(lhs.getDuration(), rhs.getDuration())) *
               ManeuverDistanceComponentWeight;
    }

    Real specimenDistance(const Specimen& lhs, const Specimen& rhs)
    {
        const std::size_t maxSize = std::max(lhs.size(), rhs.size());

        if (maxSize == 0)
        {
            return 0.0;
        }

        Real distance = 0.0;
        const std::size_t commonSize = std::min(lhs.size(), rhs.size());

        for (std::size_t i = 0; i < commonSize; ++i)
        {
            distance += maneuverDistance(lhs[i], rhs[i]);
        }

        distance += static_cast<Real>(maxSize - commonSize);

        return distance / static_cast<Real>(maxSize);
    }

    Real averagePairwiseSpecimenDistance(const std::vector<const Specimen*>& population)
    {
        if (population.size() < 2)
        {
            return 0.0;
        }

        Real distanceSum = 0.0;
        std::size_t pairCount = 0;

        for (std::size_t i = 0; i < population.size(); ++i)
        {
            for (std::size_t j = i + 1; j < population.size(); ++j)
            {
                distanceSum += specimenDistance(*population[i], *population[j]);
                ++pairCount;
            }
        }

        return distanceSum / static_cast<Real>(pairCount);
    }

    std::string specimenKey(const Specimen& specimen)
    {
        std::ostringstream key;
        key << std::setprecision(std::numeric_limits<Real>::max_digits10);

        for (const Maneuver& maneuver : specimen.getManeuvers())
        {
            const Vector3& direction = maneuver.getThrustDirection();

            key << direction.x << ',' << direction.y << ',' << direction.z << ',' << maneuver.getThrottleValue() << ','
                << maneuver.getInitDelay() << ',' << maneuver.getDuration() << ';';
        }

        return key.str();
    }

    std::vector<const Specimen*> populationView(const auto& islands)
    {
        std::vector<const Specimen*> population;

        for (const Specimen& specimen : specimensIn(islands))
        {
            population.push_back(&specimen);
        }

        return population;
    }

    Real bestIslandDistance(const std::vector<Specimen>& island)
    {
        if (island.empty())
        {
            return std::numeric_limits<Real>::quiet_NaN();
        }

        return std::ranges::min(
            island | std::views::transform([](const Specimen& specimen) { return specimen.getFitness().value().minimumDistance; }));
    }

    void printDiversityDiagnostics(std::size_t generation, const auto& islands, std::ostream& output)
    {
        const std::vector<const Specimen*> population = populationView(islands);

        RunningStats maneuverCounts;
        RunningStats distanceValues;
        RunningStats timeValues;
        RunningStats fuelValues;
        RunningStats fuelViolationValues;
        std::set<std::string> uniqueGenomes;

        for (const Specimen* specimen : population)
        {
            maneuverCounts.add(static_cast<Real>(specimen->size()));
            uniqueGenomes.insert(specimenKey(*specimen));

            const FitnessValue& fitness = specimen->getFitness().value();
            distanceValues.add(fitness.minimumDistance);
            timeValues.add(fitness.minimumDistanceTime);
            fuelValues.add(fitness.fuelUsed);
            fuelViolationValues.add(fitness.fuelConstraintViolation);
        }

        output << "ALGO diversity generation " << generation << " | population_size=" << population.size()
               << " | unique_genomes=" << uniqueGenomes.size() << " | unique_ratio="
               << (population.empty() ? 0.0 : static_cast<Real>(uniqueGenomes.size()) / static_cast<Real>(population.size()))
               << " | maneuver_count=[" << maneuverCounts.min << ", " << maneuverCounts.max << "] avg=" << maneuverCounts.mean()
               << " stddev=" << maneuverCounts.stddev() << " | objective_stddev=[distance=" << distanceValues.stddev()
               << ", time=" << timeValues.stddev() << ", fuel=" << fuelValues.stddev()
               << ", fuel_violation=" << fuelViolationValues.stddev()
               << "] | avg_pairwise_maneuver_distance=" << averagePairwiseSpecimenDistance(population) << " | island_best_distance=[";

        for (std::size_t islandIndex = 0; islandIndex < islands.size(); ++islandIndex)
        {
            if (islandIndex > 0)
            {
                output << ", ";
            }

            output << bestIslandDistance(islands[islandIndex].specimens);
        }

        output << "]\n";
    }
} // namespace

Algo::Algo(std::size_t populationSizeValue, std::size_t generationCount, std::size_t eliteCountValue, std::size_t immigrantCountValue,
           const SpecimenComparator& specimenComparatorRef, Factories factoriesValue, bool verboseValue, std::ostream* diversityLogValue)
    : populationSize(populationSizeValue), generations(generationCount), eliteCount(eliteCountValue), immigrantCount(immigrantCountValue),
      specimenComparator(specimenComparatorRef), factories(factoriesValue), verbose(verboseValue), diversityLog(diversityLogValue)
{
    if (populationSizeValue == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }
}

ParetoFrontHistory Algo::run() const
{
    auto initializer = factories.initializerFactory.create();
    auto selection = factories.selectionFactory.create();
    auto crossover = factories.crossoverFactory.create();
    auto mutation = factories.mutationFactory.create();
    auto fitnessEvaluator = factories.fitnessEvaluatorFactory.create();

    Islands islands = createIslands(*initializer);

    evaluateAndRankIslands(islands, *fitnessEvaluator);

    ParetoFrontHistory history;
    history.reserve(generations);
    ParetoFront archive = firstFrontOf(islands, specimenComparator);

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        Islands nextIslands = createCandidateIslands(islands, *initializer, *selection, *crossover, *mutation);

        evaluateAndRankIslands(nextIslands, *fitnessEvaluator);
        selectEnvironmentalSurvivors(nextIslands, islands);
        rankIslands(nextIslands);

        if (ALGO_MIGRATION_INTERVAL > 0 && (generation + 1) % ALGO_MIGRATION_INTERVAL == 0)
        {
            migrate(nextIslands);
            rankIslands(nextIslands);
        }

        ParetoFront frontBeforeArchiveReintroduction = firstFrontOf(nextIslands, specimenComparator);
        archive = updateParetoArchive(std::move(archive), frontBeforeArchiveReintroduction, specimenComparator);

        if (shouldReintroduceArchive(generation))
        {
            reintroduceArchive(nextIslands, archive, generation);
        }

        rankIslands(nextIslands);

        if (verbose)
        {
            ParetoFront finalPopulationFront = firstFrontOf(nextIslands, specimenComparator);

            printGenerationResult(generation, nextIslands, finalPopulationFront);
        }

        if (diversityLog != nullptr)
        {
            printDiversityDiagnostics(generation, nextIslands, *diversityLog);
        }

        history.push_back(archive);
        islands = std::move(nextIslands);
    }

    return history;
}

Algo::Islands Algo::createIslands(Initializer& initializer) const
{
    const std::size_t islandCount = std::min(ALGO_TARGET_ISLAND_COUNT, populationSize);
    const std::size_t baseIslandSize = populationSize / islandCount;
    const std::size_t largerIslandCount = populationSize % islandCount;

    Islands islands;
    islands.reserve(islandCount);

    for (std::size_t islandIndex = 0; islandIndex < islandCount; ++islandIndex)
    {
        const std::size_t islandSize = baseIslandSize + (islandIndex < largerIslandCount ? 1 : 0);

        islands.push_back(RankedIsland{initializer.createPopulation(islandSize), ParetoRankedPopulation{}, {}});
    }

    return islands;
}

void Algo::evaluateIslands(Islands& islands, const FitnessEvaluator& fitnessEvaluator) const
{
    std::vector<Specimen*> specimens;
    specimens.reserve(populationSize);

    for (Specimen& specimen : specimensIn(islands))
    {
        specimens.push_back(&specimen);
    }

    evaluateSpecimens(specimens, fitnessEvaluator);
}

void Algo::evaluateAndRankIslands(Islands& islands, const FitnessEvaluator& fitnessEvaluator) const
{
    evaluateIslands(islands, fitnessEvaluator);
    rankIslands(islands);
}

void Algo::rankIslands(Islands& islands) const
{
    for (RankedIsland& island : islands)
    {
        rankIsland(island);
    }
}

void Algo::rankIsland(RankedIsland& island) const
{
    island.ranking = ParetoRanking::rankPopulation(island.specimens, specimenComparator);
    island.sortedIndices = ParetoRanking::sortedIndices(island.specimens, island.ranking, specimenComparator);
}

auto Algo::createCandidateIsland(const RankedIsland& island, Initializer& initializer, Selection& selection, Crossover& crossover,
                                 Mutation& mutation) const -> RankedIsland
{
    const std::size_t islandSize = island.specimens.size();
    std::vector<Specimen> nextIsland;
    const std::size_t islandImmigrantCount = immigrantCountForIsland(islandSize);
    nextIsland.reserve(islandSize * 2 + islandImmigrantCount);

    for (const Specimen& specimen : island.specimens)
    {
        nextIsland.push_back(specimen);
    }

    const NSGAIIRankingComparator selectionComparator(island.specimens, island.ranking.ranks, specimenComparator);

    appendChildren(island.specimens, nextIsland, islandSize * 2, selectionComparator, selection, crossover, mutation);

    appendImmigrants(nextIsland, islandImmigrantCount, initializer);

    return RankedIsland{std::move(nextIsland), ParetoRankedPopulation{}, {}};
}

Algo::Islands Algo::createCandidateIslands(const Islands& islands, Initializer& initializer, Selection& selection, Crossover& crossover,
                                           Mutation& mutation) const
{
    Islands candidateIslands;
    candidateIslands.reserve(islands.size());

    for (const RankedIsland& island : islands)
    {
        candidateIslands.push_back(createCandidateIsland(island, initializer, selection, crossover, mutation));
    }

    return candidateIslands;
}

void Algo::appendChildren(const std::vector<Specimen>& parents, std::vector<Specimen>& target, std::size_t targetSize,
                          const SpecimenComparator& selectionComparator, Selection& selection, Crossover& crossover,
                          Mutation& mutation) const
{
    const auto isCloseToTarget = [](const Specimen& specimen) {
        return specimen.getFitness().has_value() && Refinement::closeToTarget(specimen.getFitness().value());
    };

    while (target.size() < targetSize)
    {
        const Specimen& parent1 = selection.select(parents, selectionComparator);
        const Specimen& parent2 = selection.select(parents, selectionComparator);
        const bool closeToTarget = isCloseToTarget(parent1) || isCloseToTarget(parent2);

        auto [child1, child2] = crossover.cross(parent1, parent2);

        mutation.mutate(child1, closeToTarget);
        mutation.mutate(child2, closeToTarget);

        target.push_back(std::move(child1));

        if (target.size() < targetSize)
        {
            target.push_back(std::move(child2));
        }
    }
}

void Algo::selectEnvironmentalSurvivors(Islands& islands, const Islands& previousIslands) const
{
    const std::size_t islandCount = std::min(islands.size(), previousIslands.size());

    for (std::size_t islandIndex = 0; islandIndex < islandCount; ++islandIndex)
    {
        RankedIsland& island = islands[islandIndex];
        const std::size_t targetSize = previousIslands[islandIndex].specimens.size();

        if (island.specimens.size() <= targetSize)
        {
            continue;
        }

        std::vector<Specimen> survivors;
        survivors.reserve(targetSize);

        const std::size_t survivorCount = std::min(targetSize, island.sortedIndices.size());

        for (std::size_t i = 0; i < survivorCount; ++i)
        {
            survivors.push_back(std::move(island.specimens[island.sortedIndices[i]]));
        }

        island.specimens = std::move(survivors);
        island.ranking = ParetoRankedPopulation{};
        island.sortedIndices.clear();
    }
}

void Algo::migrate(Islands& islands) const
{
    if (islands.size() < 2 || eliteCount == 0)
    {
        return;
    }

    const std::size_t migrantCount = std::max(ALGO_MIN_MIGRANT_COUNT, eliteCount);
    std::vector<std::vector<Specimen>> migrants;
    migrants.reserve(islands.size());

    for (const RankedIsland& island : islands)
    {
        std::vector<Specimen>& migrantGroup = migrants.emplace_back();
        migrantGroup.reserve(std::min(migrantCount, island.specimens.size()));
        appendTopRankedSpecimens(migrantGroup, island.specimens, island.sortedIndices, migrantCount);
    }

    for (std::size_t islandIndex = 0; islandIndex < islands.size(); ++islandIndex)
    {
        std::vector<Specimen>& targetIsland = islands[(islandIndex + 1) % islands.size()].specimens;
        const std::vector<Specimen>& sourceMigrants = migrants[islandIndex];
        const std::size_t replacementCount = std::min(sourceMigrants.size(), targetIsland.size());

        for (std::size_t migrantIndex = 0; migrantIndex < replacementCount; ++migrantIndex)
        {
            targetIsland[targetIsland.size() - 1 - migrantIndex] = sourceMigrants[migrantIndex];
        }
    }
}

void Algo::reintroduceArchive(Islands& islands, const ParetoFront& archive, std::size_t generation) const
{
    if (islands.empty() || archive.empty())
    {
        return;
    }

    ParetoFront sortedArchive = archive;
    sortByRankAndCrowding(sortedArchive, specimenComparator);
    std::size_t archiveIndex = generation * ALGO_ARCHIVE_REINTRODUCTION_COUNT;

    for (RankedIsland& island : islands)
    {
        const std::size_t replacementCount = archiveReintroductionCountForIsland(island.specimens.size());

        replaceWorstWithArchiveSpecimens(island.specimens, island.sortedIndices, sortedArchive, replacementCount, archiveIndex,
                                         specimenComparator);

        archiveIndex += replacementCount;
    }
}

std::size_t Algo::immigrantCountForIsland(std::size_t islandSize) const
{
    const std::size_t islandEliteCount = std::min(eliteCount, islandSize);
    const std::size_t replaceableCount = islandSize - islandEliteCount;

    return std::min(replaceableCount, islandSize * immigrantCount / populationSize);
}

std::size_t Algo::archiveReintroductionCountForIsland(std::size_t islandSize) const
{
    const std::size_t islandEliteCount = std::min(eliteCount, islandSize);
    const std::size_t replaceableCount = islandSize - islandEliteCount;

    return std::min(replaceableCount, islandSize * ALGO_ARCHIVE_REINTRODUCTION_COUNT / populationSize);
}
