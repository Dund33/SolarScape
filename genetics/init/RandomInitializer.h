#ifndef SOLARSCAPE_RANDOMINITIALIZER_H
#define SOLARSCAPE_RANDOMINITIALIZER_H

#include <cstddef>
#include "genetics/init/Initializer.h"
#include "math/ProbeProperties.h"

class RandomInitializer final : public Initializer
{
public:
    RandomInitializer(
        std::size_t minManeuvers,
        std::size_t maxManeuvers,
        long double minInitTime,
        long double maxInitTime,
        long double minDuration,
        long double maxDuration,
        const ProbeProperties& probeProperties
    );

    Specimen create() const override;
    std::vector<Specimen> createPopulation(
        std::size_t populationSize
    ) const override;

private:
    std::size_t minManeuvers;
    std::size_t maxManeuvers;

    long double minInitTime;
    long double maxInitTime;

    long double minDuration;
    long double maxDuration;

    ProbeProperties probeProperties;
};

#endif
