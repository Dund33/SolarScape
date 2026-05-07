//
// Created by Luke on 5/7/2026.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "config/SimulationConfig.h"
#include "math/Body.h"
#include "simulation/DistanceAnalysis.h"
#include "math/Verlet.h"
#include "external/indicators/indicators.hpp"

int main()
{
    SimulationConfig config =
        SimulationConfig::loadFromFile(
            "config.yaml");

    const Real gravitationalConstant =
        config.gravitationalConstant;

    const Real timeStep =
        config.timeStep;

    const Real simulationTime =
        config.simulationTime;

    const int steps =
        static_cast<int>(
            simulationTime / timeStep);

    const Vector3 targetPointFromCentralBody =
        config.targetPointFromCentralBody;

    const size_t probeBodyIndex =
        config.probeBodyIndex;

    const size_t centralBodyIndex =
        config.centralBodyIndex;

    std::vector<Body> bodies =
        std::move(config.bodies);

    const Real minimumDistance = DistanceAnalysis::minimumDistanceFromMovingPoint(
        bodies,
        probeBodyIndex,
        centralBodyIndex,
        targetPointFromCentralBody,
        simulationTime,
        timeStep,
        gravitationalConstant);

    std::ofstream resultOutput("minimum_distance.txt");
    if (!resultOutput)
    {
        std::cerr << "Nie mozna utworzyc pliku minimum_distance.txt\n";
        return 1;
    }

    resultOutput << std::setprecision(std::numeric_limits<Real>::max_digits10);
    resultOutput << "observed_body_index=" << probeBodyIndex << '\n';
    resultOutput << "target_body_index=" << centralBodyIndex << '\n';
    resultOutput << "target_point_relative_x=" << targetPointFromCentralBody.x << '\n';
    resultOutput << "target_point_relative_y=" << targetPointFromCentralBody.y << '\n';
    resultOutput << "target_point_relative_z=" << targetPointFromCentralBody.z << '\n';
    resultOutput << "simulation_time=" << simulationTime << '\n';
    resultOutput << "time_step=" << timeStep << '\n';
    resultOutput << "minimum_distance=" << minimumDistance << '\n';

    std::ofstream output("simulation.csv");
    if (!output)
    {
        std::cerr << "Nie mozna utworzyc pliku simulation.csv\n";
        return 1;
    }

    output << std::setprecision(std::numeric_limits<Real>::max_digits10);
    output << "step,time,body,x,y,z,vx,vy,vz,mass,target_x,target_y,target_z\n";

    indicators::ProgressBar progressBar{
        indicators::option::BarWidth{50},
        indicators::option::Start{"["},
        indicators::option::Fill{"="},
        indicators::option::Lead{">"},
        indicators::option::Remainder{" "},
        indicators::option::End{"]"},
        indicators::option::PrefixText{"Generating trajectory"},
        indicators::option::ShowPercentage{true},
        indicators::option::ShowElapsedTime{true},
        indicators::option::ShowRemainingTime{true}
    };

    for (int step = 0; step <= steps; ++step)
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = DistanceAnalysis::absolutePointForBody(
            bodies[centralBodyIndex],
            targetPointFromCentralBody);

        if (step % 1000 == 0)
        {
            const std::size_t progress = static_cast<std::size_t>(100 * step / steps);
            progressBar.set_progress(progress);
            for (std::size_t i = 0; i < bodies.size(); ++i)
            {
                output << step << ','
                       << time << ','
                       << i << ','
                       << bodies[i].position.x << ','
                       << bodies[i].position.y << ','
                       << bodies[i].position.z << ','
                       << bodies[i].velocity.x << ','
                       << bodies[i].velocity.y << ','
                       << bodies[i].velocity.z << ','
                       << bodies[i].mass << ','
                       << targetPoint.x << ','
                       << targetPoint.y << ','
                       << targetPoint.z << '\n';
            }
        }

        if (step < steps)
        {
            Verlet::step(bodies, timeStep, gravitationalConstant);
        }
    }

    return 0;
}
