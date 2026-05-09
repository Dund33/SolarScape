//
// Created by Luke on 5/7/2026.
//

#include <iomanip>
#include <iostream>
#include <vector>

#include "config/SimulationConfig.h"
#include "math/Body.h"
#include "simulation/DistanceAnalysis.h"
#include "external/indicators/indicators.hpp"
#include "visual/PlotTrajectory.h"

auto main() -> int
{
    SimulationConfig config;

    try
    {
        config =
            SimulationConfig::loadFromFile("config.yaml");
    }
    catch (const YAML::Exception& e)
    {
        std::cerr << "YAML error: " << e.what() << '\n';
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    const Real gravitationalConstant =
        config.gravitationalConstant;

    const Real timeStep =
        config.timeStep;

    const Real simulationTime =
        config.simulationTime;

    const Vector3 targetPointFromTargetBody =
        config.targetPointFromTargetBody;

    const size_t probeBodyIndex =
        config.probeBodyIndex;

    const size_t targetBodyIndex =
        config.targetBodyIndex;

    std::vector<Body> bodies =
        std::move(config.bodies);

    auto maneuvers = std::vector{Maneuver(Vector3(10000, 0, 0), 1000, 2000)};

    const Real minimumDistance = DistanceAnalysis::minimumDistanceFromMovingPoint(
        bodies,
        probeBodyIndex,
        targetBodyIndex,
        targetPointFromTargetBody,
        simulationTime,
        timeStep,
        gravitationalConstant,
        maneuvers);

    std::cout<<minimumDistance;

    plotTrajectory(
        gravitationalConstant,
        timeStep,
        static_cast<size_t>(simulationTime / timeStep),
        targetPointFromTargetBody,
        targetBodyIndex,
        probeBodyIndex,
        bodies,
        maneuvers
        );

    return 0;
}
