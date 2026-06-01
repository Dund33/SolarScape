#ifndef SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H
#define SOLARSCAPE_SIMULATIONFITNESSEVALUATOR_H

#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"
#include "simulation/SimulationFactory.h"

class SimulationFitnessEvaluator final : public FitnessEvaluator
{
public:
    SimulationFitnessEvaluator(
        Real timeStep,
        Real simulationTime,
        Vector3 targetPointFromTargetBody,
        const SimulationFactory& simulationFactory
    );

    void evaluate(Specimen& specimen) const override;

private:
    FitnessValue calculateFitnessValue(
        std::vector<Maneuver> maneuvers) const;

    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const SimulationFactory& simulationFactory;
};

#endif
