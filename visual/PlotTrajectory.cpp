//
// Created by Luke on 5/9/2026.
//

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <limits>
#include <ranges>
#include <stdexcept>
#include "PlotTrajectory.h"

#include "math/Verlet.h"
#include "simulation/DistanceAnalysis.h"


void plotTrajectory(
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    Body* targetBody,
    Probe* probe,
    std::vector<Body*> bodies,
    const std::vector<Maneuver>& maneuvers)
{
    if (probe == nullptr)
    {
        throw std::invalid_argument("probe must not be null");
    }

    if (targetBody == nullptr)
    {
        throw std::invalid_argument("target body must not be null");
    }

    if (std::ranges::any_of(
        bodies,
        [](const Body* body)
        {
            return body == nullptr;
        }))
    {
        throw std::invalid_argument("body pointer must not be null");
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

    for (const std::size_t step : std::views::iota(std::size_t{0}, steps + 1))
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = DistanceAnalysis::absolutePointForBody(
            targetBody,
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
            for (const std::size_t i : std::views::iota(std::size_t{0}, bodies.size()))
            {
                output << step << ','
                    << time << ','
                    << i << ','
                    << bodies[i]->position().x << ','
                    << bodies[i]->position().y << ','
                    << bodies[i]->position().z << ','
                    << bodies[i]->velocity().x << ','
                    << bodies[i]->velocity().y << ','
                    << bodies[i]->velocity().z << ','
                    << bodies[i]->mass() << ','
                    << targetPoint.x << ','
                    << targetPoint.y << ','
                    << targetPoint.z << '\n';
            }
        }

        if (step < steps)
        {
            Verlet::step(
                bodies,
                probe,
                maneuver,
                timeStep,
                gravitationalConstant);
        }
    }
}
