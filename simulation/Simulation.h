#ifndef SOLARSCAPE_SIMULATION_H
#define SOLARSCAPE_SIMULATION_H

#include <optional>
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
        Probe probe);
    virtual ~Simulation() = default;

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    virtual void step(
        const std::optional<Maneuver>& maneuver,
        Real timeStep,
        Real gravitationalConstant
    ) = 0;

    const std::vector<Body>& bodies() const;
    const Probe& probe() const;
    const Body& targetBody() const;

protected:
    std::vector<Body>& mutableBodies();
    Probe& mutableProbe();

private:
    std::vector<Body> bodies_;
    Probe probe_;
    Body* targetBody_{};
};

#endif
