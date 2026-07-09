#include "NSGAIIIAlgorithm.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/NSGAIIRankingComparator.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"
#include "genetics/utils/ParetoRanking.h"
#include "genetics/utils/ReferenceDirections.h"

namespace
{
    constexpr std::size_t PrintedFrontLimit = 5;

    struct ObjectiveBounds
    {
        std::vector<Real> ideal;
        std::vector<Real> nadir;
    };

    struct DirectionAssociation
    {
        std::size_t directionIndex;
        Real distance;
    };

    struct NichingCandidate
    {
        std::size_t specimenIndex;
        Real distance;
    };

    std::string frontsDetails(const ParetoRankedPopulation& rankedPopulation)
    {
        std::ostringstream details;
        details << "fronts=" << rankedPopulation.fronts.size() << " [";

        const auto printedFronts = rankedPopulation.fronts | std::views::take(PrintedFrontLimit);

        bool first = true;

        for (const auto& front : printedFronts)
        {
            if (!first)
            {
                details << ", ";
            }

            details << front.size();
            first = false;
        }

        if (rankedPopulation.fronts.size() > PrintedFrontLimit)
        {
            details << ", ...";
        }

        details << ']';

        return details.str();
    }

    void printGenerationResult(std::size_t generation, const std::vector<Specimen>& population,
                               const ParetoRankedPopulation& rankedPopulation)
    {
        const ParetoFrontStats stats = rankedPopulation.fronts.empty()
                                           ? ParetoFrontStats{}
                                           : ParetoFrontUtils::calculateStats(population, rankedPopulation.fronts.front());

        GenerationProgressLogger::print("NSGA-III", generation, stats, frontsDetails(rankedPopulation));
    }

    ObjectiveBounds calculateObjectiveBounds(const std::vector<Specimen>& population, const std::vector<std::size_t>& indices,
                                             const SpecimenComparator& specimenComparator)
    {
        const std::size_t objectiveCount = specimenComparator.objectiveCount();
        ObjectiveBounds bounds{std::vector<Real>(objectiveCount, std::numeric_limits<Real>::infinity()),
                               std::vector<Real>(objectiveCount, -std::numeric_limits<Real>::infinity())};

        for (std::size_t specimenIndex : indices)
        {
            const FitnessValue& fitness = population[specimenIndex].getFitness().value();

            for (std::size_t objective = 0; objective < objectiveCount; ++objective)
            {
                const Real objectiveValue = specimenComparator.objectiveValue(fitness, objective);
                bounds.ideal[objective] = std::min(bounds.ideal[objective], objectiveValue);
                bounds.nadir[objective] = std::max(bounds.nadir[objective], objectiveValue);
            }
        }

        return bounds;
    }

    std::vector<Real> normalizedObjectiveVector(const Specimen& specimen, const ObjectiveBounds& bounds,
                                                const SpecimenComparator& specimenComparator)
    {
        const FitnessValue& fitness = specimen.getFitness().value();
        std::vector<Real> objectives;
        objectives.reserve(bounds.ideal.size());

        for (std::size_t objective = 0; objective < bounds.ideal.size(); ++objective)
        {
            const Real range = bounds.nadir[objective] - bounds.ideal[objective];
            const Real value = specimenComparator.objectiveValue(fitness, objective);
            objectives.push_back(range == 0.0 ? 0.0 : (value - bounds.ideal[objective]) / range);
        }

        return objectives;
    }

    Real normSquared(const ReferenceDirections::Direction& direction)
    {
        return std::transform_reduce(direction.begin(), direction.end(), direction.begin(), Real{0.0});
    }

    Real perpendicularDistanceSquared(const std::vector<Real>& objectiveVector, const ReferenceDirections::Direction& direction)
    {
        const Real directionNormSquared = normSquared(direction);

        if (directionNormSquared == 0.0)
        {
            return std::numeric_limits<Real>::infinity();
        }

        const Real projectionScale =
            std::transform_reduce(objectiveVector.begin(), objectiveVector.end(), direction.begin(), Real{0.0}) / directionNormSquared;

        Real distanceSquared = 0.0;

        for (std::size_t objective = 0; objective < objectiveVector.size(); ++objective)
        {
            const Real difference = objectiveVector[objective] - projectionScale * direction[objective];
            distanceSquared += difference * difference;
        }

        return distanceSquared;
    }

