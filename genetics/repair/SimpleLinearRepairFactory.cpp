#include "SimpleLinearRepairFactory.h"

#include <memory>

#include "genetics/repair/SimpleLinearRepair.h"

std::unique_ptr<Repair> SimpleLinearRepairFactory::create() const
{
    return std::make_unique<SimpleLinearRepair>();
}
