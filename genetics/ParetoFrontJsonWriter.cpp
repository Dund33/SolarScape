#include "ParetoFrontJsonWriter.h"

#include <boost/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>

#include "genetics/fitness/FitnessMetrics.h"
#include "genetics/fitness/FitnessValue.h"
#include "simulation/Maneuver.h"

namespace
{
    namespace json = boost::json;

    struct RunningStats
    {
        std::size_t count{};
        Real min{std::numeric_limits<Real>::max()};
        Real max{std::numeric_limits<Real>::lowest()};
        Real sum{};
        Real sumSquares{};

        void add(Real value)
        {
            ++count;
            min = std::min(min, value);
            max = std::max(max, value);
            sum += value;
            sumSquares += value * value;
        }

        Real mean() const
        {
            return count > 0
                ? sum / static_cast<Real>(count)
                : 0.0;
        }

        Real stddev() const
        {
            if (count < 2)
            {
                return 0.0;
            }

            const Real avg = mean();
            const Real variance =
                std::max(
                    0.0,
                    sumSquares / static_cast<Real>(count) - avg * avg);

            return std::sqrt(variance);
        }
    };

    struct FrontStats
    {
        std::size_t paretoFrontSize{};
        std::size_t fuelFeasibleCount{};
        RunningStats minimumDistance;
        RunningStats targetWindowViolation;
        RunningStats minimumDistanceTime;
        RunningStats fuelUsed;
        RunningStats fuelConstraintViolation;
        RunningStats maneuverCount;
    };

    struct BestValue
    {
        bool hasValue{};
        Real value{};
        std::size_t generation{};
    };

    auto toJsonNumber(Real value) -> double
    {
        return static_cast<double>(value);
    }

    auto nullableNumberToJson(
        bool hasValue,
        Real value) -> json::value
    {
        if (!hasValue)
        {
            return nullptr;
        }

        return toJsonNumber(value);
    }

    auto nullableIndexToJson(
        bool hasValue,
        std::size_t value) -> json::value
    {
        if (!hasValue)
        {
            return nullptr;
        }

        return value;
    }

    auto vectorToJson(
        const Vector3& vector) -> json::object
    {
        return {
            {"x", toJsonNumber(vector.x)},
            {"y", toJsonNumber(vector.y)},
            {"z", toJsonNumber(vector.z)}};
    }

    auto fitnessToJson(
        const FitnessValue& fitness) -> json::object
    {
        return {
            {"minimumDistance", toJsonNumber(fitness.minimumDistance)},
            {"targetWindowViolation", toJsonNumber(targetWindowViolation(fitness))},
            {"minimumDistanceTime", toJsonNumber(fitness.minimumDistanceTime)},
            {
                "fuelUsed",
                toJsonNumber(fitness.fuelUsed)
            },
            {
                "fuelConstraintViolation",
                toJsonNumber(fitness.fuelConstraintViolation)
            }};
    }

    auto statsToJson(
        const RunningStats& stats) -> json::object
    {
        return {
            {"count", stats.count},
            {"min", nullableNumberToJson(stats.count > 0, stats.min)},
            {"max", nullableNumberToJson(stats.count > 0, stats.max)},
            {"mean", nullableNumberToJson(stats.count > 0, stats.mean())},
            {"stddev", nullableNumberToJson(stats.count > 0, stats.stddev())}};
    }

    auto calculateFrontStats(
        const ParetoFront& paretoFront) -> FrontStats
    {
        FrontStats stats;
        stats.paretoFrontSize = paretoFront.size();

        for (const Specimen& specimen : paretoFront)
        {
            stats.maneuverCount.add(
                static_cast<Real>(specimen.size()));

            if (!specimen.getFitness().has_value())
            {
                continue;
            }

            const FitnessValue& fitness =
                specimen.getFitness().value();

            if (fitness.fuelConstraintViolation <= 0.0)
            {
                ++stats.fuelFeasibleCount;
            }

            stats.minimumDistance.add(
                fitness.minimumDistance);
            stats.targetWindowViolation.add(
                targetWindowViolation(fitness));
            stats.minimumDistanceTime.add(
                fitness.minimumDistanceTime);
            stats.fuelUsed.add(
                fitness.fuelUsed);
            stats.fuelConstraintViolation.add(
                fitness.fuelConstraintViolation);
        }

        return stats;
    }

