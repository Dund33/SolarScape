#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/CommandLineOptions.h"
#include "config/SimulationConfig.h"
#include "config/consts.h"
#include "genetics/ParetoFrontJsonWriter.h"
#include "genetics/Specimen.h"
#include "genetics/comparison/TrajectorySpecimenComparator.h"
#include "genetics/crossing/RandomCutCrossoverFactory.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/VectorSimulationFitnessEvaluatorFactory.h"
#include "genetics/init/RandomInitializerFactory.h"
#include "genetics/mutation/RandomUniformMutationFactory.h"
#include "genetics/nsga/NSGAIIIAlgorithm.h"
#include "genetics/selection/TournamentSelectionFactory.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "math/ProbeProperties.h"
#include "simulation/VectorVerletFactory.h"
#include "simulation_helper.h"

namespace
{
    auto run(const std::string& configFilePath, const std::string& outputFilePath, double mutationProbability, bool verbose) -> int
    {
        SimulationConfig config = SimulationConfig::loadFromFile(configFilePath);

        SimulationState state = createSimulationState(std::move(config));

        VectorVerletFactory simulationFactory(state.gravitationalConstant, state.initialBodies, state.targetBody,
                                              ProbeFactory(state.probeProperties, state.probePosition, state.probeVelocity).create());

        RandomInitializerFactory initializerFactory(MIN_MANEUVERS, MAX_MANEUVERS, MIN_MANEUVER_TIME, state.simulationTime,
                                                    MIN_MANEUVER_DURATION, MAX_MANEUVER_DURATION, state.probeProperties);

        TournamentSelectionFactory selectionFactory(TOURNAMENT_SIZE);

        RandomCutCrossoverFactory crossoverFactory;

        RandomUniformMutationFactory mutationFactory(mutationProbability, MUTATION_TIME_RANGE, MUTATION_DURATION_RANGE,
                                                     MUTATION_DIRECTION_RANGE, MUTATION_THROTTLE_RANGE);

        VectorSimulationFitnessEvaluatorFactory fitnessEvaluatorFactory(state.timeStep, state.simulationTime,
                                                                        state.targetPointFromTargetBody, simulationFactory);

        TrajectorySpecimenComparator specimenComparator;

        NSGAIIIAlgorithm::Factories factories{initializerFactory, selectionFactory, crossoverFactory, mutationFactory,
                                              fitnessEvaluatorFactory};

        NSGAIIIAlgorithm algorithm(POPULATION_SIZE, GENERATIONS, specimenComparator, factories, verbose);

        const ParetoFrontHistory paretoFrontHistory = algorithm.run();

        writeParetoFrontJson(outputFilePath, paretoFrontHistory);

        std::cout << "Saved Pareto front history JSON to: " << outputFilePath << '\n';

        return 0;
    }
} // namespace

auto main(int argc, char* argv[]) -> int
{
    try
    {
        const CommandLineOptions options = CommandLineOptions::parse(argc, argv, "scenario1.yml", "pareto-front.json",
                                                                     NSGAIII_MUTATION_PROBABILITY);

        if (options.helpRequested())
        {
            CommandLineOptions::printUsage(std::cout, argc > 0 ? argv[0] : nullptr, "scenario1.yml", "pareto-front.json",
                                           NSGAIII_MUTATION_PROBABILITY);
            return 0;
        }

        try
        {
            return run(options.configFilePath(), options.outputFilePath(), options.mutationProbability(), options.verbose());
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
        CommandLineOptions::printUsage(std::cerr, argc > 0 ? argv[0] : nullptr, "scenario1.yml", "pareto-front.json",
                                       NSGAIII_MUTATION_PROBABILITY);
        return 2;
    }
}
