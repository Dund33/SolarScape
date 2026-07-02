#ifndef SOLARSCAPE_GENERATIONPROGRESSLOGGER_H
#define SOLARSCAPE_GENERATIONPROGRESSLOGGER_H

#include <cstddef>
#include <iosfwd>
#include <string_view>

#include "genetics/utils/ParetoFrontUtils.h"

class GenerationProgressLogger
{
public:
    static void print(
        std::string_view algorithmName,
        std::size_t generation,
        const ParetoFrontStats& paretoFrontStats,
        std::string_view details = {},
        std::ostream& output = defaultOutput());

private:
    static std::ostream& defaultOutput();

    GenerationProgressLogger() = delete;
};

#endif
