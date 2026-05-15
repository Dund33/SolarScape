#ifndef SOLARSCAPE_SIMULATIONFITNESSEVALUATORFACTORY_H
#define SOLARSCAPE_SIMULATIONFITNESSEVALUATORFACTORY_H

#include <vector>

#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "math/Vector3.h"
#include "simulation/Simulation.h"

class SimulationFitnessEvaluatorFactory final : public FitnessEvaluatorFactory
{
public:
    SimulationFitnessEvaluatorFactory(
        Real gravitationalConstant,
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        const std::vector<Body>& initialBodies,
        const Probe& probe,
        const Body& targetBody,
        const Simulation& simulation
    );

    std::unique_ptr<FitnessEvaluator> create() const override;

private:
    Real gravitationalConstant;
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const std::vector<Body>& initialBodies;
    const Probe& probe;
    const Body& targetBody;
    const Simulation& simulation;
};

#endif
