#include "NSGAIIAlgorithm.h"

#include <algorithm>
#include <compare>
#include <iterator>
#include <limits>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/NSGAIIRankingComparator.h"
#include "genetics/comparison/SpecimenRank.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"

namespace
{
    struct RankedPopulation
    {
        std::vector<std::vector<std::size_t>> fronts;
        std::vector<SpecimenRank> ranks;
    };

    std::string frontsDetails(
        const RankedPopulation& rankedPopulation)
    {
        std::ostringstream details;
        details
            << "fronts="
            << rankedPopulation.fronts.size()
            << " [";

        const auto printedFronts =
            rankedPopulation.fronts |
            std::views::take(5);

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

        if (rankedPopulation.fronts.size() > 5)
        {
            details << ", ...";
        }

        details << ']';

        return details.str();
    }

    void printGenerationResult(
        std::size_t generation,
        const std::vector<Specimen>& population,
        const RankedPopulation& rankedPopulation)
    {
        const ParetoFrontStats stats =
            rankedPopulation.fronts.empty()
                ? ParetoFrontStats{}
                : ParetoFrontUtils::calculateStats(
                    population,
                    rankedPopulation.fronts.front());

        GenerationProgressLogger::print(
            "NSGA-II",
            generation,
            stats,
            frontsDetails(
                rankedPopulation));
    }

    void calculateCrowdingDistance(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& front,
        const SpecimenComparator& specimenComparator,
        std::vector<SpecimenRank>& ranks)
    {
        if (front.empty())
        {
            return;
        }

        for (std::size_t specimenIndex : front)
        {
            ranks[specimenIndex].crowdingDistance = 0.0L;
        }

        if (front.size() <= 2)
        {
            for (std::size_t specimenIndex : front)
            {
                ranks[specimenIndex].crowdingDistance =
                    std::numeric_limits<Real>::infinity();
            }

            return;
        }

        const std::size_t objectiveCount =
            specimenComparator.objectiveCount();

        for (std::size_t objective = 0; objective < objectiveCount; ++objective)
        {
            std::vector<std::size_t> sortedFront = front;

            std::ranges::sort(
                sortedFront,
                [&](std::size_t lhs, std::size_t rhs)
                {
                    return specimenComparator.objectiveValue(
                        population[lhs].getFitness().value(),
                        objective) <
                        specimenComparator.objectiveValue(
                            population[rhs].getFitness().value(),
                            objective);
                });

            ranks[sortedFront.front()].crowdingDistance =
                std::numeric_limits<Real>::infinity();
            ranks[sortedFront.back()].crowdingDistance =
                std::numeric_limits<Real>::infinity();

            const Real minimumValue =
                specimenComparator.objectiveValue(
                    population[sortedFront.front()].getFitness().value(),
                    objective);
            const Real maximumValue =
                specimenComparator.objectiveValue(
                    population[sortedFront.back()].getFitness().value(),
                    objective);
            const Real valueRange = maximumValue - minimumValue;

            if (valueRange == 0.0L)
            {
                continue;
            }

            for (std::size_t i = 1; i + 1 < sortedFront.size(); ++i)
            {
                Real& distance =
                    ranks[sortedFront[i]].crowdingDistance;

                if (distance == std::numeric_limits<Real>::infinity())
                {
                    continue;
                }

                const Real previousValue =
                    specimenComparator.objectiveValue(
                        population[sortedFront[i - 1]].getFitness().value(),
                        objective);
                const Real nextValue =
                    specimenComparator.objectiveValue(
                        population[sortedFront[i + 1]].getFitness().value(),
                        objective);

                distance +=
                    (nextValue - previousValue) /
                    valueRange;
            }
        }
    }

    RankedPopulation rankPopulation(
        const std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator)
    {
        const std::size_t populationSize = population.size();
        std::vector<std::vector<std::size_t>> dominatedBySpecimen(
            populationSize);
        std::vector<std::size_t> dominationCounts(
            populationSize,
            0);

        RankedPopulation rankedPopulation;
        rankedPopulation.ranks.resize(populationSize);
        rankedPopulation.fronts.emplace_back();

        for (std::size_t lhs = 0; lhs < populationSize; ++lhs)
        {
            for (std::size_t rhs = 0; rhs < populationSize; ++rhs)
            {
                if (lhs == rhs)
                {
                    continue;
                }

                const std::partial_ordering comparison =
                    specimenComparator.compare(
                        population[lhs],
                        population[rhs]);

                if (comparison == std::partial_ordering::less)
                {
                    dominatedBySpecimen[lhs].push_back(rhs);
                }
                else if (comparison == std::partial_ordering::greater)
                {
                    ++dominationCounts[lhs];
                }
            }

            if (dominationCounts[lhs] == 0)
            {
                rankedPopulation.ranks[lhs].rank = 0;
                rankedPopulation.fronts.front().push_back(lhs);
            }
        }

        std::size_t frontIndex = 0;

        while (
            frontIndex < rankedPopulation.fronts.size() &&
            !rankedPopulation.fronts[frontIndex].empty())
        {
            std::vector<std::size_t> nextFront;

            for (std::size_t specimenIndex :
                 rankedPopulation.fronts[frontIndex])
            {
                for (std::size_t dominatedIndex :
                     dominatedBySpecimen[specimenIndex])
                {
                    --dominationCounts[dominatedIndex];

                    if (dominationCounts[dominatedIndex] == 0)
                    {
                        rankedPopulation.ranks[dominatedIndex].rank =
                            frontIndex + 1;
                        nextFront.push_back(dominatedIndex);
                    }
                }
            }

            if (!nextFront.empty())
            {
                rankedPopulation.fronts.push_back(
                    std::move(nextFront));
            }

            ++frontIndex;
        }

        for (const auto& front : rankedPopulation.fronts)
        {
            calculateCrowdingDistance(
                population,
                front,
                specimenComparator,
                rankedPopulation.ranks);
        }

        return rankedPopulation;
    }

