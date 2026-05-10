#include "DistanceAnalysis.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

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
        const Body* targetBody,
        const Vector3& relativePoint) -> Vector3
    {
        if (targetBody == nullptr)
        {
            throw std::invalid_argument(
                "target body must not be null");
        }

        return targetBody->position() + relativePoint;
    }

    auto minimumDistanceFromMovingPoint(
        std::vector<Body*>& bodies,
        Probe* probe,
        Body* targetBody,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant,
        const std::vector<Maneuver>& maneuvers) -> Real
    {
        if (probe == nullptr)
        {
            throw std::invalid_argument(
                "probe must not be null");
        }

        if (targetBody == nullptr)
        {
            throw std::invalid_argument(
                "target body must not be null");
        }

        if (targetBody == probe)
        {
            throw std::invalid_argument(
                "target body must not be the probe");
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

        if (std::ranges::any_of(
            bodies,
            [](const Body* body)
            {
                return body == nullptr;
            }))
        {
            throw std::invalid_argument(
                "body pointer must not be null");
        }

        if (std::ranges::find(bodies, static_cast<Body*>(probe)) == bodies.end())
        {
            throw std::invalid_argument(
                "probe is not available in bodies");
        }

        if (std::ranges::find(bodies, targetBody) == bodies.end())
        {
            throw std::invalid_argument(
                "target body is not available in bodies");
        }

        Real currentTime = 0.0L;
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

        Real minimumDistance =
            distance(
                probe->position(),
                absolutePointForBody(
                    targetBody,
                    relativePoint));

        while (currentTime < simulationTime)
        {
            const Real remainingTime =
                simulationTime - currentTime;

            const Real stepTime =
                remainingTime < timeStep
                    ? remainingTime
                    : timeStep;

            while (maneuverIndex < maneuvers.size() &&
                currentTime >= maneuverEndTime)
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

            Vector3 forceDirection{0.0L, 0.0L, 0.0L};
            Real throttle = 0.0L;

            if (maneuverIndex < maneuvers.size() &&
                maneuverStartTime <= currentTime &&
                currentTime < maneuverEndTime)
            {
                const Maneuver& maneuver = maneuvers[maneuverIndex];

                forceDirection = maneuver.getThrustDirection();
                throttle = maneuver.getThrottleValue();
            }

            Verlet::step(
                bodies,
                probe,
                throttle,
                forceDirection,
                stepTime,
                gravitationalConstant);

            currentTime += stepTime;

            const Real currentDistance =
                distance(
                    probe->position(),
                    absolutePointForBody(
                        targetBody,
                        relativePoint));

            if (currentDistance < minimumDistance)
            {
                minimumDistance = currentDistance;
            }
        }

        return minimumDistance;
    }
}
