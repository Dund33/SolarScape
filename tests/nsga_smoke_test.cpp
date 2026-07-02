#include <compare>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/SimpleSpecimenComparator.h"
#include "genetics/comparison/TrajectorySpecimenComparator.h"
#include "genetics/crossing/AlignedSimilarityCrossover.h"
#include "genetics/crossing/CrossoverFactory.h"
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

    void testComparatorDominance()
    {
        SimpleSpecimenComparator comparator;
        Specimen better =
            specimenWithFitness({1.0L, 1.0L, 5.0L});
        Specimen worse =
            specimenWithFitness({2.0L, 2.0L, 10.0L});

        expect(
            comparator.compare(better, worse) ==
                std::partial_ordering::less,
            "Expected better specimen to dominate worse specimen.");
        expect(
            comparator.compare(worse, better) ==
                std::partial_ordering::greater,
            "Expected worse specimen to be dominated by better specimen.");

        Specimen earlier =
            specimenWithFitness({1.0L, 1.0L, 5.0L});
        Specimen later =
            specimenWithFitness({1.0L, 2.0L, 5.0L});

        expect(
            comparator.compare(earlier, later) ==
                std::partial_ordering::less,
            "Expected earlier time to dominate when other criteria match.");
        expect(
            comparator.isLess(earlier, later),
            "Expected earlier time to be ordered first.");

        Specimen lowerFuelLater =
            specimenWithFitness({1.0L, 2.0L, 5.0L});
        Specimen higherFuelEarlier =
            specimenWithFitness({1.0L, 1.0L, 10.0L});

        expect(
            comparator.compare(lowerFuelLater, higherFuelEarlier) ==
                std::partial_ordering::unordered,
            "Expected fuel and time trade-off to be unordered.");
        expect(
            !comparator.isLess(lowerFuelLater, higherFuelEarlier),
            "Expected earlier time to beat lower fuel use in tie-breaker order.");

        Specimen feasible =
            specimenWithFitness({2.0L, 2.0L, 1.0L});
        Specimen infeasible =
            specimenWithFitness({1.0L, 1.0L, 10.0L, 1.0L});

        expect(
            comparator.compare(feasible, infeasible) ==
                std::partial_ordering::unordered,
            "Expected fuel violation trade-off to be unordered.");

        Specimen smallerViolation =
            specimenWithFitness({1.0L, 1.0L, 10.0L, 1.0L});
        Specimen largerViolation =
            specimenWithFitness({1.0L, 1.0L, 10.0L, 2.0L});

        expect(
            comparator.compare(smallerViolation, largerViolation) ==
                std::partial_ordering::less,
            "Expected smaller fuel constraint violation to dominate when other criteria match.");
    }

    void testTrajectoryComparatorObjectivesAndTieBreakers()
    {
        TrajectorySpecimenComparator comparator;
        Specimen betterObjectives =
            specimenWithFitness({1.0L, 1.0L, 5.0L});
        Specimen worseObjectives =
            specimenWithFitness({2.0L, 2.0L, 10.0L});

        expect(
            comparator.compare(betterObjectives, worseObjectives) ==
                std::partial_ordering::less,
            "Expected lower distance, earlier time, and lower fuel use to dominate.");

        Specimen earlier =
            specimenWithFitness({10.0L, 1.0L, 100.0L});
        Specimen later =
            specimenWithFitness({1.0L, 2.0L, 1.0L});

        expect(
            comparator.compare(earlier, later) ==
                std::partial_ordering::unordered,
            "Expected distance/time/fuel trade-off to be unordered.");
        expect(
            comparator.isLess(earlier, later),
            "Expected trajectory comparator to break ties by time, distance, then fuel.");

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
        const FitnessValue bestDistance{1.0L, 5.0L, 4.0L};
        const FitnessValue bestFuel{2.0L, 8.0L, 1.0L};
        const FitnessValue dominatedA{3.0L, 1.0L, 3.0L};
        const FitnessValue timeOnlyBest{2.0L, 1.0L, 5.0L};

        FixtureInitializerFactory initializerFactory({
            specimenWithFitness(bestDistance),
            specimenWithFitness(bestFuel),
            specimenWithFitness(dominatedA),
            specimenWithFitness(timeOnlyBest)});
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

        bool foundBestDistance = false;
        bool foundBestFuel = false;

        for (const Specimen& specimen : paretoFront)
        {
            const FitnessValue& fitness =
                specimen.getFitness().value();

            foundBestDistance =
                foundBestDistance ||
                sameFitness(fitness, bestDistance);
            foundBestFuel =
                foundBestFuel ||
                sameFitness(fitness, bestFuel);
        }

        expect(
            foundBestDistance,
            "Expected best distance specimen in Pareto front.");
        expect(
            foundBestFuel,
            "Expected best fuel specimen in Pareto front.");
    }

    void testAlignedSimilarityCrossoverSwapsAlignedManeuvers()
    {
        AlignedSimilarityCrossover crossover;

        Specimen parent1({
            Maneuver(Vector3(1.0L, 0.0L, 0.0L), 0.2L, 10.0L, 10.0L),
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 5.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L)});
        Specimen parent2({
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.75L, 25.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.95L, 5.0L, 10.0L)});

        auto [child1, child2] =
            crossover.cross(
                parent1,
                parent2);

        expect(
            child1.size() == parent1.size(),
            "Expected first child to keep first parent size.");
        expect(
            child2.size() == parent2.size(),
            "Expected second child to keep second parent size.");

        const bool firstAlignedPairSwapped =
            child1[1].getThrottleValue() == parent2[0].getThrottleValue() &&
            child2[0].getThrottleValue() == parent1[1].getThrottleValue();
        const bool secondAlignedPairSwapped =
            child1[2].getThrottleValue() == parent2[1].getThrottleValue() &&
            child2[1].getThrottleValue() == parent1[2].getThrottleValue();

        expect(
            firstAlignedPairSwapped || secondAlignedPairSwapped,
            "Expected at least one aligned maneuver pair to be swapped.");
    }

    void testAlignedSimilarityCrossoverHandlesNegativeOffset()
    {
        AlignedSimilarityCrossover crossover;

        Specimen parent1({
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.7L, 25.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.9L, 5.0L, 10.0L)});
        Specimen parent2({
            Maneuver(Vector3(1.0L, 0.0L, 0.0L), 0.2L, 10.0L, 10.0L),
            Maneuver(Vector3(0.0L, 1.0L, 0.0L), 0.75L, 5.0L, 10.0L),
            Maneuver(Vector3(0.0L, 0.0L, 1.0L), 0.95L, 5.0L, 10.0L),
            Maneuver(Vector3(1.0L, 1.0L, 0.0L), 0.4L, 5.0L, 10.0L)});

        auto [child1, child2] =
            crossover.cross(
                parent1,
                parent2);

        expect(
            child1.size() == parent1.size(),
            "Expected first child to keep first parent size for negative offset.");
        expect(
            child2.size() == parent2.size(),
            "Expected second child to keep second parent size for negative offset.");

        const bool firstAlignedPairSwapped =
            child1[0].getThrottleValue() == parent2[1].getThrottleValue() &&
            child2[1].getThrottleValue() == parent1[0].getThrottleValue();
        const bool secondAlignedPairSwapped =
            child1[1].getThrottleValue() == parent2[2].getThrottleValue() &&
            child2[2].getThrottleValue() == parent1[1].getThrottleValue();

        expect(
            firstAlignedPairSwapped || secondAlignedPairSwapped,
            "Expected at least one negative-offset aligned maneuver pair to be swapped.");
    }
}

auto main() -> int
{
    try
    {
        testComparatorDominance();
        testTrajectoryComparatorObjectivesAndTieBreakers();
        testNSGAIIReturnsParetoFrontHistory();
        testAlignedSimilarityCrossoverSwapsAlignedManeuvers();
        testAlignedSimilarityCrossoverHandlesNegativeOffset();
        std::cout << "NSGA-II smoke tests passed.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "NSGA-II smoke test failed: " << e.what() << '\n';
        return 1;
    }
}
