#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/fitness/SimulationFitnessEvaluator.h"
#include "genetics/fitness/VectorSimulationFitnessEvaluator.h"
#include "math/Body.h"
#include "math/Probe.h"
#include "simulation/Maneuver.h"
#include "simulation/VectorVerletFactory.h"
#include "simulation/VerletFactory.h"

namespace
{
    void expect(
        bool condition,
        const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    std::vector<Specimen*> pointersTo(
        std::vector<Specimen>& specimens)
    {
        std::vector<Specimen*> result;
        result.reserve(
            specimens.size());

        for (Specimen& specimen : specimens)
        {
            result.push_back(
                &specimen);
        }

        return result;
    }

    std::vector<Specimen> createFixturePopulation()
    {
        return {
            Specimen({
                Maneuver(Vector3(1.0, 0.0, 0.0), 0.40, 0.0, 120.0)}),
            Specimen({
                Maneuver(Vector3(0.0, 1.0, 0.0), 0.60, 50.0, 180.0),
                Maneuver(Vector3(-1.0, 0.2, 0.0), 0.20, 25.0, 90.0)}),
            Specimen({
                Maneuver(Vector3(0.0, 0.0, 1.0), 1.00, 10.0, 100.0)}),
            Specimen({
                Maneuver(Vector3(-0.4, 0.8, 0.1), 0.70, 200.0, 200.0)}),
            Specimen({
                Maneuver(Vector3(0.2, -0.5, 0.7), 0.30, 0.0, 300.0),
                Maneuver(Vector3(1.0, 1.0, 0.0), 0.50, 80.0, 150.0)}),
            Specimen(std::vector<Maneuver>{}),
            Specimen({
                Maneuver(Vector3(1.0, -1.0, 0.3), 0.90, 400.0, 50.0)}),
            Specimen({
                Maneuver(Vector3(-0.1, -0.2, 1.0), 0.10, 30.0, 500.0)}),
            Specimen({
                Maneuver(Vector3(0.3, 0.4, -0.6), 0.80, 15.0, 75.0)})};
    }

    bool nearlyEqual(
        Real lhs,
        Real rhs)
    {
        constexpr Real absoluteTolerance = 1.0e-5;
        constexpr Real relativeTolerance = 1.0e-9;

        const Real difference =
            std::abs(
                lhs - rhs);

        if (difference <= absoluteTolerance)
        {
            return true;
        }

        const Real scale =
            std::max(
                std::abs(lhs),
                std::abs(rhs));

        return difference <= relativeTolerance * scale;
    }

    void expectNearlyEqual(
        Real scalarValue,
        Real vectorValue,
        const char* metricName,
        std::size_t specimenIndex)
    {
        if (!nearlyEqual(
                scalarValue,
                vectorValue))
        {
            throw std::runtime_error(
                std::string("Expected scalar and vector ") +
                metricName +
                " to match for specimen " +
                std::to_string(specimenIndex) +
                ", scalar=" +
                std::to_string(scalarValue) +
                ", vector=" +
                std::to_string(vectorValue));
        }
    }

    void expectSameFitness(
        const FitnessValue& scalarFitness,
        const FitnessValue& vectorFitness,
        std::size_t specimenIndex)
    {
        expectNearlyEqual(
            scalarFitness.minimumDistance,
            vectorFitness.minimumDistance,
            "minimumDistance",
            specimenIndex);
        expectNearlyEqual(
            scalarFitness.minimumDistanceTime,
            vectorFitness.minimumDistanceTime,
            "minimumDistanceTime",
            specimenIndex);
        expectNearlyEqual(
            scalarFitness.fuelUsed,
            vectorFitness.fuelUsed,
            "fuelUsed",
            specimenIndex);
        expectNearlyEqual(
            scalarFitness.fuelConstraintViolation,
            vectorFitness.fuelConstraintViolation,
            "fuelConstraintViolation",
            specimenIndex);
    }

    void testVectorFitnessMatchesScalarFitness()
    {
        const Real gravitationalConstant = 6.67430e-11;
        const Real timeStep = 10.0;
        const Real simulationTime = 600.0;

        const std::vector<Body> bodies{
            Body(
                Vector3(0.0, 0.0, 0.0),
                Vector3(0.0, 0.0, 0.0),
                5.0e14),
            Body(
                Vector3(20000.0, 100.0, 0.0),
                Vector3(0.0, 1.0, 0.0),
                2.0e5)};

        const Body targetBody(
            Vector3(50000.0, 0.0, 0.0),
            Vector3(0.0, 0.5, 0.0),
            1.0e5);
        const Probe probe(
            Vector3(52000.0, 200.0, 0.0),
            Vector3(0.0, 0.45, 0.0),
            1000.0,
            200.0,
            0.5,
            250.0);
        const Vector3 targetPointFromTargetBody(
            50.0,
            -20.0,
            10.0);

        const VerletFactory scalarSimulationFactory(
            gravitationalConstant,
            bodies,
            targetBody,
            probe);
        const VectorVerletFactory vectorSimulationFactory(
            gravitationalConstant,
            bodies,
            targetBody,
            probe);

        const SimulationFitnessEvaluator scalarEvaluator(
            timeStep,
            simulationTime,
            targetPointFromTargetBody,
            scalarSimulationFactory);
        const VectorSimulationFitnessEvaluator vectorEvaluator(
            timeStep,
            simulationTime,
            targetPointFromTargetBody,
            vectorSimulationFactory);

        std::vector<Specimen> scalarPopulation =
            createFixturePopulation();
        std::vector<Specimen> vectorPopulation =
            createFixturePopulation();

        std::vector<Specimen*> scalarSpecimens =
            pointersTo(
                scalarPopulation);
        std::vector<Specimen*> vectorSpecimens =
            pointersTo(
                vectorPopulation);

        scalarEvaluator.evaluateBatch(
            scalarSpecimens);
        vectorEvaluator.evaluateBatch(
            vectorSpecimens);

        expect(
            scalarPopulation.size() == vectorPopulation.size(),
            "Expected fixture populations to have the same size.");

        for (std::size_t specimenIndex = 0;
             specimenIndex < scalarPopulation.size();
             ++specimenIndex)
        {
            expect(
                scalarPopulation[specimenIndex].getFitness().has_value(),
                "Expected scalar evaluator to assign fitness.");
            expect(
                vectorPopulation[specimenIndex].getFitness().has_value(),
                "Expected vector evaluator to assign fitness.");

            expectSameFitness(
                scalarPopulation[specimenIndex].getFitness().value(),
                vectorPopulation[specimenIndex].getFitness().value(),
                specimenIndex);
        }
    }
}

auto main() -> int
{
    try
    {
        testVectorFitnessMatchesScalarFitness();
        std::cout << "Vector fitness consistency tests passed.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Vector fitness consistency test failed: " << e.what() << '\n';
        return 1;
    }
}
