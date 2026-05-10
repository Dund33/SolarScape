//
// Created by Luke on 5/7/2026.
//

#include "DistanceAnalysis.h"

#include <numeric>
#include <stdexcept>

#include "../math/Verlet.h"

namespace DistanceAnalysis
{
    auto distance(
        const Vector3& left,
        const Vector3& right) -> Real
    {
        return (left - right).length();
    }

    auto absolutePointForBody(
        const Body& targetBody,
        const Vector3& relativePoint) -> Vector3
    {
        return targetBody.position + relativePoint;
    }

    auto minimumDistanceFromMovingPoint(
        std::vector<Body> bodies,
        std::size_t probeBodyIndex,
        std::size_t targetBodyIndex,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant,
        std::vector<Maneuver> const& maneuvers) -> Real
    {
        if (probeBodyIndex >= bodies.size())
        {
            throw std::out_of_range(
                "observedBodyIndex is outside bodies vector");
        }

        if (targetBodyIndex >= bodies.size())
        {
            throw std::out_of_range(
                "targetBodyIndex is outside bodies vector");
        }

        if (simulationTime < 0.0L)
        {
            throw std::invalid_argument(
                "simulationTime must be non-negative");
        }

        if (timeStep <= 0.0L)
        {
            throw std::invalid_argument(
                "timeStep must be greater than zero");
        }

        Real currentTime = 0.0L;

        Real minimumDistance =
            distance(
                bodies[probeBodyIndex].position,
                absolutePointForBody(
                    bodies[targetBodyIndex],
                    relativePoint));

        while (currentTime < simulationTime)
        {
            const Real remainingTime =
                simulationTime - currentTime;

            const Real stepTime =
                remainingTime < timeStep
                    ? remainingTime
                    : timeStep;

            auto executedManeuvers =
                maneuvers
                | std::views::filter([currentTime](const Maneuver& maneuver)
                {
                    return maneuver.getInitTime() < currentTime &&
                           maneuver.getInitTime() + maneuver.getDuration() > currentTime;
                });

            auto appliedForces = executedManeuvers | std::views::transform(&Maneuver::getThrust);

            const auto totalForce = std::accumulate(appliedForces.begin(), appliedForces.end(), Vector3{});

            Verlet::step(
                bodies,
                probeBodyIndex,
                totalForce,
                stepTime,
                gravitationalConstant);

            currentTime += stepTime;

            const Real currentDistance =
                distance(
                    bodies[probeBodyIndex].position,
                    absolutePointForBody(
                        bodies[targetBodyIndex],
                        relativePoint));

            if (currentDistance < minimumDistance)
            {
                minimumDistance = currentDistance;
            }
        }

        return minimumDistance;
    }
}
