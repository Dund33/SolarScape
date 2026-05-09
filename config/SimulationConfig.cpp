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

        return static_cast<Real>(node.as<double>());
    }

    auto readSizeT(const YAML::Node& node) -> std::size_t
    {
        if (!node)
        {
            throw std::runtime_error("Missing YAML node.");
        }

        return node.as<std::size_t>();
    }
}

auto SimulationConfig::loadFromFile(
    const std::string& filePath) -> SimulationConfig
{
    YAML::Node config = YAML::LoadFile(filePath);

    SimulationConfig result;

    // ---------------- Simulation ----------------
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

    // ---------------- Indices ----------------
    const YAML::Node indices = config["indices"];

    result.centralBodyIndex =
        readSizeT(
            indices["centralBodyIndex"]);

    result.targetBodyIndex =
        readSizeT(
            indices["targetBodyIndex"]);

    result.probeBodyIndex =
        readSizeT(
            indices["probeBodyIndex"]);

    // ---------------- Target point ----------------
    result.targetPointFromTargetBody =
        loadVector3(
            config["targetPointFromCentralBody"]);

    // ---------------- Bodies ----------------
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

    // ---------------- Probe ----------------
    const YAML::Node probeNode = config["probe"];

    const Vector3 probeRelativePosition =
        loadVector3(
            probeNode["relativePosition"]);

    const Vector3 probeRelativeVelocity =
        loadVector3(
            probeNode["relativeVelocity"]);

    const Real probeMass =
        readReal(
            probeNode["mass"]);

    if (result.targetBodyIndex >= result.bodies.size())
    {
        throw std::runtime_error(
            "secondBodyIndex is out of range.");
    }

    const Body& secondBody =
        result.bodies[result.targetBodyIndex];

    const Vector3 probeStartPosition =
        secondBody.position +
        probeRelativePosition;

    const Vector3 probeStartVelocity =
        secondBody.velocity +
        probeRelativeVelocity;

    result.bodies.emplace_back(
            probeStartPosition,
            probeStartVelocity,
            probeMass);

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