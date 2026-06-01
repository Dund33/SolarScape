#ifndef SOLARSCAPE_SPECIMENCOMPARATOR_H
#define SOLARSCAPE_SPECIMENCOMPARATOR_H

#include <compare>

class Specimen;

class SpecimenComparator
{
public:
    virtual ~SpecimenComparator() = default;

    virtual std::partial_ordering compare(
        const Specimen& lhs,
        const Specimen& rhs
    ) const = 0;

    virtual bool isLess(
        const Specimen& lhs,
        const Specimen& rhs
    ) const = 0;
};

#endif
