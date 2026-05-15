#ifndef SOLARSCAPE_MUTATIONFACTORY_H
#define SOLARSCAPE_MUTATIONFACTORY_H

#include <memory>

#include "genetics/mutation/Mutation.h"

class MutationFactory
{
public:
    virtual ~MutationFactory() = default;

    virtual std::unique_ptr<Mutation> create() const = 0;
};

#endif
