#include "NormalRandomSearchFactory.h"

#include <memory>

#include "genetics/search/NormalRandomSearch.h"

NormalRandomSearchFactory::NormalRandomSearchFactory(
    std::size_t iterations,
    Real initTimeStdDev,
    Real durationStdDev,
    Real throttleStdDev)
    : iterations(iterations),
      initTimeStdDev(initTimeStdDev),
      durationStdDev(durationStdDev),
      throttleStdDev(throttleStdDev)
{
}

std::unique_ptr<LocalImprovement> NormalRandomSearchFactory::create() const
{
    return std::make_unique<NormalRandomSearch>(
        iterations,
        initTimeStdDev,
        durationStdDev,
        throttleStdDev);
}
