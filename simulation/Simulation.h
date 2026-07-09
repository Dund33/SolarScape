#ifndef SOLARSCAPE_SIMULATION_H
#define SOLARSCAPE_SIMULATION_H

#include <cstddef>
#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"

class Simulation
{
public:
    Simulation(std::vector<Body> bodies, Body targetBody, Probe probe, std::vector<Maneuver> maneuvers, Real gravitationalConstant);
    virtual ~Simulation() = default;

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&&) = delete;
    Simulation& operator=(Simulation&&) = delete;

    virtual void step(Real timeStep) = 0;

    std::size_t bodyCount() const;
    Vector3 bodyPosition(std::size_t bodyIndex) const;
    Vector3 bodyVelocity(std::size_t bodyIndex) const;
    Real bodyMass(std::size_t bodyIndex) const;
    Vector3 probePosition() const;
    Vector3 probeVelocity() const;
    Real probeMass() const;
    Vector3 targetBodyPosition() const;
    Real requestedFuelUse() const;
    Real initialProbeFuelMass() const;

protected:
    std::vector<Body>& mutableBodies();
    Probe& mutableProbe();
    const std::vector<Maneuver>& maneuvers() const;
    Real gravitationalConstant() const;
    Real currentTime() const;
    void advanceTime(Real timeStep);

private:
    std::vector<Body> bodies_;
    Probe probe_;
    Body* targetBody_{};
    std::vector<Maneuver> maneuvers_;
    Real gravitationalConstant_{};
    Real time_{};
    Real initialProbeFuelMass_{};
};

#endif
