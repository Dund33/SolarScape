#include "PlotTrajectory.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

#include "simulation/SimulationContext.h"

namespace
{
    Vector3 absolutePointForBody(
        const Body& targetBody,
        const Vector3& relativePoint)
    {
        return targetBody.position() + relativePoint;
    }
}

void plotTrajectory(
    const SimulationFactory& simulationFactory,
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    const std::vector<Maneuver>& maneuvers)
{
    auto simulation =
        simulationFactory.create(
            SimulationContext(maneuvers));

    const std::vector<Body>& simulationBodies =
        simulation->bodies();
    const Body& simulatedTargetBody =
        simulation->targetBody();
    const Probe& simulationProbe =
        simulation->probe();

    std::ofstream output("simulation.csv");
    if (!output)
    {
        std::cerr << "Nie mozna utworzyc pliku simulation.csv\n";
        return;
    }

    output << std::setprecision(std::numeric_limits<Real>::max_digits10);
    output << "step,time,body,x,y,z,vx,vy,vz,mass,target_x,target_y,target_z\n";

    for (std::size_t step = 0; step <= steps; ++step)
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = absolutePointForBody(
            simulatedTargetBody,
            targetPointFromTargetBody);

        if (step % 500 == 0)
        {
            for (std::size_t i = 0; i < simulationBodies.size(); ++i)
            {
                const Body& body = simulationBodies[i];

                output << step << ','
                    << time << ','
                    << i << ','
                    << body.position().x << ','
                    << body.position().y << ','
                    << body.position().z << ','
                    << body.velocity().x << ','
                    << body.velocity().y << ','
                    << body.velocity().z << ','
                    << body.mass() << ','
                    << targetPoint.x << ','
                    << targetPoint.y << ','
                    << targetPoint.z << '\n';
            }

            output << step << ','
                << time << ','
                << simulationBodies.size() << ','
                << simulationProbe.position().x << ','
                << simulationProbe.position().y << ','
                << simulationProbe.position().z << ','
                << simulationProbe.velocity().x << ','
                << simulationProbe.velocity().y << ','
                << simulationProbe.velocity().z << ','
                << simulationProbe.mass() << ','
                << targetPoint.x << ','
                << targetPoint.y << ','
                << targetPoint.z << '\n';
        }

        if (step < steps)
        {
            simulation->step(
                timeStep,
                gravitationalConstant);
        }
    }
}
