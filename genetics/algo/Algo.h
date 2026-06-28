#ifndef SOLARSCAPE_ALGO_H
#define SOLARSCAPE_ALGO_H

#include <cstddef>
#include <vector>

#include "genetics/GeneticAlgorithm.h"
#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "genetics/init/InitializerFactory.h"
#include "genetics/mutation/MutationFactory.h"
#include "genetics/selection/SelectionFactory.h"

class Algo final : public GeneticAlgorithm
{
public:
    struct Factories
    {
        const InitializerFactory& initializerFactory;
        const SelectionFactory& selectionFactory;
        const CrossoverFactory& crossoverFactory;
        const MutationFactory& mutationFactory;
        const FitnessEvaluatorFactory& fitnessEvaluatorFactory;
    };

    Algo(
        std::size_t populationSize,
        std::size_t generations,
        std::size_t eliteCount,
        std::size_t immigrantCount,
        const SpecimenComparator& specimenComparator,
        Factories factories
    );

    ParetoFrontHistory run() const override;

private:
    using Islands = std::vector<std::vector<Specimen>>;

    Islands createIslands(
        Initializer& initializer) const;

    void evaluateAndSortIslands(
        Islands& islands,
        const FitnessEvaluator& fitnessEvaluator) const;

    auto createNextIsland(
        const std::vector<Specimen>& island,
        Initializer& initializer,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const -> std::vector<Specimen>;

    void migrate(
        Islands& islands) const;

    std::vector<Specimen> flatten(
        const Islands& islands) const;

    std::size_t immigrantCountForIsland(
        std::size_t islandSize) const;

    std::size_t populationSize;
    std::size_t generations;
    std::size_t eliteCount;
    std::size_t immigrantCount;
    const SpecimenComparator& specimenComparator;
    Factories factories;
};

#endif
