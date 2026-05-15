#include "SimulationFitnessEvaluatorFactory.h"

#include <memory>

#include "genetics/fitness/SimulationFitnessEvaluator.h"

SimulationFitnessEvaluatorFactory::SimulationFitnessEvaluatorFactory(
    Real gravitationalConstant,
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const std::vector<Body>& initialBodies,
    const Probe& probe,
    const Body& targetBody,
    const Simulation& simulation
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      initialBodies(initialBodies),
      probe(probe),
      targetBody(targetBody),
      simulation(simulation)
{
}

std::unique_ptr<FitnessEvaluator> SimulationFitnessEvaluatorFactory::create() const
{
    return std::make_unique<SimulationFitnessEvaluator>(
        gravitationalConstant,
        timeStep,
        simulationTime,
        targetPointFromTargetBody,
        initialBodies,
        probe,
        targetBody,
        simulation);
}
