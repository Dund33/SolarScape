#include "MOEADAlgorithm.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <random>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/crossing/Crossover.h"
#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/fitness/FitnessMetrics.h"
#include "genetics/init/Initializer.h"
#include "genetics/mutation/Mutation.h"
#include "genetics/utils/GenerationProgressLogger.h"
#include "genetics/utils/ParetoFrontUtils.h"
#include "genetics/utils/ReferenceDirections.h"

namespace
{
    using WeightVector = ReferenceDirections::Direction;

    struct ObjectiveBounds
    {
        std::vector<Real> ideal;
        std::vector<Real> nadir;
    };

    Real weightDistanceSquared(const WeightVector& lhs, const WeightVector& rhs)
    {
        Real distance = 0.0;

        for (std::size_t i = 0; i < lhs.size(); ++i)
        {
            const Real difference = lhs[i] - rhs[i];
            distance += difference * difference;
        }

        return distance;
    }

    std::vector<std::vector<std::size_t>> calculateNeighborhoods(const std::vector<WeightVector>& weightVectors,
                                                                 std::size_t neighborhoodSize)
    {
        std::vector<std::vector<std::size_t>> neighborhoods;
        neighborhoods.reserve(weightVectors.size());

        for (std::size_t i = 0; i < weightVectors.size(); ++i)
        {
            std::vector<std::size_t> neighbors(weightVectors.size());
            std::iota(neighbors.begin(), neighbors.end(), 0);

            std::ranges::sort(neighbors, [&](std::size_t lhs, std::size_t rhs) {
                return weightDistanceSquared(weightVectors[i], weightVectors[lhs]) <
                       weightDistanceSquared(weightVectors[i], weightVectors[rhs]);
            });

            neighbors.resize(std::min(neighborhoodSize, neighbors.size()));
            neighborhoods.push_back(std::move(neighbors));
        }

        return neighborhoods;
    }

    ObjectiveBounds calculateObjectiveBounds(const std::vector<Specimen>& population, const SpecimenComparator& specimenComparator)
    {
        const std::size_t objectiveCount = specimenComparator.objectiveCount();

        ObjectiveBounds bounds{std::vector<Real>(objectiveCount, std::numeric_limits<Real>::infinity()),
                               std::vector<Real>(objectiveCount, -std::numeric_limits<Real>::infinity())};

        for (const Specimen& specimen : population)
        {
            const FitnessValue& fitness = specimen.getFitness().value();

            for (std::size_t objective = 0; objective < objectiveCount; ++objective)
            {
                const Real objectiveValue = specimenComparator.objectiveValue(fitness, objective);

                bounds.ideal[objective] = std::min(bounds.ideal[objective], objectiveValue);
                bounds.nadir[objective] = std::max(bounds.nadir[objective], objectiveValue);
            }
        }

        return bounds;
    }

    void updateObjectiveBounds(ObjectiveBounds& bounds, const Specimen& specimen, const SpecimenComparator& specimenComparator)
    {
        const FitnessValue& fitness = specimen.getFitness().value();

        for (std::size_t objective = 0; objective < bounds.ideal.size(); ++objective)
        {
            const Real objectiveValue = specimenComparator.objectiveValue(fitness, objective);

            bounds.ideal[objective] = std::min(bounds.ideal[objective], objectiveValue);
            bounds.nadir[objective] = std::max(bounds.nadir[objective], objectiveValue);
        }
    }

    Real scalarizedFitness(const FitnessValue& fitness, const WeightVector& weightVector, const ObjectiveBounds& bounds,
                           const SpecimenComparator& specimenComparator)
    {
        Real score = -std::numeric_limits<Real>::infinity();

        for (std::size_t objective = 0; objective < weightVector.size(); ++objective)
        {
            const Real objectiveValue = specimenComparator.objectiveValue(fitness, objective);
            const Real objectiveRange = bounds.nadir[objective] - bounds.ideal[objective];
            const Real normalizedDifference =
                objectiveRange == 0.0 ? 0.0 : std::abs(objectiveValue - bounds.ideal[objective]) / objectiveRange;
            const Real effectiveWeight = std::max(weightVector[objective], 1.0e-12);

            score = std::max(score, effectiveWeight * normalizedDifference);
        }

        return score;
    }

