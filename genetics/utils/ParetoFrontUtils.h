#ifndef SOLARSCAPE_PARETOFRONTUTILS_H
#define SOLARSCAPE_PARETOFRONTUTILS_H

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/fitness/FitnessMetrics.h"
#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

struct ParetoFrontStats
{
    std::size_t size{};
    std::size_t fuelFeasibleCount{};
    Real minDistance{};
    Real maxDistance{};
    Real minTargetWindowViolation{};
    Real maxTargetWindowViolation{};
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
    template <std::ranges::forward_range SpecimenRange>
    requires std::is_lvalue_reference_v<
                 std::ranges::range_reference_t<SpecimenRange>> &&
             std::convertible_to<
                 std::ranges::range_reference_t<SpecimenRange>,
                 const Specimen&>
    static std::vector<Specimen> firstFront(
        SpecimenRange&& population,
        const SpecimenComparator& specimenComparator)
    {
        std::vector<Specimen> front;

        for (const Specimen& candidate : population)
        {
            bool dominated = false;

            for (const Specimen& other : population)
            {
                if (&candidate == &other)
                {
                    continue;
                }

                if (specimenComparator.compare(
                    other,
                    candidate) ==
                    std::partial_ordering::less)
                {
                    dominated = true;
                    break;
                }
            }

            if (!dominated)
            {
                front.push_back(candidate);
            }
        }

        return front;
    }

    static std::vector<Specimen> frontFromIndices(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& frontIndices);

    static std::vector<Specimen> updateArchive(
        std::vector<Specimen> archive,
        std::vector<Specimen> newFront,
        const SpecimenComparator& specimenComparator);

    template <std::ranges::forward_range SpecimenRange>
    requires std::is_lvalue_reference_v<
                 std::ranges::range_reference_t<SpecimenRange>> &&
             std::convertible_to<
                 std::ranges::range_reference_t<SpecimenRange>,
                 const Specimen&>
    static ParetoFrontStats calculateStats(
        SpecimenRange&& front)
    {
        ParetoFrontStats stats;

        for (const Specimen& specimen : front)
        {
            const FitnessValue& fitness =
                specimen.getFitness().value();

            if (stats.size == 0)
            {
                stats.minDistance = fitness.minimumDistance;
                stats.maxDistance = fitness.minimumDistance;
                stats.minTargetWindowViolation =
                    targetWindowViolation(fitness);
                stats.maxTargetWindowViolation =
                    targetWindowViolation(fitness);
                stats.minTime = fitness.minimumDistanceTime;
                stats.maxTime = fitness.minimumDistanceTime;
                stats.minFuel = fitness.fuelUsed;
                stats.maxFuel = fitness.fuelUsed;
                stats.minFuelViolation = fitness.fuelConstraintViolation;
                stats.maxFuelViolation = fitness.fuelConstraintViolation;
            }

            ++stats.size;

            if (fitness.fuelConstraintViolation <= 0.0L)
            {
                ++stats.fuelFeasibleCount;
            }

            stats.minDistance =
                std::min(stats.minDistance, fitness.minimumDistance);
            stats.maxDistance =
                std::max(stats.maxDistance, fitness.minimumDistance);
            stats.minTargetWindowViolation =
                std::min(
                    stats.minTargetWindowViolation,
                    targetWindowViolation(fitness));
            stats.maxTargetWindowViolation =
                std::max(
                    stats.maxTargetWindowViolation,
                    targetWindowViolation(fitness));
            stats.minTime =
                std::min(stats.minTime, fitness.minimumDistanceTime);
            stats.maxTime =
                std::max(stats.maxTime, fitness.minimumDistanceTime);
            stats.minFuel =
                std::min(stats.minFuel, fitness.fuelUsed);
            stats.maxFuel =
                std::max(stats.maxFuel, fitness.fuelUsed);
            stats.minFuelViolation =
                std::min(stats.minFuelViolation, fitness.fuelConstraintViolation);
            stats.maxFuelViolation =
                std::max(stats.maxFuelViolation, fitness.fuelConstraintViolation);
        }

        return stats;
    }

    static ParetoFrontStats calculateStats(
        const std::vector<Specimen>& population,
        const std::vector<std::size_t>& frontIndices);

private:
    ParetoFrontUtils() = delete;
};

#endif
