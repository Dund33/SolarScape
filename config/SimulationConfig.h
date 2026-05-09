#ifndef SOLARSCAPE_SIMULATIONCONFIG_H
#define SOLARSCAPE_SIMULATIONCONFIG_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include "../math/Body.h"

class SimulationConfig
{
public:
    Real gravitationalConstant{};
    Real timeStep{};
    Real simulationTime{};

    std::size_t centralBodyIndex{};
    std::size_t targetBodyIndex{};
    std::size_t probeBodyIndex{};

    Vector3 targetPointFromTargetBody;

    std::vector<Body> bodies;

    static SimulationConfig loadFromFile(
        const std::string& filePath);

private:
    static Vector3 loadVector3(const class YAML::Node& node);

    static Body loadBody(const class YAML::Node& node);
};

#endif // SOLARSCAPE_SIMULATIONCONFIG_H