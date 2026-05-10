#include "RandomInitializer.h"

#include <algorithm>
#include <iterator>
#include <random>
#include <stdexcept>

RandomInitializer::RandomInitializer(
    std::size_t minManeuvers,
    std::size_t maxManeuvers,
    long double minInitTime,
    long double maxInitTime,
    long double minDuration,
    long double maxDuration,
    Probe* probe
)
    : minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitTime(minInitTime),
      maxInitTime(maxInitTime),
      minDuration(minDuration),
      maxDuration(maxDuration),
      probe(probe)
{
    if (probe == nullptr)
    {
        throw std::invalid_argument("probe must not be null.");
    }

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

    if (probe->fuelMass() < 0.0L)
    {
        throw std::invalid_argument("probe fuelMass cannot be negative.");
    }

    if (probe->fuelFlow() < 0.0L)
    {
        throw std::invalid_argument("probe fuelFlow cannot be negative.");
    }
}

Specimen RandomInitializer::create() const
{
    static thread_local std::mt19937 rng(std::random_device{}());

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

    Specimen specimen(probe);
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
            probe->fuelMass() - usedFuel;

        if (remainingFuel <= 0.0L)
        {
            break;
        }

        const long double fuelUsageRate =
            throttleValue * probe->fuelFlow();

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
        const long double initTime = initTimeDist(rng);

        specimen.addManeuver(
            Maneuver(direction, throttleValue, initTime, duration)
        );

        usedFuel += fuelUsageRate * duration;
    }

    return specimen;
}

std::vector<Specimen> RandomInitializer::createPopulation(
    std::size_t populationSize
) const
{
    std::vector<Specimen> population;
    population.reserve(populationSize);

    std::generate_n(
        std::back_inserter(population),
        populationSize,
        [this]
        {
            return create();
        });

    return population;
}