    auto frontStatsToJson(
        const FrontStats& stats) -> json::object
    {
        return {
            {"paretoFrontSize", stats.paretoFrontSize},
            {"fuelFeasibleCount", stats.fuelFeasibleCount},
            {
                "fuelFeasibleRatio",
                nullableNumberToJson(
                    stats.paretoFrontSize > 0,
                    static_cast<Real>(stats.fuelFeasibleCount) /
                        static_cast<Real>(stats.paretoFrontSize))
            },
            {
                "objectives",
                {
                    {"minimumDistance", statsToJson(stats.minimumDistance)},
                    {"targetWindowViolation", statsToJson(stats.targetWindowViolation)},
                    {"minimumDistanceTime", statsToJson(stats.minimumDistanceTime)},
                    {"fuelUsed", statsToJson(stats.fuelUsed)},
                    {"fuelConstraintViolation", statsToJson(stats.fuelConstraintViolation)}
                }
            },
            {"maneuverCount", statsToJson(stats.maneuverCount)}};
    }

    auto maneuverToJson(
        const Maneuver& maneuver) -> json::object
    {
        return {
            {"thrustDirection", vectorToJson(maneuver.getThrustDirection())},
            {"throttleValue", toJsonNumber(maneuver.getThrottleValue())},
            {"initDelay", toJsonNumber(maneuver.getInitDelay())},
            {"duration", toJsonNumber(maneuver.getDuration())}};
    }

    auto maneuversToJson(
        const Specimen& specimen) -> json::array
    {
        json::array result;
        const std::vector<Maneuver>& maneuvers =
            specimen.getManeuvers();

        result.reserve(maneuvers.size());

        for (const Maneuver& maneuver : maneuvers)
        {
            result.push_back(maneuverToJson(maneuver));
        }

        return result;
    }

    auto specimenToJson(
        const Specimen& specimen) -> json::object
    {
        json::object result;

        if (specimen.getFitness().has_value())
        {
            result["fitness"] =
                fitnessToJson(
                    specimen.getFitness().value());
        }
        else
        {
            result["fitness"] = nullptr;
        }

        result["maneuvers"] =
            maneuversToJson(
                specimen);

        return result;
    }

    auto paretoFrontToJson(
        const std::vector<Specimen>& paretoFront,
        const FrontStats& stats) -> json::object
    {
        json::array front;
        front.reserve(paretoFront.size());

        for (const Specimen& specimen : paretoFront)
        {
            front.push_back(specimenToJson(specimen));
        }

        return {
            {"paretoFrontSize", paretoFront.size()},
            {"stats", frontStatsToJson(stats)},
            {"paretoFront", std::move(front)}};
    }

    auto paretoFrontToJson(
        const std::vector<Specimen>& paretoFront) -> json::object
    {
        return paretoFrontToJson(
            paretoFront,
            calculateFrontStats(
                paretoFront));
    }

    auto generationParetoFrontToJson(
        std::size_t generation,
        const ParetoFront& paretoFront,
        const FrontStats& stats) -> json::object
    {
        json::object result =
            paretoFrontToJson(
                paretoFront,
                stats);
        result["generation"] = generation;
        result["stats"] = frontStatsToJson(stats);

        return result;
    }

    void pushStatMin(
        json::array& target,
        const RunningStats& stats)
    {
        target.push_back(
            nullableNumberToJson(
                stats.count > 0,
                stats.min));
    }

    void pushStatMean(
        json::array& target,
        const RunningStats& stats)
    {
        target.push_back(
            nullableNumberToJson(
                stats.count > 0,
                stats.mean()));
    }

    void pushStatMax(
        json::array& target,
        const RunningStats& stats)
    {
        target.push_back(
            nullableNumberToJson(
                stats.count > 0,
                stats.max));
    }

    void updateBest(
        BestValue& best,
        const RunningStats& stats,
        std::size_t generation)
    {
        if (stats.count == 0)
        {
            return;
        }

        if (!best.hasValue || stats.min < best.value)
        {
            best.hasValue = true;
            best.value = stats.min;
            best.generation = generation;
        }
    }

