#ifndef SOLARSCAPE_NSGAIIRANKINGCOMPARATOR_H
#define SOLARSCAPE_NSGAIIRANKINGCOMPARATOR_H

#include <compare>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "genetics/Specimen.h"
#include "genetics/comparison/SpecimenComparator.h"
#include "genetics/comparison/SpecimenRank.h"

class NSGAIIRankingComparator final : public SpecimenComparator
{
public:
    NSGAIIRankingComparator(
        const std::vector<Specimen>& population,
        const std::vector<SpecimenRank>& ranks,
        const SpecimenComparator& objectiveComparator,
        bool usesFallbackComparator = true);

    std::partial_ordering compare(
        const Specimen& lhs,
        const Specimen& rhs
    ) const override;

    bool isLess(
        const Specimen& lhs,
        const Specimen& rhs
    ) const override;

    std::size_t objectiveCount() const override;

    Real objectiveValue(
        const FitnessValue& fitness,
        std::size_t objective) const override;

private:
    const SpecimenRank& rankFor(
        const Specimen& specimen) const;

    std::unordered_map<const Specimen*, std::size_t> indexBySpecimen;
    const std::vector<SpecimenRank>& ranks;
    const SpecimenComparator& objectiveComparator;
    bool usesFallbackComparator;
};

#endif
