#include "genetics/fitness/VectorSimulationFitnessEvaluatorFactory.h"

#include <memory>

#include "genetics/fitness/VectorSimulationFitnessEvaluator.h"

VectorSimulationFitnessEvaluatorFactory::VectorSimulationFitnessEvaluatorFactory(
    Real timeStepValue,
    Real simulationTimeValue,
    Vector3 targetPointFromTargetBodyValue,
    const VectorSimulationFactory& simulationFactoryRef)
    : timeStep(timeStepValue),
      simulationTime(simulationTimeValue),
      targetPointFromTargetBody(targetPointFromTargetBodyValue),
      simulationFactory(simulationFactoryRef)
{
}

std::unique_ptr<FitnessEvaluator>
VectorSimulationFitnessEvaluatorFactory::create() const
{
    return std::make_unique<VectorSimulationFitnessEvaluator>(
        timeStep,
        simulationTime,
        targetPointFromTargetBody,
        simulationFactory);
}
