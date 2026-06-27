#ifndef SOLARSCAPE_COMMANDLINEOPTIONS_H
#define SOLARSCAPE_COMMANDLINEOPTIONS_H

#include <iosfwd>
#include <stdexcept>
#include <string>

class CommandLineParseError final : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

class CommandLineOptions
{
public:
    static CommandLineOptions parse(
        int argc,
        char* argv[],
        std::string defaultConfigFile = "scenario1.yml",
        std::string defaultOutputFile = "pareto-front.json");

    static void printUsage(
        std::ostream& output,
        const char* programName,
        const std::string& defaultConfigFile = "scenario1.yml",
        const std::string& defaultOutputFile = "pareto-front.json");

    const std::string& configFilePath() const;

    const std::string& outputFilePath() const;

    bool helpRequested() const;

private:
    CommandLineOptions(
        std::string configFilePath,
        std::string outputFilePath,
        bool helpRequested);

    std::string configFilePath_;
    std::string outputFilePath_;
    bool helpRequested_{};
};

#endif
