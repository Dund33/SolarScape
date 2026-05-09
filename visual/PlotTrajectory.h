//
// Created by Luke on 5/9/2026.
//

#ifndef SOLARSCAPE_PLOT_TRAJECTORY_H
#define SOLARSCAPE_PLOT_TRAJECTORY_H
#include "external/indicators/indicators.hpp"
#include "math/Body.h"
#include "math/Real.h"
#include "math/Verlet.h"
#include "simulation/DistanceAnalysis.h"
void plotTrajectory(const Real gravitationalConstant,
    const Real timeStep,
    const size_t steps,
    const Vector3 targetPointFromTargetBody,
    const size_t targetBodyIndex,
    const size_t probeBodyIndex,
    std::vector<Body>& bodies,
    std::vector<Maneuver>& maneuvers);
#endif //SOLARSCAPE_PLOT_TRAJECTORY_H
