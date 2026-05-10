//
// Created by Luke on 5/7/2026.
//

#include <algorithm>
#include <execution>
#include <iostream>
#include <limits>
#include <vector>

#include "config/SimulationConfig.h"
#include "math/Body.h"
#include "simulation/DistanceAnalysis.h"
#include "visual/PlotTrajectory.h"

#include "genetics/Specimen.h"
#include "genetics/init/RandomInitializer.h"
#include "genetics/selection/TournamentSelection.h"
#include "genetics/crossing/Crossover.h"
#include "genetics/mutation/Mutation.h"

#include "config/consts.h"

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
    const Vector3 targetPointFromTargetBody =
        config.targetPointFromTargetBody;
    const std::size_t probeBodyIndex = config.probeBodyIndex;
    const std::size_t targetBodyIndex = config.targetBodyIndex;

    std::vector<Body> initialBodies = std::move(config.bodies);

    const std::size_t populationSize = 250;
    const std::size_t generations = 100;
    const std::size_t eliteCount = 2;

    RandomInitializer initializer(
        1,                  // minManeuvers
        5,                  // maxManeuvers
        0.0L,               // minInitTime
        simulationTime,     // maxInitTime
        1.0L,               // minDuration
        100000.0L,          // maxDuration
        -1000.0L,             // minThrust
        1000.0L               // maxThrust
    );

    TournamentSelection selection(5);

    Crossover crossover;

    Mutation mutation(
        0.1,        // mutationProbability
        1000.0L,    // maxTimeOffset
        1000.0L,    // maxDurationOffset
        100.0L        // maxThrustOffset
    );

    std::vector<Specimen> population =
        initializer.createPopulation(populationSize);

    auto evaluateFitness = [&](Specimen& specimen)
    {
        if (specimen.getFitness() != std::numeric_limits<double>::max())
        {
            return;
        }

        if (specimen.getTotalImpulse() > MAX_IMPULSE)
        {
            const long double scale =
                MAX_IMPULSE / specimen.getTotalImpulse();

            for (std::size_t i = 0; i < specimen.size(); ++i)
            {
                Maneuver& maneuver = specimen[i];

                const Vector3 scaledThrust =
                    maneuver.getThrust() * scale;

                maneuver = Maneuver(
                    scaledThrust,
                    maneuver.getInitTime(),
                    maneuver.getDuration()
                );
            }
        }

        std::vector<Body> bodies = initialBodies;

        const Real minimumDistance =
            DistanceAnalysis::minimumDistanceFromMovingPoint(
                bodies,
                probeBodyIndex,
                targetBodyIndex,
                targetPointFromTargetBody,
                simulationTime,
                timeStep,
                gravitationalConstant,
                specimen.getManeuvers());

        specimen.setFitness(static_cast<double>(minimumDistance));
    };

    for (std::size_t generation = 0;
         generation < generations;
         ++generation)
    {
        std::for_each(
            std::execution::par,
            population.begin(),
            population.end(),
            [&](Specimen& specimen)
            {
                evaluateFitness(specimen);
            }
        );

        std::sort(
            population.begin(),
            population.end(),
            [](const Specimen& a, const Specimen& b)
            {
                return a.getFitness() < b.getFitness();
            });

        const Specimen& best = population.front();

        std::cout
            << "Generation " << generation
            << " | Best fitness = "
            << best.getFitness()
            << '\n';

        std::vector<Specimen> newPopulation;
        newPopulation.reserve(populationSize);

        for (std::size_t i = 0;
             i < eliteCount && i < population.size();
             ++i)
        {
            newPopulation.push_back(population[i]);
        }

        while (newPopulation.size() < populationSize)
        {
            const Specimen& parent1 =
                selection.select(population);

            const Specimen& parent2 =
                selection.select(population);

            auto [child1, child2] =
                crossover.cross(parent1, parent2);

            mutation.mutate(child1);
            mutation.mutate(child2);

            newPopulation.push_back(std::move(child1));

            if (newPopulation.size() < populationSize)
            {
                newPopulation.push_back(std::move(child2));
            }
        }

        population = std::move(newPopulation);
    }

    for (auto& specimen : population)
    {
        evaluateFitness(specimen);
    }

    std::sort(
        population.begin(),
        population.end(),
        [](const Specimen& a, const Specimen& b)
        {
            return a.getFitness() < b.getFitness();
        });

    const Specimen& best = population.front();

    std::cout
        << "\nFinal best fitness: "
        << best.getFitness()
        << '\n';

    std::vector<Body> bodies = initialBodies;

    plotTrajectory(
       gravitationalConstant,
       timeStep,
       static_cast<std::size_t>(simulationTime / timeStep),
       targetPointFromTargetBody,
       targetBodyIndex,
       probeBodyIndex,
       bodies,
       best.getManeuvers()
   );

    return 0;
}
