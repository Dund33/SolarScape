#ifndef SOLARSCAPE_GENETICALGORITHM_H
#define SOLARSCAPE_GENETICALGORITHM_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "genetics/init/InitializerFactory.h"
#include "genetics/mutation/MutationFactory.h"
#include "genetics/search/LocalImprovementFactory.h"
#include "genetics/selection/SelectionFactory.h"

class GeneticAlgorithm
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

    GeneticAlgorithm(
        std::size_t populationSize,
        std::size_t generations,
        std::size_t eliteCount,
        std::size_t immigrantCount,
        Factories factories
    );

    Specimen run() const;

private:
    void evaluatePopulationUnsequenced(
        std::vector<Specimen>& population,
        const FitnessEvaluator& fitnessEvaluator) const;

    void copyElite(
        const std::vector<Specimen>& population,
        std::vector<Specimen>& newPopulation) const;

    void fillPopulationWithChildren(
        const std::vector<Specimen>& population,
        std::vector<Specimen>& newPopulation,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const;

    void addImmigrants(
        std::vector<Specimen>& population,
        Initializer& initializer) const;

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
    Factories factories;
};

#endif
