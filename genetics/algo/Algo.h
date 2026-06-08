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
#include "genetics/search/LocalImprovementFactory.h"
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
        const LocalImprovementFactory& localImprovementFactory;
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

    Specimen run() const;

private:
    void copyElite(
        const std::vector<Specimen>& population,
        std::vector<Specimen>& newPopulation) const;

    auto createNextGeneration(
        const std::vector<Specimen>& population,
        Initializer& initializer,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const -> std::vector<Specimen>;

    std::size_t populationSize;
    std::size_t generations;
    std::size_t eliteCount;
    std::size_t immigrantCount;
    const SpecimenComparator& specimenComparator;
    Factories factories;
};

#endif
