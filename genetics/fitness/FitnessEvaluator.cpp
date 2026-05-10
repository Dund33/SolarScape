#include "FitnessEvaluator.h"

#include <iostream>
#include <limits>
#include <ostream>

#include "simulation/DistanceAnalysis.h"

const double SPECIFIC_IMPULSE_PENALTY = 1000.0;

FitnessEvaluator::FitnessEvaluator(
    Real gravitationalConstant,
    Real timeStep,
    Real simulationTime,
    Vector3 targetPointFromTargetBody,
    std::size_t probeBodyIndex,
    std::size_t targetBodyIndex,
    std::vector<Body> initialBodies,
    long double maxImpulse
)
    : gravitationalConstant(gravitationalConstant),
      timeStep(timeStep),
      simulationTime(simulationTime),
      targetPointFromTargetBody(targetPointFromTargetBody),
      probeBodyIndex(probeBodyIndex),
      targetBodyIndex(targetBodyIndex),
      initialBodies(std::move(initialBodies)),
      maxImpulse(maxImpulse)
{
}

void FitnessEvaluator::evaluate(Specimen& specimen) const
{
    if (specimen.getFitness().has_value())
    {
        return;
    }
    const long double totalImpulse = specimen.getTotalImpulse();

    if (totalImpulse > maxImpulse)
    {
        const long double scale = maxImpulse / totalImpulse;

        for (std::size_t i = 0; i < specimen.size(); ++i)
        {
            Maneuver& maneuver = specimen[i];

            maneuver = Maneuver(
                maneuver.getThrust() * scale,
                maneuver.getInitTime(),
                maneuver.getDuration()
            );
        }
    }

    std::vector<Body> bodies = initialBodies;

    const Real minimumDistance =
        DistanceAnalysis::minimumDistanceFromMovingPoint(
            bodies,
            probeBodyIndex,
            targetBodyIndex,
            targetPointFromTargetBody,
            simulationTime,
            timeStep,
            gravitationalConstant,
            specimen.getManeuvers()
        );

    const Real specificImpulse = specimen.getTotalImpulse();

    const Real fitness =
        SPECIFIC_IMPULSE_PENALTY * specificImpulse + minimumDistance;

    specimen.setFitness(static_cast<double>(fitness));
}