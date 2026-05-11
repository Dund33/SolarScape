#include "DistanceAnalysis.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>
#include <vector>

#include "math/Verlet.h"

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

            std::optional<Maneuver> maneuver;
            auto maneuverIt = std::ranges::upper_bound(
                sortedManeuvers,
                currentTime,
                std::less<>{},
                [](const Maneuver& candidate)
                {
                    return candidate.getInitTime();
                });

            if (maneuverIt != sortedManeuvers.begin())
            {
                --maneuverIt;

                const Real maneuverStartTime =
                    maneuverIt->getInitTime();
                const Real maneuverEndTime =
                    maneuverStartTime + maneuverIt->getDuration();

                if (currentTime < maneuverEndTime)
                {
                    maneuver = *maneuverIt;
                }
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
