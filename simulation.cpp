//
// Created by Luke on 5/7/2026.
//

#include <algorithm>
#include <execution>
#include <iostream>
#include <iterator>
#include <ranges>
#include <vector>

#include "config/SimulationConfig.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "visual/PlotTrajectory.h"

#include "genetics/Specimen.h"
#include "genetics/init/RandomInitializer.h"
#include "genetics/selection/TournamentSelection.h"
#include "genetics/crossing/Crossover.h"
#include "genetics/mutation/Mutation.h"
#include "genetics/fitness/FitnessEvaluator.h"

auto main() -> int
{
    SimulationConfig config;

    try
    {
        config = SimulationConfig::loadFromFile("config.yaml");
    }
    catch (const YAML::Exception& e)
    {
        std::cerr << "YAML error: " << e.what() << '\n';
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    const Real gravitationalConstant = config.gravitationalConstant;
    const Real timeStep = config.timeStep;
    const Real simulationTime = config.simulationTime;
    const Vector3 targetPointFromTargetBody = config.targetPointFromTargetBody;
    const std::size_t probeBodyIndex = config.probeBodyIndex;
    const std::size_t targetBodyIndex = config.targetBodyIndex;

    std::vector<Body> initialBodies = std::move(config.bodies);
    Probe probe(
        initialBodies[probeBodyIndex].position(),
        initialBodies[probeBodyIndex].velocity(),
        initialBodies[probeBodyIndex].mass());

    std::vector<Body*> initialBodyPointers;
    initialBodyPointers.reserve(initialBodies.size());

    auto bodyPointerAt =
        [&](std::size_t index) -> Body*
        {
            if (index == probeBodyIndex)
            {
                return &probe;
            }

            return &initialBodies[index];
        };

    std::ranges::transform(
        std::views::iota(std::size_t{0}, initialBodies.size()),
        std::back_inserter(initialBodyPointers),
        bodyPointerAt);

    Body* targetBody = bodyPointerAt(targetBodyIndex);

    const std::size_t populationSize = 250;
    const std::size_t generations = 250;
    const std::size_t eliteCount = 2;

    RandomInitializer initializer(
        1,
        25,
        0.0L,
        simulationTime,
        1.0L,
        10000.0L,
        -1000.0L,
        1000.0L,
        &probe
    );

    TournamentSelection selection(5);
    Crossover crossover;

    Mutation mutation(
        0.1,
        10000.0L,
        5000.0L,
        1000.0L
    );

    std::vector<Specimen> population = initializer.createPopulation(populationSize);

    FitnessEvaluator fitnessEvaluator(
        gravitationalConstant,
        timeStep,
        simulationTime,
        targetPointFromTargetBody,
        initialBodyPointers,
        &probe,
        targetBody
    );

    for (const std::size_t generation : std::views::iota(std::size_t{0}, generations))
    {
        std::for_each(
            std::execution::par,
            population.begin(),
            population.end(),
            [&](Specimen& specimen)
            {
                fitnessEvaluator.evaluate(specimen);
            }
        );

        std::ranges::sort(
            population,
            {},
            [](const Specimen& specimen)
            {
                return specimen.getFitness();
            });

        const Specimen& best = population.front();

        std::cout
            << "Generation " << generation
            << " | Best fitness = "
            << best.getFitness().value()
            << '\n';

        std::vector<Specimen> newPopulation;
        newPopulation.reserve(populationSize);

        std::ranges::copy(
            population |
            std::views::take(std::min(eliteCount, population.size())),
            std::back_inserter(newPopulation));

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

        // Add migration
        constexpr std::size_t immigrants = populationSize / 25;

        std::ranges::generate(
            newPopulation |
            std::views::reverse |
            std::views::take(immigrants),
            [&initializer]
            {
                return initializer.create();
            });

        population = std::move(newPopulation);
    }

    std::for_each(
        std::execution::par_unseq,
        population.begin(),
        population.end(),
        [&](Specimen& specimen)
        {
            fitnessEvaluator.evaluate(specimen);
        }
    );

    std::ranges::sort(
        population,
        {},
        [](const Specimen& specimen)
        {
            return specimen.getFitness();
        });

    const Specimen& best = population.front();

    std::cout
        << "\nFinal best fitness: "
        << best.getFitness().value()
        << '\n';

    std::vector<Maneuver> maneuvers = best.getManeuvers();

    plotTrajectory(
        gravitationalConstant,
        timeStep,
        static_cast<std::size_t>(simulationTime / timeStep),
        targetPointFromTargetBody,
        targetBody,
        &probe,
        initialBodyPointers,
        maneuvers
    );

    return 0;
}