    std::vector<Specimen> selectNextGeneration(
        std::vector<Specimen>& combinedPopulation,
        const RankedPopulation& rankedPopulation,
        std::size_t populationSize,
        const SpecimenComparator& specimenComparator)
    {
        std::vector<Specimen> nextPopulation;
        nextPopulation.reserve(populationSize);

        for (const auto& front : rankedPopulation.fronts)
        {
            if (nextPopulation.size() + front.size() <= populationSize)
            {
                for (std::size_t specimenIndex : front)
                {
                    nextPopulation.push_back(
                        std::move(
                            combinedPopulation[specimenIndex]));
                }

                if (nextPopulation.size() == populationSize)
                {
                    break;
                }

                continue;
            }

            std::vector<std::size_t> sortedFront = front;
            const NSGAIIRankingComparator comparator(
                combinedPopulation,
                rankedPopulation.ranks,
                specimenComparator);

            std::ranges::sort(
                sortedFront,
                [&](std::size_t lhs, std::size_t rhs)
                {
                    return comparator.isLess(
                        combinedPopulation[lhs],
                        combinedPopulation[rhs]);
                });

            for (std::size_t specimenIndex : sortedFront)
            {
                if (nextPopulation.size() == populationSize)
                {
                    break;
                }

                nextPopulation.push_back(
                    std::move(
                        combinedPopulation[specimenIndex]));
            }

            break;
        }

        return nextPopulation;
    }

    ParetoFront firstParetoFront(
        const std::vector<Specimen>& population,
        const RankedPopulation& rankedPopulation)
    {
        ParetoFront front;

        if (rankedPopulation.fronts.empty())
        {
            return front;
        }

        return ParetoFrontUtils::frontFromIndices(
            population,
            rankedPopulation.fronts.front());
    }
}

NSGAIIAlgorithm::NSGAIIAlgorithm(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories,
    bool verbose
)
    : populationSize(populationSize),
      generations(generations),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories),
      verbose(verbose)
{
    if (populationSize == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }
}

ParetoFrontHistory NSGAIIAlgorithm::run() const
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

    std::vector<Specimen> population =
        initializer->createPopulation(
            populationSize);
    ParetoFrontHistory history;
    history.reserve(
        generations);

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        evaluatePopulationUnsequenced(
            population,
            *fitnessEvaluator);

        const RankedPopulation rankedParents =
            rankPopulation(
                population,
                specimenComparator);

        const NSGAIIRankingComparator selectionComparator(
            population,
            rankedParents.ranks,
            specimenComparator);

        std::vector<Specimen> offspring =
            createOffspringPopulation(
                population,
                selectionComparator,
                *initializer,
                *selection,
                *crossover,
                *mutation);

        evaluatePopulationUnsequenced(
            offspring,
            *fitnessEvaluator);

        std::vector<Specimen> combinedPopulation;
        combinedPopulation.reserve(
            population.size() + offspring.size());
        std::ranges::move(
            population,
            std::back_inserter(combinedPopulation));
        std::ranges::move(
            offspring,
            std::back_inserter(combinedPopulation));

        const RankedPopulation rankedCombined =
            rankPopulation(
                combinedPopulation,
                specimenComparator);

        population =
            selectNextGeneration(
                combinedPopulation,
                rankedCombined,
                populationSize,
                specimenComparator);

        evaluatePopulationUnsequenced(
            population,
            *fitnessEvaluator);

        const RankedPopulation rankedPopulation =
            rankPopulation(
                population,
                specimenComparator);

        if (verbose)
        {
            printGenerationResult(
                generation,
                population,
                rankedPopulation);
        }

        history.push_back(
            firstParetoFront(
                population,
                rankedPopulation));
    }

    return history;
}

std::vector<Specimen> NSGAIIAlgorithm::createOffspringPopulation(
    const std::vector<Specimen>& population,
    const SpecimenComparator& selectionComparator,
    Initializer& initializer,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation
) const
{
    const std::size_t effectiveImmigrantCount =
        std::min(
            immigrantCount,
            populationSize);
    const std::size_t childrenTarget =
        populationSize - effectiveImmigrantCount;

    std::vector<Specimen> offspring;
    offspring.reserve(populationSize);

    appendChildren(
        population,
        offspring,
        childrenTarget,
        selectionComparator,
        selection,
        crossover,
        mutation);

    appendImmigrants(
        offspring,
        effectiveImmigrantCount,
        initializer);

    return offspring;
}
