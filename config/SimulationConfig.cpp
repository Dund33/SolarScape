#include "SimulationConfig.h"

#include <stdexcept>

#include <yaml-cpp/yaml.h>

namespace
{
    auto readReal(const YAML::Node& node) -> Real
    {
        if (!node)
        {
            throw std::runtime_error("Missing YAML node.");
        }

        return node.as<Real>();
    }
}

auto SimulationConfig::loadFromFile(
    const std::string& filePath) -> SimulationConfig
{
    YAML::Node config = YAML::LoadFile(filePath);

    SimulationConfig result;

    const YAML::Node simulation = config["simulation"];

    result.gravitationalConstant =
        readReal(
            simulation["gravitationalConstant"]);

    result.timeStep =
        readReal(
            simulation["timeStep"]);

    result.simulationTime =
        readReal(
            simulation["simulationTime"]);

    result.targetPointFromTargetBody =
        loadVector3(
            config["targetPointFromTargetBody"]);

    const YAML::Node bodiesNode = config["bodies"];

    if (!bodiesNode || !bodiesNode.IsSequence())
    {
        throw std::runtime_error(
            "'bodies' must be a YAML sequence.");
    }

    for (const YAML::Node& bodyNode : bodiesNode)
    {
        result.bodies.push_back(
            loadBody(bodyNode));
    }

    result.targetBody =
        loadBody(
            config["targetBody"]);

    const YAML::Node probe =
        config["probe"];

    result.probePosition =
        loadVector3(
            probe["position"]);

    result.probeVelocity =
        loadVector3(
            probe["velocity"]);

    result.probeProperties =
        loadProbeProperties(
            probe);

    return result;
}

auto SimulationConfig::loadVector3(
    const YAML::Node& node) -> Vector3
{
    if (!node)
    {
        throw std::runtime_error(
            "Missing Vector3 node.");
    }

    return {
        readReal(node["x"]),
        readReal(node["y"]),
        readReal(node["z"])};
}

auto SimulationConfig::loadBody(
    const YAML::Node& node) -> Body
{
    if (!node)
    {
        throw std::runtime_error(
            "Missing Body node.");
    }

    return Body(
        loadVector3(
            node["position"]),
        loadVector3(
            node["velocity"]),
        readReal(
            node["mass"]));
}

auto SimulationConfig::loadProbeProperties(
    const YAML::Node& node) -> ProbeProperties
{
    if (!node)
    {
        throw std::runtime_error(
            "Missing ProbeProperties node.");
    }

    return ProbeProperties(
        readReal(
            node["emptyMass"]),
        readReal(
            node["fuelMass"]),
        readReal(
            node["fuelFlow"]),
        readReal(
            node["specificImpulse"]));
}
