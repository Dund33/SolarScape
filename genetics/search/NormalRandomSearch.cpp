#include "NormalRandomSearch.h"

#include <algorithm>
#include <random>
#include <stdexcept>

#include "genetics/comparison/SpecimenComparator.h"
#include "math/Vector3.h"

NormalRandomSearch::NormalRandomSearch(
    std::size_t iterations,
    Real initTimeStdDev,
    Real durationStdDev,
    Real throttleStdDev,
    const ProbeProperties& probeProperties)
    : iterations(iterations),
      initTimeStdDev(initTimeStdDev),
      durationStdDev(durationStdDev),
      throttleStdDev(throttleStdDev),
      probeProperties(probeProperties)
{
    if (initTimeStdDev < 0.0L)
    {
        throw std::invalid_argument("initTimeStdDev must be non-negative.");
    }

    if (durationStdDev < 0.0L)
    {
        throw std::invalid_argument("durationStdDev must be non-negative.");
    }

    if (throttleStdDev < 0.0L)
    {
        throw std::invalid_argument("throttleStdDev must be non-negative.");
    }
}

void NormalRandomSearch::improve(
    Specimen& specimen,
    const FitnessEvaluator& fitnessEvaluator,
    const SpecimenComparator& specimenComparator) const
{
    if (specimen.empty() || iterations == 0)
    {
        return;
    }

    fitnessEvaluator.evaluate(specimen);

    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<std::size_t> maneuverDist(
        0,
        specimen.size() - 1);

    Specimen best = specimen;

    for (std::size_t i = 0; i < iterations; ++i)
    {
        Specimen candidate = best;
        Maneuver& maneuver = candidate[maneuverDist(rng)];
        maneuver = perturbManeuver(maneuver);
        candidate.clearFitness();

        if (
            candidate.getTotalFuelUse(probeProperties) >
            probeProperties.fuelMass())
        {
            continue;
        }

        fitnessEvaluator.evaluate(candidate);

        if (specimenComparator.isLess(candidate, best))
        {
            best = std::move(candidate);
        }
    }

    specimen = std::move(best);
}

Maneuver NormalRandomSearch::perturbManeuver(
    const Maneuver& maneuver) const
{
    thread_local std::mt19937 rng(std::random_device{}());

    std::normal_distribution<Real> initTimeDelta(
        0.0L,
        initTimeStdDev);
    std::normal_distribution<Real> durationDelta(
        0.0L,
        durationStdDev);
    std::normal_distribution<Real> throttleDelta(
        0.0L,
        throttleStdDev);

    Vector3 throttleVector =
        maneuver.getThrustDirection() *
        maneuver.getThrottleValue();

    throttleVector.x += throttleDelta(rng);
    throttleVector.y += throttleDelta(rng);
    throttleVector.z += throttleDelta(rng);

    const Real throttleNorm =
        std::clamp(
            throttleVector.norm(),
            0.0L,
            1.0L);

    const Real initDelay =
        std::max(0.0L, maneuver.getInitDelay() + initTimeDelta(rng));

    const Real duration =
        std::max(0.0L, maneuver.getDuration() + durationDelta(rng));

    if (throttleNorm <= 0.0L)
    {
        return Maneuver(Vector3{}, 0.0L, initDelay, duration);
    }

    return Maneuver(
        throttleVector / throttleVector.norm(),
        throttleNorm,
        initDelay,
        duration);
}
