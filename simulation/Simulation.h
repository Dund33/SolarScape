#ifndef SOLARSCAPE_SIMULATION_H
#define SOLARSCAPE_SIMULATION_H

#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "math/Real.h"
#include "simulation/Maneuver.h"

class Simulation
{
public:
    Simulation(
        std::vector<Body> bodies,
        Body targetBody,
        Probe probe,
        std::vector<Maneuver> maneuvers,
        Real gravitationalConstant);
    virtual ~Simulation() = default;

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    virtual void step(
        Real timeStep
    ) = 0;

    const std::vector<Body>& bodies() const;
    const Probe& probe() const;
    const Body& targetBody() const;
    Real time() const;

protected:
    std::vector<Body>& mutableBodies();
    Probe& mutableProbe();
    const std::vector<Maneuver>& maneuvers() const;
    Real gravitationalConstant() const;
    void advanceTime(Real timeStep);

private:
    std::vector<Body> bodies_;
    Probe probe_;
    Body* targetBody_{};
    std::vector<Maneuver> maneuvers_;
    Real gravitationalConstant_{};
    Real time_{};
};

#endif
