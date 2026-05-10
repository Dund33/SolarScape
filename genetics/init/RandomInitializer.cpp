#include "RandomInitializer.h"

#include <random>
#include <stdexcept>

RandomInitializer::RandomInitializer(
    std::size_t minManeuvers,
    std::size_t maxManeuvers,
    long double minInitTime,
    long double maxInitTime,
    long double minDuration,
    long double maxDuration,
    long double minThrust,
    long double maxThrust
)
    : minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitTime(minInitTime),
      maxInitTime(maxInitTime),
      minDuration(minDuration),
      maxDuration(maxDuration),
      minThrust(minThrust),
      maxThrust(maxThrust)
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

    if (minThrust > maxThrust)
    {
        throw std::invalid_argument("minThrust cannot be greater than maxThrust.");
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

    std::uniform_real_distribution<long double> thrustDist(
        minThrust,
        maxThrust
    );

    const std::size_t maneuverCount = maneuverCountDist(rng);

    Specimen specimen;
    long double totalImpulse = 0.0L;

    for (std::size_t i = 0; i < maneuverCount; ++i)
    {
        Vector3 thrust(
            thrustDist(rng),
            thrustDist(rng),
            thrustDist(rng)
        );

        const long double thrustNorm = thrust.norm();

        if (thrustNorm <= 0.0L)
        {
            continue;
        }

        const long double remainingImpulse =
            MAX_IMPULSE - totalImpulse;

        if (remainingImpulse <= 0.0L)
        {
            break;
        }

        const long double maxAllowedDuration =
            remainingImpulse / thrustNorm;

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
            Maneuver(thrust, initTime, duration)
        );

        totalImpulse += thrustNorm * duration;
    }

    return specimen;
}

std::vector<Specimen> RandomInitializer::createPopulation(
    std::size_t populationSize
) const
{
    std::vector<Specimen> population;
    population.reserve(populationSize);

    for (std::size_t i = 0; i < populationSize; ++i)
    {
        population.push_back(create());
    }

    return population;
}