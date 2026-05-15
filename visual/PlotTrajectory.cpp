#include "PlotTrajectory.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
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
    const Simulation& simulation,
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    const Body& targetBody,
    const Probe& probe,
    const std::vector<Body*>& bodies,
    const std::vector<Maneuver>& maneuvers)
{
    if (std::ranges::any_of(
        bodies,
        [](const Body* body)
        {
            return body == nullptr;
        }))
    {
        throw std::invalid_argument("body pointer must not be null");
    }

    const Body* targetBodyPointer = &targetBody;
    const Body* probePointer = static_cast<const Body*>(&probe);

    if (std::ranges::find(bodies, targetBodyPointer) == bodies.end())
    {
        throw std::invalid_argument("target body is not available in bodies");
    }

    if (std::ranges::find(bodies, probePointer) == bodies.end())
    {
        throw std::invalid_argument("probe is not available in bodies");
    }

    std::vector<Body> bodyCopies;
    bodyCopies.reserve(bodies.size());

    Body targetBodyCopy = targetBody;
    Probe probeCopy = probe;

    std::vector<Body*> simulationBodies;
    simulationBodies.reserve(bodies.size());

    for (const Body* body : bodies)
    {
        if (body == targetBodyPointer)
        {
            simulationBodies.push_back(&targetBodyCopy);
        }
        else if (body == probePointer)
        {
            simulationBodies.push_back(&probeCopy);
        }
        else
        {
            bodyCopies.push_back(*body);
            simulationBodies.push_back(&bodyCopies.back());
        }
    }

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
            targetBodyCopy,
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
            for (std::size_t i = 0; i < simulationBodies.size(); ++i)
            {
                output << step << ','
                    << time << ','
                    << i << ','
                    << simulationBodies[i]->position().x << ','
                    << simulationBodies[i]->position().y << ','
                    << simulationBodies[i]->position().z << ','
                    << simulationBodies[i]->velocity().x << ','
                    << simulationBodies[i]->velocity().y << ','
                    << simulationBodies[i]->velocity().z << ','
                    << simulationBodies[i]->mass() << ','
                    << targetPoint.x << ','
                    << targetPoint.y << ','
                    << targetPoint.z << '\n';
            }
        }

        if (step < steps)
        {
            simulation.step(
                simulationBodies,
                probeCopy,
                maneuver,
                timeStep,
                gravitationalConstant);
        }
    }
}
