#ifndef SOLARSCAPE_PARETOFRONTUTILS_H
#define SOLARSCAPE_PARETOFRONTUTILS_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "math/Real.h"

struct ParetoFrontStats
{
    std::size_t size{};
    std::size_t fuelFeasibleCount{};
    Real minDistance{};
    Real maxDistance{};
    Real minTime{};
    Real maxTime{};
    Real minFuel{};
    Real maxFuel{};
    Real minFuelViolation{};
    Real maxFuelViolation{};
};

class ParetoFrontUtils
{
public:
    static std::vector<Specimen> firstFront(
        const std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator);

    static std::vector<Specimen> frontFromIndices(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& frontIndices);

    static ParetoFrontStats calculateStats(
        const std::vector<Specimen>& front);

    static ParetoFrontStats calculateStats(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& frontIndices);

private:
    ParetoFrontUtils() = delete;
};

#endif
