#include "PlotTrajectory.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

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
        simulationFactory.create();

    std::ofstream output("simulation.csv");
    if (!output)
    {
        std::cerr << "Nie mozna utworzyc pliku simulation.csv\n";
        return;
    }

    output << std::setprecision(std::numeric_limits<Real>::max_digits10);
    output << "step,time,body,x,y,z,vx,vy,vz,mass,target_x,target_y,target_z\n";

    std::vector<Maneuver> sortedManeuvers = maneuvers;
    std::ranges::sort(
        sortedManeuvers,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    Real maneuverStartTime = 0.0L;
    Real maneuverEndTime = 0.0L;
    std::size_t maneuverIndex = 0;

    if (!sortedManeuvers.empty())
    {
        maneuverStartTime = sortedManeuvers[0].getInitTime();
        maneuverEndTime =
            maneuverStartTime + sortedManeuvers[0].getDuration();
    }

    for (std::size_t step = 0; step <= steps; ++step)
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = absolutePointForBody(
            simulation->targetBody(),
            targetPointFromTargetBody);

        while (maneuverIndex < sortedManeuvers.size() &&
            time >= maneuverEndTime)
        {
            ++maneuverIndex;

            if (maneuverIndex < sortedManeuvers.size())
            {
                maneuverStartTime =
                    sortedManeuvers[maneuverIndex].getInitTime();
                maneuverEndTime =
                    maneuverStartTime +
                    sortedManeuvers[maneuverIndex].getDuration();
            }
        }

        std::optional<Maneuver> maneuver;
        if (maneuverIndex < sortedManeuvers.size() &&
            maneuverStartTime <= time &&
            time < maneuverEndTime)
        {
            maneuver = sortedManeuvers[maneuverIndex];
        }

        if (step % 500 == 0)
        {
            for (std::size_t i = 0; i < simulation->bodies().size(); ++i)
            {
                const Body& body = simulation->bodies()[i];

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

            const Probe& simulationProbe = simulation->probe();
            output << step << ','
                << time << ','
                << simulation->bodies().size() << ','
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
                maneuver,
                timeStep,
                gravitationalConstant);
        }
    }
}
