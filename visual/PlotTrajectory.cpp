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

    Real previousManeuverEndTime = 0.0L;
    Real maneuverStartTime = 0.0L;
    Real maneuverEndTime = 0.0L;
    std::size_t maneuverIndex = 0;

    if (!maneuvers.empty())
    {
        maneuverStartTime = maneuvers[0].getInitTime();
        maneuverEndTime =
            maneuverStartTime + maneuvers[0].getDuration();
    }

    for (const std::size_t step : std::views::iota(std::size_t{0}, steps + 1))
    {
        const Real time = step * timeStep;
        const Vector3 targetPoint = DistanceAnalysis::absolutePointForBody(
            targetBody,
            targetPointFromTargetBody);

        Real thrustValue = 0.0L;
        Vector3 thrustDirection;

        while (maneuverIndex < maneuvers.size() &&
            time >= maneuverEndTime)
        {
            previousManeuverEndTime = maneuverEndTime;
            ++maneuverIndex;

            if (maneuverIndex < maneuvers.size())
            {
                maneuverStartTime =
                    previousManeuverEndTime +
                    maneuvers[maneuverIndex].getInitTime();
                maneuverEndTime =
                    maneuverStartTime +
                    maneuvers[maneuverIndex].getDuration();
            }
        }

        if (maneuverIndex < maneuvers.size() &&
            maneuverStartTime <= time &&
            time < maneuverEndTime)
        {
            const Maneuver& maneuver = maneuvers[maneuverIndex];

            thrustValue = maneuver.getThrottleValue();
            thrustDirection = maneuver.getThrustDirection();
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
                thrustValue,
                thrustDirection,
                timeStep,
                gravitationalConstant);
        }
    }
}
