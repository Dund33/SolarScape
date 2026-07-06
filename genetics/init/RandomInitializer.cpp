#include "RandomInitializer.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "config/consts.h"

RandomInitializer::RandomInitializer(
    std::size_t minManeuverCount,
    std::size_t maxManeuverCount,
    Real minInitTimeValue,
    Real maxInitTimeValue,
    Real minDurationValue,
    Real maxDurationValue,
    const ProbeProperties& probePropertiesValue
)
    : minManeuvers(minManeuverCount),
      maxManeuvers(maxManeuverCount),
      minInitTime(minInitTimeValue),
      maxInitTime(maxInitTimeValue),
      minDuration(minDurationValue),
      maxDuration(maxDurationValue),
      probeProperties(probePropertiesValue)
{
    if (minManeuverCount > maxManeuverCount)
    {
        throw std::invalid_argument("minManeuvers cannot be greater than maxManeuvers.");
    }

    if (minInitTimeValue > maxInitTimeValue)
    {
        throw std::invalid_argument("minInitTime cannot be greater than maxInitTime.");
    }

    if (minDurationValue > maxDurationValue)
    {
        throw std::invalid_argument("minDuration cannot be greater than maxDuration.");
    }

}

Specimen RandomInitializer::createCandidate(
    std::mt19937& rng) const
{
    std::uniform_int_distribution<std::size_t> maneuverCountDist(
        minManeuvers,
        maxManeuvers
    );

    std::uniform_real_distribution<Real> initTimeDist(
        minInitTime,
        maxInitTime
    );

    std::uniform_real_distribution<Real> directionDist(
        -1.0,
        1.0
    );

    std::uniform_real_distribution<Real> throttleDist(
        0.0,
        1.0
    );

    const std::size_t maneuverCount = maneuverCountDist(rng);

    Specimen specimen;
    Real usedFuel = 0.0;

    for (std::size_t i = 0; i < maneuverCount; ++i)
    {
        Vector3 direction(
            directionDist(rng),
            directionDist(rng),
            directionDist(rng)
        );

        const Real directionNorm = direction.norm();

        if (directionNorm <= 0.0)
        {
            continue;
        }

        direction = direction / directionNorm;

        const Real throttleValue = throttleDist(rng);

        if (throttleValue <= 0.0)
        {
            continue;
        }

        const Real remainingFuel =
            probeProperties.fuelMass() - usedFuel;

        if (remainingFuel <= 0.0)
        {
            break;
        }

        const Real fuelUsageRate =
            throttleValue * probeProperties.fuelFlow();

        if (fuelUsageRate <= 0.0)
        {
            continue;
        }

        const Real maxAllowedDuration =
            remainingFuel / fuelUsageRate;

        if (maxAllowedDuration < minDuration)
        {
            break;
        }

        std::uniform_real_distribution<Real> durationDist(
            minDuration,
            std::min(maxDuration, maxAllowedDuration)
        );

        const Real duration = durationDist(rng);
        const Real initDelay = initTimeDist(rng);

        specimen.addManeuver(
            Maneuver(direction, throttleValue, initDelay, duration)
        );

        usedFuel += fuelUsageRate * duration;
    }

    return specimen;
}

Specimen RandomInitializer::create() const
{
    static thread_local std::mt19937 rng(std::random_device{}());

    Specimen bestSpecimen;

    for (std::size_t attempt = 0;
         attempt < RANDOM_INITIALIZER_MIN_MANEUVERS_RETRY_COUNT;
         ++attempt)
    {
        Specimen specimen = createCandidate(rng);

        if (specimen.size() >= minManeuvers)
        {
            return specimen;
        }

        if (specimen.size() > bestSpecimen.size())
        {
            bestSpecimen = std::move(specimen);
        }
    }

    return bestSpecimen;
}

std::vector<Specimen> RandomInitializer::createPopulation(
    std::size_t populationSize
) const
{
    std::vector<Specimen> population;
    population.reserve(populationSize);

    const auto createdSpecimens =
        std::views::iota(std::size_t{0}, populationSize) |
        std::views::transform(
            [this](std::size_t)
            {
                return create();
            });

    for (Specimen specimen : createdSpecimens)
    {
        population.push_back(std::move(specimen));
    }

    return population;
}
