#ifndef SOLARSCAPE_ALGO_H
#define SOLARSCAPE_ALGO_H

#include <cstddef>
#include <iosfwd>
#include <vector>

#include "genetics/GeneticAlgorithm.h"
#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "genetics/init/InitializerFactory.h"
#include "genetics/mutation/MutationFactory.h"
#include "genetics/selection/SelectionFactory.h"
#include "genetics/utils/ParetoRanking.h"

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
        Factories factories,
        bool verbose = false,
        std::ostream* diversityLog = nullptr
    );

    ParetoFrontHistory run() const override;

private:
    struct RankedIsland
    {
        std::vector<Specimen> specimens;
        ParetoRankedPopulation ranking;
        std::vector<std::size_t> sortedIndices;
    };

    using Islands = std::vector<RankedIsland>;

    Islands createIslands(
        Initializer& initializer) const;

    void evaluateIslands(
        Islands& islands,
        const FitnessEvaluator& fitnessEvaluator) const;

    void finalizeIslands(
        Islands& islands) const;

    void finalizeIsland(
        RankedIsland& island) const;

    auto createNextIsland(
        const RankedIsland& island,
        Initializer& initializer,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const -> RankedIsland;

    void migrate(
        Islands& islands) const;

    void reintroduceArchive(
        Islands& islands,
        const ParetoFront& archive,
        std::size_t generation) const;

    std::size_t immigrantCountForIsland(
        std::size_t islandSize) const;

    std::size_t archiveReintroductionCountForIsland(
        std::size_t islandSize) const;

    std::size_t populationSize;
    std::size_t generations;
    std::size_t eliteCount;
    std::size_t immigrantCount;
    const SpecimenComparator& specimenComparator;
    Factories factories;
    bool verbose;
    std::ostream* diversityLog;
};

#endif
