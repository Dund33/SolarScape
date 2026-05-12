#ifndef SOLARSCAPE_FITNESSEVALUATOR_H
#define SOLARSCAPE_FITNESSEVALUATOR_H

#include <vector>

#include "math/Body.h"
#include "math/Probe.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"
#include "genetics/fitness/FitnessResult.h"
#include "genetics/Specimen.h"

class FitnessEvaluator
{
public:
    FitnessEvaluator(
        Real gravitationalConstant,
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        const std::vector<Body>& initialBodies,
        const Probe& probe,
        const Body& targetBody
    );

    void evaluate(Specimen& specimen) const;

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

#endif // SOLARSCAPE_FITNESSEVALUATOR_H