    bool isBetterForSubproblem(const Specimen& candidate, const Specimen& current, const WeightVector& weightVector,
                               const ObjectiveBounds& bounds, const SpecimenComparator& specimenComparator)
    {
        const FitnessValue& candidateFitness = candidate.getFitness().value();
        const FitnessValue& currentFitness = current.getFitness().value();

        const Real candidateScore = scalarizedFitness(candidateFitness, weightVector, bounds, specimenComparator);
        const Real currentScore = scalarizedFitness(currentFitness, weightVector, bounds, specimenComparator);

        if (candidateScore < currentScore)
        {
            return true;
        }

        if (currentScore < candidateScore)
        {
            return false;
        }

        return false;
    }

    const Specimen& selectRandomNeighbor(const std::vector<Specimen>& population, const std::vector<std::size_t>& neighborhood,
                                         std::mt19937& rng)
    {
        std::uniform_int_distribution<std::size_t> dist(0, neighborhood.size() - 1);

        return population[neighborhood[dist(rng)]];
    }

    Specimen createChild(const std::vector<Specimen>& population, const std::vector<std::size_t>& neighborhood, Crossover& crossover,
                         Mutation& mutation, std::mt19937& rng)
    {
        const Specimen& parent1 = selectRandomNeighbor(population, neighborhood, rng);
        const Specimen& parent2 = selectRandomNeighbor(population, neighborhood, rng);

        auto [child1, child2] = crossover.cross(parent1, parent2);

        std::uniform_int_distribution<int> childDist(0, 1);
        Specimen child = childDist(rng) == 0 ? std::move(child1) : std::move(child2);

        mutation.mutate(child);
        child.clearFitness();

        return child;
    }

    void printGenerationResult(std::size_t generation, const ParetoFront& paretoFront)
    {
        GenerationProgressLogger::print("MOEA-D", generation, ParetoFrontUtils::calculateStats(paretoFront));
    }
} // namespace

MOEADAlgorithm::MOEADAlgorithm(std::size_t populationSizeValue, std::size_t generationCount, std::size_t neighborhoodSizeValue,
                               const SpecimenComparator& specimenComparatorRef, Factories factoriesValue, bool verboseValue)
    : populationSize(populationSizeValue), generations(generationCount), neighborhoodSize(neighborhoodSizeValue),
      specimenComparator(specimenComparatorRef), factories(factoriesValue), verbose(verboseValue)
{
    if (populationSizeValue == 0)
    {
        throw std::invalid_argument("Population size must be greater than zero.");
    }

    if (neighborhoodSizeValue == 0)
    {
        throw std::invalid_argument("Neighborhood size must be greater than zero.");
    }

    if (specimenComparator.objectiveCount() == 0)
    {
        throw std::invalid_argument("Objective count must be greater than zero.");
    }
}

ParetoFrontHistory MOEADAlgorithm::run() const
{
    auto initializer = factories.initializerFactory.create();
    auto crossover = factories.crossoverFactory.create();
    auto mutation = factories.mutationFactory.create();
    auto fitnessEvaluator = factories.fitnessEvaluatorFactory.create();

    std::vector<Specimen> population = initializer->createPopulation(populationSize);

    evaluatePopulation(population, *fitnessEvaluator);

    const std::vector<WeightVector> weightVectors = ReferenceDirections::generate(populationSize, specimenComparator.objectiveCount());
    const std::vector<std::vector<std::size_t>> neighborhoods = calculateNeighborhoods(weightVectors, neighborhoodSize);

    static thread_local std::mt19937 rng(std::random_device{}());
    ParetoFrontHistory history;
    history.reserve(generations);
    ParetoFront archive;

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        ObjectiveBounds bounds = calculateObjectiveBounds(population, specimenComparator);

        for (std::size_t subproblemIndex = 0; subproblemIndex < populationSize; ++subproblemIndex)
        {
            Specimen child = createChild(population, neighborhoods[subproblemIndex], *crossover, *mutation, rng);

            fitnessEvaluator->evaluate(child);
            updateObjectiveBounds(bounds, child, specimenComparator);

            for (std::size_t replacementIndex : neighborhoods[subproblemIndex])
            {
                if (isBetterForSubproblem(child, population[replacementIndex], weightVectors[replacementIndex], bounds, specimenComparator))
                {
                    population[replacementIndex] = child;
                }
            }
        }

        ParetoFront paretoFront = ParetoFrontUtils::firstFront(population, specimenComparator);

        if (verbose)
        {
            printGenerationResult(generation, paretoFront);
        }

        archive = ParetoFrontUtils::updateArchive(std::move(archive), std::move(paretoFront), specimenComparator);

        history.push_back(archive);
    }

    return history;
}
