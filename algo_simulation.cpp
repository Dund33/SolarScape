#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/CommandLineOptions.h"
#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/algo/Algo.h"
#include "genetics/ParetoFrontJsonWriter.h"
#include "genetics/crossing/AlignedSimilarityCrossoverFactory.h"
#include "genetics/comparison/TrajectorySpecimenComparator.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/VectorSimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/ExtensiveMutationFactory.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "genetics/Specimen.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation_helper.h"
#include "simulation/SimulationFactory.h"
#include "simulation/VectorVerletFactory.h"
#include "simulation/VerletFactory.h"
#include "visual/PlotTrajectory.h"

namespace
{
    void plotRepresentativeTrajectory(
        const SimulationFactory& simulationFactory,
        const SimulationState& state,
        const ParetoFrontHistory& paretoFrontHistory)
    {
        if (paretoFrontHistory.empty())
        {
            return;
        }

        const ParetoFront& paretoFront =
            paretoFrontHistory.back();

        if (paretoFront.empty())
        {
            return;
        }

        plotTrajectory(
            simulationFactory,
            state.timeStep,
            static_cast<std::size_t>(state.simulationTime / state.timeStep),
            state.targetPointFromTargetBody,
            paretoFront.front().getManeuvers()
        );
    }

    auto run(
        const std::string& configFilePath,
        const std::string& outputFilePath,
        const std::string& diversityLogFilePath,
        bool verbose) -> int
    {
        SimulationConfig config =
            SimulationConfig::loadFromFile(
                configFilePath);

        SimulationState state =
            createSimulationState(
                std::move(config));

        VerletFactory trajectorySimulationFactory(
            state.gravitationalConstant,
            state.initialBodies,
            state.targetBody,
            ProbeFactory(
                state.probeProperties,
                state.probePosition,
                state.probeVelocity).create());

        VectorVerletFactory fitnessSimulationFactory(
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

        AlignedSimilarityCrossoverFactory crossoverFactory;

        ExtensiveMutationFactory mutationFactory(
            MUTATION_PROBABILITY,
            EXTENSIVE_MUTATION_ADD_PROBABILITY,
            EXTENSIVE_MUTATION_REMOVE_PROBABILITY,
            MIN_MANEUVERS,
            MAX_MANEUVERS,
            MIN_MANEUVER_TIME,
            state.simulationTime,
            MIN_MANEUVER_DURATION,
            MAX_MANEUVER_DURATION,
            MUTATION_TIME_RANGE,
            MUTATION_DURATION_RANGE,
            MUTATION_DIRECTION_RANGE,
            MUTATION_THROTTLE_RANGE);

        VectorSimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(
            state.timeStep,
            state.simulationTime,
            state.targetPointFromTargetBody,
            fitnessSimulationFactory);

        TrajectorySpecimenComparator specimenComparator;

        Algo::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            fitnessEvaluatorFactory};

        std::ofstream diversityLogFile;

        if (!diversityLogFilePath.empty())
        {
            diversityLogFile.open(diversityLogFilePath);

            if (!diversityLogFile)
            {
                throw std::runtime_error(
                    "Cannot open diversity diagnostics log file: " +
                    diversityLogFilePath);
            }
        }

        Algo algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            ELITE_COUNT,
            POPULATION_SIZE / 25,
            specimenComparator,
            factories,
            verbose,
            diversityLogFilePath.empty()
                ? nullptr
                : &diversityLogFile);

        const ParetoFrontHistory paretoFrontHistory =
            algorithm.run();

        writeParetoFrontJson(
            outputFilePath,
            paretoFrontHistory);

        std::cout
            << "Saved Pareto front history JSON to: "
            << outputFilePath
            << '\n';

        if (!diversityLogFilePath.empty())
        {
            std::cout
                << "Saved ALGO diversity diagnostics log to: "
                << diversityLogFilePath
                << '\n';
        }

        plotRepresentativeTrajectory(
            trajectorySimulationFactory,
            state,
            paretoFrontHistory);

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
                options.hasDiversityLogFilePath()
                    ? options.diversityLogFilePath()
                    : std::string(),
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
