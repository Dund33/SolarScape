#ifndef SOLARSCAPE_SIMULATIONCONTEXT_H
#define SOLARSCAPE_SIMULATIONCONTEXT_H

#include <vector>

#include "simulation/Maneuver.h"

struct SimulationContext
{
    std::vector<Maneuver> maneuvers;
};

#endif
