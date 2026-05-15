#include "RandomInitializerFactory.h"

#include <memory>

#include "genetics/init/RandomInitializer.h"

RandomInitializerFactory::RandomInitializerFactory(
    std::size_t minManeuvers,
    std::size_t maxManeuvers,
    Real minInitTime,
    Real maxInitTime,
    Real minDuration,
    Real maxDuration,
    Probe& probe
)
    : minManeuvers(minManeuvers),
      maxManeuvers(maxManeuvers),
      minInitTime(minInitTime),
      maxInitTime(maxInitTime),
      minDuration(minDuration),
      maxDuration(maxDuration),
      probe(probe)
{
}

std::unique_ptr<Initializer> RandomInitializerFactory::create() const
{
    return std::make_unique<RandomInitializer>(
        minManeuvers,
        maxManeuvers,
        minInitTime,
        maxInitTime,
        minDuration,
        maxDuration,
        &probe);
}
