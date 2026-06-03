#ifndef SOLARSCAPE_SPECIMENCOMPARATOR_H
#define SOLARSCAPE_SPECIMENCOMPARATOR_H

#include <compare>
#include <span>

#include "genetics/fitness/FitnessValue.h"
#include "math/Real.h"

class Specimen;

struct FitnessField
{
    Real FitnessValue::* value;
    Real scale;

    Real comparableValue(
        const FitnessValue& fitness) const
    {
        return (fitness.*value) * scale;
    }
};

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

    virtual std::span<const FitnessField> objectiveFields() const = 0;

protected:
    static std::partial_ordering compareByPareto(
        const FitnessValue& lhs,
        const FitnessValue& rhs,
        std::span<const FitnessField> fields)
    {
        bool lhsStrictlyBetter = false;
        bool rhsStrictlyBetter = false;

        for (const FitnessField& field : fields)
        {
            const Real lhsValue =
                field.comparableValue(lhs);
            const Real rhsValue =
                field.comparableValue(rhs);

            if (lhsValue < rhsValue)
            {
                lhsStrictlyBetter = true;
            }

            if (rhsValue < lhsValue)
            {
                rhsStrictlyBetter = true;
            }
        }

        if (lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return std::partial_ordering::less;
        }

        if (rhsStrictlyBetter && !lhsStrictlyBetter)
        {
            return std::partial_ordering::greater;
        }

        if (!lhsStrictlyBetter && !rhsStrictlyBetter)
        {
            return std::partial_ordering::equivalent;
        }

        return std::partial_ordering::unordered;
    }

    static bool lexicographicallyLess(
        const FitnessValue& lhs,
        const FitnessValue& rhs,
        std::span<const FitnessField> fields)
    {
        for (const FitnessField& field : fields)
        {
            const Real lhsValue =
                field.comparableValue(lhs);
            const Real rhsValue =
                field.comparableValue(rhs);

            if (lhsValue < rhsValue)
            {
                return true;
            }

            if (rhsValue < lhsValue)
            {
                return false;
            }
        }

        return false;
    }
};

#endif
