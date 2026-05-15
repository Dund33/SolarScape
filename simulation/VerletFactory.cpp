#include "VerletFactory.h"

#include <memory>

#include "simulation/Verlet.h"

std::unique_ptr<Simulation> VerletFactory::create() const
{
    return std::make_unique<Verlet>();
}
