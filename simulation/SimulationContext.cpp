#include "SimulationContext.h"

#include <algorithm>
#include <ranges>
#include <utility>

SimulationContext::SimulationContext(std::vector<Maneuver> maneuvers)
    : maneuvers_(std::move(maneuvers))
{
    std::ranges::sort(
        maneuvers_,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });
}

const std::vector<Maneuver>& SimulationContext::maneuvers() const
{
    return maneuvers_;
}

std::optional<Maneuver> SimulationContext::activeManeuverAt(Real time) const
{
    auto maneuverIt = std::ranges::upper_bound(
        maneuvers_,
        time,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    if (maneuverIt == maneuvers_.begin())
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