    DirectionAssociation associateDirection(const Specimen& specimen, const ObjectiveBounds& bounds,
                                            const std::vector<ReferenceDirections::Direction>& directions,
                                            const SpecimenComparator& specimenComparator)
    {
        const std::vector<Real> objectiveVector = normalizedObjectiveVector(specimen, bounds, specimenComparator);

        DirectionAssociation best{0, std::numeric_limits<Real>::infinity()};

        for (std::size_t directionIndex = 0; directionIndex < directions.size(); ++directionIndex)
        {
            const Real distanceSquared = perpendicularDistanceSquared(objectiveVector, directions[directionIndex]);

            if (distanceSquared < best.distance)
            {
                best = DirectionAssociation{directionIndex, distanceSquared};
            }
        }

        return best;
    }

    std::size_t chooseDirectionWithSmallestNiche(const std::vector<std::vector<NichingCandidate>>& candidatesByDirection,
                                                const std::vector<std::size_t>& nicheCounts)
    {
        std::size_t bestDirection = candidatesByDirection.size();
        std::size_t bestCount = std::numeric_limits<std::size_t>::max();

        for (std::size_t directionIndex = 0; directionIndex < candidatesByDirection.size(); ++directionIndex)
        {
            if (candidatesByDirection[directionIndex].empty())
            {
                continue;
            }

            if (nicheCounts[directionIndex] < bestCount)
            {
                bestDirection = directionIndex;
                bestCount = nicheCounts[directionIndex];
            }
        }

        return bestDirection;
    }

    void selectPartialFrontByNiching(const std::vector<Specimen>& population, std::vector<std::size_t>& selectedIndices,
                                     const std::vector<std::size_t>& front, std::size_t populationSize,
                                     const SpecimenComparator& specimenComparator,
                                     const std::vector<ReferenceDirections::Direction>& directions)
    {
        std::vector<std::size_t> normalizationIndices = selectedIndices;
        normalizationIndices.insert(normalizationIndices.end(), front.begin(), front.end());

        const ObjectiveBounds bounds = calculateObjectiveBounds(population, normalizationIndices, specimenComparator);
        std::vector<std::size_t> nicheCounts(directions.size(), 0);

        for (std::size_t specimenIndex : selectedIndices)
        {
            const DirectionAssociation association =
                associateDirection(population[specimenIndex], bounds, directions, specimenComparator);
            ++nicheCounts[association.directionIndex];
        }

        std::vector<std::vector<NichingCandidate>> candidatesByDirection(directions.size());

        for (std::size_t specimenIndex : front)
        {
            const DirectionAssociation association =
                associateDirection(population[specimenIndex], bounds, directions, specimenComparator);
            candidatesByDirection[association.directionIndex].push_back(NichingCandidate{specimenIndex, association.distance});
        }

        std::size_t remainingCandidates = 0;

        for (std::vector<NichingCandidate>& candidates : candidatesByDirection)
        {
            remainingCandidates += candidates.size();
            std::ranges::sort(candidates, {}, &NichingCandidate::distance);
        }

        while (selectedIndices.size() < populationSize && remainingCandidates > 0)
        {
            const std::size_t directionIndex = chooseDirectionWithSmallestNiche(candidatesByDirection, nicheCounts);

            if (directionIndex == candidatesByDirection.size())
            {
                break;
            }

            std::vector<NichingCandidate>& candidates = candidatesByDirection[directionIndex];
            selectedIndices.push_back(candidates.front().specimenIndex);
            candidates.erase(candidates.begin());
            --remainingCandidates;
            ++nicheCounts[directionIndex];
        }

        for (std::size_t specimenIndex : front)
        {
            if (selectedIndices.size() == populationSize)
            {
                break;
            }

            if (std::ranges::find(selectedIndices, specimenIndex) == selectedIndices.end())
            {
                selectedIndices.push_back(specimenIndex);
            }
        }
    }

