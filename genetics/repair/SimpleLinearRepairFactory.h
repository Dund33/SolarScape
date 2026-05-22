#ifndef SOLARSCAPE_SIMPLELINEARREPAIRFACTORY_H
#define SOLARSCAPE_SIMPLELINEARREPAIRFACTORY_H

#include "genetics/repair/RepairFactory.h"

class SimpleLinearRepairFactory final : public RepairFactory
{
public:
    std::unique_ptr<Repair> create() const override;
};

#endif
