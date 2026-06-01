#include "NSGAIIAlgorithm.h"

#include <algorithm>
#include <compare>
#include <iostream>
#include <iterator>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/FitnessObjectives.h"
#include "genetics/comparison/NSGAIIRankingComparator.h"
#include "genetics/comparison/SpecimenRank.h"

namespace
{
    struct RankedPopulation
    {
        std::vector<std::vector<std::size_t>> fronts;
        std::vector<SpecimenRank> ranks;
    };

    struct FrontStats
    {
        std::size_t size{};
        std::size_t fuelFeasibleCount{};
        Real minDistance{};
        Real maxDistance{};
        Real minTime{};
        Real maxTime{};
        Real minFuel{};
        Real maxFuel{};
        Real minFuelViolation{};
        Real maxFuelViolation{};
    };

    FrontStats calculateFrontStats(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& front)
    {
        FrontStats stats;
        stats.size = front.size();

        if (front.empty())
        {
            return stats;
        }

        const FitnessValue& firstFitness =
            population[front.front()].getFitness().value();

        stats.minDistance = firstFitness.minimumDistance;
        stats.maxDistance = firstFitness.minimumDistance;
        stats.minTime = firstFitness.minimumDistanceTime;
        stats.maxTime = firstFitness.minimumDistanceTime;
        stats.minFuel = firstFitness.minimumDistanceFuelMass;
        stats.maxFuel = firstFitness.minimumDistanceFuelMass;
        stats.minFuelViolation = firstFitness.fuelConstraintViolation;
        stats.maxFuelViolation = firstFitness.fuelConstraintViolation;

        for (std::size_t specimenIndex : front)
        {
            const FitnessValue& fitness =
                population[specimenIndex].getFitness().value();

            if (fitnessObjectives::satisfiesFuelConstraint(fitness))
            {
                ++stats.fuelFeasibleCount;
            }

            stats.minDistance =
                std::min(
                    stats.minDistance,
                    fitness.minimumDistance);
            stats.maxDistance =
                std::max(
                    stats.maxDistance,
                    fitness.minimumDistance);
            stats.minTime =
                std::min(
                    stats.minTime,
                    fitness.minimumDistanceTime);
            stats.maxTime =
                std::max(
                    stats.maxTime,
                    fitness.minimumDistanceTime);
            stats.minFuel =
                std::min(
                    stats.minFuel,
                    fitness.minimumDistanceFuelMass);
            stats.maxFuel =
                std::max(
                    stats.maxFuel,
                    fitness.minimumDistanceFuelMass);
            stats.minFuelViolation =
                std::min(
                    stats.minFuelViolation,
                    fitness.fuelConstraintViolation);
            stats.maxFuelViolation =
                std::max(
                    stats.maxFuelViolation,
                    fitness.fuelConstraintViolation);
        }

        return stats;
    }

    void printGenerationResult(
        std::size_t generation,
        const std::vector<Specimen>& population,
        const RankedPopulation& rankedPopulation)
    {
        if (rankedPopulation.fronts.empty())
        {
            std::cout
                << "NSGA-II generation " << generation
                << " | Pareto front size = 0\n";
            return;
        }

        const FrontStats stats =
            calculateFrontStats(
                population,
                rankedPopulation.fronts.front());

        std::cout
            << "NSGA-II generation " << generation
            << " | Pareto front size = " << stats.size
            << " | fuel feasible = "
            << stats.fuelFeasibleCount << '/' << stats.size
            << " | distance = ["
            << stats.minDistance << ", " << stats.maxDistance << ']'
            << " | time = ["
            << stats.minTime << ", " << stats.maxTime << ']'
            << " | fuel = ["
            << stats.minFuel << ", " << stats.maxFuel << ']'
            << " | fuel violation = ["
            << stats.minFuelViolation << ", "
            << stats.maxFuelViolation << ']'
            << '\n';
    }

    bool dominates(
        const Specimen& lhs,
        const Specimen& rhs,
        const SpecimenComparator& specimenComparator)
    {
        return specimenComparator.compare(lhs, rhs) ==
            std::partial_ordering::less;
    }

