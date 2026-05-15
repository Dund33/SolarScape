#ifndef SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H
#define SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H

#include <vector>

#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/fitness/FitnessResult.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"

class SimulationFitnessEvaluator final : public FitnessEvaluator
{
public:
    SimulationFitnessEvaluator(
        Real gravitationalConstant,
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        const std::vector<Body>& initialBodies,
        const Probe& probe,
        const Body& targetBody
    );

    void evaluate(Specimen& specimen) const override;

private:
    FitnessResult calculateFitnessResult(
        const std::vector<Maneuver>& maneuvers) const;

    Real gravitationalConstant;
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const std::vector<Body>& initialBodies;
    const Probe& probe;
    const Body& targetBody;
};

#endif
