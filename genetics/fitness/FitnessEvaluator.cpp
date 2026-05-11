#include "FitnessEvaluator.h"

#include <algorithm>
#include <stdexcept>

#include "simulation/DistanceAnalysis.h"

const double FUEL_USE_PENALTY = 1000.0;

FitnessEvaluator::FitnessEvaluator(
    Real gravitationalConstant,
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    const std::vector<Body>& initialBodies,
    const Probe& probe,
    const Body& targetBody
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      initialBodies(initialBodies),
      probe(probe),
      targetBody(targetBody)
{
}

void FitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    if (&targetBody == static_cast<const Body*>(&probe))
    {
        throw std::invalid_argument("target body cannot point to the probe");
    }

    std::vector<Body> bodyCopies = initialBodies;
    Body targetBodyCopy = targetBody;
    Probe probeCopy = probe;

    std::vector<Body*> bodyPointers;
    bodyPointers.reserve(bodyCopies.size() + 2);

    for (Body& bodyCopy : bodyCopies)
    {
        bodyPointers.push_back(&bodyCopy);
    }

    bodyPointers.push_back(&targetBodyCopy);
    bodyPointers.push_back(&probeCopy);

    const Real minimumDistance =
        DistanceAnalysis::minimumDistanceFromMovingPoint(
            bodyPointers,
            probeCopy,
            targetBodyCopy,
            targetPointFromTargetBody,
            simulationTime,
            timeStep,
            gravitationalConstant,
            specimen.getManeuvers()
        );

    const Real totalFuelUse = specimen.getTotalFuelUse();

    const Real fitness =
        minimumDistance + FUEL_USE_PENALTY * totalFuelUse;

    specimen.setFitness(static_cast<double>(fitness));
}
