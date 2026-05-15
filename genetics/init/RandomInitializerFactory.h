#ifndef SOLARSCAPE_RANDOMINITIALIZERFACTORY_H
#define SOLARSCAPE_RANDOMINITIALIZERFACTORY_H

#include <cstddef>

#include "genetics/init/InitializerFactory.h"
#include "math/Real.h"

class Probe;

class RandomInitializerFactory final : public InitializerFactory
{
public:
    RandomInitializerFactory(
        std::size_t minManeuvers,
        std::size_t maxManeuvers,
        Real minInitTime,
        Real maxInitTime,
        Real minDuration,
        Real maxDuration,
        Probe& probe
    );

    std::unique_ptr<Initializer> create() const override;

private:
    std::size_t minManeuvers;
    std::size_t maxManeuvers;

    Real minInitTime;
    Real maxInitTime;

    Real minDuration;
    Real maxDuration;

    Probe& probe;
};

#endif
