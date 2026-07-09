#include "PlotTrajectory.h"

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace
{
    Vector3 absolutePointForPosition(const Vector3& targetBodyPosition, const Vector3& relativePoint)
    {
        return targetBodyPosition + relativePoint;
    }
} // namespace

void plotTrajectory(const SimulationFactory& simulationFactory, Real timeStep, std::size_t steps, const Vector3& targetPointFromTargetBody,
                    const std::vector<Maneuver>& maneuvers)
{
    auto simulation = simulationFactory.create(maneuvers);

    const std::size_t bodyCount = simulation->bodyCount();

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
        const Real time = static_cast<Real>(step) * timeStep;
        const Vector3 targetPoint = absolutePointForPosition(simulation->targetBodyPosition(), targetPointFromTargetBody);

        if (step % 500 == 0)
        {
            for (std::size_t bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex)
            {
                const Vector3 bodyPosition = simulation->bodyPosition(bodyIndex);
                const Vector3 bodyVelocity = simulation->bodyVelocity(bodyIndex);

                output << step << ',' << time << ',' << bodyIndex << ',' << bodyPosition.x << ',' << bodyPosition.y << ',' << bodyPosition.z
                       << ',' << bodyVelocity.x << ',' << bodyVelocity.y << ',' << bodyVelocity.z << ',' << simulation->bodyMass(bodyIndex)
                       << ',' << targetPoint.x << ',' << targetPoint.y << ',' << targetPoint.z << '\n';
            }

            const Vector3 probePosition = simulation->probePosition();
            const Vector3 probeVelocity = simulation->probeVelocity();

            output << step << ',' << time << ',' << bodyCount << ',' << probePosition.x << ',' << probePosition.y << ',' << probePosition.z
                   << ',' << probeVelocity.x << ',' << probeVelocity.y << ',' << probeVelocity.z << ',' << simulation->probeMass() << ','
                   << targetPoint.x << ',' << targetPoint.y << ',' << targetPoint.z << '\n';
        }

        if (step < steps)
        {
            simulation->step(timeStep);
        }
    }
}
