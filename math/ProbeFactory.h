#ifndef SOLARSCAPE_PROBEFACTORY_H
#define SOLARSCAPE_PROBEFACTORY_H

#include "math/Probe.h"
#include "math/ProbeProperties.h"
#include "math/Vector3.h"

class ProbeFactory
{
public:
    explicit ProbeFactory(ProbeProperties properties);

    auto create(
        const Vector3& position,
        const Vector3& velocity) const -> Probe;

    auto properties() const -> const ProbeProperties&;

private:
    ProbeProperties properties_;
};

#endif
