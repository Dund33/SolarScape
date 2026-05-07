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
    inline Real distance(const Vector3& left, const Vector3& right)
    {
        return (left - right).length();
    }

    inline Vector3 absolutePointForBody(const Body& targetBody, const Vector3& relativePoint)
    {
        return targetBody.position + relativePoint;
    }

    inline Real minimumDistanceFromMovingPoint(
        std::vector<Body> bodies,
        std::size_t observedBodyIndex,
        std::size_t targetBodyIndex,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant)
    {
        if (observedBodyIndex >= bodies.size())
        {
            throw std::out_of_range("observedBodyIndex is outside bodies vector");
        }

        if (targetBodyIndex >= bodies.size())
        {
            throw std::out_of_range("targetBodyIndex is outside bodies vector");
        }

        if (simulationTime < 0.0L)
        {
            throw std::invalid_argument("simulationTime must be non-negative");
        }

        if (timeStep <= 0.0L)
        {
            throw std::invalid_argument("timeStep must be greater than zero");
        }

        Real currentTime = 0.0L;
        Real minimumDistance = distance(
            bodies[observedBodyIndex].position,
            absolutePointForBody(bodies[targetBodyIndex], relativePoint));

        while (currentTime < simulationTime)
        {
            const Real remainingTime = simulationTime - currentTime;
            const Real stepTime = remainingTime < timeStep ? remainingTime : timeStep;

            Verlet::step(bodies, stepTime, gravitationalConstant);
            currentTime += stepTime;

            const Real currentDistance = distance(
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
