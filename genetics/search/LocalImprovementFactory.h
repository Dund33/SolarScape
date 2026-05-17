#ifndef SOLARSCAPE_LOCALIMPROVEMENTFACTORY_H
#define SOLARSCAPE_LOCALIMPROVEMENTFACTORY_H

#include <memory>

#include "genetics/search/LocalImprovement.h"

class LocalImprovementFactory
{
public:
    virtual ~LocalImprovementFactory() = default;

    virtual std::unique_ptr<LocalImprovement> create() const = 0;
};

#endif
