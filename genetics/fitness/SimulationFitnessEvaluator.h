#ifndef SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H
#define SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H

#include <vector>

#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/fitness/FitnessResult.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"
#include "simulation/SimulationFactory.h"

class SimulationFitnessEvaluator final : public FitnessEvaluator
{
public:
    SimulationFitnessEvaluator(
        Real gravitationalConstant,
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        const SimulationFactory& simulationFactory
    );

    void evaluate(Specimen& specimen) const override;

private:
    FitnessResult calculateFitnessResult(
        const std::vector<Maneuver>& maneuvers) const;

    Real gravitationalConstant;
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const SimulationFactory& simulationFactory;
};

#endif
