#ifndef SOLARSCAPE_NSGAIIALGORITHM_H
#define SOLARSCAPE_NSGAIIALGORITHM_H

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

class NSGAIIAlgorithm final : public GeneticAlgorithm
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

    NSGAIIAlgorithm(
        std::size_t populationSize,
        std::size_t generations,
        std::size_t immigrantCount,
        const SpecimenComparator& specimenComparator,
        Factories factories,
        bool verbose = false
    );

    ParetoFrontHistory run() const override;

private:
    std::vector<Specimen> createOffspringPopulation(
        const std::vector<Specimen>& population,
        const SpecimenComparator& selectionComparator,
        Initializer& initializer,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const;

    std::size_t populationSize;
    std::size_t generations;
    std::size_t immigrantCount;
    const SpecimenComparator& specimenComparator;
    Factories factories;
    bool verbose;
};

#endif
