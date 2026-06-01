#include <compare>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "genetics/comparison/SimpleSpecimenComparator.h"
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
            lhs.minimumDistanceFuelMass == rhs.minimumDistanceFuelMass;
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
            specimenWithFitness({1.0L, 1.0L, 10.0L});
        Specimen worse =
            specimenWithFitness({2.0L, 2.0L, 5.0L});

        expect(
            comparator.compare(better, worse) ==
                std::partial_ordering::less,
            "Expected better specimen to dominate worse specimen.");
        expect(
            comparator.compare(worse, better) ==
                std::partial_ordering::greater,
            "Expected worse specimen to be dominated by better specimen.");
    }

    void testNSGAIIReturnsFirstParetoFront()
    {
        const FitnessValue bestDistanceFast{1.0L, 1.0L, 10.0L};
        const FitnessValue bestFuel{1.0L, 3.0L, 11.0L};
        const FitnessValue dominatedA{2.0L, 2.0L, 5.0L};
        const FitnessValue dominatedB{4.0L, 4.0L, 1.0L};

        FixtureInitializerFactory initializerFactory({
            specimenWithFitness(bestDistanceFast),
            specimenWithFitness(bestFuel),
            specimenWithFitness(dominatedA),
            specimenWithFitness(dominatedB)});
        FirstSelectionFactory selectionFactory;
        CopyCrossoverFactory crossoverFactory;
        NoopMutationFactory mutationFactory;
        NoopFitnessEvaluatorFactory fitnessEvaluatorFactory;
        SimpleSpecimenComparator comparator;

        NSGAIIAlgorithm algorithm(
            4,
            0,
            0,
            comparator,
            {
                initializerFactory,
                selectionFactory,
                crossoverFactory,
                mutationFactory,
                fitnessEvaluatorFactory});

        const std::vector<Specimen> paretoFront =
            algorithm.run();

        expect(
            paretoFront.size() == 2,
            "Expected two specimens in the first Pareto front.");

        bool foundBestDistanceFast = false;
        bool foundBestFuel = false;

        for (const Specimen& specimen : paretoFront)
        {
            const FitnessValue& fitness =
                specimen.getFitness().value();

            foundBestDistanceFast =
                foundBestDistanceFast ||
                sameFitness(fitness, bestDistanceFast);
            foundBestFuel =
                foundBestFuel ||
                sameFitness(fitness, bestFuel);
        }

        expect(
            foundBestDistanceFast,
            "Expected first nondominated specimen in Pareto front.");
        expect(
            foundBestFuel,
            "Expected second nondominated specimen in Pareto front.");
    }
}

auto main() -> int
{
    try
    {
        testComparatorDominance();
        testNSGAIIReturnsFirstParetoFront();
        std::cout << "NSGA-II smoke tests passed.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "NSGA-II smoke test failed: " << e.what() << '\n';
        return 1;
    }
}
