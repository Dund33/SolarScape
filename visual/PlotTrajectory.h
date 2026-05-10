//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_PLOT_TRAJECTORY_H
#define SOLARSCAPE_PLOT_TRAJECTORY_H

#include <vector>

#include "genetics/Maneuver.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "math/Real.h"

void plotTrajectory(
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    Body* targetBody,
    Probe* probe,
    std::vector<Body*> bodies,
    const std::vector<Maneuver>& maneuvers);

#endif //SOLARSCAPE_PLOT_TRAJECTORY_H
