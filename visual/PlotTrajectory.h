#ifndef SOLARSCAPE_PLOT_TRAJECTORY_H
#define SOLARSCAPE_PLOT_TRAJECTORY_H

#include <cstddef>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/Simulation.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "math/Real.h"

void plotTrajectory(
    const Simulation& simulation,
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    const Body& targetBody,
    const Probe& probe,
    const std::vector<Body*>& bodies,
    const std::vector<Maneuver>& maneuvers);

#endif
