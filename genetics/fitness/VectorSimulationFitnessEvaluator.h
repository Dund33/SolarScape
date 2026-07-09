#ifndef SOLARSCAPE_VECTORSIMULATIONFITNESSEVALUATOR_H
#define SOLARSCAPE_VECTORSIMULATIONFITNESSEVALUATOR_H

#include <vector>

#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/Maneuver.h"
#include "simulation/VectorSimulationFactory.h"

class VectorSimulationFitnessEvaluator final : public FitnessEvaluator
{
public:
    VectorSimulationFitnessEvaluator(Real timeStep, Real simulationTime, Vector3 targetPointFromTargetBody,
                                     const VectorSimulationFactory& simulationFactory);

    void evaluate(Specimen& specimen) const override;
    void evaluateBatch(std::vector<Specimen*>& specimens) const override;

private:
    std::vector<FitnessValue> calculateFitnessValues(std::vector<std::vector<Maneuver>> maneuverBatch) const;

    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const VectorSimulationFactory& simulationFactory;
};

#endif
