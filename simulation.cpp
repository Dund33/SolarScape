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
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/repair/SimpleLinearRepairFactory.h"
#include "genetics/search/NormalRandomSearchFactory.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "genetics/Specimen.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
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

    void printFinalResult(
        const Specimen& best)
    {
        std::cout
            << "\nFinal best fitness: ";
        printFitnessValue(best.getFitness().value());
        std::cout << '\n';
    }

    void plotBestTrajectory(
        const SimulationFactory& simulationFactory,
        const SimulationState& state,
        const Specimen& best)
    {
        plotTrajectory(
            simulationFactory,
            state.timeStep,
            static_cast<std::size_t>(state.simulationTime / state.timeStep),
            state.targetPointFromTargetBody,
            best.getManeuvers()
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

        VerletFactory verletFactory(
            state.gravitationalConstant,
            state.initialBodies,
            state.targetBody,
            ProbeFactory(
                state.probeProperties,
                state.probePosition,
                state.probeVelocity));

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

        SimpleLinearRepairFactory repairFactory;

        RandomUniformMutationFactory mutationFactory(
            MUTATION_PROBABILITY,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE,
            state.probeProperties,
            repairFactory);

        const Real maxPhysicalThrust =
            std::max(
                state.probeProperties.fuelFlow() *
                    state.probeProperties.specificImpulse(),
                1.0L);

        NormalRandomSearchFactory localImprovementFactory(
            LOCAL_SEARCH_ITERATIONS,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_THRUST_RANGE / maxPhysicalThrust,
            state.probeProperties);

        SimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            verletFactory);

        GeneticAlgorithm::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            localImprovementFactory,
            fitnessEvaluatorFactory};

        GeneticAlgorithm algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            ELITE_COUNT,
            POPULATION_SIZE / 25,
            factories);

        const Specimen best = algorithm.run();

        printFinalResult(
            best);

        plotBestTrajectory(
            verletFactory,
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
