#include <algorithm>
#include <exception>
#include <iostream>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/comparison/SimpleSpecimenComparator.h"
#include "genetics/crossing/RandomCutCrossoverFactory.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/nsga/NSGAIIAlgorithm.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation/VerletFactory.h"

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
        Vector3 probePosition;
        Vector3 probeVelocity;
        ProbeProperties probeProperties;
    };

    auto createSimulationState(SimulationConfig&& config) -> SimulationState
    {
        return {
            config.gravitationalConstant,
            config.timeStep,
            config.simulationTime,
            config.targetPointFromTargetBody,
            std::move(config.bodies),
            std::move(config.targetBody),
            config.probePosition,
            config.probeVelocity,
            config.probeProperties};
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

    void printParetoFront(
        const std::vector<Specimen>& paretoFront)
    {
        std::cout
            << "\nFinal Pareto front size: "
            << paretoFront.size()
            << '\n';

        for (std::size_t i = 0; i < paretoFront.size(); ++i)
        {
            std::cout
                << "Pareto front specimen " << i
                << " fitness = ";
            printFitnessValue(
                paretoFront[i].getFitness().value());
            std::cout << '\n';
        }
    }

    auto run() -> int
    {
        SimulationConfig config =
            SimulationConfig::loadFromFile(
                "config.yaml");

        SimulationState state =
            createSimulationState(
                std::move(config));

        VerletFactory verletFactory(
            state.gravitationalConstant,
            state.initialBodies,
            state.targetBody,
            ProbeFactory(
                state.probeProperties,
                state.probePosition,
                state.probeVelocity).create());

        RandomInitializerFactory initializerFactory(
            MIN_MANEUVERS,
            MAX_MANEUVERS,
            MIN_MANEUVER_TIME,
            state.simulationTime,
            MIN_MANEUVER_DURATION,
            MAX_MANEUVER_DURATION,
            state.probeProperties);

        TournamentSelectionFactory selectionFactory(
            TOURNAMENT_SIZE);

        RandomCutCrossoverFactory crossoverFactory;

        RandomUniformMutationFactory mutationFactory(
            MUTATION_PROBABILITY,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE,
            state.probeProperties);

        SimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            verletFactory);

        SimpleSpecimenComparator specimenComparator;

        NSGAIIAlgorithm::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            fitnessEvaluatorFactory};

        NSGAIIAlgorithm algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            POPULATION_SIZE / 25,
            specimenComparator,
            factories);

        const std::vector<Specimen> paretoFront =
            algorithm.run();

        printParetoFront(
            paretoFront);

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
