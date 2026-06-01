#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "config/SimulationConfig.h"
#include "math/Body.h"
#include "math/ProbeFactory.h"
#include "simulation/VerletFactory.h"
#include "validation/RecordingValidator.h"

namespace
{
    constexpr const char* DEFAULT_CONFIG_FILE = "validation-basic.yaml";

    struct ValidationState
    {
        std::string outputFilename;
        SimulationConfig simulationConfig;
    };

    auto loadOutputFilename(const YAML::Node& config) -> std::string
    {
        const YAML::Node outputFilename = config["validation"]["outputFilename"];
        if (!outputFilename)
        {
            throw std::runtime_error("Missing validation.outputFilename YAML node.");
        }
        return outputFilename.as<std::string>();
    }

    auto loadValidationState(const std::string& configFile) -> ValidationState
    {
        SimulationConfig simulationConfig =
            SimulationConfig::loadFromFile(
                configFile);

        if (simulationConfig.timeStep <= 0.0L)
        {
            throw std::invalid_argument("simulation.timeStep musi byc dodatnia liczba");
        }

        return {
            loadOutputFilename(
                YAML::LoadFile(
                    configFile)),
            std::move(simulationConfig)};
    }

    auto configFileFromArguments(int argc, char* argv[]) -> std::string
    {
        if (argc > 2)
        {
            throw std::invalid_argument("Uzycie: validation [plik_yaml]");
        }

        if (argc == 2)
        {
            return argv[1];
        }

        return DEFAULT_CONFIG_FILE;
    }

    auto run(const ValidationState& state) -> int
    {
        const SimulationConfig& config = state.simulationConfig;

        VerletFactory simulationFactory(
            config.gravitationalConstant,
            config.bodies,
            config.targetBody,
            ProbeFactory(
                config.probeProperties,
                config.probePosition,
                config.probeVelocity).create());

        const std::size_t steps =
            static_cast<std::size_t>(
                config.simulationTime / config.timeStep);

        RecordingValidator validator(
            simulationFactory,
            config.timeStep,
            steps);

        const std::vector<Status> recording =
            validator.record();

        std::ofstream output(state.outputFilename);
        if (!output)
        {
            std::cerr << "Nie mozna utworzyc pliku " << state.outputFilename << '\n';
            return 1;
        }

        output << std::setprecision(std::numeric_limits<Real>::max_digits10);
        output << "bodyId,time,x,y,z,vx,vy,vz\n";
        for (const Status& status : recording)
        {
            output
                << status.bodyId << ','
                << status.time << ','
                << status.position.x << ','
                << status.position.y << ','
                << status.position.z << ','
                << status.velocity.x << ','
                << status.velocity.y << ','
                << status.velocity.z << '\n';
        }

        return 0;
    }
}

auto main(int argc, char* argv[]) -> int
{
    try
    {
        return run(
            loadValidationState(
                configFileFromArguments(
                    argc,
                    argv)));
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
}
