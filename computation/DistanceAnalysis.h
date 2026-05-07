//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_DISTANCE_ANALYSIS_H
#define SOLARSCAPE_DISTANCE_ANALYSIS_H

#include <stdexcept>
#include <vector>

#include "../data/Body.h"
#include "Verlet.h"

namespace DistanceAnalysis
{
    inline double distance(const Vector3& left, const Vector3& right)
    {
        return (left - right).length();
    }

    inline Vector3 absolutePointForBody(const Body& targetBody, const Vector3& relativePoint)
    {
        return targetBody.position + relativePoint;
    }

    inline double minimumDistanceFromMovingPoint(
        std::vector<Body> bodies,
        std::size_t observedBodyIndex,
        std::size_t targetBodyIndex,
        const Vector3& relativePoint,
        double simulationTime,
        double timeStep,
        double gravitationalConstant)
    {
        if (observedBodyIndex >= bodies.size())
        {
            throw std::out_of_range("observedBodyIndex is outside bodies vector");
        }

        if (targetBodyIndex >= bodies.size())
        {
            throw std::out_of_range("targetBodyIndex is outside bodies vector");
        }

        if (simulationTime < 0.0)
        {
            throw std::invalid_argument("simulationTime must be non-negative");
        }

        if (timeStep <= 0.0)
        {
            throw std::invalid_argument("timeStep must be greater than zero");
        }

        double currentTime = 0.0;
        double minimumDistance = distance(
            bodies[observedBodyIndex].position,
            absolutePointForBody(bodies[targetBodyIndex], relativePoint));

        while (currentTime < simulationTime)
        {
            const double remainingTime = simulationTime - currentTime;
            const double stepTime = remainingTime < timeStep ? remainingTime : timeStep;

            Verlet::step(bodies, stepTime, gravitationalConstant);
            currentTime += stepTime;

            const double currentDistance = distance(
                bodies[observedBodyIndex].position,
                absolutePointForBody(bodies[targetBodyIndex], relativePoint));

            if (currentDistance < minimumDistance)
            {
                minimumDistance = currentDistance;
            }
        }

        return minimumDistance;
    }
}

#endif //SOLARSCAPE_DISTANCE_ANALYSIS_H
