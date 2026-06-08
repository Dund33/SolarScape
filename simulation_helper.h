#ifndef SOLARSCAPE_SIMULATION_HELPER_H
#define SOLARSCAPE_SIMULATION_HELPER_H

#include <iostream>
#include <utility>
#include <vector>

#include "config/SimulationConfig.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Body.h"
#include "math/ProbeProperties.h"

struct SimulationState
{
    Real gravitationalConstant{};
    Real timeStep{};
    Real simulationTime{};

    Vector3 targetPointFromTargetBody;

    std::vector<Body> initialBodies;
    Body targetBody;
    Vector3 probePosition;
    Vector3 probeVelocity;
    ProbeProperties probeProperties;
};

inline auto createSimulationState(
    SimulationConfig&& config) -> SimulationState
{
    return {
        config.gravitationalConstant,
        config.timeStep,
        config.simulationTime,
        config.targetPointFromTargetBody,
        std::move(config.bodies),
        std::move(config.targetBody),
        config.probePosition,
        config.probeVelocity,
        config.probeProperties};
}

inline void printFitnessValue(
    const FitnessValue& fitness)
{
    std::cout
        << "[minimumDistance=" << fitness.minimumDistance
        << ", minimumDistanceTime=" << fitness.minimumDistanceTime
        << ", minimumDistanceFuelMass=" << fitness.minimumDistanceFuelMass
        << ", fuelConstraintViolation=" << fitness.fuelConstraintViolation
        << ']';
}

#endif
