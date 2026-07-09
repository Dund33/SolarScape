#ifndef SOLARSCAPE_SIMULATIONFITNESSEVALUATORFACTORY_H
#define SOLARSCAPE_SIMULATIONFITNESSEVALUATORFACTORY_H

#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/SimulationFactory.h"

class SimulationFitnessEvaluatorFactory final : public FitnessEvaluatorFactory
{
public:
    SimulationFitnessEvaluatorFactory(Real timeStep, Real simulationTime, Vector3 targetPointFromTargetBody,
                                      const SimulationFactory& simulationFactory);

    std::unique_ptr<FitnessEvaluator> create() const override;

private:
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const SimulationFactory& simulationFactory;
};

#endif
