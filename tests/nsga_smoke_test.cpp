#include <compare>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "config/consts.h"
#include "genetics/comparison/TrajectorySpecimenComparator.h"
#include "genetics/crossing/AlignedSimilarityCrossover.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "genetics/crossing/RandomCutCrossover.h"
#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "genetics/init/InitializerFactory.h"
#include "genetics/mutation/MutationFactory.h"
#include "genetics/nsga/NSGAIIAlgorithm.h"
#include "genetics/selection/SelectionFactory.h"

namespace
{
    Specimen specimenWithFitness(
        FitnessValue fitness)
    {
        Specimen specimen;
        specimen.setFitness(fitness);
        return specimen;
    }

    Specimen specimenWithManeuverCount(
        std::size_t maneuverCount)
    {
        std::vector<Maneuver> maneuvers;
        maneuvers.reserve(maneuverCount);

        for (std::size_t i = 0; i < maneuverCount; ++i)
        {
            maneuvers.emplace_back(
                Vector3(1.0L, 0.0L, 0.0L),
                0.5L,
                static_cast<Real>(i),
                10.0L);
        }

        return Specimen(std::move(maneuvers));
    }

    bool sameFitness(
        const FitnessValue& lhs,
        const FitnessValue& rhs)
    {
        return
            lhs.minimumDistance == rhs.minimumDistance &&
            lhs.minimumDistanceTime == rhs.minimumDistanceTime &&
            lhs.fuelUsed == rhs.fuelUsed &&
            lhs.fuelConstraintViolation == rhs.fuelConstraintViolation;
    }

    class FixtureInitializer final : public Initializer
    {
    public:
        explicit FixtureInitializer(
            std::vector<Specimen> specimens)
            : specimens(std::move(specimens))
        {
        }

        Specimen create() const override
        {
            return specimens.front();
        }

        std::vector<Specimen> createPopulation(
            std::size_t populationSize
        ) const override
        {
            if (populationSize > specimens.size())
            {
                throw std::invalid_argument(
                    "Fixture population is too small.");
            }

            return {
                specimens.begin(),
                specimens.begin() + static_cast<std::ptrdiff_t>(populationSize)};
        }

    private:
        std::vector<Specimen> specimens;
    };

    class FixtureInitializerFactory final : public InitializerFactory
    {
    public:
        explicit FixtureInitializerFactory(
            std::vector<Specimen> specimens)
            : specimens(std::move(specimens))
        {
        }

        std::unique_ptr<Initializer> create() const override
        {
            return std::make_unique<FixtureInitializer>(specimens);
        }

    private:
        std::vector<Specimen> specimens;
    };

    class NoopFitnessEvaluator final : public FitnessEvaluator
    {
    public:
        void evaluate(Specimen&) const override
        {
        }
    };

    class NoopFitnessEvaluatorFactory final : public FitnessEvaluatorFactory
    {
    public:
        std::unique_ptr<FitnessEvaluator> create() const override
        {
            return std::make_unique<NoopFitnessEvaluator>();
        }
    };

    class FirstSelection final : public Selection
    {
    public:
        const Specimen& select(
            const std::vector<Specimen>& population,
            const SpecimenComparator&
        ) const override
        {
            return population.front();
        }
    };

    class FirstSelectionFactory final : public SelectionFactory
    {
    public:
        std::unique_ptr<Selection> create() const override
        {
            return std::make_unique<FirstSelection>();
        }
    };

    class CopyCrossover final : public Crossover
    {
    public:
        std::pair<Specimen, Specimen> cross(
            const Specimen& parent1,
            const Specimen& parent2
        ) const override
        {
            return {parent1, parent2};
        }
    };

    class CopyCrossoverFactory final : public CrossoverFactory
    {
    public:
        std::unique_ptr<Crossover> create() const override
        {
            return std::make_unique<CopyCrossover>();
        }
    };

    class NoopMutation final : public Mutation
    {
    public:
        void mutate(Specimen&) const override
        {
        }
    };

    class NoopMutationFactory final : public MutationFactory
    {
    public:
        std::unique_ptr<Mutation> create() const override
        {
            return std::make_unique<NoopMutation>();
        }
    };

    void expect(
        bool condition,
        const char* message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }

