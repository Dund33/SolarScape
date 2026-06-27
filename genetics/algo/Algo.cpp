#include "Algo.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>

#include "genetics/fitness/FitnessValue.h"
#include "genetics/utils/ParetoFrontUtils.h"

namespace
{
    void sortPopulationByFitness(
        std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator)
    {
        std::ranges::sort(
            population,
            [&specimenComparator](const Specimen& lhs, const Specimen& rhs)
            {
                return specimenComparator.isLess(lhs, rhs);
            });
    }

    void printFitnessValue(
        const FitnessValue& fitness)
    {
        std::cout
            << "[minimumDistance=" << fitness.minimumDistance
            << ", minimumDistanceTime=" << fitness.minimumDistanceTime
            << ", minimumDistanceFuelMass=" << fitness.minimumDistanceFuelMass
            << ", fuelConstraintViolation=" << fitness.fuelConstraintViolation
            << ']';
    }

    void printGenerationResult(
        std::size_t generation,
        const Specimen& best)
    {
        std::cout
            << "Generation " << generation
            << " | Best fitness = ";
        printFitnessValue(best.getFitness().value());
        std::cout << '\n';
    }

}

Algo::Algo(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t eliteCount,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories
)
    : populationSize(populationSize),
      generations(generations),
      eliteCount(eliteCount),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories)
{
}

std::vector<Specimen> Algo::run() const
{
    auto initializer =
        factories.initializerFactory.create();
    auto selection =
        factories.selectionFactory.create();
    auto crossover =
        factories.crossoverFactory.create();
    auto mutation =
        factories.mutationFactory.create();
    auto localImprovement =
        factories.localImprovementFactory.create();
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

        sortPopulationByFitness(
            population,
            specimenComparator);

        localImprovement->improve(
            population.front(),
            *fitnessEvaluator,
            specimenComparator);

        printGenerationResult(
            generation,
            population.front());

        population =
            createNextGeneration(
                population,
                *initializer,
                *selection,
                *crossover,
                *mutation);
    }

    evaluatePopulationUnsequenced(
        population,
        *fitnessEvaluator);

    sortPopulationByFitness(
        population,
        specimenComparator);

    return ParetoFrontUtils::firstFront(
        population,
        specimenComparator);
}

void Algo::copyElite(
    const std::vector<Specimen>& population,
    std::vector<Specimen>& newPopulation) const
{
    std::ranges::copy(
        population |
        std::views::take(std::min(eliteCount, population.size())),
        std::back_inserter(newPopulation));
}

auto Algo::createNextGeneration(
    const std::vector<Specimen>& population,
    Initializer& initializer,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation) const -> std::vector<Specimen>
{
    std::vector<Specimen> newPopulation;
    newPopulation.reserve(populationSize);

    copyElite(
        population,
        newPopulation);

    appendChildren(
        population,
        newPopulation,
        populationSize,
        specimenComparator,
        selection,
        crossover,
        mutation);

    replaceTailWithImmigrants(
        newPopulation,
        immigrantCount,
        initializer);

    return newPopulation;
}
