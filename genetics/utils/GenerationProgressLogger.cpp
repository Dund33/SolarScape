#include "GenerationProgressLogger.h"

#include <iostream>

std::ostream& GenerationProgressLogger::defaultOutput()
{
    return std::cout;
}

void GenerationProgressLogger::print(std::string_view algorithmName, std::size_t generation, const ParetoFrontStats& paretoFrontStats,
                                     std::string_view details, std::ostream& output)
{
    output << algorithmName << " generation " << generation << " | pareto_front_size=" << paretoFrontStats.size;

    if (!details.empty())
    {
        output << " | " << details;
    }

    if (paretoFrontStats.size == 0)
    {
        output << '\n';
        return;
    }

    output << " | fuel_feasible=" << paretoFrontStats.fuelFeasibleCount << '/' << paretoFrontStats.size << " | distance=["
           << paretoFrontStats.minDistance << ", " << paretoFrontStats.maxDistance << "] | target_window_violation=["
           << paretoFrontStats.minTargetWindowViolation << ", " << paretoFrontStats.maxTargetWindowViolation << "] | time=["
           << paretoFrontStats.minTime << ", " << paretoFrontStats.maxTime << "] | fuel=[" << paretoFrontStats.minFuel << ", "
           << paretoFrontStats.maxFuel << "] | fuel_violation=[" << paretoFrontStats.minFuelViolation << ", "
           << paretoFrontStats.maxFuelViolation << "]\n";
}
