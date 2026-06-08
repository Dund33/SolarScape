#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/Specimen.h"
#include "genetics/comparison/NSGAIIComparator.h"
#include "genetics/crossing/AlignedSimilarityCrossoverFactory.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/moead/MOEADAlgorithm.h"
#include "genetics/mutation/ExtensiveMutationFactory.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation/VerletFactory.h"
#include "simulation_helper.h"

namespace
{
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

        AlignedSimilarityCrossoverFactory crossoverFactory;

        ExtensiveMutationFactory mutationFactory(
            MUTATION_PROBABILITY,
            0.5,
            0.5,
            MIN_MANEUVERS,
            MAX_MANEUVERS,
            MIN_MANEUVER_TIME,
            state.simulationTime,
            MIN_MANEUVER_DURATION,
            MAX_MANEUVER_DURATION,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE,
            state.probeProperties);

        SimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            verletFactory);

        NSGAIIComparator specimenComparator;

        MOEADAlgorithm::Factories factories{
            initializerFactory,
            crossoverFactory,
            mutationFactory,
            fitnessEvaluatorFactory};

        const std::size_t neighborhoodSize =
            std::max<std::size_t>(
                2,
                POPULATION_SIZE / 10);

        MOEADAlgorithm algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            neighborhoodSize,
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
