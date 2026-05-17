#include <algorithm>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/crossing/RandomCutCrossoverFactory.h"
#include "genetics/GeneticAlgorithm.h"
#include "genetics/fitness/FitnessResult.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/search/NormalRandomSearchFactory.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "genetics/Specimen.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationFactory.h"
#include "simulation/VerletFactory.h"
#include "visual/PlotTrajectory.h"

namespace
{
    constexpr std::size_t LOCAL_SEARCH_ITERATIONS = 25;

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

    auto createLocalImprovementFactory(
        const Probe& probe) -> NormalRandomSearchFactory
    {
        const Real maxPhysicalThrust =
            std::max(
                probe.fuelFlow() * probe.specificImpulse(),
                1.0L);

        return NormalRandomSearchFactory(
            LOCAL_SEARCH_ITERATIONS,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE / maxPhysicalThrust);
    }

    void printFitnessResult(
        const FitnessResult& fitness)
    {
        std::cout << '[';

        for (std::size_t i = 0; i < FitnessResult::kSize; ++i)
        {
            if (i > 0)
            {
                std::cout << ", ";
            }

            std::cout << fitness.get(i);
        }

        std::cout << ']';
    }

    void printFinalResult(
        const Specimen& best)
    {
        std::cout
            << "\nFinal best fitness: ";
        printFitnessResult(best.getFitness().value());
        std::cout << '\n';
    }

    auto runGeneticAlgorithm(
        const SimulationState& state,
        GeneticAlgorithm::Factories factories) -> Specimen
    {
        GeneticAlgorithm algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            ELITE_COUNT,
            POPULATION_SIZE / 25,
            factories);

        return algorithm.run();
    }

    void plotBestTrajectory(
        const Simulation& simulation,
        const SimulationState& state,
        const Specimen& best)
    {
        std::vector<Maneuver> maneuvers =
            best.getManeuvers();

        plotTrajectory(
            simulation,
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
            SimulationConfig::loadFromFile(
                "config.yaml");

        SimulationState state =
            createSimulationState(
                std::move(config));

        VerletFactory verletFactory;
        const SimulationFactory& simulationFactory = verletFactory;
        auto simulation =
            simulationFactory.create();

        RandomInitializerFactory initializerFactory(
            MIN_MANEUVERS,
            MAX_MANEUVERS,
            MIN_MANEUVER_TIME,
            state.simulationTime,
            MIN_MANEUVER_DURATION,
            MAX_MANEUVER_DURATION,
            state.probe);

        TournamentSelectionFactory selectionFactory(
            TOURNAMENT_SIZE);

        RandomCutCrossoverFactory crossoverFactory;

        RandomUniformMutationFactory mutationFactory(
            MUTATION_PROBABILITY,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE);

        NormalRandomSearchFactory localImprovementFactory =
            createLocalImprovementFactory(state.probe);

        SimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.gravitationalConstant,
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            state.initialBodies,
            state.probe,
            state.targetBody,
            *simulation);

        GeneticAlgorithm::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            localImprovementFactory,
            fitnessEvaluatorFactory};

        const Specimen best =
            runGeneticAlgorithm(
                state,
                factories);

        printFinalResult(
            best);

        plotBestTrajectory(
            *simulation,
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
