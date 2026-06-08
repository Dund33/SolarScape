#ifndef SOLARSCAPE_NORMALRANDOMSEARCH_H
#define SOLARSCAPE_NORMALRANDOMSEARCH_H

#include <cstddef>

#include "genetics/search/LocalImprovement.h"
#include "math/ProbeProperties.h"
#include "math/Real.h"
#include "simulation/Maneuver.h"

class NormalRandomSearch final : public LocalImprovement
{
public:
    NormalRandomSearch(
        std::size_t iterations,
        Real initTimeStdDev,
        Real durationStdDev,
        Real throttleStdDev,
        const ProbeProperties& probeProperties);

    void improve(
        Specimen& specimen,
        const FitnessEvaluator& fitnessEvaluator,
        const SpecimenComparator& specimenComparator) const override;

private:
    Maneuver perturbManeuver(
        const Maneuver& maneuver) const;

    std::size_t iterations;
    Real initTimeStdDev;
    Real durationStdDev;
    Real throttleStdDev;
    ProbeProperties probeProperties;
};

#endif
