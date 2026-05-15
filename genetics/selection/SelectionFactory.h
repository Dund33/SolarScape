#ifndef SOLARSCAPE_SELECTIONFACTORY_H
#define SOLARSCAPE_SELECTIONFACTORY_H

#include <memory>

#include "genetics/selection/Selection.h"

class SelectionFactory
{
public:
    virtual ~SelectionFactory() = default;

    virtual std::unique_ptr<Selection> create() const = 0;
};

#endif
