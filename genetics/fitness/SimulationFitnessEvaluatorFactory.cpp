#include "SimulationFitnessEvaluatorFactory.h"

#include <memory>

#include "genetics/fitness/SimulationFitnessEvaluator.h"

SimulationFitnessEvaluatorFactory::SimulationFitnessEvaluatorFactory(
    Real gravitationalConstant,
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const SimulationFactory& simulationFactory
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      simulationFactory(simulationFactory)
{
}

std::unique_ptr<FitnessEvaluator> SimulationFitnessEvaluatorFactory::create() const
{
    return std::make_unique<SimulationFitnessEvaluator>(
        gravitationalConstant,
        timeStep,
        simulationTime,
        targetPointFromTargetBody,
        simulationFactory);
}
