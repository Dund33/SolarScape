#include "GeneticAlgorithm.h"

#include <algorithm>
#include <execution>
#include <ranges>
#include <utility>

#include "genetics/crossing/Crossover.h"
#include "genetics/fitness/FitnessEvaluator.h"
#include "genetics/init/Initializer.h"
#include "genetics/mutation/Mutation.h"
#include "genetics/selection/Selection.h"

GeneticAlgorithm::~GeneticAlgorithm() = default;

void GeneticAlgorithm::evaluatePopulation(
    std::vector<Specimen>& population,
    const FitnessEvaluator& fitnessEvaluator) const
{
    std::vector<Specimen*> specimens;
    specimens.reserve(
        population.size());

    for (Specimen& specimen : population)
    {
        specimens.push_back(
            &specimen);
    }

    evaluateSpecimens(
        specimens,
        fitnessEvaluator);
}

void GeneticAlgorithm::evaluateSpecimens(
    std::vector<Specimen*>& specimens,
    const FitnessEvaluator& fitnessEvaluator) const
{
    fitnessEvaluator.evaluateBatch(
        specimens);
}

void GeneticAlgorithm::appendChildren(
    const std::vector<Specimen>& parents,
    std::vector<Specimen>& target,
    std::size_t targetSize,
    const SpecimenComparator& selectionComparator,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation) const
{
    while (target.size() < targetSize)
    {
        const Specimen& parent1 =
            selection.select(
                parents,
                selectionComparator);
        const Specimen& parent2 =
            selection.select(
                parents,
                selectionComparator);

        auto [child1, child2] =
            crossover.cross(
                parent1,
                parent2);

        mutation.mutate(child1);
        mutation.mutate(child2);

        target.push_back(std::move(child1));

        if (target.size() < targetSize)
        {
            target.push_back(std::move(child2));
        }
    }
}

void GeneticAlgorithm::appendImmigrants(
    std::vector<Specimen>& population,
    std::size_t count,
    Initializer& initializer) const
{
    const auto immigrants =
        std::views::iota(std::size_t{0}, count) |
        std::views::transform(
            [&initializer](std::size_t)
            {
                return initializer.create();
            });

    for (Specimen immigrant : immigrants)
    {
        population.push_back(std::move(immigrant));
    }
}

void GeneticAlgorithm::replaceTailWithImmigrants(
    std::vector<Specimen>& population,
    std::size_t count,
    Initializer& initializer) const
{
    std::ranges::generate(
        population |
        std::views::reverse |
        std::views::take(count),
        [&initializer]
        {
            return initializer.create();
        });
}
