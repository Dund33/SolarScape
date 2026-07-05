#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/CommandLineOptions.h"
#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/ParetoFrontJsonWriter.h"
#include "genetics/comparison/TrajectorySpecimenComparator.h"
#include "genetics/crossing/RandomCutCrossoverFactory.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/nsga/NSGAIIAlgorithm.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "genetics/Specimen.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation_helper.h"
#include "simulation/VerletFactory.h"

namespace
{
    auto run(
        const std::string& configFilePath,
        const std::string& outputFilePath,
        bool verbose) -> int
    {
        SimulationConfig config =
            SimulationConfig::loadFromFile(
                configFilePath);

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
            MUTATION_DIRECTION_RANGE,
            MUTATION_THROTTLE_RANGE);

        SimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            verletFactory);

        TrajectorySpecimenComparator specimenComparator;

        NSGAIIAlgorithm::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            fitnessEvaluatorFactory};

        NSGAIIAlgorithm algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            specimenComparator,
            factories,
            verbose);

        const ParetoFrontHistory paretoFrontHistory =
            algorithm.run();

        writeParetoFrontJson(
            outputFilePath,
            paretoFrontHistory);

        std::cout
            << "Saved Pareto front history JSON to: "
            << outputFilePath
            << '\n';

        return 0;
    }
}

auto main(
    int argc,
    char* argv[]) -> int
{
    try
    {
        const CommandLineOptions options =
            CommandLineOptions::parse(
                argc,
                argv);

        if (options.helpRequested())
        {
            CommandLineOptions::printUsage(
                std::cout,
                argc > 0 ? argv[0] : nullptr);
            return 0;
        }

        try
        {
            return run(
                options.configFilePath(),
                options.outputFilePath(),
                options.verbose());
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
    catch (const CommandLineParseError& e)
    {
        std::cerr << "Argument error: " << e.what() << '\n';
        CommandLineOptions::printUsage(
            std::cerr,
            argc > 0 ? argv[0] : nullptr);
        return 2;
    }
}
