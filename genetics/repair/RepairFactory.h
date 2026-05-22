#ifndef SOLARSCAPE_REPAIRFACTORY_H
#define SOLARSCAPE_REPAIRFACTORY_H

#include <memory>

#include "genetics/repair/Repair.h"

class RepairFactory
{
public:
    virtual ~RepairFactory() = default;

    virtual std::unique_ptr<Repair> create() const = 0;
};

#endif
