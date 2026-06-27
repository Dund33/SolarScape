#include "CommandLineOptions.h"

#include <ostream>
#include <string>
#include <string_view>
#include <utility>

namespace
{
    auto isOption(const std::string& argument) -> bool
    {
        return !argument.empty() && argument[0] == '-';
    }

    auto programDisplayName(const char* programName) -> const char*
    {
        return programName != nullptr && programName[0] != '\0'
            ? programName
            : "simulation";
    }
}

CommandLineOptions::CommandLineOptions(
    std::string configFilePath,
    bool helpRequested)
    : configFilePath_(std::move(configFilePath)),
      helpRequested_(helpRequested)
{
}

auto CommandLineOptions::parse(
    int argc,
    char* argv[],
    std::string defaultConfigFile) -> CommandLineOptions
{
    std::string configFilePath = defaultConfigFile;
    bool configFileSet = false;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i] != nullptr
            ? argv[i]
            : "";

        if (argument == "-h" || argument == "--help")
        {
            return {std::move(configFilePath), true};
        }

        if (argument == "-c" || argument == "--config")
        {
            if (i + 1 >= argc)
            {
                throw CommandLineParseError(
                    "Missing value after " + argument + ".");
            }

            if (configFileSet)
            {
                throw CommandLineParseError(
                    "Configuration file specified more than once.");
            }

            const char* configArgument = argv[++i];
            if (configArgument == nullptr || configArgument[0] == '\0')
            {
                throw CommandLineParseError(
                    "Missing value after " + argument + ".");
            }

            configFilePath = configArgument;
            configFileSet = true;
            continue;
        }

        constexpr std::string_view configPrefix = "--config=";
        if (argument.starts_with(configPrefix))
        {
            if (configFileSet)
            {
                throw CommandLineParseError(
                    "Configuration file specified more than once.");
            }

            configFilePath = argument.substr(configPrefix.size());
            if (configFilePath.empty())
            {
                throw CommandLineParseError(
                    "Missing value after --config=.");
            }

            configFileSet = true;
            continue;
        }

        if (isOption(argument))
        {
            throw CommandLineParseError(
                "Unknown option: " + argument + ".");
        }

        if (configFileSet)
        {
            throw CommandLineParseError(
                "Configuration file specified more than once.");
        }

        if (argument.empty())
        {
            throw CommandLineParseError(
                "Configuration file path cannot be empty.");
        }

        configFilePath = argument;
        configFileSet = true;
    }

    return {std::move(configFilePath), false};
}

void CommandLineOptions::printUsage(
    std::ostream& output,
    const char* programName,
    const std::string& defaultConfigFile)
{
    const char* displayName = programDisplayName(programName);

    output
        << "Usage: " << displayName << " [config-file]\n"
        << "       " << displayName << " --config <file>\n"
        << '\n'
        << "Options:\n"
        << "  -c, --config <file>  YAML configuration file"
        << " (default: " << defaultConfigFile << ")\n"
        << "  -h, --help           Show this help message\n";
}

const std::string& CommandLineOptions::configFilePath() const
{
    return configFilePath_;
}

bool CommandLineOptions::helpRequested() const
{
    return helpRequested_;
}
