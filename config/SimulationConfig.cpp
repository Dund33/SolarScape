#include "SimulationConfig.h"

#include <yaml-cpp/yaml.h>

SimulationConfig SimulationConfig::loadFromFile(
    const std::string& filePath)
{
    YAML::Node config = YAML::LoadFile(filePath);

    SimulationConfig result;

    result.gravitationalConstant =
        config["simulation"]["gravitationalConstant"]
            .as<Real>();

    result.timeStep =
        config["simulation"]["timeStep"]
            .as<Real>();

    result.simulationTime =
        config["simulation"]["simulationTime"]
            .as<Real>();

    result.centralBodyIndex =
        config["indices"]["centralBodyIndex"]
            .as<std::size_t>();

    result.secondBodyIndex =
        config["indices"]["secondBodyIndex"]
            .as<std::size_t>();

    result.probeBodyIndex =
        config["indices"]["probeBodyIndex"]
            .as<std::size_t>();

    result.targetPointFromCentralBody =
        loadVector3(
            config["targetPointFromCentralBody"]);

    for (const YAML::Node& bodyNode : config["bodies"])
    {
        result.bodies.push_back(
            loadBody(bodyNode));
    }

    const Vector3 probeRelativePosition =
        loadVector3(
            config["probe"]["relativePosition"]);

    const Vector3 probeRelativeVelocity =
        loadVector3(
            config["probe"]["relativeVelocity"]);

    const Real probeMass =
        config["probe"]["mass"]
            .as<Real>();

    const Vector3 probeStartPosition =
        result.bodies[result.secondBodyIndex].position +
        probeRelativePosition;

    const Vector3 probeStartVelocity =
        result.bodies[result.secondBodyIndex].velocity +
        probeRelativeVelocity;

    result.bodies.push_back(
        Body(
            probeStartPosition,
            probeStartVelocity,
            probeMass));

    return result;
}

Vector3 SimulationConfig::loadVector3(
    const YAML::Node& node)
{
    return Vector3(
        node["x"].as<Real>(),
        node["y"].as<Real>(),
        node["z"].as<Real>());
}

Body SimulationConfig::loadBody(
    const YAML::Node& node)
{
    return Body(
        loadVector3(node["position"]),
        loadVector3(node["velocity"]),
        node["mass"].as<Real>());
}