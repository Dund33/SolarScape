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
    std::vector<Body*> initialBodies,
    Probe* probe,
    Body* targetBody
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      initialBodies(std::move(initialBodies)),
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

    if (probe == nullptr)
    {
        throw std::invalid_argument("probe must not be null");
    }

    if (targetBody == nullptr)
    {
        throw std::invalid_argument("target body must not be null");
    }

    if (targetBody == probe)
    {
        throw std::invalid_argument("target body cannot point to the probe");
    }

    Probe probeCopy = *probe;

    if (std::ranges::any_of(
        initialBodies,
        [](const Body* body)
        {
            return body == nullptr;
        }))
    {
        throw std::invalid_argument("body pointer must not be null");
    }

    std::vector<Body> bodyCopies;
    bodyCopies.reserve(initialBodies.size());
    std::vector<Body*> bodyPointers;
    bodyPointers.reserve(initialBodies.size());
    Body* targetBodyCopy = nullptr;

    for (Body* body : initialBodies)
    {
        if (body == probe)
        {
            bodyPointers.push_back(&probeCopy);
            continue;
        }

        bodyCopies.push_back(*body);
        Body* bodyCopy = &bodyCopies.back();

        if (body == targetBody)
        {
            targetBodyCopy = bodyCopy;
        }

        bodyPointers.push_back(bodyCopy);
    }

    if (targetBodyCopy == nullptr)
    {
        throw std::invalid_argument("target body is not available in initialBodies");
    }

    const Real minimumDistance =
        DistanceAnalysis::minimumDistanceFromMovingPoint(
            bodyPointers,
            &probeCopy,
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
