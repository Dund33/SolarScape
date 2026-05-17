#ifndef SOLARSCAPE_NORMALRANDOMSEARCHFACTORY_H
#define SOLARSCAPE_NORMALRANDOMSEARCHFACTORY_H

#include <cstddef>
#include <memory>

#include "genetics/search/LocalImprovementFactory.h"
#include "math/Real.h"

class NormalRandomSearchFactory final : public LocalImprovementFactory
{
public:
    NormalRandomSearchFactory(
        std::size_t iterations,
        Real initTimeStdDev,
        Real durationStdDev,
        Real throttleStdDev);

    std::unique_ptr<LocalImprovement> create() const override;

private:
    std::size_t iterations;
    Real initTimeStdDev;
    Real durationStdDev;
    Real throttleStdDev;
};

#endif
