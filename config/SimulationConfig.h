#ifndef SOLARSCAPE_SIMULATIONCONFIG_H
#define SOLARSCAPE_SIMULATIONCONFIG_H

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "math/Body.h"
#include "math/Probe.h"

class SimulationConfig
{
public:
    Real gravitationalConstant{};
    Real timeStep{};
    Real simulationTime{};

    Vector3 targetPointFromTargetBody;

    std::vector<Body> bodies;
    Body targetBody;
    Probe probe;

    static SimulationConfig loadFromFile(
        const std::string& filePath);

private:
    static Vector3 loadVector3(const YAML::Node& node);

    static Body loadBody(const YAML::Node& node);

    static Probe loadProbe(const YAML::Node& node);
};

#endif