    std::vector<Specimen> selectNextGeneration(std::vector<Specimen>& combinedPopulation, const ParetoRankedPopulation& rankedPopulation,
                                               std::size_t populationSize, const SpecimenComparator& specimenComparator,
                                               const std::vector<ReferenceDirections::Direction>& directions)
    {
        std::vector<std::size_t> selectedIndices;
        selectedIndices.reserve(populationSize);

        for (const auto& front : rankedPopulation.fronts)
        {
            if (selectedIndices.size() + front.size() <= populationSize)
            {
                selectedIndices.insert(selectedIndices.end(), front.begin(), front.end());

                if (selectedIndices.size() == populationSize)
                {
                    break;
                }

                continue;
            }

            selectPartialFrontByNiching(combinedPopulation, selectedIndices, front, populationSize, specimenComparator, directions);
            break;
        }

        std::vector<Specimen> nextPopulation;
        nextPopulation.reserve(selectedIndices.size());

        for (std::size_t specimenIndex : selectedIndices)
        {
            nextPopulation.push_back(std::move(combinedPopulation[specimenIndex]));
        }

        return nextPopulation;
    }

    void appendMovedPopulation(std::vector<Specimen>& target, std::vector<Specimen>& source)
    {
        std::ranges::move(source, std::back_inserter(target));
    }

    ParetoFront firstParetoFront(const std::vector<Specimen>& population, const ParetoRankedPopulation& rankedPopulation)
    {
        if (rankedPopulation.fronts.empty())
        {
            return {};
        }

        return ParetoFrontUtils::frontFromIndices(population, rankedPopulation.fronts.front());
    }
} // namespace

NSGAIIIAlgorithm::NSGAIIIAlgorithm(std::size_t populationSizeValue, std::size_t generationCount,
                                   const SpecimenComparator& specimenComparatorRef, Factories factoriesValue, bool verboseValue)
    : populationSize(populationSizeValue), generations(generationCount), specimenComparator(specimenComparatorRef),
      factories(factoriesValue), verbose(verboseValue)
{
    if (populationSizeValue == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }

    if (specimenComparator.objectiveCount() == 0)
    {
        throw std::invalid_argument("Objective count must be greater than zero.");
    }
}

ParetoFrontHistory NSGAIIIAlgorithm::run() const
{
    auto initializer = factories.initializerFactory.create();
    auto selection = factories.selectionFactory.create();
    auto crossover = factories.crossoverFactory.create();
    auto mutation = factories.mutationFactory.create();
    auto fitnessEvaluator = factories.fitnessEvaluatorFactory.create();

    const std::vector<ReferenceDirections::Direction> referenceDirections =
        ReferenceDirections::generate(populationSize, specimenComparator.objectiveCount());

    std::vector<Specimen> population = initializer->createPopulation(populationSize);
    ParetoFrontHistory history;
    history.reserve(generations);
    ParetoFront archive;

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        evaluatePopulation(population, *fitnessEvaluator);

        const ParetoRankedPopulation rankedParents = ParetoRanking::rankPopulation(population, specimenComparator);
        const NSGAIIRankingComparator selectionComparator(population, rankedParents.ranks, specimenComparator, false);

        std::vector<Specimen> offspring = createOffspringPopulation(population, selectionComparator, *selection, *crossover, *mutation);

        evaluatePopulation(offspring, *fitnessEvaluator);

        std::vector<Specimen> combinedPopulation;
        combinedPopulation.reserve(population.size() + offspring.size());
        appendMovedPopulation(combinedPopulation, population);
        appendMovedPopulation(combinedPopulation, offspring);

        const ParetoRankedPopulation rankedCombined = ParetoRanking::rankPopulation(combinedPopulation, specimenComparator);

        population = selectNextGeneration(combinedPopulation, rankedCombined, populationSize, specimenComparator, referenceDirections);

        evaluatePopulation(population, *fitnessEvaluator);

        const ParetoRankedPopulation rankedPopulation = ParetoRanking::rankPopulation(population, specimenComparator);

        if (verbose)
        {
            printGenerationResult(generation, population, rankedPopulation);
        }

        ParetoFront currentFront = firstParetoFront(population, rankedPopulation);
        archive = ParetoFrontUtils::updateArchive(std::move(archive), std::move(currentFront), specimenComparator);

        history.push_back(archive);
    }

    return history;
}

std::vector<Specimen> NSGAIIIAlgorithm::createOffspringPopulation(const std::vector<Specimen>& population,
                                                                  const SpecimenComparator& selectionComparator, Selection& selection,
                                                                  Crossover& crossover, Mutation& mutation) const
{
    std::vector<Specimen> offspring;
    offspring.reserve(populationSize);

    appendChildren(population, offspring, populationSize, selectionComparator, selection, crossover, mutation);

    return offspring;
}
