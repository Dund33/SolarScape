#ifndef SOLARSCAPE_VERLET_H
#define SOLARSCAPE_VERLET_H

#include <cstddef>
#include <vector>

#include "simulation/Simulation.h"

class Verlet final : public Simulation
{
public:
    Verlet(
        std::vector<Body> bodies,
        Body targetBody,
        Probe probe,
        std::vector<Maneuver> maneuvers,
        Real gravitationalConstant);

    void step(
        Real timeStep
    ) override;

private:
    static Vector3 calculateAccelerationForBody(
        const std::vector<Body*>& bodies,
        std::size_t bodyIndex,
        Real gravitationalConstant);

    static void calculateAccelerations(
        const std::vector<Body*>& bodies,
        Real gravitationalConstant,
        std::vector<Vector3>& accelerations);

    std::vector<Body*> bodyPointers_;
    std::vector<Vector3> previousAccelerations_;
    std::vector<Vector3> nextAccelerations_;
};

#endif
