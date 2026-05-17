#ifndef SOLARSCAPE_PLOT_TRAJECTORY_H
#define SOLARSCAPE_PLOT_TRAJECTORY_H

#include <cstddef>
#include <vector>

#include "simulation/Maneuver.h"
#include "simulation/SimulationFactory.h"
#include "math/Real.h"

void plotTrajectory(
    const SimulationFactory& simulationFactory,
    Real gravitationalConstant,
    Real timeStep,
    std::size_t steps,
    const Vector3& targetPointFromTargetBody,
    const std::vector<Maneuver>& maneuvers);

#endif
