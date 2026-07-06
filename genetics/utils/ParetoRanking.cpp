#include "ParetoRanking.h"

#include <algorithm>
#include <compare>
#include <limits>
#include <numeric>
#include <ranges>
#include <utility>
#include <vector>

#include "genetics/comparison/NSGAIIRankingComparator.h"

ParetoRankedPopulation ParetoRanking::rankPopulation(
    const std::vector<Specimen>& population,
    const SpecimenComparator& specimenComparator)
{
    const std::size_t populationSize = population.size();
    std::vector<std::vector<std::size_t>> dominatedBySpecimen(
        populationSize);
    std::vector<std::size_t> dominationCounts(
        populationSize,
        0);

    ParetoRankedPopulation rankedPopulation;
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
            rankedPopulation.fronts.push_back(std::move(nextFront));
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

std::vector<std::size_t> ParetoRanking::sortedIndices(
    const std::vector<Specimen>& population,
    const ParetoRankedPopulation& rankedPopulation,
    const SpecimenComparator& specimenComparator)
{
    std::vector<std::size_t> indices(population.size());
    std::iota(
        indices.begin(),
        indices.end(),
        0);

    const NSGAIIRankingComparator comparator(
        population,
        rankedPopulation.ranks,
        specimenComparator);

    std::ranges::sort(
        indices,
        [&](std::size_t lhs, std::size_t rhs)
        {
            return comparator.isLess(
                population[lhs],
                population[rhs]);
        });

    return indices;
}

void ParetoRanking::calculateCrowdingDistance(
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
        ranks[specimenIndex].crowdingDistance = 0.0;
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

    const std::size_t objectiveCount = specimenComparator.objectiveCount();

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

        if (valueRange == 0.0)
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