    auto bestValueToJson(
        const BestValue& best) -> json::object
    {
        return {
            {"value", nullableNumberToJson(best.hasValue, best.value)},
            {"generation", nullableIndexToJson(best.hasValue, best.generation)}};
    }

    auto summaryToJson(
        const std::vector<FrontStats>& frontStats) -> json::object
    {
        std::size_t maxParetoFrontSize = 0;
        std::size_t maxFuelFeasibleCount = 0;
        bool hasFirstFuelFeasibleGeneration = false;
        std::size_t firstFuelFeasibleGeneration = 0;
        BestValue bestMinimumDistance;
        BestValue bestTargetWindowViolation;
        BestValue bestMinimumDistanceTime;
        BestValue bestFuelUsed;
        BestValue bestFuelConstraintViolation;

        for (std::size_t generation = 0;
             generation < frontStats.size();
             ++generation)
        {
            const FrontStats& stats = frontStats[generation];

            maxParetoFrontSize =
                std::max(
                    maxParetoFrontSize,
                    stats.paretoFrontSize);
            maxFuelFeasibleCount =
                std::max(
                    maxFuelFeasibleCount,
                    stats.fuelFeasibleCount);

            if (
                !hasFirstFuelFeasibleGeneration &&
                stats.fuelFeasibleCount > 0)
            {
                hasFirstFuelFeasibleGeneration = true;
                firstFuelFeasibleGeneration = generation;
            }

            updateBest(
                bestMinimumDistance,
                stats.minimumDistance,
                generation);
            updateBest(
                bestTargetWindowViolation,
                stats.targetWindowViolation,
                generation);
            updateBest(
                bestMinimumDistanceTime,
                stats.minimumDistanceTime,
                generation);
            updateBest(
                bestFuelUsed,
                stats.fuelUsed,
                generation);
            updateBest(
                bestFuelConstraintViolation,
                stats.fuelConstraintViolation,
                generation);
        }

        const bool hasFinalGeneration = !frontStats.empty();
        const FrontStats* finalStats =
            hasFinalGeneration
                ? &frontStats.back()
                : nullptr;

        return {
            {
                "finalGeneration",
                nullableIndexToJson(
                    hasFinalGeneration,
                    frontStats.size() - 1)
            },
            {
                "finalParetoFrontSize",
                hasFinalGeneration
                    ? json::value(finalStats->paretoFrontSize)
                    : json::value(nullptr)
            },
            {"maxParetoFrontSize", maxParetoFrontSize},
            {"maxFuelFeasibleCount", maxFuelFeasibleCount},
            {
                "firstFuelFeasibleGeneration",
                nullableIndexToJson(
                    hasFirstFuelFeasibleGeneration,
                    firstFuelFeasibleGeneration)
            },
            {
                "bestOverall",
                {
                    {"minimumDistance", bestValueToJson(bestMinimumDistance)},
                    {"targetWindowViolation", bestValueToJson(bestTargetWindowViolation)},
                    {"minimumDistanceTime", bestValueToJson(bestMinimumDistanceTime)},
                    {"fuelUsed", bestValueToJson(bestFuelUsed)},
                    {"fuelConstraintViolation", bestValueToJson(bestFuelConstraintViolation)}
                }
            }};
    }

