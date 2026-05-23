#ifndef SOLARSCAPE_SIMULATIONCONTEXT_H
#define SOLARSCAPE_SIMULATIONCONTEXT_H

#include <optional>
#include <vector>

#include "math/Real.h"
#include "simulation/Maneuver.h"

class SimulationContext
{
public:
    SimulationContext() = default;
    explicit SimulationContext(std::vector<Maneuver> maneuvers);

    const std::vector<Maneuver>& maneuvers() const;
    std::optional<Maneuver> activeManeuverAt(Real time) const;

private:
    std::vector<Maneuver> maneuvers_;
};

#endif
