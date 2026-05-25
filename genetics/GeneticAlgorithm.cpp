#include "GeneticAlgorithm.h"

#include <algorithm>
#include <execution>
#include <iostream>
#include <iterator>
#include <ranges>
#include <utility>

#include "genetics/fitness/FitnessValue.h"

namespace
{
    void sortPopulationByFitness(
        std::vector<Specimen>& population)
    {
        std::ranges::sort(population);
    }

    void printFitnessValue(
        const FitnessValue& fitness)
    {
        std::cout
            << "[minimumDistance=" << fitness.minimumDistance
            << ", minimumDistanceTime=" << fitness.minimumDistanceTime
            << ", minimumDistanceFuelMass=" << fitness.minimumDistanceFuelMass
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

GeneticAlgorithm::GeneticAlgorithm(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t eliteCount,
    std::size_t immigrantCount,
    Factories factories
)
    : populationSize(populationSize),
      generations(generations),
      eliteCount(eliteCount),
      immigrantCount(immigrantCount),
      factories(factories)
{
}

Specimen GeneticAlgorithm::run() const
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
            population);

        localImprovement->improve(
            population.front(),
            *fitnessEvaluator);

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
        population);

    return population.front();
}

void GeneticAlgorithm::evaluatePopulationUnsequenced(
    std::vector<Specimen>& population,
    const FitnessEvaluator& fitnessEvaluator) const
{
    std::for_each(
        std::execution::par_unseq,
        population.begin(),
        population.end(),
        [&](Specimen& specimen)
        {
            fitnessEvaluator.evaluate(specimen);
        });
}

void GeneticAlgorithm::copyElite(
    const std::vector<Specimen>& population,
    std::vector<Specimen>& newPopulation) const
{
    std::ranges::copy(
        population |
        std::views::take(std::min(eliteCount, population.size())),
        std::back_inserter(newPopulation));
}

void GeneticAlgorithm::fillPopulationWithChildren(
    const std::vector<Specimen>& population,
    std::vector<Specimen>& newPopulation,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation) const
{
    while (newPopulation.size() < populationSize)
    {
        const Specimen& parent1 = selection.select(population);
        const Specimen& parent2 = selection.select(population);

        auto [child1, child2] = crossover.cross(parent1, parent2);

        mutation.mutate(child1);
        mutation.mutate(child2);

        newPopulation.push_back(std::move(child1));

        if (newPopulation.size() < populationSize)
        {
            newPopulation.push_back(std::move(child2));
        }
    }
}

void GeneticAlgorithm::addImmigrants(
    std::vector<Specimen>& population,
    Initializer& initializer) const
{
    std::ranges::generate(
        population |
        std::views::reverse |
        std::views::take(immigrantCount),
        [&initializer]
        {
            return initializer.create();
        });
}

auto GeneticAlgorithm::createNextGeneration(
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

    fillPopulationWithChildren(
        population,
        newPopulation,
        selection,
        crossover,
        mutation);

    addImmigrants(
        newPopulation,
        initializer);

    return newPopulation;
}
