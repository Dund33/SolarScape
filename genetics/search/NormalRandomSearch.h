#ifndef SOLARSCAPE_NORMALRANDOMSEARCH_H
#define SOLARSCAPE_NORMALRANDOMSEARCH_H

#include <cstddef>

#include "genetics/Specimen.h"
#include "genetics/fitness/FitnessEvaluator.h"
#include "math/Real.h"
#include "simulation/Maneuver.h"

class NormalRandomSearch
{
public:
    NormalRandomSearch(
        std::size_t iterations,
        Real initTimeStdDev,
        Real durationStdDev,
        Real throttleStdDev);

    void improve(
        Specimen& specimen,
        const FitnessEvaluator& fitnessEvaluator) const;

private:
    Maneuver perturbManeuver(
        const Maneuver& maneuver) const;

    bool isBetter(
        const Specimen& candidate,
        const Specimen& currentBest) const;

    std::size_t iterations;
    Real initTimeStdDev;
    Real durationStdDev;
    Real throttleStdDev;
};

#endif // SOLARSCAPE_NORMALRANDOMSEARCH_H