    void calculateCrowdingDistance(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& front,
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

        for (std::size_t objective = 0;
             objective < fitnessObjectives::objectiveCount;
             ++objective)
        {
            std::vector<std::size_t> sortedFront = front;

            std::ranges::sort(
                sortedFront,
                [&](std::size_t lhs, std::size_t rhs)
                {
                    return fitnessObjectives::comparableValues(
                        population[lhs].getFitness().value())[objective] <
                        fitnessObjectives::comparableValues(
                            population[rhs].getFitness().value())[objective];
                });

            ranks[sortedFront.front()].crowdingDistance =
                std::numeric_limits<Real>::infinity();
            ranks[sortedFront.back()].crowdingDistance =
                std::numeric_limits<Real>::infinity();

            const Real minimumValue =
                fitnessObjectives::comparableValues(
                    population[sortedFront.front()].getFitness().value())[objective];
            const Real maximumValue =
                fitnessObjectives::comparableValues(
                    population[sortedFront.back()].getFitness().value())[objective];
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
                    fitnessObjectives::comparableValues(
                        population[sortedFront[i - 1]].getFitness().value())[objective];
                const Real nextValue =
                    fitnessObjectives::comparableValues(
                        population[sortedFront[i + 1]].getFitness().value())[objective];

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

                if (dominates(
                    population[lhs],
                    population[rhs],
                    specimenComparator))
                {
                    dominatedBySpecimen[lhs].push_back(rhs);
                }
                else if (dominates(
                    population[rhs],
                    population[lhs],
                    specimenComparator))
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
                rankedPopulation.ranks);
        }

        return rankedPopulation;
    }

    std::vector<Specimen> selectNextGeneration(
        const std::vector<Specimen>& combinedPopulation,
        const RankedPopulation& rankedPopulation,
        std::size_t populationSize)
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
                        combinedPopulation[specimenIndex]);
                }

                continue;
            }

            std::vector<std::size_t> sortedFront = front;

            std::ranges::sort(
                sortedFront,
                [&](std::size_t lhs, std::size_t rhs)
                {
                    const Real lhsDistance =
                        rankedPopulation.ranks[lhs].crowdingDistance;
                    const Real rhsDistance =
                        rankedPopulation.ranks[rhs].crowdingDistance;

                    if (lhsDistance != rhsDistance)
                    {
                        return lhsDistance > rhsDistance;
                    }

                    return lhs < rhs;
                });

            for (std::size_t specimenIndex : sortedFront)
            {
                if (nextPopulation.size() == populationSize)
                {
                    break;
                }

                nextPopulation.push_back(
                    combinedPopulation[specimenIndex]);
            }

            break;
        }

        return nextPopulation;
    }

    std::vector<Specimen> firstParetoFront(
        const std::vector<Specimen>& population,
        const RankedPopulation& rankedPopulation)
    {
        std::vector<Specimen> front;

        if (rankedPopulation.fronts.empty())
        {
            return front;
        }

        front.reserve(
            rankedPopulation.fronts.front().size());

        for (std::size_t specimenIndex : rankedPopulation.fronts.front())
        {
            front.push_back(
                population[specimenIndex]);
        }

        return front;
    }
}

NSGAIIAlgorithm::NSGAIIAlgorithm(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories
)
    : populationSize(populationSize),
      generations(generations),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories)
{
    if (populationSize == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }
}

std::vector<Specimen> NSGAIIAlgorithm::run() const
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

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        evaluatePopulationUnsequenced(
            population,
            *fitnessEvaluator);

        const RankedPopulation rankedParents =
            rankPopulation(
                population,
                specimenComparator);

        printGenerationResult(
            generation,
            population,
            rankedParents);

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
        std::ranges::copy(
            population,
            std::back_inserter(combinedPopulation));
        std::ranges::copy(
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
                populationSize);
    }

    evaluatePopulationUnsequenced(
        population,
        *fitnessEvaluator);

    const RankedPopulation rankedPopulation =
        rankPopulation(
            population,
            specimenComparator);

    return firstParetoFront(
        population,
        rankedPopulation);
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
