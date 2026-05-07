//
// Created by Luke on 5/7/2026.
//

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "Body.h"
#include "computation/DistanceAnalysis.h"
#include "computation/Verlet.h"
#include "external/indicators/indicators.hpp"

int main()
{
    const Real gravitationalConstant = 6.67430e-11L;
    const Real timeStep = 5 * 60.0L;
    const Real simulationTime = 60.0L * 60.0L * 24.0L * 365.0L;
    const int steps = static_cast<int>(simulationTime / timeStep);

    const std::size_t centralBodyIndex = 0;
    const std::size_t secondBodyIndex = 1;
    const std::size_t probeBodyIndex = 2;
    const Vector3 targetPointFromCentralBody(10.0e9L, 0.0L, 0.0L);

    std::vector<Body> bodies;
    bodies.push_back(Body(Vector3(0.0L, 0.0L, 0.0L), Vector3(0.0L, 0.0L, 0.0L), 1.98847e30L));
    bodies.push_back(Body(Vector3(149597870700.0L, 0.0L, 0.0L), Vector3(0.0L, 29780.0L, 0.0L), 5.9722e24L));

    const Vector3 probeStartPosition = bodies[secondBodyIndex].position + Vector3(0.0L, 1.0e9L, 0.0L);
    const Vector3 probeStartVelocity = bodies[secondBodyIndex].velocity + Vector3(-42000.0L, 0.0L, 0.0L);
    bodies.push_back(Body(probeStartPosition, probeStartVelocity, 1000.0L));

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
        if (step % 1000 == 0)
        {
            const std::size_t progress = static_cast<std::size_t>(100 * step / steps);
            progressBar.set_progress(progress);
        }

        const Real time = step * timeStep;
        const Vector3 targetPoint = DistanceAnalysis::absolutePointForBody(
            bodies[centralBodyIndex],
            targetPointFromCentralBody);

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

        if (step < steps)
        {
            Verlet::step(bodies, timeStep, gravitationalConstant);
        }
    }

    return 0;
}
