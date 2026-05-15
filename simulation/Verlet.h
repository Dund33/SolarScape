#ifndef SOLARSCAPE_VERLET_H
#define SOLARSCAPE_VERLET_H

#include <cstddef>
#include <vector>

#include "simulation/Simulation.h"

class Verlet final : public Simulation
{
public:
    void step(
        std::vector<Body*>& bodies,
        Probe& probe,
        const std::optional<Maneuver>& maneuver,
        Real timeStep,
        Real gravitationalConstant
    ) const override;

private:
    static Vector3 calculateAccelerationForBody(
        const std::vector<Body*>& bodies,
        std::size_t bodyIndex,
        Real gravitationalConstant);

    static std::vector<Vector3> calculateAccelerations(
        const std::vector<Body*>& bodies,
        Real gravitationalConstant);
};

#endif
