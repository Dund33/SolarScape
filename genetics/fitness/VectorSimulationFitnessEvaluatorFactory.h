#ifndef SOLARSCAPE_VECTORSIMULATIONFITNESSEVALUATORFACTORY_H
#define SOLARSCAPE_VECTORSIMULATIONFITNESSEVALUATORFACTORY_H

#include <memory>

#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "math/Real.h"
#include "math/Vector3.h"
#include "simulation/VectorSimulationFactory.h"

class VectorSimulationFitnessEvaluatorFactory final : public FitnessEvaluatorFactory
{
public:
    VectorSimulationFitnessEvaluatorFactory(Real timeStep, Real simulationTime, Vector3 targetPointFromTargetBody,
                                            const VectorSimulationFactory& simulationFactory);

    std::unique_ptr<FitnessEvaluator> create() const override;

private:
    Real timeStep;
    Real simulationTime;
    Vector3 targetPointFromTargetBody;
    const VectorSimulationFactory& simulationFactory;
};

#endif
