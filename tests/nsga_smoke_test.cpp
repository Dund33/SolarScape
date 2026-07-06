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
                Vector3(1.0, 0.0, 0.0),
                0.5,
                static_cast<Real>(i),
                10.0);
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
            std::vector<Specimen> fixtureSpecimens)
            : specimens(std::move(fixtureSpecimens))
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
            std::vector<Specimen> fixtureSpecimens)
            : specimens(std::move(fixtureSpecimens))
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

        void evaluateBatch(
            std::vector<Specimen*>&) const override
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
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 1.0, 10.0, 100.0});
        Specimen worseObjectives =
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 2.0, 1.0, 1.0});

        expect(
            comparator.compare(betterObjectives, worseObjectives) ==
                std::partial_ordering::less,
            "Expected smaller target-window violation to dominate.");

        Specimen lowerFuelLater =
            specimenWithFitness({10.0, 2.0, 1.0});
        Specimen higherFuelEarlier =
            specimenWithFitness({1.0, 1.0, 10.0});

        expect(
            comparator.compare(lowerFuelLater, higherFuelEarlier) ==
                std::partial_ordering::unordered,
            "Expected in-window fuel/time trade-off to be unordered.");
        expect(
            !comparator.isLess(lowerFuelLater, higherFuelEarlier),
            "Expected trajectory comparator not to order an in-window fuel/time trade-off.");

        Specimen lowerFuelSameObjectives =
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 1.0, 2.0, 1.0});
        Specimen higherFuelSameObjectives =
            specimenWithFitness({TARGET_WINDOW_DISTANCE + 1.0, 1.0, 10.0});

        expect(
            comparator.compare(lowerFuelSameObjectives, higherFuelSameObjectives) ==
                std::partial_ordering::less,
            "Expected trajectory comparator to break equivalent objectives by fuel.");

        Specimen closerInsideWindow =
            specimenWithFitness({1.0, 10.0, 5.0});
        Specimen fartherInsideWindow =
            specimenWithFitness({TARGET_WINDOW_DISTANCE - 1.0, 10.0, 5.0});

        expect(
            comparator.compare(closerInsideWindow, fartherInsideWindow) ==
                std::partial_ordering::equivalent,
            "Expected raw distance to stop dominating inside target window.");

        Specimen feasible =
            specimenWithFitness({2.0, 10.0, 5.0});
        Specimen infeasible =
            specimenWithFitness({1.0, 1.0, 100.0, 1.0});

        expect(
            comparator.compare(feasible, infeasible) ==
                std::partial_ordering::less,
            "Expected feasible trajectory specimen to dominate infeasible specimen.");

        Specimen smallerViolation =
            specimenWithFitness({10.0, 10.0, 1.0, 1.0});
        Specimen largerViolation =
            specimenWithFitness({1.0, 1.0, 100.0, 2.0});

        expect(
            comparator.compare(smallerViolation, largerViolation) ==
                std::partial_ordering::less,
            "Expected smaller fuel violation to dominate larger violation.");
    }

    void testNSGAIIReturnsParetoFrontHistory()
    {
        const FitnessValue bestFuel{1.0, 8.0, 1.0};
        const FitnessValue bestTime{2.0, 1.0, 5.0};
        const FitnessValue dominatedA{3.0, 9.0, 6.0};
        const FitnessValue dominatedB{4.0, 10.0, 10.0};

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
        AlignedSimilarityCrossover crossover(0.9);

        Specimen parent1({
            Maneuver(Vector3(1.0, 0.0, 0.0), 0.2, 10.0, 10.0),
            Maneuver(Vector3(0.0, 1.0, 0.0), 0.7, 5.0, 10.0),
            Maneuver(Vector3(0.0, 0.0, 1.0), 0.9, 5.0, 10.0),
            Maneuver(Vector3(1.0, 1.0, 0.0), 0.0, 5.0, 10.0),
            Maneuver(Vector3(1.0, 0.0, 1.0), 0.6, 5.0, 10.0)});
        Specimen parent2({
            Maneuver(Vector3(0.0, 1.0, 0.0), 0.7, 25.0, 10.0),
            Maneuver(Vector3(0.0, 0.0, 1.0), 0.9, 5.0, 10.0),
            Maneuver(Vector3(1.0, 1.0, 0.0), 0.5, 5.0, 10.0)});

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
        AlignedSimilarityCrossover crossover(0.9);

        Specimen parent1({
            Maneuver(Vector3(0.0, 1.0, 0.0), 0.7, 25.0, 10.0),
            Maneuver(Vector3(0.0, 0.0, 1.0), 0.9, 5.0, 10.0),
            Maneuver(Vector3(1.0, 1.0, 0.0), 0.5, 5.0, 10.0)});
        Specimen parent2({
            Maneuver(Vector3(1.0, 0.0, 0.0), 0.2, 10.0, 10.0),
            Maneuver(Vector3(0.0, 1.0, 0.0), 0.7, 5.0, 10.0),
            Maneuver(Vector3(0.0, 0.0, 1.0), 0.9, 5.0, 10.0),
            Maneuver(Vector3(1.0, 1.0, 0.0), 0.0, 5.0, 10.0),
            Maneuver(Vector3(1.0, 0.0, 1.0), 0.6, 5.0, 10.0)});

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
