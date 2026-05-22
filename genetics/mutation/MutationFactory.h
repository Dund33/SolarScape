#ifndef SOLARSCAPE_MUTATIONFACTORY_H
#define SOLARSCAPE_MUTATIONFACTORY_H

#include <memory>

#include "genetics/mutation/Mutation.h"
#include "genetics/repair/RepairFactory.h"

class MutationFactory
{
public:
    virtual ~MutationFactory() = default;

    virtual std::unique_ptr<Mutation> create() const = 0;

    std::unique_ptr<Repair> createRepair() const
    {
        return repairFactory.create();
    }

protected:
    explicit MutationFactory(
        const RepairFactory& repairFactory)
        : repairFactory(repairFactory)
    {
    }

private:
    const RepairFactory& repairFactory;
};

#endif
