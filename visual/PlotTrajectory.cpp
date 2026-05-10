//
// Created by Luke on 5/9/2026.
//

#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include "PlotTrajectory.h"

#include <numeric>


void plotTrajectory(const Real gravitationalConstant,
    const Real timeStep,
    const size_t steps,
    const Vector3 targetPointFromTargetBody,
    const size_t targetBodyIndex,
    const size_t probeBodyIndex,
    std::vector<Body>& bodies,
    const std::vector<Maneuver>& maneuvers)
{
    std::ofstream output("simulation.csv");
    if (!output)
    {
        std::cerr << "Nie mozna utworzyc pliku simulation.csv\n";
        return;
    }

    output << std::setprecision(std::numeric_limits<Real>::max_digits10);
    output << "step,time,body,x,y,z,vx,vy,vz,mass,target_x,target_y,target_z\n";

    for (int step = 0; step <= steps; ++step)
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = DistanceAnalysis::absolutePointForBody(
            bodies[targetBodyIndex],
            targetPointFromTargetBody);

        auto executedManeuvers =
                maneuvers
                | std::views::filter([time](const Maneuver& maneuver)
                {
                    return maneuver.getInitTime() < time &&
                           maneuver.getInitTime() + maneuver.getDuration() > time;
                });

        auto appliedForces = executedManeuvers | std::views::transform(&Maneuver::getThrust);

        const auto totalForce = std::accumulate(appliedForces.begin(), appliedForces.end(), Vector3{});

        if (step % 500 == 0)
        {
            for (std::size_t i = 0; i < bodies.size(); ++i)
            {
                output << step << ','
                    << time << ','
                    << i << ','
                    << bodies[i].position().x << ','
                    << bodies[i].position().y << ','
                    << bodies[i].position().z << ','
                    << bodies[i].velocity().x << ','
                    << bodies[i].velocity().y << ','
                    << bodies[i].velocity().z << ','
                    << bodies[i].mass() << ','
                    << targetPoint.x << ','
                    << targetPoint.y << ','
                    << targetPoint.z << '\n';
            }
        }

        if (step < steps)
        {
            Verlet::step(bodies, probeBodyIndex, totalForce, timeStep, gravitationalConstant);
        }
    }
}
