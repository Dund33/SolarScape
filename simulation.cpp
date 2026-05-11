//
// Created by Luke on 5/7/2026.
//

#include <algorithm>
#include <execution>
#include <iostream>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "config/consts.h"
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

namespace
{
    struct SimulationState
    {
        Real gravitationalConstant{};
        Real timeStep{};
        Real simulationTime{};

        Vector3 targetPointFromTargetBody;

        std::vector<Body> initialBodies;
        Body targetBody;
        Probe probe;

        std::vector<Body*> bodyPointers;
    };

    auto loadConfig(const std::string& filePath) -> SimulationConfig
    {
        return SimulationConfig::loadFromFile(filePath);
    }

    auto createBodyPointers(
        std::vector<Body>& bodies,
        Body& targetBody,
        Probe& probe) -> std::vector<Body*>
    {
        std::vector<Body*> bodyPointers;
        bodyPointers.reserve(bodies.size() + 2);

        for (Body& body : bodies)
        {
            bodyPointers.push_back(&body);
        }

        bodyPointers.push_back(&targetBody);
        bodyPointers.push_back(&probe);

        return bodyPointers;
    }

    auto createSimulationState(SimulationConfig&& config) -> SimulationState
    {
        SimulationState state;

        state.gravitationalConstant = config.gravitationalConstant;
        state.timeStep = config.timeStep;
        state.simulationTime = config.simulationTime;
        state.targetPointFromTargetBody = config.targetPointFromTargetBody;

        state.initialBodies = std::move(config.bodies);
        state.targetBody = std::move(config.targetBody);
        state.probe = std::move(config.probe);

        state.bodyPointers =
            createBodyPointers(
                state.initialBodies,
                state.targetBody,
                state.probe);

        return state;
    }

    auto createInitializer(
        Real simulationTime,
        Probe& probe) -> RandomInitializer
    {
        return {
            MIN_MANEUVERS,
            MAX_MANEUVERS,
            MIN_MANEUVER_TIME,
            simulationTime,
            MIN_MANEUVER_DURATION,
            MAX_MANEUVER_DURATION,
            &probe
        };
    }

    auto createMutation() -> Mutation
    {
        return Mutation(
            MUTATION_PROBABILITY,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE
        );
    }

    void evaluatePopulationUnsequenced(
        std::vector<Specimen>& population,
        const FitnessEvaluator& fitnessEvaluator)
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

    void sortPopulationByFitness(
        std::vector<Specimen>& population)
    {
        std::ranges::sort(
            population,
            {},
            [](const Specimen& specimen)
            {
                return specimen.getFitness();
            });
    }

    void printGenerationResult(
        std::size_t generation,
        const Specimen& best)
    {
        std::cout
            << "Generation " << generation
            << " | Best fitness = "
            << best.getFitness().value()
            << '\n';
    }

    void printFinalResult(
        const Specimen& best)
    {
        std::cout
            << "\nFinal best fitness: "
            << best.getFitness().value()
            << '\n';
    }

    void copyElite(
        const std::vector<Specimen>& population,
        std::vector<Specimen>& newPopulation)
    {
        std::ranges::copy(
            population |
            std::views::take(std::min(ELITE_COUNT, population.size())),
            std::back_inserter(newPopulation));
    }

    void fillPopulationWithChildren(
        const std::vector<Specimen>& population,
        std::vector<Specimen>& newPopulation,
        TournamentSelection& selection,
        Crossover& crossover,
        Mutation& mutation)
    {
        while (newPopulation.size() < POPULATION_SIZE)
        {
            const Specimen& parent1 = selection.select(population);
            const Specimen& parent2 = selection.select(population);

            auto [child1, child2] = crossover.cross(parent1, parent2);

            mutation.mutate(child1);
            mutation.mutate(child2);

            newPopulation.push_back(std::move(child1));

            if (newPopulation.size() < POPULATION_SIZE)
            {
                newPopulation.push_back(std::move(child2));
            }
        }
    }

    void addImmigrants(
        std::vector<Specimen>& population,
        RandomInitializer& initializer)
    {
        constexpr std::size_t immigrants = POPULATION_SIZE / 25;

        std::ranges::generate(
            population |
            std::views::reverse |
            std::views::take(immigrants),
            [&initializer]
            {
                return initializer.create();
            });
    }

    auto createNextGeneration(
        const std::vector<Specimen>& population,
        RandomInitializer& initializer,
        TournamentSelection& selection,
        Crossover& crossover,
        Mutation& mutation) -> std::vector<Specimen>
    {
        std::vector<Specimen> newPopulation;
        newPopulation.reserve(POPULATION_SIZE);

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

    auto runGeneticAlgorithm(
        SimulationState& state) -> Specimen
    {
        RandomInitializer initializer =
            createInitializer(
                state.simulationTime,
                state.probe);

        TournamentSelection selection(
            TOURNAMENT_SIZE);

        Crossover crossover;

        Mutation mutation =
            createMutation();

        FitnessEvaluator fitnessEvaluator(
            state.gravitationalConstant,
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            state.initialBodies,
            state.probe,
            state.targetBody
        );

        std::vector<Specimen> population =
            initializer.createPopulation(
                POPULATION_SIZE);

        for (std::size_t generation = 0; generation < GENERATIONS; ++generation)
        {
            evaluatePopulationUnsequenced(
                population,
                fitnessEvaluator);

            sortPopulationByFitness(
                population);

            printGenerationResult(
                generation,
                population.front());

            population =
                createNextGeneration(
                    population,
                    initializer,
                    selection,
                    crossover,
                    mutation);
        }

        evaluatePopulationUnsequenced(
            population,
            fitnessEvaluator);

        sortPopulationByFitness(
            population);

        return population.front();
    }

    void plotBestTrajectory(
        const SimulationState& state,
        const Specimen& best)
    {
        std::vector<Maneuver> maneuvers =
            best.getManeuvers();

        plotTrajectory(
            state.gravitationalConstant,
            state.timeStep,
            static_cast<std::size_t>(state.simulationTime / state.timeStep),
            state.targetPointFromTargetBody,
            state.targetBody,
            state.probe,
            state.bodyPointers,
            maneuvers
        );
    }

    auto run() -> int
    {
        SimulationConfig config =
            loadConfig(
                "config.yaml");

        SimulationState state =
            createSimulationState(
                std::move(config));

        const Specimen best =
            runGeneticAlgorithm(
                state);

        printFinalResult(
            best);

        plotBestTrajectory(
            state,
            best);

        return 0;
    }
}

auto main() -> int
{
    try
    {
        return run();
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
}
