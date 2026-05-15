#ifndef SOLARSCAPE_INITIALIZERFACTORY_H
#define SOLARSCAPE_INITIALIZERFACTORY_H

#include <memory>

#include "genetics/init/Initializer.h"

class InitializerFactory
{
public:
    virtual ~InitializerFactory() = default;

    virtual std::unique_ptr<Initializer> create() const = 0;
};

#endif
