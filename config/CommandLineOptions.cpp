#include "CommandLineOptions.h"

#include <boost/program_options.hpp>

#include <ostream>
#include <string>
#include <utility>

namespace
{
    namespace po = boost::program_options;

    auto createOptionsDescription(const std::string& defaultConfigFile, const std::string& defaultOutputFile) -> po::options_description
    {
        po::options_description options("Options");

        options.add_options()("help,h", "Show this help message")("config,c", po::value<std::string>()->default_value(defaultConfigFile),
                                                                  "YAML configuration file")(
            "output,o", po::value<std::string>()->default_value(defaultOutputFile), "Pareto front JSON output file")(
            "diversity-log", po::value<std::string>(), "Optional ALGO population diversity diagnostics log file")(
            "verbose,v", "Print generation progress while the algorithm is running");

        return options;
    }

    auto programDisplayName(const char* programName) -> const char*
    {
        return programName != nullptr && programName[0] != '\0' ? programName : "simulation";
    }
} // namespace

CommandLineOptions::CommandLineOptions(std::string configFilePath, std::string outputFilePath, std::string diversityLogFilePath,
                                       bool verbose, bool helpRequested)
    : configFilePath_(std::move(configFilePath)), outputFilePath_(std::move(outputFilePath)),
      diversityLogFilePath_(std::move(diversityLogFilePath)), verbose_(verbose), helpRequested_(helpRequested)
{
}

auto CommandLineOptions::parse(int argc, char* argv[], std::string defaultConfigFile, std::string defaultOutputFile) -> CommandLineOptions
{
    const po::options_description options = createOptionsDescription(defaultConfigFile, defaultOutputFile);

    po::positional_options_description positionalOptions;
    positionalOptions.add("config", 1);

    po::variables_map variables;

    try
    {
        po::store(po::command_line_parser(argc, argv).options(options).positional(positionalOptions).run(), variables);
        po::notify(variables);
    }
    catch (const po::error& e)
    {
        throw CommandLineParseError(e.what());
    }

    std::string configFilePath = variables["config"].as<std::string>();
    std::string outputFilePath = variables["output"].as<std::string>();
    std::string diversityLogFilePath;

    if (variables.count("diversity-log") > 0)
    {
        diversityLogFilePath = variables["diversity-log"].as<std::string>();
    }

    if (configFilePath.empty())
    {
        throw CommandLineParseError("Configuration file path cannot be empty.");
    }

    if (outputFilePath.empty())
    {
        throw CommandLineParseError("Output file path cannot be empty.");
    }

    if (variables.count("diversity-log") > 0 && diversityLogFilePath.empty())
    {
        throw CommandLineParseError("Diversity log file path cannot be empty.");
    }

    return {std::move(configFilePath), std::move(outputFilePath), std::move(diversityLogFilePath), variables.count("verbose") > 0,
            variables.count("help") > 0};
}

void CommandLineOptions::printUsage(std::ostream& output, const char* programName, const std::string& defaultConfigFile,
                                    const std::string& defaultOutputFile)
{
    const char* displayName = programDisplayName(programName);
    const po::options_description options = createOptionsDescription(defaultConfigFile, defaultOutputFile);

    output << "Usage: " << displayName << " [config-file] [--output <file>]\n"
           << "       " << displayName << " --config <file> --output <file> [--diversity-log <file>]\n"
           << '\n'
           << options;
}

const std::string& CommandLineOptions::configFilePath() const
{
    return configFilePath_;
}

const std::string& CommandLineOptions::outputFilePath() const
{
    return outputFilePath_;
}

const std::string& CommandLineOptions::diversityLogFilePath() const
{
    return diversityLogFilePath_;
}

bool CommandLineOptions::hasDiversityLogFilePath() const
{
    return !diversityLogFilePath_.empty();
}

bool CommandLineOptions::verbose() const
{
    return verbose_;
}

bool CommandLineOptions::helpRequested() const
{
    return helpRequested_;
}
