#include "ParetoFrontJsonWriter.h"

#include <boost/json.hpp>

#include <fstream>
#include <stdexcept>

#include "genetics/fitness/FitnessValue.h"
#include "simulation/Maneuver.h"

namespace
{
    namespace json = boost::json;

    auto toJsonNumber(Real value) -> double
    {
        return static_cast<double>(value);
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
            {"minimumDistanceTime", toJsonNumber(fitness.minimumDistanceTime)},
            {
                "minimumDistanceFuelMass",
                toJsonNumber(fitness.minimumDistanceFuelMass)
            },
            {
                "fuelConstraintViolation",
                toJsonNumber(fitness.fuelConstraintViolation)
            }};
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

        result.reserve(
            maneuvers.size());

        for (const Maneuver& maneuver : maneuvers)
        {
            result.push_back(
                maneuverToJson(maneuver));
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
        const std::vector<Specimen>& paretoFront) -> json::object
    {
        json::array front;
        front.reserve(
            paretoFront.size());

        for (const Specimen& specimen : paretoFront)
        {
            front.push_back(
                specimenToJson(specimen));
        }

        return {
            {"paretoFrontSize", paretoFront.size()},
            {"paretoFront", std::move(front)}};
    }

    auto generationParetoFrontToJson(
        std::size_t generation,
        const ParetoFront& paretoFront) -> json::object
    {
        json::object result =
            paretoFrontToJson(
                paretoFront);
        result["generation"] = generation;

        return result;
    }

    auto paretoFrontHistoryToJson(
        const ParetoFrontHistory& paretoFrontHistory) -> json::object
    {
        json::array generations;
        generations.reserve(
            paretoFrontHistory.size());

        for (std::size_t generation = 0;
             generation < paretoFrontHistory.size();
             ++generation)
        {
            generations.push_back(
                generationParetoFrontToJson(
                    generation,
                    paretoFrontHistory[generation]));
        }

        return {
            {"generationCount", paretoFrontHistory.size()},
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
