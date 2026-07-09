#include "SimulationFitnessEvaluatorFactory.h"

#include <memory>

#include "genetics/fitness/SimulationFitnessEvaluator.h"

SimulationFitnessEvaluatorFactory::SimulationFitnessEvaluatorFactory(Real timeStepValue, Real simulationTimeValue,
                                                                     Vector3 targetPointFromTargetBodyValue,
                                                                     const SimulationFactory& simulationFactoryRef)
    : timeStep(timeStepValue), simulationTime(simulationTimeValue), targetPointFromTargetBody(targetPointFromTargetBodyValue),
      simulationFactory(simulationFactoryRef)
{
}

std::unique_ptr<FitnessEvaluator> SimulationFitnessEvaluatorFactory::create() const
{
    return std::make_unique<SimulationFitnessEvaluator>(timeStep, simulationTime, targetPointFromTargetBody, simulationFactory);
}
