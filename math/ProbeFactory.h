#ifndef SOLARSCAPE_PROBEFACTORY_H
#define SOLARSCAPE_PROBEFACTORY_H

#include "math/Probe.h"
#include "math/ProbeProperties.h"
#include "math/Vector3.h"

class ProbeFactory
{
public:
    ProbeFactory(ProbeProperties properties, Vector3 position, Vector3 velocity);

    auto create() const -> Probe;

    auto properties() const -> const ProbeProperties&;

private:
    ProbeProperties properties_;
    Vector3 position_;
    Vector3 velocity_;
};

#endif
