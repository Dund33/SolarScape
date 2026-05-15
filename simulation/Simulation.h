#ifndef SOLARSCAPE_SIMULATION_H
#define SOLARSCAPE_SIMULATION_H

#include <optional>
#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/Maneuver.h"

class Simulation
{
public:
    virtual ~Simulation() = default;

    virtual void step(
        std::vector<Body*>& bodies,
        Probe& probe,
        const std::optional<Maneuver>& maneuver,
        Real timeStep,
        Real gravitationalConstant
    ) const = 0;
};

#endif
