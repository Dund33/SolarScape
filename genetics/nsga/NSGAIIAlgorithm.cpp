#include "NSGAIIAlgorithm.h"

#include <algorithm>
#include <iterator>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/NSGAIIRankingComparator.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"
#include "genetics/utils/ParetoRanking.h"

namespace
{
    constexpr std::size_t PrintedFrontLimit = 5;

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

        GenerationProgressLogger::print("NSGA-II", generation, stats, frontsDetails(rankedPopulation));
    }

    void appendMovedPopulation(std::vector<Specimen>& target, std::vector<Specimen>& source)
    {
        std::ranges::move(source, std::back_inserter(target));
    }

    void appendMovedSpecimensByIndex(std::vector<Specimen>& target, std::vector<Specimen>& source, const std::vector<std::size_t>& indices,
                                     std::size_t maxCount)
    {
        for (std::size_t specimenIndex : indices | std::views::take(maxCount))
        {
            target.push_back(std::move(source[specimenIndex]));
        }
    }

    std::vector<Specimen> selectNextGeneration(std::vector<Specimen>& combinedPopulation, const ParetoRankedPopulation& rankedPopulation,
                                               std::size_t populationSize, const SpecimenComparator& specimenComparator)
    {
        std::vector<Specimen> nextPopulation;
        nextPopulation.reserve(populationSize);

        for (const auto& front : rankedPopulation.fronts)
        {
            if (nextPopulation.size() + front.size() <= populationSize)
            {
                appendMovedSpecimensByIndex(nextPopulation, combinedPopulation, front, front.size());

                if (nextPopulation.size() == populationSize)
                {
                    break;
                }

                continue;
            }

            std::vector<std::size_t> sortedFront = front;
            const NSGAIIRankingComparator comparator(combinedPopulation, rankedPopulation.ranks, specimenComparator, false);

            std::ranges::sort(sortedFront, [&](std::size_t lhs, std::size_t rhs) {
                return comparator.isLess(combinedPopulation[lhs], combinedPopulation[rhs]);
            });

            appendMovedSpecimensByIndex(nextPopulation, combinedPopulation, sortedFront, populationSize - nextPopulation.size());

            break;
        }

        return nextPopulation;
    }

    ParetoFront firstParetoFront(const std::vector<Specimen>& population, const ParetoRankedPopulation& rankedPopulation)
    {
        ParetoFront front;

        if (rankedPopulation.fronts.empty())
        {
            return front;
        }

        return ParetoFrontUtils::frontFromIndices(population, rankedPopulation.fronts.front());
    }
} // namespace

NSGAIIAlgorithm::NSGAIIAlgorithm(std::size_t populationSizeValue, std::size_t generationCount,
                                 const SpecimenComparator& specimenComparatorRef, Factories factoriesValue, bool verboseValue)
    : populationSize(populationSizeValue), generations(generationCount), specimenComparator(specimenComparatorRef),
      factories(factoriesValue), verbose(verboseValue)
{
    if (populationSizeValue == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }
}

ParetoFrontHistory NSGAIIAlgorithm::run() const
{
    auto initializer = factories.initializerFactory.create();
    auto selection = factories.selectionFactory.create();
    auto crossover = factories.crossoverFactory.create();
    auto mutation = factories.mutationFactory.create();
    auto fitnessEvaluator = factories.fitnessEvaluatorFactory.create();

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

        population = selectNextGeneration(combinedPopulation, rankedCombined, populationSize, specimenComparator);

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

std::vector<Specimen> NSGAIIAlgorithm::createOffspringPopulation(const std::vector<Specimen>& population,
                                                                 const SpecimenComparator& selectionComparator, Selection& selection,
                                                                 Crossover& crossover, Mutation& mutation) const
{
    std::vector<Specimen> offspring;
    offspring.reserve(populationSize);

    appendChildren(population, offspring, populationSize, selectionComparator, selection, crossover, mutation);

    return offspring;
}
