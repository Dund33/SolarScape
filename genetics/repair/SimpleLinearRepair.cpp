#include "SimpleLinearRepair.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

#include "genetics/Specimen.h"

namespace
{
    long double nextStartAfter(
        long double time)
    {
        return std::nextafter(
            time,
            std::numeric_limits<long double>::infinity());
    }
}

void SimpleLinearRepair::repair(
    Specimen& specimen) const
{
    if (specimen.size() < 2)
    {
        return;
    }

    auto maneuvers = specimen.getManeuvers();

    std::ranges::stable_sort(
        maneuvers,
        [](const Maneuver& lhs, const Maneuver& rhs)
        {
            return lhs.getInitTime() < rhs.getInitTime();
        });

    long double previousEnd =
        maneuvers.front().getInitTime() +
        maneuvers.front().getDuration();

    for (std::size_t i = 1; i < maneuvers.size(); ++i)
    {
        long double initTime = maneuvers[i].getInitTime();

        if (initTime <= previousEnd)
        {
            initTime = nextStartAfter(previousEnd);

            maneuvers[i] = Maneuver(
                maneuvers[i].getThrustDirection(),
                maneuvers[i].getThrottleValue(),
                initTime,
                maneuvers[i].getDuration());
        }

        previousEnd =
            initTime +
            maneuvers[i].getDuration();
    }

    for (std::size_t i = 0; i < maneuvers.size(); ++i)
    {
        specimen[i] = maneuvers[i];
    }

    specimen.clearFitness();
}