    auto seriesToJson(
        const std::vector<FrontStats>& frontStats) -> json::object
    {
        json::array generations;
        json::array paretoFrontSize;
        json::array fuelFeasibleCount;
        json::array fuelFeasibleRatio;
        json::array bestMinimumDistance;
        json::array bestTargetWindowViolation;
        json::array bestMinimumDistanceTime;
        json::array bestFuelUsed;
        json::array bestFuelConstraintViolation;
        json::array meanManeuverCount;
        json::array maxManeuverCount;

        generations.reserve(frontStats.size());
        paretoFrontSize.reserve(frontStats.size());
        fuelFeasibleCount.reserve(frontStats.size());
        fuelFeasibleRatio.reserve(frontStats.size());
        bestMinimumDistance.reserve(frontStats.size());
        bestTargetWindowViolation.reserve(frontStats.size());
        bestMinimumDistanceTime.reserve(frontStats.size());
        bestFuelUsed.reserve(frontStats.size());
        bestFuelConstraintViolation.reserve(frontStats.size());
        meanManeuverCount.reserve(frontStats.size());
        maxManeuverCount.reserve(frontStats.size());

        for (std::size_t generation = 0;
             generation < frontStats.size();
             ++generation)
        {
            const FrontStats& stats = frontStats[generation];

            generations.push_back(generation);
            paretoFrontSize.push_back(stats.paretoFrontSize);
            fuelFeasibleCount.push_back(stats.fuelFeasibleCount);
            fuelFeasibleRatio.push_back(
                nullableNumberToJson(
                    stats.paretoFrontSize > 0,
                    static_cast<Real>(stats.fuelFeasibleCount) /
                        static_cast<Real>(stats.paretoFrontSize)));
            pushStatMin(
                bestMinimumDistance,
                stats.minimumDistance);
            pushStatMin(
                bestTargetWindowViolation,
                stats.targetWindowViolation);
            pushStatMin(
                bestMinimumDistanceTime,
                stats.minimumDistanceTime);
            pushStatMin(
                bestFuelUsed,
                stats.fuelUsed);
            pushStatMin(
                bestFuelConstraintViolation,
                stats.fuelConstraintViolation);
            pushStatMean(
                meanManeuverCount,
                stats.maneuverCount);
            pushStatMax(
                maxManeuverCount,
                stats.maneuverCount);
        }

        return {
            {"generation", std::move(generations)},
            {"paretoFrontSize", std::move(paretoFrontSize)},
            {"fuelFeasibleCount", std::move(fuelFeasibleCount)},
            {"fuelFeasibleRatio", std::move(fuelFeasibleRatio)},
            {"bestMinimumDistance", std::move(bestMinimumDistance)},
            {"bestTargetWindowViolation", std::move(bestTargetWindowViolation)},
            {"bestMinimumDistanceTime", std::move(bestMinimumDistanceTime)},
            {"bestFuelUsed", std::move(bestFuelUsed)},
            {"bestFuelConstraintViolation", std::move(bestFuelConstraintViolation)},
            {"meanManeuverCount", std::move(meanManeuverCount)},
            {"maxManeuverCount", std::move(maxManeuverCount)}};
    }

    auto paretoFrontHistoryToJson(
        const ParetoFrontHistory& paretoFrontHistory) -> json::object
    {
        json::array generations;
        generations.reserve(paretoFrontHistory.size());
        std::vector<FrontStats> statsByGeneration;
        statsByGeneration.reserve(paretoFrontHistory.size());

        for (const ParetoFront& paretoFront : paretoFrontHistory)
        {
            statsByGeneration.push_back(
                calculateFrontStats(
                    paretoFront));
        }

        for (std::size_t generation = 0;
             generation < paretoFrontHistory.size();
             ++generation)
        {
            generations.push_back(
                generationParetoFrontToJson(
                    generation,
                    paretoFrontHistory[generation],
                    statsByGeneration[generation]));
        }

        return {
            {"schemaVersion", 2},
            {"generationCount", paretoFrontHistory.size()},
            {"summary", summaryToJson(statsByGeneration)},
            {"series", seriesToJson(statsByGeneration)},
            {"generations", std::move(generations)}};
    }
}

void writeParetoFrontJson(
    const std::string& filePath,
    const std::vector<Specimen>& paretoFront)
{
    std::ofstream output(filePath);

    if (!output)
    {
        throw std::runtime_error(
            "Could not open Pareto front output file: " + filePath);
    }

    output
        << json::serialize(
            paretoFrontToJson(
                paretoFront))
        << '\n';
}

void writeParetoFrontJson(
    const std::string& filePath,
    const ParetoFrontHistory& paretoFrontHistory)
{
    std::ofstream output(filePath);

    if (!output)
    {
        throw std::runtime_error(
            "Could not open Pareto front output file: " + filePath);
    }

    output
        << json::serialize(
            paretoFrontHistoryToJson(
                paretoFrontHistory))
        << '\n';
}
