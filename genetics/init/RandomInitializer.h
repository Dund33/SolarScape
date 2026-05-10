#ifndef SOLARSCAPE_RANDOMINITIALIZER_H
#define SOLARSCAPE_RANDOMINITIALIZER_H

#include <vector>
#include "../Specimen.h"

#include "config/consts.h"

class RandomInitializer
{
public:
    RandomInitializer(
        std::size_t minManeuvers,
        std::size_t maxManeuvers,
        long double minInitTime,
        long double maxInitTime,
        long double minDuration,
        long double maxDuration,
        long double minThrust,
        long double maxThrust
    );

    Specimen create() const;
    std::vector<Specimen> createPopulation(std::size_t populationSize) const;

private:
    std::size_t minManeuvers;
    std::size_t maxManeuvers;

    long double minInitTime;
    long double maxInitTime;

    long double minDuration;
    long double maxDuration;

    long double minThrust;
    long double maxThrust;
};

#endif // SOLARSCAPE_RANDOMINITIALIZER_H