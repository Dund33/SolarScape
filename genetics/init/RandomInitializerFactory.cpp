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
        probeProperties);
}
