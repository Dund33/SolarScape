//
// Created by Luke on 5/7/2026.
//

#ifndef SOLARSCAPE_VERLET_H
#define SOLARSCAPE_VERLET_H

#include <cstddef>
#include <optional>
#include <vector>

#include "genetics/Maneuver.h"
#include "math/Body.h"
#include "math/Probe.h"

namespace Verlet
{
    Vector3 calculateAccelerationForBody(
        const std::vector<Body*>& bodies,
        std::size_t bodyIndex,
        Real gravitationalConstant);

    std::vector<Vector3> calculateAccelerations(
        const std::vector<Body*>& bodies,
        Real gravitationalConstant);

    void step(
        std::vector<Body*>& bodies,
        Probe& probe,
        const std::optional<Maneuver>& maneuver,
        Real timeStep,
        Real gravitationalConstant);
}

#endif // SOLARSCAPE_VERLET_H
