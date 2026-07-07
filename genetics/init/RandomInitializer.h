#ifndef SOLARSCAPE_RANDOMINITIALIZER_H
#define SOLARSCAPE_RANDOMINITIALIZER_H

#include <cstddef>
#include <random>

#include "genetics/init/Initializer.h"
#include "math/ProbeProperties.h"

class RandomInitializer final : public Initializer
{
public:
    RandomInitializer(std::size_t minManeuvers, std::size_t maxManeuvers, Real minInitTime, Real maxInitTime, Real minDuration,
                      Real maxDuration, const ProbeProperties& probeProperties);

    Specimen create() const override;
    std::vector<Specimen> createPopulation(std::size_t populationSize) const override;

private:
    Specimen createCandidate(std::mt19937& rng) const;

    std::size_t minManeuvers;
    std::size_t maxManeuvers;

    Real minInitTime;
    Real maxInitTime;

    Real minDuration;
    Real maxDuration;

    ProbeProperties probeProperties;
};

#endif
