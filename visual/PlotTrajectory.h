#ifndef SOLARSCAPE_PLOT_TRAJECTORY_H
#define SOLARSCAPE_PLOT_TRAJECTORY_H

#include <cstddef>
#include <vector>

#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"
#include "simulation/SimulationFactory.h"

void plotTrajectory(const SimulationFactory& simulationFactory, Real timeStep, std::size_t steps, const Vector3& targetPointFromTargetBody,
                    const std::vector<Maneuver>& maneuvers);

#endif
