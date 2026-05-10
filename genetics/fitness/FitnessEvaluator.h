#ifndef SOLARSCAPE_FITNESSEVALUATOR_H
#define SOLARSCAPE_FITNESSEVALUATOR_H

#include <vector>

#include "math/Body.h"
#include "math/Vector3.h"
#include "genetics/Specimen.h"

class FitnessEvaluator
{
public:
    FitnessEvaluator(
        Real gravitationalConstant,
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        std::size_t probeBodyIndex,
        std::size_t targetBodyIndex,
        std::vector<Body> initialBodies,
        long double maxImpulse
    );

    void evaluate(Specimen& specimen) const;

private:
    Real gravitationalConstant;
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    std::size_t probeBodyIndex;
    std::size_t targetBodyIndex;
    std::vector<Body> initialBodies;
    long double maxImpulse;
};

#endif // SOLARSCAPE_FITNESSEVALUATOR_H