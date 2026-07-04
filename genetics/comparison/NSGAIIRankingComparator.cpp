#include "NSGAIIRankingComparator.h"

#include <stdexcept>

NSGAIIRankingComparator::NSGAIIRankingComparator(
    const std::vector<Specimen>& population,
    const std::vector<SpecimenRank>& ranks,
    const SpecimenComparator& fallbackComparator)
    : ranks(ranks),
      fallbackComparator(fallbackComparator)
{
    if (ranks.size() != population.size())
    {
        throw std::invalid_argument(
            "Rank count must match population size.");
    }

    indexBySpecimen.reserve(population.size());

    for (std::size_t i = 0; i < population.size(); ++i)
    {
        indexBySpecimen.emplace(&population[i], i);
    }
}

std::partial_ordering NSGAIIRankingComparator::compare(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const SpecimenRank& lhsRank = rankFor(lhs);
    const SpecimenRank& rhsRank = rankFor(rhs);

    if (lhsRank.rank < rhsRank.rank)
    {
        return std::partial_ordering::less;
    }

    if (rhsRank.rank < lhsRank.rank)
    {
        return std::partial_ordering::greater;
    }

    if (lhsRank.crowdingDistance > rhsRank.crowdingDistance)
    {
        return std::partial_ordering::less;
    }

    if (rhsRank.crowdingDistance > lhsRank.crowdingDistance)
    {
        return std::partial_ordering::greater;
    }

    return fallbackComparator.compare(lhs, rhs);
}

bool NSGAIIRankingComparator::isLess(
    const Specimen& lhs,
    const Specimen& rhs
) const
{
    const std::partial_ordering result =
        compare(lhs, rhs);

    if (result == std::partial_ordering::less)
    {
        return true;
    }

    if (result == std::partial_ordering::greater)
    {
        return false;
    }

    return fallbackComparator.isLess(lhs, rhs);
}

std::size_t NSGAIIRankingComparator::objectiveCount() const
{
    return fallbackComparator.objectiveCount();
}

Real NSGAIIRankingComparator::objectiveValue(
    const FitnessValue& fitness,
    std::size_t objective) const
{
    return fallbackComparator.objectiveValue(
        fitness,
        objective);
}

const SpecimenRank& NSGAIIRankingComparator::rankFor(
    const Specimen& specimen) const
{
    const auto it =
        indexBySpecimen.find(&specimen);

    if (it == indexBySpecimen.end())
    {
        throw std::invalid_argument(
            "Specimen is not part of the ranked population.");
    }

    return ranks[it->second];
}
