#include "SimulationFitnessEvaluatorFactory.h"

#include <memory>

#include "genetics/fitness/SimulationFitnessEvaluator.h"

SimulationFitnessEvaluatorFactory::SimulationFitnessEvaluatorFactory(
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const SimulationFactory& simulationFactory
)
    : timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      simulationFactory(simulationFactory)
{
}

std::unique_ptr<FitnessEvaluator> SimulationFitnessEvaluatorFactory::create() const
{
    return std::make_unique<SimulationFitnessEvaluator>(
        timeStep,
        simulationTime,
        targetPointFromTargetBody,
        simulationFactory);
}
