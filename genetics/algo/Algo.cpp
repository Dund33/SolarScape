#include "Algo.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ranges>
#include <utility>

#include "genetics/algo/PopulationPyramid.h"
#include "genetics/fitness/FitnessValue.h"
#include "genetics/utils/ParetoFrontUtils.h"

namespace
{
    void sortPopulationByFitness(
        std::vector<Specimen>& population,
        const SpecimenComparator& specimenComparator)
    {
        std::ranges::sort(
            population,
            [&specimenComparator](const Specimen& lhs, const Specimen& rhs)
            {
                return specimenComparator.isLess(lhs, rhs);
            });
    }

    void printFitnessValue(
        const FitnessValue& fitness)
    {
        std::cout
            << "[minimumDistance=" << fitness.minimumDistance
            << ", minimumDistanceTime=" << fitness.minimumDistanceTime
            << ", minimumDistanceFuelMass=" << fitness.minimumDistanceFuelMass
            << ", fuelConstraintViolation=" << fitness.fuelConstraintViolation
            << ']';
    }

    const Specimen* bestSpecimen(
        const PopulationPyramid& pyramid,
        const SpecimenComparator& specimenComparator)
    {
        const Specimen* best = nullptr;

        for (const auto& level : pyramid.levels())
        {
            if (level.empty())
            {
                continue;
            }

            if (
                best == nullptr ||
                specimenComparator.isLess(
                    level.front(),
                    *best))
            {
                best = &level.front();
            }
        }

        return best;
    }

    void printLevelSizes(
        const PopulationPyramid& pyramid)
    {
        std::cout << '[';

        for (std::size_t i = 0; i < pyramid.levels().size(); ++i)
        {
            if (i > 0)
            {
                std::cout << ", ";
            }

            std::cout << pyramid.levels()[i].size();
        }

        std::cout << ']';
    }

    void printGenerationResult(
        std::size_t generation,
        const PopulationPyramid& pyramid,
        const SpecimenComparator& specimenComparator)
    {
        const Specimen* best =
            bestSpecimen(
                pyramid,
                specimenComparator);

        if (best == nullptr)
        {
            std::cout
                << "Generation " << generation
                << " | Population pyramid is empty\n";
            return;
        }

        std::cout
            << "Generation " << generation
            << " | Pyramid levels = ";
        printLevelSizes(
            pyramid);
        std::cout << " | Best fitness = ";
        printFitnessValue(best->getFitness().value());
        std::cout << '\n';
    }

}

Algo::Algo(
    std::size_t populationSize,
    std::size_t generations,
    std::size_t eliteCount,
    std::size_t immigrantCount,
    const SpecimenComparator& specimenComparator,
    Factories factories
)
    : populationSize(populationSize),
      generations(generations),
      eliteCount(eliteCount),
      immigrantCount(immigrantCount),
      specimenComparator(specimenComparator),
      factories(factories)
{
}

std::vector<Specimen> Algo::run() const
{
    auto initializer =
        factories.initializerFactory.create();
    auto selection =
        factories.selectionFactory.create();
    auto crossover =
        factories.crossoverFactory.create();
    auto mutation =
        factories.mutationFactory.create();
    auto localImprovement =
        factories.localImprovementFactory.create();
    auto fitnessEvaluator =
        factories.fitnessEvaluatorFactory.create();

    PopulationPyramid pyramid =
        PopulationPyramid::create(
            populationSize,
            *initializer);

    for (std::size_t generation = 0; generation < generations; ++generation)
    {
        for (auto& level : pyramid.levels())
        {
            evaluatePopulationUnsequenced(
                level,
                *fitnessEvaluator);

            sortPopulationByFitness(
                level,
                specimenComparator);

            localImprovement->improve(
                level.front(),
                *fitnessEvaluator,
                specimenComparator);

            sortPopulationByFitness(
                level,
                specimenComparator);
        }

        pyramid.promoteElite(
            eliteCount,
            specimenComparator);

        printGenerationResult(
            generation,
            pyramid,
            specimenComparator);

        std::vector<std::vector<Specimen>> nextLevels;
        nextLevels.reserve(
            pyramid.levels().size());

        for (const auto& level : pyramid.levels())
        {
            nextLevels.push_back(
                createNextGeneration(
                    level,
                    level.size(),
                    immigrantCountForLevel(level.size()),
                    *initializer,
                    *selection,
                    *crossover,
                    *mutation));
        }

        pyramid =
            PopulationPyramid(
                std::move(nextLevels));
    }

    for (auto& level : pyramid.levels())
    {
        evaluatePopulationUnsequenced(
            level,
            *fitnessEvaluator);

        sortPopulationByFitness(
            level,
            specimenComparator);
    }

    pyramid.promoteElite(
        eliteCount,
        specimenComparator);

    std::vector<Specimen> population =
        pyramid.flatten();

    return ParetoFrontUtils::firstFront(
        population,
        specimenComparator);
}

void Algo::copyElite(
    const std::vector<Specimen>& population,
    std::vector<Specimen>& newPopulation) const
{
    std::ranges::copy(
        population |
        std::views::take(std::min(eliteCount, population.size())),
        std::back_inserter(newPopulation));
}

auto Algo::createNextGeneration(
    const std::vector<Specimen>& population,
    std::size_t targetSize,
    std::size_t nextGenerationImmigrantCount,
    Initializer& initializer,
    Selection& selection,
    Crossover& crossover,
    Mutation& mutation) const -> std::vector<Specimen>
{
    std::vector<Specimen> newPopulation;
    newPopulation.reserve(targetSize);

    copyElite(
        population,
        newPopulation);

    appendChildren(
        population,
        newPopulation,
        targetSize,
        specimenComparator,
        selection,
        crossover,
        mutation);

    replaceTailWithImmigrants(
        newPopulation,
        nextGenerationImmigrantCount,
        initializer);

    return newPopulation;
}

std::size_t Algo::immigrantCountForLevel(
    std::size_t levelSize) const
{
    if (immigrantCount == 0 || populationSize == 0)
    {
        return 0;
    }

    const std::size_t levelEliteCount =
        std::min(
            eliteCount,
            levelSize);
    const std::size_t replaceableCount =
        levelSize - levelEliteCount;

    return std::min(
        replaceableCount,
        levelSize * immigrantCount / populationSize);
}
