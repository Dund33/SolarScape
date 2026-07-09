#include "RandomInitializerFactory.h"

#include <memory>

#include "genetics/init/RandomInitializer.h"

RandomInitializerFactory::RandomInitializerFactory(std::size_t minManeuverCount, std::size_t maxManeuverCount, Real minInitTimeValue,
                                                   Real maxInitTimeValue, Real minDurationValue, Real maxDurationValue,
                                                   const ProbeProperties& probePropertiesValue)
    : minManeuvers(minManeuverCount), maxManeuvers(maxManeuverCount), minInitTime(minInitTimeValue), maxInitTime(maxInitTimeValue),
      minDuration(minDurationValue), maxDuration(maxDurationValue), probeProperties(probePropertiesValue)
{
}

std::unique_ptr<Initializer> RandomInitializerFactory::create() const
{
    return std::make_unique<RandomInitializer>(minManeuvers, maxManeuvers, minInitTime, maxInitTime, minDuration, maxDuration,
                                               probeProperties);
}
