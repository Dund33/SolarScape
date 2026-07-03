#include "RandomInitializer.h"

#include <algorithm>
#include <random>
#include <ranges>
#include <stdexcept>
#include <utility>

#include "config/consts.h"

RandomInitializer::RandomInitializer(
    std::size_t minManeuvers,
    std::size_t maxManeuvers,
    long double minInitTime,
    long double maxInitTime,
    long double minDuration,
    long double maxDuration,
    const ProbeProperties& probeProperties
)
    : minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitTime(minInitTime),
      maxInitTime(maxInitTime),
      minDuration(minDuration),
      maxDuration(maxDuration),
      probeProperties(probeProperties)
{
    if (minManeuvers > maxManeuvers)
    {
        throw std::invalid_argument("minManeuvers cannot be greater than maxManeuvers.");
    }

    if (minInitTime > maxInitTime)
    {
        throw std::invalid_argument("minInitTime cannot be greater than maxInitTime.");
    }

    if (minDuration > maxDuration)
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

    std::uniform_real_distribution<long double> initTimeDist(
        minInitTime,
        maxInitTime
    );

    std::uniform_real_distribution<long double> directionDist(
        -1.0L,
        1.0L
    );

    std::uniform_real_distribution<long double> throttleDist(
        0.0L,
        1.0L
    );

    const std::size_t maneuverCount = maneuverCountDist(rng);

    Specimen specimen;
    long double usedFuel = 0.0L;

    for (std::size_t i = 0; i < maneuverCount; ++i)
    {
        Vector3 direction(
            directionDist(rng),
            directionDist(rng),
            directionDist(rng)
        );

        const long double directionNorm = direction.norm();

        if (directionNorm <= 0.0L)
        {
            continue;
        }

        direction = direction / directionNorm;

        const long double throttleValue = throttleDist(rng);

        if (throttleValue <= 0.0L)
        {
            continue;
        }

        const long double remainingFuel =
            probeProperties.fuelMass() - usedFuel;

        if (remainingFuel <= 0.0L)
        {
            break;
        }

        const long double fuelUsageRate =
            throttleValue * probeProperties.fuelFlow();

        if (fuelUsageRate <= 0.0L)
        {
            continue;
        }

        const long double maxAllowedDuration =
            remainingFuel / fuelUsageRate;

        if (maxAllowedDuration < minDuration)
        {
            break;
        }

        std::uniform_real_distribution<long double> durationDist(
            minDuration,
            std::min(maxDuration, maxAllowedDuration)
        );

        const long double duration = durationDist(rng);
        const long double initDelay = initTimeDist(rng);

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
