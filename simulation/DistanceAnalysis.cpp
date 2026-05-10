#include "DistanceAnalysis.h"

#include <numeric>
#include <stdexcept>
#include <vector>

#include "../math/Verlet.h"
#include "config/consts.h"

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
        return targetBody.position() + relativePoint;
    }

    auto minimumDistanceFromMovingPoint(
        std::vector<Body> bodies,
        std::size_t probeBodyIndex,
        std::size_t targetBodyIndex,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant,
        const std::vector<Maneuver>& maneuvers) -> Real
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

        struct ScheduledManeuver
        {
            Real startTime;
            Real endTime;
            Vector3 thrust;
        };

        std::vector<ScheduledManeuver> scheduledManeuvers;
        scheduledManeuvers.reserve(maneuvers.size());

        Real previousEndTime = 0.0L;

        for (const auto& maneuver : maneuvers)
        {
            const Real startTime = previousEndTime + maneuver.getInitTime();
            const Real endTime = startTime + maneuver.getDuration();

            scheduledManeuvers.push_back(
                ScheduledManeuver{
                    startTime,
                    endTime,
                    maneuver.getThrust()
                });

            previousEndTime = endTime;
        }

        Real currentTime = 0.0L;

        Real minimumDistance =
            distance(
                bodies[probeBodyIndex].position(),
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

            Vector3 totalForce;

            for (const auto& scheduledManeuver : scheduledManeuvers)
            {
                if (scheduledManeuver.startTime <= currentTime &&
                    currentTime < scheduledManeuver.endTime)
                {
                    totalForce += scheduledManeuver.thrust * MAX_THRUST;
                }
            }

            Verlet::step(
                bodies,
                probeBodyIndex,
                totalForce,
                stepTime,
                gravitationalConstant);

            currentTime += stepTime;

            const Real currentDistance =
                distance(
                    bodies[probeBodyIndex].position(),
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
