#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/CommandLineOptions.h"
#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/algo/Algo.h"
#include "genetics/ParetoFrontJsonWriter.h"
#include "genetics/crossing/RandomCutCrossoverFactory.h"
#include "genetics/comparison/SimpleSpecimenComparator.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/search/NormalRandomSearchFactory.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "genetics/Specimen.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation_helper.h"
#include "simulation/SimulationFactory.h"
#include "simulation/VerletFactory.h"
#include "visual/PlotTrajectory.h"

namespace
{
    constexpr std::size_t LOCAL_SEARCH_ITERATIONS = 25;

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

    void plotRepresentativeTrajectory(
        const SimulationFactory& simulationFactory,
        const SimulationState& state,
        const std::vector<Specimen>& paretoFront)
    {
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
        const std::string& outputFilePath) -> int
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
            MUTATION_THRUST_RANGE,
            state.probeProperties);

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

        SimpleSpecimenComparator specimenComparator;

        Algo::Factories factories{
            initializerFactory,
            selectionFactory,
            crossoverFactory,
            mutationFactory,
            localImprovementFactory,
            fitnessEvaluatorFactory};

        Algo algorithm(
            POPULATION_SIZE,
            GENERATIONS,
            ELITE_COUNT,
            POPULATION_SIZE / 25,
            specimenComparator,
            factories);

        const std::vector<Specimen> paretoFront =
            algorithm.run();

        printParetoFront(
            paretoFront);

        writeParetoFrontJson(
            outputFilePath,
            paretoFront);

        std::cout
            << "Saved Pareto front JSON to: "
            << outputFilePath
            << '\n';

        plotRepresentativeTrajectory(
            verletFactory,
            state,
            paretoFront);

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
                options.outputFilePath());
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
