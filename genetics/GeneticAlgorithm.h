#ifndef SOLARSCAPE_GENETICALGORITHM_H
#define SOLARSCAPE_GENETICALGORITHM_H

#include <cstddef>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"

class Crossover;
class FitnessEvaluator;
class Initializer;
class Mutation;
class Selection;

class GeneticAlgorithm
{
public:
    virtual ~GeneticAlgorithm() = 0;

protected:
    void evaluatePopulationUnsequenced(
        std::vector<Specimen>& population,
        const FitnessEvaluator& fitnessEvaluator) const;

    void appendChildren(
        const std::vector<Specimen>& parents,
        std::vector<Specimen>& target,
        std::size_t targetSize,
        const SpecimenComparator& selectionComparator,
        Selection& selection,
        Crossover& crossover,
        Mutation& mutation) const;

    void appendImmigrants(
        std::vector<Specimen>& population,
        std::size_t count,
        Initializer& initializer) const;

    void replaceTailWithImmigrants(
        std::vector<Specimen>& population,
        std::size_t count,
        Initializer& initializer) const;
};

#endif
