#ifndef SOLARSCAPE_MOEADALGORITHM_H
#define SOLARSCAPE_MOEADALGORITHM_H

#include <cstddef>
#include <vector>

#include "genetics/GeneticAlgorithm.h"
#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/crossing/CrossoverFactory.h"
#include "genetics/fitness/FitnessEvaluatorFactory.h"
#include "genetics/init/InitializerFactory.h"
#include "genetics/mutation/MutationFactory.h"

class MOEADAlgorithm final : public GeneticAlgorithm
{
public:
    struct Factories
    {
        const InitializerFactory& initializerFactory;
        const CrossoverFactory& crossoverFactory;
        const MutationFactory& mutationFactory;
        const FitnessEvaluatorFactory& fitnessEvaluatorFactory;
    };

    MOEADAlgorithm(
        std::size_t populationSize,
        std::size_t generations,
        std::size_t neighborhoodSize,
        const SpecimenComparator& specimenComparator,
        Factories factories
    );

    ParetoFrontHistory run() const override;

private:
    std::size_t populationSize;
    std::size_t generations;
    std::size_t neighborhoodSize;
    const SpecimenComparator& specimenComparator;
    Factories factories;
};

#endif
