#include "SimulationFitnessEvaluator.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <stdexcept>

#include "simulation/Verlet.h"

namespace
{
    Real distance(
        const Vector3& left,
        const Vector3& right)
    {
        return (left - right).length();
    }

    Vector3 absolutePointForBody(
        const Body& targetBody,
        const Vector3& relativePoint)
    {
        return targetBody.position() + relativePoint;
    }
}

SimulationFitnessEvaluator::SimulationFitnessEvaluator(
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

void SimulationFitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }

    if (&targetBody == static_cast<const Body*>(&probe))
    {
        throw std::invalid_argument("target body cannot point to the probe");
    }

    const FitnessResult fitnessResult =
        calculateFitnessResult(specimen.getManeuvers());

    specimen.setFitness(fitnessResult);
}

FitnessResult SimulationFitnessEvaluator::calculateFitnessResult(
    const std::vector<Maneuver>& maneuvers) const
{
    if (simulationTime < 0.0L)
    {
        throw std::invalid_argument("simulationTime must be non-negative");
    }

    if (timeStep <= 0.0L)
    {
        throw std::invalid_argument("timeStep must be greater than zero");
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

    Real currentTime = 0.0L;
    std::vector<Maneuver> sortedManeuvers = maneuvers;
    std::ranges::sort(
        sortedManeuvers,
        {},
        [](const Maneuver& maneuver)
        {
            return maneuver.getInitTime();
        });

    Real minimumDistance =
        distance(
            probeCopy.position(),
            absolutePointForBody(
                targetBodyCopy,
                targetPointFromTargetBody));

    Real minimumDistanceTime = currentTime;
    Real minimumDistanceFuelMass = probeCopy.fuelMass();

    while (currentTime < simulationTime)
    {
        const Real remainingTime =
            simulationTime - currentTime;

        const Real stepTime =
            remainingTime < timeStep
                ? remainingTime
                : timeStep;

        std::optional<Maneuver> maneuver;
        auto maneuverIt = std::ranges::upper_bound(
            sortedManeuvers,
            currentTime,
            std::less<>{},
            [](const Maneuver& candidate)
            {
                return candidate.getInitTime();
            });

        if (maneuverIt != sortedManeuvers.begin())
        {
            --maneuverIt;

            const Real maneuverStartTime =
                maneuverIt->getInitTime();
            const Real maneuverEndTime =
                maneuverStartTime + maneuverIt->getDuration();

            if (currentTime < maneuverEndTime)
            {
                maneuver = *maneuverIt;
            }
        }

        Verlet::step(
            bodyPointers,
            probeCopy,
            maneuver,
            stepTime,
            gravitationalConstant);

        currentTime += stepTime;

        const Real currentDistance =
            distance(
                probeCopy.position(),
                absolutePointForBody(
                    targetBodyCopy,
                    targetPointFromTargetBody));

        if (currentDistance < minimumDistance)
        {
            minimumDistance = currentDistance;
            minimumDistanceTime = currentTime;
            minimumDistanceFuelMass = probeCopy.fuelMass();
        }
    }

    return FitnessResult(
        minimumDistance,
        minimumDistanceTime,
        minimumDistanceFuelMass);
}
