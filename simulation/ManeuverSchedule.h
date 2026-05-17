#ifndef SOLARSCAPE_MANEUVERSCHEDULE_H
#define SOLARSCAPE_MANEUVERSCHEDULE_H

#include <algorithm>
#include <optional>
#include <vector>

#include "simulation/Maneuver.h"

inline std::vector<Maneuver> sortManeuversByInitTime(
    const std::vector<Maneuver>& maneuvers)
{
    std::vector<Maneuver> sortedManeuvers = maneuvers;
    std::ranges::sort(
        sortedManeuvers,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    return sortedManeuvers;
}

inline std::optional<Maneuver> activeManeuverAt(
    const std::vector<Maneuver>& sortedManeuvers,
    Real time)
{
    auto maneuverIt = std::ranges::upper_bound(
        sortedManeuvers,
        time,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    if (maneuverIt == sortedManeuvers.begin())
    {
        return std::nullopt;
    }

    --maneuverIt;

    const Real maneuverStartTime =
        maneuverIt->getInitTime();
    const Real maneuverEndTime =
        maneuverStartTime + maneuverIt->getDuration();

    if (time < maneuverEndTime)
    {
        return *maneuverIt;
    }

    return std::nullopt;
}

#endif