    void testTrajectoryComparatorObjectivesAndTieBreakers()
    {
        TrajectorySpecimenComparator comparator;
        Specimen betterObjectives =
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 1.0L, 10.0L, 100.0L});
        Specimen worseObjectives =
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 2.0L, 1.0L, 1.0L});

        expect(
            comparator.compare(betterObjectives, worseObjectives) ==
                std::partial_ordering::less,
            "Expected smaller target-window violation to dominate.");

        Specimen lowerFuelLater =
            specimenWithFitness({10.0L, 2.0L, 1.0L});
        Specimen higherFuelEarlier =
            specimenWithFitness({1.0L, 1.0L, 10.0L});

        expect(
            comparator.compare(lowerFuelLater, higherFuelEarlier) ==
                std::partial_ordering::unordered,
            "Expected in-window fuel/time trade-off to be unordered.");
        expect(
            comparator.isLess(lowerFuelLater, higherFuelEarlier),
            "Expected trajectory comparator to break ties by fuel, then time.");

        Specimen closerInsideWindow =
            specimenWithFitness({1.0L, 10.0L, 5.0L});
        Specimen fartherInsideWindow =
            specimenWithFitness({TARGET_WINDOW_DISTANCE - 1.0L, 10.0L, 5.0L});

        expect(
            comparator.compare(closerInsideWindow, fartherInsideWindow) ==
                std::partial_ordering::equivalent,
            "Expected raw distance to stop dominating inside target window.");

        Specimen feasible =
            specimenWithFitness({2.0L, 10.0L, 5.0L});
        Specimen infeasible =
            specimenWithFitness({1.0L, 1.0L, 100.0L, 1.0L});

        expect(
            comparator.compare(feasible, infeasible) ==
                std::partial_ordering::less,
            "Expected feasible trajectory specimen to dominate infeasible specimen.");

        Specimen smallerViolation =
            specimenWithFitness({10.0L, 10.0L, 1.0L, 1.0L});
        Specimen largerViolation =
            specimenWithFitness({1.0L, 1.0L, 100.0L, 2.0L});

        expect(
            comparator.compare(smallerViolation, largerViolation) ==
                std::partial_ordering::less,
            "Expected smaller fuel violation to dominate larger violation.");
    }

    void testNSGAIIReturnsParetoFrontHistory()
    {
        const FitnessValue bestFuel{1.0L, 8.0L, 1.0L};
        const FitnessValue bestTime{2.0L, 1.0L, 5.0L};
        const FitnessValue dominatedA{3.0L, 9.0L, 6.0L};
        const FitnessValue dominatedB{4.0L, 10.0L, 10.0L};

        FixtureInitializerFactory initializerFactory({
            specimenWithFitness(bestFuel),
            specimenWithFitness(bestTime),
            specimenWithFitness(dominatedA),
            specimenWithFitness(dominatedB)});
        FirstSelectionFactory selectionFactory;
        CopyCrossoverFactory crossoverFactory;
        NoopMutationFactory mutationFactory;
        NoopFitnessEvaluatorFactory fitnessEvaluatorFactory;
        TrajectorySpecimenComparator comparator;

        NSGAIIAlgorithm algorithm(
            4,
            1,
            0,
            comparator,
            {
                initializerFactory,
                selectionFactory,
                crossoverFactory,
                mutationFactory,
                fitnessEvaluatorFactory});

        const ParetoFrontHistory paretoFrontHistory =
            algorithm.run();

        expect(
            paretoFrontHistory.size() == 1,
            "Expected one Pareto front entry for one NSGA-II generation.");

        const ParetoFront& paretoFront =
            paretoFrontHistory.back();

        bool foundBestFuel = false;
        bool foundBestTime = false;

        for (const Specimen& specimen : paretoFront)
        {
            const FitnessValue& fitness =
                specimen.getFitness().value();

            foundBestFuel =
                foundBestFuel ||
                sameFitness(fitness, bestFuel);
            foundBestTime =
                foundBestTime ||
                sameFitness(fitness, bestTime);
        }

        expect(
            foundBestFuel,
            "Expected best fuel specimen in Pareto front.");
        expect(
            foundBestTime,
            "Expected best time specimen in Pareto front.");
    }

    void testAlignedSimilarityCrossoverExchangesSuffixAfterAlignedRegion()
    {
        AlignedSimilarityCrossover crossover(0.9L);

        Specimen parent1({
            Maneuver(Vector3(1.0L, 0.0L, 0.0L), 0.2L, 10.0L, 10.0L),
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 5.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 1.0L, 0.0L), 0.0L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 0.0L, 1.0L), 0.6L, 5.0L, 10.0L)});
        Specimen parent2({
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 25.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 1.0L, 0.0L), 0.5L, 5.0L, 10.0L)});

        auto [child1, child2] =
            crossover.cross(
                parent1,
                parent2);

        expect(
            child1.size() == 4,
            "Expected first child to contain longer prefix and shorter suffix.");
        expect(
            child2.size() == 4,
            "Expected second child to contain shorter prefix and longer suffix.");
        expect(
            child1[0].getThrottleValue() == parent1[0].getThrottleValue() &&
            child1[1].getThrottleValue() == parent1[1].getThrottleValue() &&
            child1[2].getThrottleValue() == parent1[2].getThrottleValue() &&
            child1[3].getThrottleValue() == parent2[2].getThrottleValue(),
            "Expected first child to keep aligned longer prefix and receive shorter suffix.");
        expect(
            child2[0].getThrottleValue() == parent2[0].getThrottleValue() &&
            child2[1].getThrottleValue() == parent2[1].getThrottleValue() &&
            child2[2].getThrottleValue() == parent1[3].getThrottleValue() &&
            child2[3].getThrottleValue() == parent1[4].getThrottleValue(),
            "Expected second child to keep aligned shorter prefix and receive longer suffix.");
    }

    void testAlignedSimilarityCrossoverHandlesNegativeOffset()
    {
        AlignedSimilarityCrossover crossover(0.9L);

        Specimen parent1({
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 25.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 1.0L, 0.0L), 0.5L, 5.0L, 10.0L)});
        Specimen parent2({
            Maneuver(Vector3(1.0L, 0.0L, 0.0L), 0.2L, 10.0L, 10.0L),
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 5.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 1.0L, 0.0L), 0.0L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 0.0L, 1.0L), 0.6L, 5.0L, 10.0L)});

        auto [child1, child2] =
            crossover.cross(
                parent1,
                parent2);

        expect(
            child1.size() == 4,
            "Expected first child to contain shorter prefix and longer suffix for negative offset.");
        expect(
            child2.size() == 4,
            "Expected second child to contain longer prefix and shorter suffix for negative offset.");
        expect(
            child1[0].getThrottleValue() == parent1[0].getThrottleValue() &&
            child1[1].getThrottleValue() == parent1[1].getThrottleValue() &&
            child1[2].getThrottleValue() == parent2[3].getThrottleValue() &&
            child1[3].getThrottleValue() == parent2[4].getThrottleValue(),
            "Expected first child to keep aligned shorter prefix and receive longer suffix.");
        expect(
            child2[0].getThrottleValue() == parent2[0].getThrottleValue() &&
            child2[1].getThrottleValue() == parent2[1].getThrottleValue() &&
            child2[2].getThrottleValue() == parent2[2].getThrottleValue() &&
            child2[3].getThrottleValue() == parent1[2].getThrottleValue(),
            "Expected second child to keep aligned longer prefix and receive shorter suffix.");
    }

    void testRandomCutCrossoverKeepsMinimumManeuverCount()
    {
        RandomCutCrossover crossover;
        const Specimen parent1 =
            specimenWithManeuverCount(
                MIN_MANEUVERS);
        const Specimen parent2 =
            specimenWithManeuverCount(
                MIN_MANEUVERS);

        for (std::size_t i = 0; i < 100; ++i)
        {
            auto [child1, child2] =
                crossover.cross(
                    parent1,
                    parent2);

            expect(
                child1.size() >= MIN_MANEUVERS,
                "Expected random cut crossover to keep first child above minimum maneuver count.");
            expect(
                child2.size() >= MIN_MANEUVERS,
                "Expected random cut crossover to keep second child above minimum maneuver count.");
        }
    }
}

auto main() -> int
{
    try
    {
        testTrajectoryComparatorObjectivesAndTieBreakers();
        testNSGAIIReturnsParetoFrontHistory();
        testAlignedSimilarityCrossoverExchangesSuffixAfterAlignedRegion();
        testAlignedSimilarityCrossoverHandlesNegativeOffset();
        testRandomCutCrossoverKeepsMinimumManeuverCount();
        std::cout << "NSGA-II smoke tests passed.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "NSGA-II smoke test failed: " << e.what() << '\n';
        return 1;
    }
}
