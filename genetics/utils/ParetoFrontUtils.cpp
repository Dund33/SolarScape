#include "ParetoFrontUtils.h"

#include <algorithm>
#include <ranges>
#include <utility>

namespace
{
    bool hasSameObjectiveValues(
        const Specimen& lhs,
        const Specimen& rhs,
        const SpecimenComparator& specimenComparator)
    {
        for (std::size_t objective = 0;
             objective < specimenComparator.objectiveCount();
             ++objective)
        {
            if (
                specimenComparator.objectiveValue(
                    lhs.getFitness().value(),
                    objective) !=
                specimenComparator.objectiveValue(
                    rhs.getFitness().value(),
                    objective))
            {
                return false;
            }
        }

        return true;
    }

    bool containsSameObjectiveValues(
        const std::vector<Specimen>& front,
        const Specimen& specimen,
        const SpecimenComparator& specimenComparator)
    {
        return std::ranges::any_of(
            front,
            [&](const Specimen& frontSpecimen)
            {
                return hasSameObjectiveValues(
                    frontSpecimen,
                    specimen,
                    specimenComparator);
            });
    }

    void appendDistinctByObjectiveValues(
        std::vector<Specimen>& target,
        std::vector<Specimen>& source,
        const SpecimenComparator& specimenComparator)
    {
        for (Specimen& specimen : source)
        {
            if (
                containsSameObjectiveValues(
                    target,
                    specimen,
                    specimenComparator))
            {
                continue;
            }

            target.push_back(std::move(specimen));
        }
    }
}

std::vector<Specimen> ParetoFrontUtils::frontFromIndices(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    std::vector<Specimen> front;
    front.reserve(frontIndices.size());

    const auto frontSpecimens =
        frontIndices |
        std::views::transform(
            [&population](std::size_t specimenIndex) -> const Specimen&
            {
                return population[specimenIndex];
            });

    for (const Specimen& specimen : frontSpecimens)
    {
        front.push_back(specimen);
    }

    return front;
}

std::vector<Specimen> ParetoFrontUtils::updateArchive(
    std::vector<Specimen> archive,
    std::vector<Specimen> newFront,
    const SpecimenComparator& specimenComparator)
{
    std::vector<Specimen> candidates;
    candidates.reserve(archive.size() + newFront.size());

    appendDistinctByObjectiveValues(
        candidates,
        archive,
        specimenComparator);
    appendDistinctByObjectiveValues(
        candidates,
        newFront,
        specimenComparator);

    if (candidates.empty())
    {
        return {};
    }

    return firstFront(
        candidates,
        specimenComparator);
}

ParetoFrontStats ParetoFrontUtils::calculateStats(
    const std::vector<Specimen>& population,
    const std::vector<std::size_t>& frontIndices)
{
    return calculateStats(
        frontIndices |
        std::views::transform(
            [&population](std::size_t specimenIndex) -> const Specimen&
            {
                return population[specimenIndex];
            }));
}
