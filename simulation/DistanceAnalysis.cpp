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
        const Body& targetBody,
        const Vector3& relativePoint) -> Vector3
    {
        return targetBody.position() + relativePoint;
    }

    auto minimumDistanceFromMovingPoint(
        std::vector<Body*>& bodies,
        Probe& probe,
        Body& targetBody,
        const Vector3& relativePoint,
        Real simulationTime,
        Real timeStep,
        Real gravitationalConstant,
        const std::vector<Maneuver>& maneuvers) -> Real
    {

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

        Real currentTime = 0.0L;
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

        Real minimumDistance =
            distance(
                probe.position(),
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

            while (maneuverIndex < sortedManeuvers.size() &&
                currentTime >= maneuverEndTime)
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
                maneuverStartTime <= currentTime &&
                currentTime < maneuverEndTime)
            {
                maneuver = sortedManeuvers[maneuverIndex];
            }

            Verlet::step(
                bodies,
                probe,
                maneuver,
                stepTime,
                gravitationalConstant);

            currentTime += stepTime;

            const Real currentDistance =
                distance(
                    probe.position(),
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
